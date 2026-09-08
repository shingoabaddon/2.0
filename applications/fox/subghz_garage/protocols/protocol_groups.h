#pragma once
#include <lib/subghz/registry.h>

/* The 40 non-automotive protocols (plus Read RAW/raw+bin_raw, duplicated in
 * every group) are split across 11 plugins instead of one, because the
 * combined plugin can't be resident in RAM1 alongside the main .fap. Only
 * one group is ever loaded at a time - "Read" listens against whichever
 * group is currently selected (see subghz_txrx_set_protocol_group). Groups
 * are organized by manufacturer/region/device-type, capped at 4 protocols
 * each, so same-brand protocols stay together, users can find what they're
 * looking for, and each group's loaded-plugin RAM cost stays small - see
 * subghz_garage_protocol_group_names in protocol_groups.c for what's in
 * each one. */
#define SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT 11

typedef enum {
    SubGhzGarageProtocolGroup1 = 0,
    SubGhzGarageProtocolGroup2 = 1,
    SubGhzGarageProtocolGroup3 = 2,
    SubGhzGarageProtocolGroup4 = 3,
    SubGhzGarageProtocolGroup5 = 4,
    SubGhzGarageProtocolGroup6 = 5,
    SubGhzGarageProtocolGroup7 = 6,
    SubGhzGarageProtocolGroup8 = 7,
    SubGhzGarageProtocolGroup9 = 8,
    SubGhzGarageProtocolGroup10 = 9,
    SubGhzGarageProtocolGroup11 = 10,
} SubGhzGarageProtocolGroup;

extern const char* const subghz_garage_protocol_group_names[SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT];
extern const char* const subghz_garage_protocol_group_paths[SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT];

/* Comma-separated protocol names in each group, for display (e.g. the
 * Protocols list scene) without having to load that group's .fal plugin
 * just to read its member names - only one group's plugin is ever resident
 * at a time, so listing all 11 groups' contents can't go through the actual
 * loaded SubGhzProtocolRegistry. Keep in sync with the registries in
 * protocols/plugins/subghz_garage_protocol_plugin_g1..g11.c by hand. */
extern const char* const subghz_garage_protocol_group_members[SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT];

/* Registries, one per group - each group's plugin exports its own. */
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g1;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g2;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g3;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g4;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g5;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g6;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g7;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g8;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g9;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g10;
extern const SubGhzProtocolRegistry subghz_garage_protocol_registry_g11;

/* Protocols with a real dedicated encoder (as opposed to the generic
 * gen_data_protocol path) get their own tiny single-protocol TX plugin
 * instead of bundling their encoder code into the always-loaded-for-RX
 * group plugin - the RX group plugin is compiled RX_ONLY (no encoders at
 * all), so replaying a captured/saved signal with "Send" loads one of
 * these instead, on demand - see subghz_txrx_ensure_tx_protocol_plugin()
 * in helpers/subghz_txrx.c. This list is independent of which RX group a
 * protocol is grouped into above. KeeLoq isn't here - it has no real
 * encoder (unknown manufacturer key), so it can't be sent at all. */
typedef enum {
    SubGhzGarageTxProtocolAlutechAt4n = 0,
    SubGhzGarageTxProtocolSomfyTelis,
    SubGhzGarageTxProtocolJarolift,
    SubGhzGarageTxProtocolNiceFlo,
    SubGhzGarageTxProtocolCameTwee,
    SubGhzGarageTxProtocolSecPlusV1,
    SubGhzGarageTxProtocolSmc5326,
    SubGhzGarageTxProtocolDickertMahs,
    SubGhzGarageTxProtocolRoger,
    SubGhzGarageTxProtocolKeyFinder,
    SubGhzGarageTxProtocolSecPlusV2,
    SubGhzGarageTxProtocolPrinceton,
    SubGhzGarageTxProtocolGangQi,
    SubGhzGarageTxProtocolHay21,
    SubGhzGarageTxProtocolDooya,
    SubGhzGarageTxProtocolHoltek,
    SubGhzGarageTxProtocolCame,
    SubGhzGarageTxProtocolNiceFlorS,
    SubGhzGarageTxProtocolBenincaArc,
    SubGhzGarageTxProtocolMarantec,
    SubGhzGarageTxProtocolHoltekHt12x,
    SubGhzGarageTxProtocolReversRb2,
    SubGhzGarageTxProtocolMegaCode,
    SubGhzGarageTxProtocolMarantec24,
    SubGhzGarageTxProtocolFaacSlh,
    SubGhzGarageTxProtocolCameAtomo,
    SubGhzGarageTxProtocolChambCode,
    SubGhzGarageTxProtocolMastercode,
    SubGhzGarageTxProtocolLinear,
    SubGhzGarageTxProtocolLinearDelta3,
    SubGhzGarageTxProtocolGateTx,
    SubGhzGarageTxProtocolKingGatesStylo4k,
    SubGhzGarageTxProtocolSomfyKeytis,
    SubGhzGarageTxProtocolPhoenixV2,
    SubGhzGarageTxProtocolClemsa,
    SubGhzGarageTxProtocolAnsonic,
    SubGhzGarageTxProtocolDoitrand,
    SubGhzGarageTxProtocolHormann,
    SubGhzGarageTxProtocolX10,
    SubGhzGarageTxProtocolTelcomaEdge,
    /* RAW and BinRAW are compiled into every group's plugin (raw.c/
     * bin_raw.c are core protocols, duplicated across groups 1-8), so
     * stripping their encoders saves space in all 8 groups at once. */
    SubGhzGarageTxProtocolRaw,
    SubGhzGarageTxProtocolBinRaw,
    SubGhzGarageTxProtocolCount,
} SubGhzGarageTxProtocol;

extern const char* const subghz_garage_tx_protocol_paths[SubGhzGarageTxProtocolCount];

/* True (with *out_tx_protocol set) if protocol_name is one of the ones
 * split into its own TX plugin above - used by subghz_txrx_tx_start() to
 * route an actual "Send" of a saved signal through the right registry. */
bool subghz_garage_tx_protocol_for_name(
    const char* protocol_name,
    SubGhzGarageTxProtocol* out_tx_protocol);
