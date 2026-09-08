#pragma once
/* lib/subghz/types.h's SubGhzProtocolType enum: ARF/Unleashed/Momentum each
 * carry a SubGhzProtocolTypeBinRAW member in their own copy (value 6, after
 * Unknown/Static/Dynamic/RAW/WeatherStation/Custom); Stock's copy stops at
 * SubGhzProtocolCustom (value 5).
 *
 * Gated by SUBGHZ_GARAGE_HAS_BINRAW_TYPE, set per-fork (not uniformly) by
 * build_all_firmwares.ps1/build_changed_apps.ps1 after copying
 * subghz_garage_COMPATIBLE into each fork's own applications_user -
 * present for ARF/Unleashed/Momentum, stripped for Stock. */

#include <lib/subghz/types.h>

#ifdef SUBGHZ_GARAGE_HAS_BINRAW_TYPE

#define SUBGHZ_GARAGE_TYPE_BIN_RAW SubGhzProtocolTypeBinRAW

#else

/* Stock's enum only defines values 0-5 - 6 is unused there, and is the
 * exact value BinRAW actually holds on the other 3 forks, so BinRAW
 * signals still get a distinct, stable .type/array-index value instead of
 * colliding with a real protocol type. */
#define SUBGHZ_GARAGE_TYPE_BIN_RAW ((SubGhzProtocolType)6)

#endif
