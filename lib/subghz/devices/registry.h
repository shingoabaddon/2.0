#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SubGhzDevice SubGhzDevice;

void subghz_device_registry_init(void);

/**
 * Same as subghz_device_registry_init(), but registers only the built-in
 * internal CC1101 device - skips scanning/loading external radio device
 * .fal plugins (e.g. radio_device_cc1101_ext.fal). For callers that are
 * themselves memory-constrained external apps and only need the external
 * module on demand - see subghz_device_registry_load_external().
 */
void subghz_device_registry_init_internal_only(void);

/**
 * Scans and loads any external radio device .fal plugins not already
 * loaded, appending them to an already-initialized registry. Safe to call
 * more than once. Returns false if the plugin scan itself failed (device
 * count is unaffected either way).
 */
bool subghz_device_registry_load_external(void);

void subghz_device_registry_deinit(void);

bool subghz_device_registry_is_valid(void);

/**
 * Registration by name SubGhzDevice.
 * @param name SubGhzDevice name
 * @return SubGhzDevice* pointer to a SubGhzDevice instance
 */
const SubGhzDevice* subghz_device_registry_get_by_name(const char* name);

/**
 * Registration subghzdevice by index in array SubGhzDevice.
 * @param index SubGhzDevice by index in array
 * @return SubGhzDevice* pointer to a SubGhzDevice instance
 */
const SubGhzDevice* subghz_device_registry_get_by_index(size_t index);

/**
 * Getting the number of registered subghzdevices.
 * @param subghz_device SubGhzDeviceRegistry
 * @return Number of subghzdevices
 */
size_t subghz_device_registry_count(void);

#ifdef __cplusplus
}
#endif
