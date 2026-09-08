#pragma once
/* lib/subghz/types.h's SubGhzProtocolType enum: FoxFW2.0 added
 * SubGhzProtocolTypeTpms (value 7), the newest member, right after
 * SubGhzProtocolTypeBinRAW (6). ARF/Unleashed/Momentum's own copies of
 * this enum stop at BinRAW=6; Stock's stops at SubGhzProtocolCustom=5.
 * None of the 4 target forks use value 7 for anything, so this literal
 * proxy is correct cross-fork with no per-fork cdefine gating needed -
 * same trick as subghz_garage's SUBGHZ_GARAGE_TYPE_BIN_RAW. */

#include <lib/subghz/types.h>

#define FOX_TPMS_PROTOCOL_TYPE ((SubGhzProtocolType)7)
