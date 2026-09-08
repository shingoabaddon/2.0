#include "protocol_groups.h"
#include "protocol_items.h"
#include <storage/storage.h>
#include <string.h>

const char* const subghz_garage_protocol_group_names[SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT] = {
    "General",
    "General 2",
    "Chamberlain (USA)",
    "Linear (USA)",
    "Italian Brands 1",
    "Italian Brands 2",
    "Italian Brands 3",
    "Spain, Russia",
    "Germany",
    "China / Gate Only",
    "Blinds / Shutters",
};

const char* const subghz_garage_protocol_group_paths[SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT] = {
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g1.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g2.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g3.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g4.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g5.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g6.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g7.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g8.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g9.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g10.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_protocols_g11.fal"),
};

/* Keep in sync by hand with each group's registry in
 * protocols/plugins/subghz_garage_protocol_plugin_g1..g11.c - see the
 * comment on this array's declaration in protocol_groups.h for why this
 * can't just be read off the loaded registry. */
const char* const subghz_garage_protocol_group_members[SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT] = {
    "SMC5326, Holtek, Holtek HT12X, Princeton",
    "X10, KeyFinder, KeeLoq",
    "Chamberlain Code, Security+ 1.0, Security+ 2.0",
    "Linear, LinearDelta3, MegaCode",
    "Beninca ARC, CAME, CAME Atomo, CAME TWEE",
    "KingGates Stylo4K, Nice FLO, Nice FloR-S, Phoenix_V2",
    "FAAC SLH, Roger, Doitrand, Telcoma/Cardin EDGE",
    "Alutech AT-4N, Ansonic, Clemsa, Mastercode",
    "Dickert MAHS, Hormann HSM, Marantec, Marantec24",
    "GangQi, GateTX, Hay21, Revers RB2",
    "Somfy Telis, Somfy Keytis, Jarolift, Dooya",
};

const char* const subghz_garage_tx_protocol_paths[SubGhzGarageTxProtocolCount] = {
    APP_ASSETS_PATH("plugins/subghz_garage_tx_alutech_at_4n.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_somfy_telis.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_jarolift.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_nice_flo.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_came_twee.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_secplus_v1.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_smc5326.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_dickert_mahs.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_roger.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_keyfinder.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_secplus_v2.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_princeton.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_gangqi.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_hay21.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_dooya.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_holtek.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_came.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_nice_flor_s.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_beninca_arc.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_marantec.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_holtek_ht12x.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_revers_rb2.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_megacode.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_marantec24.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_faac_slh.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_came_atomo.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_chamb_code.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_mastercode.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_linear.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_linear_delta3.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_gate_tx.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_kinggates_stylo_4k.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_somfy_keytis.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_phoenix_v2.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_clemsa.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_ansonic.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_doitrand.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_hormann.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_x10.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_telcoma_edge.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_raw.fal"),
    APP_ASSETS_PATH("plugins/subghz_garage_tx_bin_raw.fal"),
};

typedef struct {
    const char* name;
    SubGhzGarageTxProtocol tx_protocol;
} SubGhzGarageTxProtocolMapEntry;

static const SubGhzGarageTxProtocolMapEntry subghz_garage_tx_protocol_map[] = {
    {SUBGHZ_PROTOCOL_ALUTECH_AT_4N_NAME, SubGhzGarageTxProtocolAlutechAt4n},
    {SUBGHZ_PROTOCOL_SOMFY_TELIS_NAME, SubGhzGarageTxProtocolSomfyTelis},
    {SUBGHZ_PROTOCOL_JAROLIFT_NAME, SubGhzGarageTxProtocolJarolift},
    {SUBGHZ_PROTOCOL_NICE_FLO_NAME, SubGhzGarageTxProtocolNiceFlo},
    {SUBGHZ_PROTOCOL_CAME_TWEE_NAME, SubGhzGarageTxProtocolCameTwee},
    {SUBGHZ_PROTOCOL_SECPLUS_V1_NAME, SubGhzGarageTxProtocolSecPlusV1},
    {SUBGHZ_PROTOCOL_SMC5326_NAME, SubGhzGarageTxProtocolSmc5326},
    {SUBGHZ_PROTOCOL_DICKERT_MAHS_NAME, SubGhzGarageTxProtocolDickertMahs},
    {SUBGHZ_PROTOCOL_ROGER_NAME, SubGhzGarageTxProtocolRoger},
    {SUBGHZ_PROTOCOL_KEYFINDER_NAME, SubGhzGarageTxProtocolKeyFinder},
    {SUBGHZ_PROTOCOL_SECPLUS_V2_NAME, SubGhzGarageTxProtocolSecPlusV2},
    {SUBGHZ_PROTOCOL_PRINCETON_NAME, SubGhzGarageTxProtocolPrinceton},
    {SUBGHZ_PROTOCOL_GANGQI_NAME, SubGhzGarageTxProtocolGangQi},
    {SUBGHZ_PROTOCOL_HAY21_NAME, SubGhzGarageTxProtocolHay21},
    {SUBGHZ_PROTOCOL_DOOYA_NAME, SubGhzGarageTxProtocolDooya},
    {SUBGHZ_PROTOCOL_HOLTEK_NAME, SubGhzGarageTxProtocolHoltek},
    {SUBGHZ_PROTOCOL_CAME_NAME, SubGhzGarageTxProtocolCame},
    {SUBGHZ_PROTOCOL_NICE_FLOR_S_NAME, SubGhzGarageTxProtocolNiceFlorS},
    {SUBGHZ_PROTOCOL_BENINCA_ARC_NAME, SubGhzGarageTxProtocolBenincaArc},
    {SUBGHZ_PROTOCOL_MARANTEC_NAME, SubGhzGarageTxProtocolMarantec},
    {SUBGHZ_PROTOCOL_HOLTEK_HT12X_NAME, SubGhzGarageTxProtocolHoltekHt12x},
    {SUBGHZ_PROTOCOL_REVERSRB2_NAME, SubGhzGarageTxProtocolReversRb2},
    {SUBGHZ_PROTOCOL_MEGACODE_NAME, SubGhzGarageTxProtocolMegaCode},
    {SUBGHZ_PROTOCOL_MARANTEC24_NAME, SubGhzGarageTxProtocolMarantec24},
    {SUBGHZ_PROTOCOL_FAAC_SLH_NAME, SubGhzGarageTxProtocolFaacSlh},
    {SUBGHZ_PROTOCOL_CAME_ATOMO_NAME, SubGhzGarageTxProtocolCameAtomo},
    {SUBGHZ_PROTOCOL_CHAMB_CODE_NAME, SubGhzGarageTxProtocolChambCode},
    {SUBGHZ_PROTOCOL_MASTERCODE_NAME, SubGhzGarageTxProtocolMastercode},
    {SUBGHZ_PROTOCOL_LINEAR_NAME, SubGhzGarageTxProtocolLinear},
    {SUBGHZ_PROTOCOL_LINEAR_DELTA3_NAME, SubGhzGarageTxProtocolLinearDelta3},
    {SUBGHZ_PROTOCOL_GATE_TX_NAME, SubGhzGarageTxProtocolGateTx},
    {SUBGHZ_PROTOCOL_KINGGATES_STYLO_4K_NAME, SubGhzGarageTxProtocolKingGatesStylo4k},
    {SUBGHZ_PROTOCOL_SOMFY_KEYTIS_NAME, SubGhzGarageTxProtocolSomfyKeytis},
    {SUBGHZ_PROTOCOL_PHOENIX_V2_NAME, SubGhzGarageTxProtocolPhoenixV2},
    {SUBGHZ_PROTOCOL_CLEMSA_NAME, SubGhzGarageTxProtocolClemsa},
    {SUBGHZ_PROTOCOL_ANSONIC_NAME, SubGhzGarageTxProtocolAnsonic},
    {SUBGHZ_PROTOCOL_DOITRAND_NAME, SubGhzGarageTxProtocolDoitrand},
    {SUBGHZ_PROTOCOL_HORMANN_HSM_NAME, SubGhzGarageTxProtocolHormann},
    {SUBGHZ_PROTOCOL_X10_NAME, SubGhzGarageTxProtocolX10},
    {SUBGHZ_PROTOCOL_TELCOMA_EDGE_NAME, SubGhzGarageTxProtocolTelcomaEdge},
    {SUBGHZ_PROTOCOL_RAW_NAME, SubGhzGarageTxProtocolRaw},
    {SUBGHZ_PROTOCOL_BIN_RAW_NAME, SubGhzGarageTxProtocolBinRaw},
};

bool subghz_garage_tx_protocol_for_name(
    const char* protocol_name,
    SubGhzGarageTxProtocol* out_tx_protocol) {
    for(size_t i = 0; i < COUNT_OF(subghz_garage_tx_protocol_map); i++) {
        if(strcmp(subghz_garage_tx_protocol_map[i].name, protocol_name) == 0) {
            *out_tx_protocol = subghz_garage_tx_protocol_map[i].tx_protocol;
            return true;
        }
    }
    return false;
}
