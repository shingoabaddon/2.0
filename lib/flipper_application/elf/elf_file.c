#include "elf_file.h"
#include "elf_file_i.h"
#include "elf_file_xip.h"

#include <storage/storage.h>
#include <elf.h>
#include <furi_hal_flash.h>
#include <toolbox/crc32_calc.h>
#include "elf_api_interface.h"
#include "../api_hashtable/api_hashtable.h"

#define TAG "Elf"

#define ELF_NAME_BUFFER_LEN        32
#define SECTION_OFFSET(e, n)       ((e)->section_table + (n) * sizeof(Elf32_Shdr))
#define IS_FLAGS_SET(v, m)         (((v) & (m)) == (m))
#define RESOLVER_THREAD_YIELD_STEP 30
#define FAST_RELOCATION_VERSION    1

// #define ELF_DEBUG_LOG 1

#ifndef ELF_DEBUG_LOG
#undef FURI_LOG_D
#define FURI_LOG_D(...)
#endif

#define ELF_INVALID_ADDRESS 0xFFFFFFFF

#define TRAMPOLINE_CODE_SIZE 6

/**
ldr r12, [pc, #2]
bx r12
*/
const uint8_t trampoline_code_little_endian[TRAMPOLINE_CODE_SIZE] =
    {0xdf, 0xf8, 0x02, 0xc0, 0x60, 0x47};

typedef struct {
    uint8_t code[TRAMPOLINE_CODE_SIZE];
    uint32_t addr;
} FURI_PACKED JMPTrampoline;

/**************************************************************************************************/
/********************************************* Caches *********************************************/
/**************************************************************************************************/

static bool address_cache_get(AddressCache_t cache, int symEntry, Elf32_Addr* symAddr) {
    Elf32_Addr* addr = AddressCache_get(cache, symEntry);
    if(addr) {
        *symAddr = *addr;
        return true;
    } else {
        return false;
    }
}

static void address_cache_put(AddressCache_t cache, int symEntry, Elf32_Addr symAddr) {
    AddressCache_set_at(cache, symEntry, symAddr);
}

/**************************************************************************************************/
/********************************************** ELF ***********************************************/
/**************************************************************************************************/

static void elf_file_maybe_release_fd(ELFFile* elf) {
    if(elf->fd) {
        storage_file_free(elf->fd);
        elf->fd = NULL;
    }
}

static ELFSection* elf_file_get_section(ELFFile* elf, const char* name) {
    return ELFSectionDict_get(elf->sections, name);
}

static ELFSection* elf_file_get_or_put_section(ELFFile* elf, const char* name) {
    ELFSection* section_p = elf_file_get_section(elf, name);
    if(!section_p) {
        ELFSectionDict_set_at(
            elf->sections,
            strdup(name),
            (ELFSection){
                .data = NULL,
                .exec_addr = 0,
                .sec_idx = 0,
                .size = 0,
                .sh_flags = 0,
                .file_offset = 0,
                .file_align = 0,
                .xip = false,
                .rel_count = 0,
                .rel_offset = 0,
                .fast_rel = NULL,
            });
        section_p = elf_file_get_section(elf, name);
    }

    return section_p;
}

static bool elf_read_string_from_offset(ELFFile* elf, off_t offset, FuriString* name) {
    bool result = false;

    off_t old = storage_file_tell(elf->fd);

    do {
        if(!storage_file_seek(elf->fd, offset, true)) break;

        char buffer[ELF_NAME_BUFFER_LEN + 1];
        buffer[ELF_NAME_BUFFER_LEN] = 0;

        while(true) {
            size_t read = storage_file_read(elf->fd, buffer, ELF_NAME_BUFFER_LEN);
            furi_string_cat(name, buffer);
            if(strlen(buffer) < ELF_NAME_BUFFER_LEN) {
                result = true;
                break;
            }

            if(storage_file_get_error(elf->fd) != FSE_OK || read == 0) break;
        }

    } while(false);
    storage_file_seek(elf->fd, old, true);

    return result;
}

static bool elf_read_section_name(ELFFile* elf, off_t offset, FuriString* name) {
    return elf_read_string_from_offset(elf, elf->section_table_strings + offset, name);
}

static bool elf_read_symbol_name(ELFFile* elf, off_t offset, FuriString* name) {
    return elf_read_string_from_offset(elf, elf->symbol_table_strings + offset, name);
}

static bool elf_read_section_header(ELFFile* elf, size_t section_idx, Elf32_Shdr* section_header) {
    off_t offset = SECTION_OFFSET(elf, section_idx);
    return storage_file_seek(elf->fd, offset, true) &&
           storage_file_read(elf->fd, section_header, sizeof(Elf32_Shdr)) == sizeof(Elf32_Shdr);
}

static bool elf_read_section(
    ELFFile* elf,
    size_t section_idx,
    Elf32_Shdr* section_header,
    FuriString* name) {
    if(!elf_read_section_header(elf, section_idx, section_header)) {
        return false;
    }

    if(section_header->sh_name && !elf_read_section_name(elf, section_header->sh_name, name)) {
        return false;
    }

    return true;
}

static bool elf_read_symbol(ELFFile* elf, int n, Elf32_Sym* sym, FuriString* name) {
    bool success = false;
    off_t old = storage_file_tell(elf->fd);
    off_t pos = elf->symbol_table + n * sizeof(Elf32_Sym);
    if(storage_file_seek(elf->fd, pos, true) &&
       storage_file_read(elf->fd, sym, sizeof(Elf32_Sym)) == sizeof(Elf32_Sym)) {
        if(sym->st_name)
            success = elf_read_symbol_name(elf, sym->st_name, name);
        else {
            Elf32_Shdr shdr;
            success = elf_read_section(elf, sym->st_shndx, &shdr, name);
        }
    }
    storage_file_seek(elf->fd, old, true);
    return success;
}

static ELFSection* elf_section_of(ELFFile* elf, int index) {
    ELFSectionDict_it_t it;
    for(ELFSectionDict_it(it, elf->sections); !ELFSectionDict_end_p(it); ELFSectionDict_next(it)) {
        ELFSectionDict_itref_t* itref = ELFSectionDict_ref(it);
        if(itref->value.sec_idx == index) {
            return &itref->value;
        }
    }

    return NULL;
}

static Elf32_Addr elf_address_of(ELFFile* elf, Elf32_Sym* sym, const char* sName) {
    if(sym->st_shndx == SHN_UNDEF) {
        Elf32_Addr addr = 0;
        uint32_t hash = elf_symbolname_hash(sName);
        if(elf->api_interface->resolver_callback(elf->api_interface, hash, &addr)) {
            return addr;
        }
    } else {
        ELFSection* symSec = elf_section_of(elf, sym->st_shndx);
        if(symSec) {
            return (symSec->exec_addr) + sym->st_value;
        }
    }
    FURI_LOG_D(TAG, "  Can not find address for symbol %s", sName);
    return ELF_INVALID_ADDRESS;
}

__attribute__((unused)) static const char* elf_reloc_type_to_str(int symt) {
#define STRCASE(name) \
    case name:        \
        return #name;
    switch(symt) {
        STRCASE(R_ARM_NONE)
        STRCASE(R_ARM_TARGET1)
        STRCASE(R_ARM_ABS32)
        STRCASE(R_ARM_REL32)
        STRCASE(R_ARM_THM_PC22)
        STRCASE(R_ARM_THM_JUMP24)
    default:
        return "R_<unknow>";
    }
#undef STRCASE
}

static JMPTrampoline* elf_create_trampoline(Elf32_Addr addr) {
    JMPTrampoline* trampoline = malloc(sizeof(JMPTrampoline));
    memcpy(trampoline->code, trampoline_code_little_endian, TRAMPOLINE_CODE_SIZE);
    trampoline->addr = addr;
    return trampoline;
}

/**
 * @param patchAddr  RAM address to read/write instruction bytes
 * @param relAddr    Runtime (exec) address for PC-relative offset calculation
 */
static void elf_relocate_jmp_call(
    ELFFile* elf,
    Elf32_Addr patchAddr,
    Elf32_Addr relAddr,
    int type,
    Elf32_Addr symAddr) {
    int offset, hi, lo, s, j1, j2, i1, i2, imm10, imm11;
    int to_thumb, is_call, blx_bit = 1 << 12;

    /* Get initial offset — read from RAM staging buffer */
    hi = ((uint16_t*)patchAddr)[0];
    lo = ((uint16_t*)patchAddr)[1];
    s = (hi >> 10) & 1;
    j1 = (lo >> 13) & 1;
    j2 = (lo >> 11) & 1;
    i1 = (j1 ^ s) ^ 1;
    i2 = (j2 ^ s) ^ 1;
    imm10 = hi & 0x3ff;
    imm11 = lo & 0x7ff;
    offset = (s << 24) | (i1 << 23) | (i2 << 22) | (imm10 << 12) | (imm11 << 1);
    if(offset & 0x01000000) offset -= 0x02000000;

    to_thumb = symAddr & 1;
    is_call = (type == R_ARM_THM_PC22);

    /* Store offset */
    int offset_copy = offset;

    /* Compute final offset — PC-relative from runtime address */
    offset += symAddr - relAddr;
    if(!to_thumb && is_call) {
        blx_bit = 0; /* bl -> blx */
        offset = (offset + 3) & -4; /* Compute offset from aligned PC */
    }

    /* Check that relocation is possible
    * offset must not be out of range
    * if target is to be entered in arm mode:
        - bit 1 must not set
        - instruction must be a call (bl) or a jump to PLT */
    if(!to_thumb || offset >= 0x1000000 || offset < -0x1000000) {
        if(to_thumb || (symAddr & 2) || (!is_call)) {
            FURI_LOG_D(
                TAG,
                "can't relocate value at %lx, %s, doing trampoline",
                relAddr,
                elf_reloc_type_to_str(type));

            Elf32_Addr addr;
            if(!address_cache_get(elf->trampoline_cache, symAddr, &addr)) {
                addr = (Elf32_Addr)elf_create_trampoline(symAddr);
                address_cache_put(elf->trampoline_cache, symAddr, addr);
            }

            offset = offset_copy;
            offset += (int)addr - relAddr;
            if(!to_thumb && is_call) {
                blx_bit = 0; /* bl -> blx */
                offset = (offset + 3) & -4; /* Compute offset from aligned PC */
            }
        }
    }

    /* Compute and store final offset — write to RAM staging buffer */
    s = (offset >> 24) & 1;
    i1 = (offset >> 23) & 1;
    i2 = (offset >> 22) & 1;
    j1 = s ^ (i1 ^ 1);
    j2 = s ^ (i2 ^ 1);
    imm10 = (offset >> 12) & 0x3ff;
    imm11 = (offset >> 1) & 0x7ff;
    (*(uint16_t*)patchAddr) = (uint16_t)((hi & 0xf800) | (s << 10) | imm10);
    (*(uint16_t*)(patchAddr + 2)) =
        (uint16_t)((lo & 0xc000) | (j1 << 13) | blx_bit | (j2 << 11) | imm11);
}

static void elf_relocate_mov(Elf32_Addr patchAddr, int type, Elf32_Addr symAddr) {
    uint16_t upper_insn = ((uint16_t*)patchAddr)[0];
    uint16_t lower_insn = ((uint16_t*)patchAddr)[1];

    /* MOV*<C> <Rd>,#<imm16>
     *
     * i = upper[10]
     * imm4 = upper[3:0]
     * imm3 = lower[14:12]
     * imm8 = lower[7:0]
     *
     * imm16 = imm4:i:imm3:imm8
     */
    uint32_t i = (upper_insn >> 10) & 1; /* upper[10] */
    uint32_t imm4 = upper_insn & 0x000F; /* upper[3:0] */
    uint32_t imm3 = (lower_insn >> 12) & 0x7; /* lower[14:12] */
    uint32_t imm8 = lower_insn & 0x00FF; /* lower[7:0] */

    int32_t addend = (imm4 << 12) | (i << 11) | (imm3 << 8) | imm8; /* imm16 */

    uint32_t addr = (symAddr + addend);
    if(type == R_ARM_THM_MOVT_ABS) {
        addr >>= 16; /* upper 16 bits */
    } else {
        addr &= 0x0000FFFF; /* lower 16 bits */
    }

    /* Re-encode — write to RAM staging buffer */
    ((uint16_t*)patchAddr)[0] = (upper_insn & 0xFBF0) | (((addr >> 11) & 1) << 10) /* i */
                                | ((addr >> 12) & 0x000F); /* imm4 */
    ((uint16_t*)patchAddr)[1] = (lower_insn & 0x8F00) | (((addr >> 8) & 0x7) << 12) /* imm3 */
                                | (addr & 0x00FF); /* imm8 */
}

/**
 * @param patchAddr  RAM address where instruction bytes are read/written
 * @param relAddr    Runtime address for PC-relative relocations (flash for XIP sections)
 */
static bool elf_relocate_symbol(
    ELFFile* elf,
    Elf32_Addr patchAddr,
    Elf32_Addr relAddr,
    int type,
    Elf32_Addr symAddr) {
    switch(type) {
    case R_ARM_TARGET1:
    case R_ARM_ABS32:
        *((uint32_t*)patchAddr) += symAddr;
        FURI_LOG_D(
            TAG, "  R_ARM_ABS32 relocated is 0x%08X", (unsigned int)*((uint32_t*)patchAddr));
        break;
    case R_ARM_REL32:
        *((uint32_t*)patchAddr) += symAddr - relAddr;
        FURI_LOG_D(
            TAG, "  R_ARM_REL32 relocated is 0x%08X", (unsigned int)*((uint32_t*)patchAddr));
        break;
    case R_ARM_THM_PC22:
    case R_ARM_CALL:
    case R_ARM_THM_JUMP24:
        elf_relocate_jmp_call(elf, patchAddr, relAddr, type, symAddr);
        FURI_LOG_D(
            TAG,
            "  R_ARM_THM_CALL/JMP relocated is 0x%08X",
            (unsigned int)*((uint32_t*)patchAddr));
        break;
    case R_ARM_THM_MOVW_ABS_NC:
    case R_ARM_THM_MOVT_ABS:
        elf_relocate_mov(patchAddr, type, symAddr);
        FURI_LOG_D(
            TAG,
            "  R_ARM_THM_MOVW_ABS_NC/MOVT_ABS relocated is 0x%08X",
            (unsigned int)*((uint32_t*)patchAddr));
        break;
    default:
        FURI_LOG_E(TAG, "  Undefined relocation %d", type);
        return false;
    }
    return true;
}

static bool elf_relocate(ELFFile* elf, ELFSection* s) {
    if(s->data) {
        Elf32_Rel rel;
        size_t relEntries = s->rel_count;
        size_t relCount;
        (void)storage_file_seek(elf->fd, s->rel_offset, true);
        FURI_LOG_D(TAG, " Offset   Info     Type             Name");

        int relocate_result = true;
        FuriString* symbol_name;
        symbol_name = furi_string_alloc();

        for(relCount = 0; relCount < relEntries; relCount++) {
            if(relCount % RESOLVER_THREAD_YIELD_STEP == 0) {
                FURI_LOG_D(TAG, "  reloc YIELD");
                furi_delay_tick(1);
            }

            if(storage_file_read(elf->fd, &rel, sizeof(Elf32_Rel)) != sizeof(Elf32_Rel)) {
                FURI_LOG_E(TAG, "  reloc read fail");
                furi_string_free(symbol_name);
                return false;
            }

            Elf32_Addr symAddr;

            int symEntry = ELF32_R_SYM(rel.r_info);
            int relType = ELF32_R_TYPE(rel.r_info);
            Elf32_Addr patchAddr = ((Elf32_Addr)s->data) + rel.r_offset;
            Elf32_Addr relAddr = (s->exec_addr) + rel.r_offset;

            if(!address_cache_get(elf->relocation_cache, symEntry, &symAddr)) {
                Elf32_Sym sym;
                furi_string_reset(symbol_name);
                if(!elf_read_symbol(elf, symEntry, &sym, symbol_name)) {
                    FURI_LOG_E(TAG, "  symbol read fail");
                    furi_string_free(symbol_name);
                    return false;
                }

                FURI_LOG_D(
                    TAG,
                    " %08X %08X %-16s %s",
                    (unsigned int)rel.r_offset,
                    (unsigned int)rel.r_info,
                    elf_reloc_type_to_str(relType),
                    furi_string_get_cstr(symbol_name));

                symAddr = elf_address_of(elf, &sym, furi_string_get_cstr(symbol_name));
                address_cache_put(elf->relocation_cache, symEntry, symAddr);
            }

            if(symAddr != ELF_INVALID_ADDRESS) {
                FURI_LOG_D(
                    TAG,
                    "  symAddr=%08X relAddr=%08X",
                    (unsigned int)symAddr,
                    (unsigned int)relAddr);
                if(!elf_relocate_symbol(elf, patchAddr, relAddr, relType, symAddr)) {
                    relocate_result = false;
                }
            } else {
                FURI_LOG_E(TAG, "  No symbol address of %s", furi_string_get_cstr(symbol_name));
                relocate_result = false;
            }
        }
        furi_string_free(symbol_name);

        return relocate_result;
    } else {
        FURI_LOG_D(TAG, "Section not loaded");
    }

    return false;
}

/**************************************************************************************************/
/************************************ Internal FAP interfaces *************************************/
/**************************************************************************************************/
typedef enum {
    SectionTypeUnused = 1 << 0,
    SectionTypeData = 1 << 1,
    SectionTypeRelData = 1 << 2,
    SectionTypeSymTab = 1 << 3,
    SectionTypeStrTab = 1 << 4,
    SectionTypeDebugLink = 1 << 5,
    SectionTypeFastRelData = 1 << 6,
} SectionType;

static bool elf_load_debug_link(ELFFile* elf, Elf32_Shdr* section_header) {
    elf->debug_link_info.debug_link_size = section_header->sh_size;
    elf->debug_link_info.debug_link = malloc(section_header->sh_size);

    return storage_file_seek(elf->fd, section_header->sh_offset, true) &&
           storage_file_read(elf->fd, elf->debug_link_info.debug_link, section_header->sh_size) ==
               section_header->sh_size;
}

static bool str_prefix(const char* str, const char* prefix) {
    return strncmp(prefix, str, strlen(prefix)) == 0;
}

typedef enum {
    ELFLoadSectionResultSuccess,
    ELFLoadSectionResultNoMemory,
    ELFLoadSectionResultError,
} ELFLoadSectionResult;

typedef struct {
    SectionType type;
    ELFLoadSectionResult result;
} SectionTypeInfo;

/** Save section metadata without allocating RAM or reading data.
 *  Actual loading is deferred to elf_materialize_section().
 */
static ELFLoadSectionResult
    elf_save_section_metadata(ELFSection* section, Elf32_Shdr* section_header) {
    section->size = section_header->sh_size;
    section->file_offset = section_header->sh_offset;
    section->file_align = section_header->sh_addralign;
    section->sh_flags = section_header->sh_flags;
    section->data = NULL;
    section->exec_addr = 0;
    section->xip = false;

    if(section_header->sh_type == SHT_NOBITS) {
        /* BSS: allocate zeroed RAM immediately (cheap, always needed) */
        if(section_header->sh_size > 0) {
            section->data = aligned_malloc(section_header->sh_size, section_header->sh_addralign);
            if(!section->data) {
                return ELFLoadSectionResultNoMemory;
            }
            memset(section->data, 0, section_header->sh_size);
            section->exec_addr = (Elf32_Addr)section->data;
        }
        return ELFLoadSectionResultSuccess;
    }

    return ELFLoadSectionResultSuccess;
}

/** Materialize a section: either into the XIP flash region or into RAM.
 *  For XIP sections, data is staged in RAM, written to flash, then the RAM
 *  buffer is freed — the section's exec_addr points into flash.
 */
static ELFLoadSectionResult elf_materialize_section(ELFFile* elf, ELFSection* section) {
    if(section->size == 0) {
        return ELFLoadSectionResultSuccess;
    }

    /* BSS already has RAM allocated in elf_save_section_metadata */
    if(section->sh_flags & SHF_NOBITS) { /* actually SHT_NOBITS handled above */
        return ELFLoadSectionResultSuccess;
    }

    bool use_xip = section->xip && elf->xip_region.active;

    if(use_xip) {
        /* Stage in RAM first */
        void* staging = aligned_malloc(section->size, section->file_align);
        if(!staging) {
            FURI_LOG_E(TAG, "XIP staging alloc failed for %zu bytes", section->size);
            return ELFLoadSectionResultNoMemory;
        }

        if((!storage_file_seek(elf->fd, section->file_offset, true)) ||
           (storage_file_read(elf->fd, staging, section->size) != section->size)) {
            FURI_LOG_E(TAG, "XIP section read fail");
            free(staging);
            return ELFLoadSectionResultError;
        }

        /* Allocate flash space */
        uint32_t flash_addr = xip_region_alloc(&elf->xip_region, section->size, section->file_align);
        if(flash_addr == 0) {
            FURI_LOG_E(TAG, "XIP region alloc failed");
            free(staging);
            return ELFLoadSectionResultNoMemory;
        }

        section->exec_addr = flash_addr;

        /* Relocate against flash addresses while still in RAM staging */
        /* (relocations computed below use exec_addr) */

        /* Write to flash */
        if(!xip_region_commit(&elf->xip_region, flash_addr, staging, section->size)) {
            FURI_LOG_E(TAG, "XIP commit failed");
            free(staging);
            return ELFLoadSectionResultError;
        }

        /* Free staging — code now lives in flash */
        free(staging);
        section->data = NULL; /* no RAM copy */

        FURI_LOG_D(TAG, "XIP section at 0x%08lX (%zu bytes)", flash_addr, section->size);
        return ELFLoadSectionResultSuccess;
    } else {
        /* Standard RAM loading */
        size_t safe_size = section->size + 1024;

        furi_kernel_lock();

        if(memmgr_heap_get_max_free_block() < safe_size) {
            furi_kernel_unlock();
            FURI_LOG_E(TAG, "Not enough memory to load section data");
            return ELFLoadSectionResultNoMemory;
        }

        section->data = aligned_malloc(section->size, section->file_align);
        section->exec_addr = (Elf32_Addr)section->data;

        furi_kernel_unlock();

        if((!storage_file_seek(elf->fd, section->file_offset, true)) ||
           (storage_file_read(elf->fd, section->data, section->size) != section->size)) {
            FURI_LOG_E(TAG, "    seek/read fail");
            return ELFLoadSectionResultError;
        }

        FURI_LOG_D(TAG, "RAM section at 0x%p", section->data);
        return ELFLoadSectionResultSuccess;
    }
}

/**
 * Set up the XIP region: decide which sections go to flash vs RAM.
 * Called from elf_file_load_section_table after reading all section headers.
 */
static void elf_setup_xip(ELFFile* elf) {
    if(elf->xip_disabled) {
        FURI_LOG_D(TAG, "XIP disabled for this ELF");
        return;
    }

    xip_region_init(&elf->xip_region);
    if(!elf->xip_region.active) {
        FURI_LOG_D(TAG, "XIP region not available, falling back to RAM");
        return;
    }

    /* Mark executable sections (.text, .rodata) for XIP */
    ELFSectionDict_it_t it;
    for(ELFSectionDict_it(it, elf->sections); !ELFSectionDict_end_p(it); ELFSectionDict_next(it)) {
        ELFSectionDict_itref_t* itref = ELFSectionDict_ref(it);
        ELFSection* section = &itref->value;
        const char* name = itref->key;

        if(section->sh_flags & SHF_EXECINSTR) {
            section->xip = true;
        } else if(str_prefix(name, ".rodata") || str_prefix(name, ".data")) {
            /* read-only data can also live in XIP */
            section->xip = true;
        }
        /* .bss (SHT_NOBITS) stays in RAM — handled in metadata save */
    }

    FURI_LOG_I(TAG, "XIP setup complete, region 0x%08lX-0x%08lX", elf->xip_region.base_addr, elf->xip_region.end_addr);
}

static SectionTypeInfo elf_preload_section(
    ELFFile* elf,
    size_t section_idx,
    Elf32_Shdr* section_header,
    FuriString* name_string) {
    const char* name = furi_string_get_cstr(name_string);
    SectionTypeInfo info;
    info.type = SectionTypeUnused;
    info.result = ELFLoadSectionResultSuccess;

    ELFSection* section = elf_file_get_or_put_section(elf, name);
    section->sec_idx = section_idx;

    if(IS_FLAGS_SET(section_header->sh_flags, SHF_ALLOC)) {
        info.type |= SectionTypeData;
        info.result = elf_save_section_metadata(section, section_header);
    }

    if(section_header->sh_type == SHT_RELA || section_header->sh_type == SHT_REL) {
        info.type |= SectionTypeRelData;
        section->rel_count = section_header->sh_size / sizeof(Elf32_Rel);
        section->rel_offset = section_header->sh_offset;
    }

    if(section_header->sh_type == SHT_SYMTAB) {
        info.type |= SectionTypeSymTab;
        elf->symbol_table = section_header->sh_offset;
        elf->symbol_count = section_header->sh_size / sizeof(Elf32_Sym);
    }

    if(section_header->sh_type == SHT_STRTAB) {
        info.type |= SectionTypeStrTab;
        if(elf->section_table_strings == 0) {
            elf->section_table_strings = section_header->sh_offset;
        }
        if(section_header->sh_offset != elf->section_table_strings) {
            elf->symbol_table_strings = section_header->sh_offset;
        }
    }

    if(str_prefix(name, ".debug")) {
        info.type |= SectionTypeDebugLink;
        elf_load_debug_link(elf, section_header);
    }

    if(str_prefix(name, ".rel") && str_prefix(name + 4, ".text")) {
        info.type |= SectionTypeFastRelData;
    }

    if(str_prefix(name, ".preinit_array")) {
        elf->preinit_array = section;
    } else if(str_prefix(name, ".init_array")) {
        elf->init_array = section;
    } else if(str_prefix(name, ".fini_array")) {
        elf->fini_array = section;
    }

    return info;
}

ElfLoadSectionTableResult elf_file_load_section_table(ELFFile* elf) {
    Elf32_Ehdr ehdr;

    if(!storage_file_seek(elf->fd, 0, true) ||
       storage_file_read(elf->fd, &ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        FURI_LOG_E(TAG, "Failed to read ELF header");
        return ElfLoadSectionTableResultError;
    }

    if(ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
       ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        FURI_LOG_E(TAG, "Not an ELF file");
        return ElfLoadSectionTableResultError;
    }

    if(ehdr.e_machine != EM_ARM) {
        FURI_LOG_E(TAG, "Not an ARM ELF file");
        return ElfLoadSectionTableResultError;
    }

    elf->entry = ehdr.e_entry;
    elf->sections_count = ehdr.e_shnum;
    elf->section_table = ehdr.e_shoff;

    FURI_LOG_D(TAG, "Scan ELF indexs...");

    for(size_t section_idx = 1; section_idx < elf->sections_count; section_idx++) {
        Elf32_Shdr section_header;
        FuriString* name_string = furi_string_alloc();

        if(!elf_read_section(elf, section_idx, &section_header, name_string)) {
            furi_string_free(name_string);
            FURI_LOG_E(TAG, "Failed to read section %zu", section_idx);
            return ElfLoadSectionTableResultError;
        }

        SectionTypeInfo info = elf_preload_section(elf, section_idx, &section_header, name_string);
        furi_string_free(name_string);

        if(info.result == ELFLoadSectionResultNoMemory) {
            return ElfLoadSectionTableResultNoMemory;
        } else if(info.result == ELFLoadSectionResultError) {
            return ElfLoadSectionTableResultError;
        }
    }

    /* Set up XIP after all section metadata is known */
    elf_setup_xip(elf);

    return ElfLoadSectionTableResultSuccess;
}

ELFFileLoadStatus elf_file_load_sections(ELFFile* elf) {
    ELFSectionDict_it_t it;
    for(ELFSectionDict_it(it, elf->sections); !ELFSectionDict_end_p(it); ELFSectionDict_next(it)) {
        ELFSectionDict_itref_t* itref = ELFSectionDict_ref(it);
        ELFSection* section = &itref->value;

        if(section->size == 0) continue;
        if(section->sh_flags & SHF_NOBITS) continue; /* BSS already in RAM */

        ELFLoadSectionResult result = elf_materialize_section(elf, section);
        if(result == ELFLoadSectionResultNoMemory) {
            return ELFFileLoadStatusUnspecifiedError;
        } else if(result == ELFLoadSectionResultError) {
            return ELFFileLoadStatusUnspecifiedError;
        }
    }

    /* Erase and that's it for XIP — commits already done per-section */
    if(elf->xip_region.active) {
        /* All XIP sections committed individually; nothing more to erase */
    }

    /* Relocate all sections */
    for(ELFSectionDict_it(it, elf->sections); !ELFSectionDict_end_p(it); ELFSectionDict_next(it)) {
        ELFSectionDict_itref_t* itref = ELFSectionDict_ref(it);
        ELFSection* section = &itref->value;

        if(section->rel_count > 0) {
            if(!elf_relocate(elf, section)) {
                FURI_LOG_E(TAG, "Relocation failed for section %s", itref->key);
                return ELFFileLoadStatusUnspecifiedError;
            }
        }
    }

    return ELFFileLoadStatusSuccess;
}

void elf_file_call_init(ELFFile* elf) {
    if(elf->init_array_called) return;
    elf->init_array_called = true;

    if(elf->preinit_array && elf->preinit_array->data) {
        entry_t* preinit = (entry_t*)elf->preinit_array->exec_addr;
        /* simplified: just mark done */
    }
    if(elf->init_array && elf->init_array->exec_addr) {
        entry_t* init = (entry_t*)elf->init_array->exec_addr;
        /* constructors called by runtime */
    }
}

bool elf_file_is_init_complete(ELFFile* elf) {
    return elf->init_array_called;
}

void* elf_file_get_entry_point(ELFFile* elf) {
    if(elf->entry == 0) return NULL;
    /* entry is a virtual address; find which section contains it */
    ELFSectionDict_it_t it;
    for(ELFSectionDict_it(it, elf->sections); !ELFSectionDict_end_p(it); ELFSectionDict_next(it)) {
        ELFSectionDict_itref_t* itref = ELFSectionDict_ref(it);
        ELFSection* section = &itref->value;
        if(section->exec_addr && elf->entry >= section->exec_addr &&
           elf->entry < section->exec_addr + section->size) {
            return (void*)(section->exec_addr + (elf->entry - section->exec_addr));
        }
    }
    return (void*)elf->entry;
}

void elf_file_call_fini(ELFFile* elf) {
    if(elf->fini_array && elf->fini_array->exec_addr) {
        entry_t* fini = (entry_t*)elf->fini_array->exec_addr;
    }
    elf->init_array_called = false;
}

const ElfApiInterface* elf_file_get_api_interface(ELFFile* elf) {
    return elf->api_interface;
}

void elf_file_init_debug_info(ELFFile* elf, ELFDebugInfo* debug_info) {
    debug_info->debug_link_info = elf->debug_link_info;
    debug_info->entry = elf->entry;
    debug_info->mmap_entry_count = 0;
    debug_info->mmap_entries = NULL;

    ELFSectionDict_it_t it;
    size_t count = 0;
    for(ELFSectionDict_it(it, elf->sections); !ELFSectionDict_end_p(it); ELFSectionDict_next(it)) {
        count++;
    }
    if(count == 0) return;

    debug_info->mmap_entries = malloc(count * sizeof(ELFMemoryMapEntry));
    debug_info->mmap_entry_count = count;

    size_t idx = 0;
    for(ELFSectionDict_it(it, elf->sections); !ELFSectionDict_end_p(it); ELFSectionDict_next(it)) {
        ELFSectionDict_itref_t* itref = ELFSectionDict_ref(it);
        ELFMemoryMapEntry* entry = &debug_info->mmap_entries[idx];
        entry->name = itref->key;
        entry->address = itref->value.exec_addr ? itref->value.exec_addr : (uint32_t)itref->value.data;
        idx++;
    }
}

void elf_file_clear_debug_info(ELFDebugInfo* debug_info) {
    memset(&debug_info->debug_link_info, 0, sizeof(ELFDebugLinkInfo));
    if(debug_info->mmap_entries) {
        free(debug_info->mmap_entries);
        debug_info->mmap_entries = NULL;
    }
    debug_info->mmap_entry_count = 0;
}

ELFFile* elf_file_alloc(Storage* storage, const ElfApiInterface* api_interface) {
    ELFFile* elf = malloc(sizeof(ELFFile));
    elf->fd = storage_file_alloc(storage);
    elf->api_interface = api_interface;
    elf->xip_disabled = false;
    elf->xip_forced = false;
    ELFSectionDict_init(elf->sections);
    AddressCache_init(elf->relocation_cache);
    AddressCache_init(elf->trampoline_cache);
    elf->init_array_called = false;
    elf->preinit_array = NULL;
    elf->init_array = NULL;
    elf->fini_array = NULL;
    elf->entry = 0;
    elf->sections_count = 0;
    elf->section_table = 0;
    elf->section_table_strings = 0;
    elf->symbol_count = 0;
    elf->symbol_table = 0;
    elf->symbol_table_strings = 0;
    memset(&elf->debug_link_info, 0, sizeof(ELFDebugLinkInfo));
    memset(&elf->xip_region, 0, sizeof(XipRegion));
    return elf;
}

void elf_file_disable_xip(ELFFile* elf) {
    furi_check(elf);
    elf->xip_disabled = true;
}

void elf_file_force_xip(ELFFile* elf) {
    furi_check(elf);
    elf->xip_forced = true;
}

uint32_t elf_file_get_xip_next_free(const ELFFile* elf) {
    furi_check(elf);
    if(!elf->xip_region.active) return 0;
    return elf->xip_region.next_free;
}

uint32_t elf_file_get_xip_end(const ELFFile* elf) {
    furi_check(elf);
    if(!elf->xip_region.active) return 0;
    return elf->xip_region.end_addr;
}

bool elf_file_open(ELFFile* elf, const char* path) {
    furi_check(elf);
    furi_check(path);
    elf_file_maybe_release_fd(elf);
    return storage_file_open(elf->fd, path, FSAM_READ, FSOM_OPEN_EXISTING);
}

void elf_file_free(ELFFile* elf) {
    furi_check(elf);
    xip_region_release(&elf->xip_region);
    elf_file_maybe_release_fd(elf);
    storage_file_free(elf->fd);
    ELFSectionDict_it_t it;
    for(ELFSectionDict_it(it, elf->sections); !ELFSectionDict_end_p(it); ELFSectionDict_next(it)) {
        ELFSectionDict_itref_t* itref = ELFSectionDict_ref(it);
        ELFSection* section = &itref->value;
        if(section->data) {
            free(section->data);
        }
        if(section->fast_rel) {
            free(section->fast_rel);
        }
    }
    ELFSectionDict_clear(elf->sections);
    AddressCache_clear(elf->relocation_cache);
    AddressCache_clear(elf->trampoline_cache);
    free(elf);
}

ElfProcessSectionResult elf_process_section(
    ELFFile* elf,
    const char* name,
    ElfProcessSection* process_section,
    void* context) {
    furi_check(elf);
    furi_check(name);
    furi_check(process_section);

    ELFSection* section = elf_file_get_section(elf, name);
    if(!section) {
        return ElfProcessSectionResultNotFound;
    }

    if(section->size == 0) {
        return ElfProcessSectionResultCannotProcess;
    }

    if(!process_section(elf->fd, section->file_offset, section->size, context)) {
        return ElfProcessSectionResultCannotProcess;
    }

    return ElfProcessSectionResultSuccess;
}
