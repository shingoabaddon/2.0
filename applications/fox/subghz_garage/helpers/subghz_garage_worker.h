#pragma once

#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Garage's own fork of lib/subghz/subghz_worker.c/.h - identical behavior,
 * except the RX ring buffer is sized smaller (see subghz_garage_worker.c)
 * to leave more heap free for loading a protocol group's .fal plugin at RX
 * start. The shared lib/subghz/subghz_worker.h used by Automotive and every
 * other Sub-GHz app is untouched. */

typedef struct SubGhzGarageWorker SubGhzGarageWorker;

typedef void (*SubGhzGarageWorkerOverrunCallback)(void* context);

typedef void (*SubGhzGarageWorkerPairCallback)(void* context, bool level, uint32_t duration);

void subghz_garage_worker_rx_callback(bool level, uint32_t duration, void* context);

/**
 * Allocate SubGhzGarageWorker.
 * @return SubGhzGarageWorker* Pointer to a SubGhzGarageWorker instance
 */
SubGhzGarageWorker* subghz_garage_worker_alloc(void);

/**
 * Free SubGhzGarageWorker.
 * @param instance Pointer to a SubGhzGarageWorker instance
 */
void subghz_garage_worker_free(SubGhzGarageWorker* instance);

/**
 * Overrun callback SubGhzGarageWorker.
 * @param instance Pointer to a SubGhzGarageWorker instance
 * @param callback SubGhzGarageWorkerOverrunCallback callback
 */
void subghz_garage_worker_set_overrun_callback(
    SubGhzGarageWorker* instance,
    SubGhzGarageWorkerOverrunCallback callback);

/**
 * Pair callback SubGhzGarageWorker.
 * @param instance Pointer to a SubGhzGarageWorker instance
 * @param callback SubGhzGarageWorkerOverrunCallback callback
 */
void subghz_garage_worker_set_pair_callback(
    SubGhzGarageWorker* instance,
    SubGhzGarageWorkerPairCallback callback);

/**
 * Context callback SubGhzGarageWorker.
 * @param instance Pointer to a SubGhzGarageWorker instance
 * @param context
 */
void subghz_garage_worker_set_context(SubGhzGarageWorker* instance, void* context);

/**
 * Start SubGhzGarageWorker.
 * @param instance Pointer to a SubGhzGarageWorker instance
 */
void subghz_garage_worker_start(SubGhzGarageWorker* instance);

/** Stop SubGhzGarageWorker
 * @param instance Pointer to a SubGhzGarageWorker instance
 */
void subghz_garage_worker_stop(SubGhzGarageWorker* instance);

/**
 * Check if worker is running.
 * @param instance Pointer to a SubGhzGarageWorker instance
 * @return bool - true if running
 */
bool subghz_garage_worker_is_running(SubGhzGarageWorker* instance);

/**
 * Short duration filter setting.
 * glues short durations into 1. The default setting is 30 us, if set to 0 the filter will be disabled
 * @param instance Pointer to a SubGhzGarageWorker instance
 * @param timeout time in us
 */
void subghz_garage_worker_set_filter(SubGhzGarageWorker* instance, uint16_t timeout);

#ifdef __cplusplus
}
#endif
