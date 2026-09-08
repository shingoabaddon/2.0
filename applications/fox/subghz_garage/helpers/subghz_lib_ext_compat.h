#pragma once
/* Wraps six small additions FoxFW2.0 makes to stock subghz/furi_hal
 * headers - present in FoxFW2.0's own copies of lib/subghz/environment.h,
 * lib/subghz/subghz_setting.h, lib/subghz/subghz_file_encoder_worker.h and
 * targets/f7/furi_hal/furi_hal_subghz.h, but not in other forks' copies of
 * those same (shared, not app-local) headers - so __has_include can't tell
 * them apart. Gated by SUBGHZ_GARAGE_HAS_LIB_EXTENSIONS, a cdefine set only
 * on FoxFW2.0's own native build (see application.fam; stripped for the
 * _COMPATIBLE cross-fork variant by sync_apps_from_foxfw.ps1). The
 * SubGhzBlockGenericGlobal counter/button override hooks used to live here
 * too, but they're part of lib/subghz/blocks/generic.h's own addition
 * (ARF/Unleashed/Momentum genuinely have it, not just FoxFW2.0) - see
 * subghz_generic_global_compat.h instead, gated per-fork. */

#include <lib/subghz/environment.h>
#include <lib/subghz/subghz_setting.h>
#include <lib/subghz/blocks/generic.h>
#include <lib/subghz/subghz_file_encoder_worker.h>
#include <furi_hal_subghz.h>

#ifdef SUBGHZ_GARAGE_HAS_LIB_EXTENSIONS

static inline void subghz_garage_env_reset_keeloq(SubGhzEnvironment* env) {
    subghz_environment_reset_keeloq(env);
}

static inline void subghz_garage_setting_mark_default_frequency(
    SubGhzSetting* setting, uint32_t frequency) {
    subghz_setting_set_default_frequency(setting, frequency);
}

static inline void subghz_garage_set_rolling_counter_mult(int32_t mult) {
    furi_hal_subghz_set_rolling_counter_mult(mult);
}

static inline int32_t subghz_garage_get_rolling_counter_mult(void) {
    return furi_hal_subghz_get_rolling_counter_mult();
}

static inline void subghz_garage_encoder_get_text_progress(
    SubGhzFileEncoderWorker* worker, FuriString* output) {
    subghz_file_encoder_worker_get_text_progress(worker, output);
}

static inline void subghz_garage_set_ext_leds_and_amp(bool enabled) {
    furi_hal_subghz_set_ext_leds_and_amp(enabled);
}

#else

static inline void subghz_garage_env_reset_keeloq(SubGhzEnvironment* env) {
    UNUSED(env);
    /* No equivalent hook on this fork - nothing else resets the KeeLoq
     * keystore's runtime state outside a full environment reload, so
     * this is just skipped. */
}

static inline void subghz_garage_setting_mark_default_frequency(
    SubGhzSetting* setting, uint32_t frequency) {
    UNUSED(setting);
    UNUSED(frequency);
    /* Cosmetic only (flags a frequency as "default" in the preset list)
     * - skipped; the boot-time default (433.92MHz) still shows
     * correctly, it just won't track later preset switches. */
}

static inline void subghz_garage_set_rolling_counter_mult(int32_t mult) {
    UNUSED(mult);
}

static inline int32_t subghz_garage_get_rolling_counter_mult(void) {
    /* No equivalent hook on this fork - see subghz_garage_set_rolling_counter_mult()
     * above. Must return the real function's "unset" sentinel, not 0 (0 is itself a
     * valid mult value) - protocol encoders check against this sentinel to decide
     * whether to apply Counter BruteForce arithmetic at all. */
    return -0x7FFFFFFF;
}

static inline void subghz_garage_encoder_get_text_progress(
    SubGhzFileEncoderWorker* worker, FuriString* output) {
    UNUSED(worker);
    /* SubGhzFileEncoderWorker's internal stream position isn't exposed
     * by the stock public API, so a real "NN%" readout can't be
     * recomputed here - Decode RAW's progress line just shows this
     * fixed placeholder instead on forks without the extension. */
    furi_string_set(output, "Decoding...");
}

static inline void subghz_garage_set_ext_leds_and_amp(bool enabled) {
    UNUSED(enabled);
    /* External-CC1101 LED/amp GPIO control - no equivalent hook, and no
     * external-module-detection feature to serve on this fork anyway. */
}

#endif
