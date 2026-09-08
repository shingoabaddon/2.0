#pragma once
/* Wraps FoxFW2.0's Fox Theme module (gui/modules/fox_theme.h), which isn't
 * present on forks without FoxFW's menu-theme system. Only
 * fox_theme_is_active() is used by this app (subghz_scene_start.c, to pick
 * between the grid view and a classic submenu) - falls back to always
 * false there, so the app just takes its Classic-Theme submenu path. */

#if __has_include(<gui/modules/fox_theme.h>)
#include <gui/modules/fox_theme.h>
#else
#include <stdbool.h>

static inline bool fox_theme_is_active(void) {
    return false;
}
#endif
