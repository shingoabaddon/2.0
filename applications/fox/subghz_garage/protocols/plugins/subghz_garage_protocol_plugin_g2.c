#include "../subghz_garage_protocol_plugin.h"
#include "../protocol_groups.h"

/* Group 2 - General 2: the two protocols that aren't gate/garage remotes at
 * all (KeyFinder, X10), plus KeeLoq - it has no real encoder
 * (SubGhzProtocolFlag_Send isn't set - its manufacturer key is unknown so
 * it can't be re-transmitted), so it costs nothing extra to include here. */
static const SubGhzProtocol* const subghz_garage_protocol_registry_g2_items[] = {
    &subghz_protocol_raw,
    &subghz_protocol_bin_raw,
    &subghz_protocol_x10,
    &subghz_protocol_keyfinder,
    &subghz_protocol_keeloq,
};

const SubGhzProtocolRegistry subghz_garage_protocol_registry_g2 = {
    .items = subghz_garage_protocol_registry_g2_items,
    .size = COUNT_OF(subghz_garage_protocol_registry_g2_items)};

static const SubGhzGarageProtocolPlugin subghz_garage_protocol_plugin_g2 = {
    .registry = &subghz_garage_protocol_registry_g2,
};

static const FlipperAppPluginDescriptor subghz_garage_protocol_plugin_g2_descriptor = {
    .appid = SUBGHZ_GARAGE_PROTOCOL_PLUGIN_APP_ID,
    .ep_api_version = SUBGHZ_GARAGE_PROTOCOL_PLUGIN_API_VERSION,
    .entry_point = &subghz_garage_protocol_plugin_g2,
};

const FlipperAppPluginDescriptor* subghz_garage_protocol_plugin_g2_ep(void) {
    return &subghz_garage_protocol_plugin_g2_descriptor;
}
