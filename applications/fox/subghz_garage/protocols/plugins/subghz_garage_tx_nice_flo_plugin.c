#include "../subghz_garage_protocol_plugin.h"

/* No gen_ function here - Nice Flo has no "Add Manually" entry, but a
 * captured/saved signal still needs a real encoder to be replayed with
 * "Send". This plugin exists purely so subghz_txrx_tx_start() can find
 * one without it having to be resident in the RX group plugin - see
 * subghz_garage_tx_protocol_for_name() in protocol_groups.c. */
static const SubGhzProtocol* const subghz_garage_tx_nice_flo_registry_items[] = {
    &subghz_protocol_nice_flo,
};

static const SubGhzProtocolRegistry subghz_garage_tx_nice_flo_registry = {
    .items = subghz_garage_tx_nice_flo_registry_items,
    .size = COUNT_OF(subghz_garage_tx_nice_flo_registry_items)};

static const SubGhzGarageProtocolPlugin subghz_garage_tx_nice_flo_plugin = {
    .registry = &subghz_garage_tx_nice_flo_registry,
};

static const FlipperAppPluginDescriptor subghz_garage_tx_nice_flo_plugin_descriptor = {
    .appid = SUBGHZ_GARAGE_PROTOCOL_PLUGIN_APP_ID,
    .ep_api_version = SUBGHZ_GARAGE_PROTOCOL_PLUGIN_API_VERSION,
    .entry_point = &subghz_garage_tx_nice_flo_plugin,
};

const FlipperAppPluginDescriptor* subghz_garage_tx_nice_flo_plugin_ep(void) {
    return &subghz_garage_tx_nice_flo_plugin_descriptor;
}
