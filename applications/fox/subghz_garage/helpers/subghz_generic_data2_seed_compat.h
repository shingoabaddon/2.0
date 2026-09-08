#pragma once
/* SubGhzBlockGeneric (lib/subghz/blocks/generic.h) carries three extra
 * members, data_2 (uint64_t), cnt_2 (uint8_t) and seed (uint32_t), on
 * FoxFW2.0/ARF/Unleashed/Momentum's own copy - used by several rolling-
 * code protocols that need a second data word, second counter, or decrypt
 * seed beyond the base struct's single `data`/`cnt` fields (Jarolift, Nice
 * FlorS, FAAC SLH, Alutech AT-4N, BENINCA ARC, CAME Atomo, KingGates
 * Stylo4K, Somfy Keytis). Absent from Stock's copy.
 *
 * Gated by SUBGHZ_GARAGE_HAS_GENERIC_DATA2_SEED, set per-fork (not
 * uniformly) by build_all_firmwares.ps1/build_changed_apps.ps1 after
 * copying subghz_garage_COMPATIBLE into each fork's own
 * applications_user - present for ARF/Unleashed/Momentum, stripped for
 * Stock.
 *
 * SubGhzGenericCompat mirrors Stock's own SubGhzBlockGeneric field-for-
 * field (protocol_name/data/serial/data_count_bit/btn/cnt) with data_2/
 * cnt_2/seed appended - the protocol files above embed this in place of
 * SubGhzBlockGeneric directly, so every existing .data_2/.cnt_2/.seed
 * access keeps working unmodified. Since the two structs' leading members
 * line up exactly, a pointer to it can still be passed - with an explicit
 * cast - to the three real firmware functions in generic.h that take a
 * SubGhzBlockGeneric*; those only ever touch the shared leading fields. */

#include <lib/subghz/blocks/generic.h>

#ifdef SUBGHZ_GARAGE_HAS_GENERIC_DATA2_SEED

typedef SubGhzBlockGeneric SubGhzGenericCompat;

#else

typedef struct {
    const char* protocol_name;
    uint64_t data;
    uint32_t serial;
    uint16_t data_count_bit;
    uint8_t btn;
    uint32_t cnt;
    uint64_t data_2;
    uint8_t cnt_2;
    uint32_t seed;
} SubGhzGenericCompat;

#endif
