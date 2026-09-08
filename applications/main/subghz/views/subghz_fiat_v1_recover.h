#pragma once

#include <gui/view.h>
#include "../helpers/subghz_custom_event.h"

typedef struct SubGhzViewFiatV1Recover SubGhzViewFiatV1Recover;

typedef void (*SubGhzViewFiatV1RecoverCallback)(SubGhzCustomEvent event, void* context);

SubGhzViewFiatV1Recover* subghz_view_fiat_v1_recover_alloc(void);
void subghz_view_fiat_v1_recover_free(SubGhzViewFiatV1Recover* instance);
View* subghz_view_fiat_v1_recover_get_view(SubGhzViewFiatV1Recover* instance);

void subghz_view_fiat_v1_recover_set_callback(
    SubGhzViewFiatV1Recover* instance,
    SubGhzViewFiatV1RecoverCallback callback,
    void* context);

void subghz_view_fiat_v1_recover_update_stats(
    SubGhzViewFiatV1Recover* instance,
    uint8_t progress,
    uint32_t combos_tested,
    uint32_t combos_per_sec,
    uint32_t elapsed_sec,
    uint32_t eta_sec);

void subghz_view_fiat_v1_recover_reset(SubGhzViewFiatV1Recover* instance);
