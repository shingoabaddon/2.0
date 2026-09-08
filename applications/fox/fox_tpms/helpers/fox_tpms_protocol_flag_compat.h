#pragma once
/* lib/subghz/types.h's SubGhzProtocolFlag enum: FoxFW2.0/ARF/Unleashed all
 * carry SubGhzProtocolFlag_Sensors at the same value (1 << 13). Stock's and
 * Momentum's copies both stop their SubGhzProtocolFlag enum at
 * BinRAW = (1 << 10) - bit 13 is unused on both, so this literal proxy is
 * correct cross-fork with no per-fork cdefine gating needed, same trick as
 * fox_tpms_type_compat.h's FOX_TPMS_PROTOCOL_TYPE. (Momentum separately
 * defines an unrelated SubGhzProtocolFilter enum with its own
 * Filter_Sensors member - different type, different purpose, not a
 * substitute for this flag.) */

#include <lib/subghz/types.h>

#define FOX_TPMS_FLAG_SENSORS ((SubGhzProtocolFlag)(1 << 13))
