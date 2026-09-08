#pragma once
/* furi_hal_subghz_is_tx_allowed() (targets/f7/furi_hal/furi_hal_subghz.h) -
 * a per-frequency RX-only-band check, separate from the general frequency-
 * validity check in subghz_setting.h - is declared on FoxFW2.0/ARF/
 * Unleashed/Momentum's own copies of furi_hal_subghz.h, but not Stock's.
 *
 * Gated by SUBGHZ_GARAGE_HAS_TX_ALLOWED_CHECK, set per-fork (not uniformly)
 * by build_all_firmwares.ps1/build_changed_apps.ps1 after copying
 * subghz_garage_COMPATIBLE into each fork's own applications_user -
 * present for ARF/Unleashed/Momentum, stripped for Stock. */

#include <furi_hal_subghz.h>

#ifdef SUBGHZ_GARAGE_HAS_TX_ALLOWED_CHECK

static inline bool subghz_garage_is_tx_allowed(uint32_t frequency) {
    return furi_hal_subghz_is_tx_allowed(frequency);
}

#else

static inline bool subghz_garage_is_tx_allowed(uint32_t frequency) {
    UNUSED(frequency);
    /* No RX-only-band concept on this fork's own furi_hal_subghz - every
     * frequency that already passed the general validity check is
     * treated as TX-allowed too. */
    return true;
}

#endif
