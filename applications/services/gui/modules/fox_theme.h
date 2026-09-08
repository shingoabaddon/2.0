#pragma once
/* Menu style: 0=Classic, 1=Fox Theme, 2=Carousel, 3=Slider, 4=Tiny (see
 * MenuTheme, desktop_settings.h). is_active() is true for non-Classic; only
 * gui/modules/menu.c needs the raw value, via get_style(). Persisted to
 * /int/Fox.cfg. */

#include <stdbool.h>
#include <stdint.h>

#define FOX_THEME_FILE  "/int/Fox.cfg"

#ifdef __cplusplus
extern "C" {
#endif

bool fox_theme_is_active(void);
uint8_t fox_theme_get_style(void);
void fox_theme_set(bool active);
void fox_theme_set_style(uint8_t style);

#ifdef __cplusplus
}
#endif
