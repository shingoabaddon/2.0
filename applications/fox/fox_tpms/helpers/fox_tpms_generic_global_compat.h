#pragma once
/* lib/subghz/blocks/generic.h's SubGhzBlockGenericGlobal.endless_tx field
 * (hold-to-repeat TX flag, set by the transmitter UI, read by protocol
 * yield functions to skip decrementing the repeat counter so TX continues
 * until the user releases OK). ARF/Unleashed/Momentum each carry the same
 * struct in their own copy of that shared header - not a port of
 * FoxFW2.0's copy, so no drift risk in using it directly (same situation
 * as subghz_garage's subghz_generic_global_compat.h). Stock's copy has no
 * equivalent at all.
 *
 * Gated by FOX_TPMS_HAS_GENERIC_GLOBAL, set per-fork by
 * build_all_firmwares.ps1/build_changed_apps.ps1 after copying
 * fox_tpms_COMPATIBLE into each fork's own applications_user - present for
 * ARF/Unleashed/Momentum, stripped for Stock. */

#ifdef FOX_TPMS_HAS_GENERIC_GLOBAL

#include <lib/subghz/blocks/generic.h>

#define FOX_TPMS_ENDLESS_TX (subghz_block_generic_global.endless_tx)

#else

/* Stock has no endless-TX hold-to-repeat feature at all, so this always
 * reads false - fox_tpms's encoder falls back to its normal bounded-repeat
 * send. */
#define FOX_TPMS_ENDLESS_TX (false)

#endif
