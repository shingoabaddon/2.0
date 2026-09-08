#include "../subghz_garage_protocol_plugin.h"

/* Replay-only TX plugin - see subghz_garage_tx_nice_flo_plugin.c's comment.
 * RAW's encoder pulls in the whole SubGhzFileEncoderWorker chain, so
 * stripping it out of every group plugin (it's duplicated in all 4, since
 * raw.c is compiled into each) is worth more than a single-protocol split
 * usually is. */
static const SubGhzProtocol* const subghz_garage_tx_raw_registry_items[] = {
    &subghz_protocol_raw,
};

static const SubGhzProtocolRegistry subghz_garage_tx_raw_registry = {
    .items = subghz_garage_tx_raw_registry_items,
    .size = COUNT_OF(subghz_garage_tx_raw_registry_items)};

static const SubGhzGarageProtocolPlugin subghz_garage_tx_raw_plugin = {
    .registry = &subghz_garage_tx_raw_registry,
};

static const FlipperAppPluginDescriptor subghz_garage_tx_raw_plugin_descriptor = {
    .appid = SUBGHZ_GARAGE_PROTOCOL_PLUGIN_APP_ID,
    .ep_api_version = SUBGHZ_GARAGE_PROTOCOL_PLUGIN_API_VERSION,
    .entry_point = &subghz_garage_tx_raw_plugin,
};

const FlipperAppPluginDescriptor* subghz_garage_tx_raw_plugin_ep(void) {
    return &subghz_garage_tx_raw_plugin_descriptor;
}
