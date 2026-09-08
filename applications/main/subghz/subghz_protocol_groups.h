#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file subghz_protocol_groups.h
 *
 * Purely a UI grouping for the Protocol List screen (manufacturer/family) -
 * unlike subghz_garage's protocol_groups.h, this has nothing to do with
 * memory/plugin loading. Automotive's registry is small enough to stay
 * fully resident, so this only exists to organize the toggle list and give
 * each group a master ON/OFF switch.
 */

#define SUBGHZ_PROTOCOL_GROUP_COUNT 17

extern const char* const subghz_protocol_group_names[SUBGHZ_PROTOCOL_GROUP_COUNT];

/**
 * Look up which group a protocol belongs to by its SubGhzProtocol.name.
 * @param protocol_name e.g. "KIA/HYU V0"
 * @return Index into subghz_protocol_group_names, or the "Other" group if
 *         the name isn't recognized.
 */
size_t subghz_protocol_group_for_name(const char* protocol_name);

#ifdef __cplusplus
}
#endif
