#pragma once
/* variable_item_set_locked() (applications/services/gui/modules/
 * variable_item_list.h) - greys out a VariableItemList row and shows a
 * message instead of dispatching its enter callback when OK is pressed.
 * ARF/Unleashed/Momentum each carry it in their own gui modules; Stock's
 * copy doesn't.
 *
 * Gated by SUBGHZ_GARAGE_HAS_ITEM_LOCK, set per-fork (not uniformly) by
 * build_all_firmwares.ps1/build_changed_apps.ps1 after copying
 * subghz_garage_COMPATIBLE into each fork's own applications_user -
 * present for ARF/Unleashed/Momentum, stripped for Stock. */

#include <gui/modules/variable_item_list.h>

#ifdef SUBGHZ_GARAGE_HAS_ITEM_LOCK

static inline void subghz_garage_variable_item_set_locked(
    VariableItem* item,
    bool locked,
    const char* locked_message) {
    variable_item_set_locked(item, locked, locked_message);
}

#else

static inline void subghz_garage_variable_item_set_locked(
    VariableItem* item,
    bool locked,
    const char* locked_message) {
    /* No equivalent hook on this fork - the row stays selectable even
     * when "not available for this protocol" instead of showing greyed
     * out. subghz_scene_signal_settings.c's enter callback separately
     * guards against acting on a NULL counter/button buffer, so this is
     * a cosmetic-only gap, not a functional one. */
    UNUSED(item);
    UNUSED(locked);
    UNUSED(locked_message);
}

#endif
