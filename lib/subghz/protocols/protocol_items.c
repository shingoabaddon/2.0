#include "protocol_items.h" // IWYU pragma: keep

const SubGhzProtocol* const subghz_protocol_registry_items[] = {
    // The 18 entries below are commented out on purpose, not accidentally
    // disabled: FoxFW2.0 already ships a dedicated "Garage/Gate/Other" app
    // (applications/fox/subghz_garage, appid subghz_garage) with its own
    // complete, working copy of every one of these protocols. Keeping them
    // registered here as well means paying their full compiled size twice -
    // once in core firmware, once in the external .fap - for zero added
    // capability, since Garage already covers exactly this set. Matches the
    // same trade ARF made in their own firmware. If you want one of these
    // back in the always-on auto-detect list, just uncomment it.
    //&subghz_protocol_gate_tx,
    //&subghz_protocol_keeloq,
    //&subghz_protocol_nice_flo,
    //&subghz_protocol_came,
    //&subghz_protocol_faac_slh,
    //&subghz_protocol_nice_flor_s,
    //&subghz_protocol_came_twee,
    //&subghz_protocol_came_atomo,
    //&subghz_protocol_nero_sketch,
    //&subghz_protocol_ido,
    //&subghz_protocol_hormann,
    //&subghz_protocol_nero_radio,
    //&subghz_protocol_somfy_telis,
    //&subghz_protocol_somfy_keytis,
    // Princeton also shows up in aftermarket automotive central-locking/
    // alarm units (user-confirmed), so it stays active here too.
    &subghz_protocol_princeton,
    // The block below (through keyfinder/x10) is commented out for the same
    // reason as the 18 entries above: FoxFW2.0's external
    // Garage/Gate app (applications/fox/subghz_garage) now covers every one of
    // these non-automotive generic protocols. Read RAW (raw/bin_raw) is the
    // one exception kept active in both core and the external app, since
    // it's a generic capture mode rather than a named protocol.
    &subghz_protocol_raw,
    //&subghz_protocol_linear,
    //&subghz_protocol_secplus_v2,
    //&subghz_protocol_secplus_v1,
    //&subghz_protocol_megacode,
    //&subghz_protocol_holtek,
    //&subghz_protocol_chamb_code,
    //&subghz_protocol_power_smart,
    //&subghz_protocol_marantec,
    //&subghz_protocol_bett,
    //&subghz_protocol_doitrand,
    //&subghz_protocol_phoenix_v2,
    //&subghz_protocol_honeywell_wdb,
    //&subghz_protocol_magellan,
    //&subghz_protocol_intertechno_v3,
    //&subghz_protocol_clemsa,
    //&subghz_protocol_ansonic,
    //&subghz_protocol_smc5326,
    //&subghz_protocol_holtek_th12x,
    //&subghz_protocol_linear_delta3,
    //&subghz_protocol_dooya,
    //&subghz_protocol_alutech_at_4n,
    //&subghz_protocol_kinggates_stylo_4k,
    &subghz_protocol_bin_raw,
    //&subghz_protocol_mastercode,
    //&subghz_protocol_honeywell,
    //&subghz_protocol_legrand,
    //&subghz_protocol_dickert_mahs,
    //&subghz_protocol_gangqi,
    //&subghz_protocol_marantec24,
    //&subghz_protocol_hollarm,
    //&subghz_protocol_hay21,
    //&subghz_protocol_revers_rb2,
    //&subghz_protocol_feron,
    //&subghz_protocol_roger,
    //&subghz_protocol_elplast,
    //&subghz_protocol_telcoma_edge,
    //&subghz_protocol_treadmill37,
    //&subghz_protocol_beninca_arc,
    //&subghz_protocol_keyfinder,
    //&subghz_protocol_jarolift,
    &subghz_protocol_vag,          
    &subghz_protocol_porsche_cayenne,  
    &subghz_protocol_ford_v0,
    &subghz_protocol_psa,
    &subghz_protocol_fiat_spa,       
    &subghz_protocol_fiat_marelli,
 // &subghz_protocol_bmw_cas4,
    &subghz_protocol_subaru, 
    &subghz_protocol_mazda_siemens,
    &subghz_protocol_kia_v0,       
    &subghz_protocol_kia_v1,
    &subghz_protocol_kia_v2,       
    &subghz_protocol_kia_v3_v4,
    &subghz_protocol_kia_v5,       
    &subghz_protocol_kia_v6,
    &subghz_protocol_suzuki, 
    &subghz_protocol_mitsubishi_v0,
    &subghz_protocol_star_line,
    &subghz_protocol_scher_khan,
    &subghz_protocol_sheriff_cfm,
    &subghz_protocol_chrysler,
    &subghz_protocol_kia_v7,
    &subghz_protocol_mazda_v0,
    &honda_static_protocol,
    &honda_v1_protocol,
    &honda_v2_protocol,
    &ford_protocol_v1,
    &ford_protocol_v2,
    &ford_protocol_v3,
    &fiat_protocol_v0,
    &fiat_v1_protocol,
    &fiat_v2_protocol,
    &renault_v0_protocol,
    //&subghz_protocol_land_rover_v0, // ported from ARF; ARF itself leaves this disabled (unverified)
    //&subghz_protocol_toyota,        // ported from ARF; ARF itself leaves this disabled (unverified)
    //&subghz_protocol_x10, // moved to the external Garage/Gate app; not automotive
};

const SubGhzProtocolRegistry subghz_protocol_registry = {
    .items = subghz_protocol_registry_items,
    .size = COUNT_OF(subghz_protocol_registry_items)};
