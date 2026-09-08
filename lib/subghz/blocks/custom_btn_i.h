#pragma once

#include "custom_btn.h"

#define PROG_MODE_OFF              (0U)
#define PROG_MODE_KEELOQ_BFT       (1U)
#define PROG_MODE_KEELOQ_APRIMATIC (2U)
#define PROG_MODE_KEELOQ_DEA_MIO   (3U)

typedef uint8_t ProgMode;

/* set_original()/set_max() moved to the public custom_btn.h - protocol
 * files compiled as external .fal plugins need them SDK-exported (see
 * targets/f7/api_symbols.csv), and they're genuinely public API (every
 * decoder that supports button remapping calls them), not internal-only. */

void subghz_custom_btn_set_prog_mode(ProgMode prog_mode);

ProgMode subghz_custom_btn_get_prog_mode(void);
