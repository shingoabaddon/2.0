#pragma once
/* memmgr_pool_get_free()/memmgr_pool_get_max_block() (furi/core/memmgr.h)
 * are declared identically on every fork's own copy of memmgr.h, but their
 * implementations are disabled in ARF/Unleashed/Momentum/Stock's compiled
 * firmware (api_symbols.csv marks both "-" on all four) - only FoxFW2.0's
 * own build actually exports them. Referencing them directly compiles
 * fine but fails the cross-fork SDK's final "app may not be runnable"
 * symbol-resolution check at build time.
 *
 * Gated by SUBGHZ_GARAGE_HAS_MEMMGR_POOL_STATS, a cdefine set only on
 * FoxFW2.0's own native build (see application.fam; stripped for the
 * _COMPATIBLE cross-fork variant by sync_apps_from_foxfw.ps1) - same
 * uniform-strip shape as SUBGHZ_GARAGE_HAS_LIB_EXTENSIONS. */

#include <furi/core/memmgr.h>

#ifdef SUBGHZ_GARAGE_HAS_MEMMGR_POOL_STATS

static inline size_t subghz_garage_pool_get_free(void) {
    return memmgr_pool_get_free();
}

static inline size_t subghz_garage_pool_get_max_block(void) {
    return memmgr_pool_get_max_block();
}

#else

static inline size_t subghz_garage_pool_get_free(void) {
    /* Diagnostic-log-only fields on this fork - the FURI_LOG/debug-log
     * pool readings just show 0 instead of a real pool-fragmentation
     * figure. memmgr_get_free_heap()/memmgr_heap_get_max_free_block()
     * (logged alongside these in the same lines) are unaffected and still
     * real - no actual app behavior reads these values. */
    return 0;
}

static inline size_t subghz_garage_pool_get_max_block(void) {
    return 0;
}

#endif
