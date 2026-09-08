#pragma once
/* FoxFW2.0's lib/toolbox/name_generator.h carries name_generator_make_auto_
 * datetime() (and the _detailed_ counterpart, unused here) alongside the
 * base name_generator_make_auto() - lets a caller stamp a generated
 * filename with a specific DateTime instead of always using "now". ARF/
 * Unleashed/Momentum each carry the same addition in their own lib/
 * toolbox; Stock's copy only has the base function.
 *
 * Gated by SUBGHZ_GARAGE_HAS_NAME_GEN_DATETIME, set per-fork (not
 * uniformly) by build_all_firmwares.ps1/build_changed_apps.ps1 after
 * copying subghz_garage_COMPATIBLE into each fork's own applications_user -
 * present for ARF/Unleashed/Momentum, stripped for Stock. */

#include <toolbox/name_generator.h>

#ifdef SUBGHZ_GARAGE_HAS_NAME_GEN_DATETIME

static inline void subghz_garage_name_generator_make_auto_datetime(
    char* name,
    size_t max_name_size,
    const char* prefix,
    DateTime* custom_time) {
    name_generator_make_auto_datetime(name, max_name_size, prefix, custom_time);
}

#else

static inline void subghz_garage_name_generator_make_auto_datetime(
    char* name,
    size_t max_name_size,
    const char* prefix,
    DateTime* custom_time) {
    /* No equivalent hook on this fork - the saved-file name always uses
     * the current time instead of the signal's actual capture time
     * (subghz_scene_receiver_info.c's save_datetime). Every other part of
     * Save is unaffected. */
    UNUSED(custom_time);
    name_generator_make_auto(name, max_name_size, prefix);
}

#endif
