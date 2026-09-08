#pragma once

#include <lib/subghz/devices/devices.h>

/* subghz_devices_init_internal_only()/subghz_devices_load_external() are
 * FoxFW2.0's own split of the stock subghz_devices_init() into two steps
 * (internal radio only, then optionally load the external CC1101 .fal
 * plugin later) - a lazy-loading optimization tied to the CC1101
 * external-module-detection feature (see SubGhz_Garage_cc1101_check.c and
 * desktop.c's periodic probe). Same header (devices.h) exists on every
 * fork, but only FoxFW2.0's copy declares these two - so __has_include
 * can't tell them apart; gated by SUBGHZ_GARAGE_HAS_DEVICES_LAZY_INIT
 * instead, a cdefine set only on FoxFW2.0's own native build (see
 * application.fam; stripped for the _COMPATIBLE cross-fork variant by
 * sync_apps_from_foxfw.ps1). Without it, subghz_devices_init() (the
 * universal stock call - a single eager init, no lazy split) is used
 * instead, and "load external" is simply never available - there's no
 * CC1101 external-detection feature on these forks to preserve anyway, so
 * this always leaves the internal radio active, same as Garage behaved
 * before that feature existed. */

#ifdef SUBGHZ_GARAGE_HAS_DEVICES_LAZY_INIT

static inline void subghz_garage_devices_init_radio_only(void) {
    subghz_devices_init_internal_only();
}

static inline bool subghz_garage_devices_load_external(void) {
    return subghz_devices_load_external();
}

#else

static inline void subghz_garage_devices_init_radio_only(void) {
    subghz_devices_init();
}

static inline bool subghz_garage_devices_load_external(void) {
    return false;
}

#endif
