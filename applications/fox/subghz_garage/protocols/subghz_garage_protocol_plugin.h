#pragma once

#include <lib/flipper_application/flipper_application.h>
#include <lib/subghz/types.h>
#include "protocol_items.h"

#define SUBGHZ_GARAGE_PROTOCOL_PLUGIN_APP_ID      "subghz_garage_protocol_plugin"
#define SUBGHZ_GARAGE_PROTOCOL_PLUGIN_API_VERSION 1U

/* Everything the main subghz_garage app needs from a protocols plugin: the
 * decodable registry itself (group plugins only - TX plugins leave this
 * NULL), plus the one piece of cross-plugin state a protocol needs the main
 * app to be able to reach without linking its .c file directly. The
 * "generate a signal for protocol X from typed-in fields" helpers that used
 * to live here (and in helpers/subghz_txrx_create_protocol_key.c) were
 * removed with the "Add Manually" feature - retransmitting a captured/saved
 * signal doesn't need them, it goes through the protocol's own decoder-
 * paired encoder via subghz_txrx_ensure_tx_protocol_plugin(). */
typedef struct {
    const SubGhzProtocolRegistry* registry;

    /* FAAC SLH keeps a tiny bit of static "programming mode" state in its
     * own translation unit; this lets the main app's reset-custom-buttons
     * path clear it without linking faac_slh.c directly. */
    void (*faac_slh_reset_prog_mode)(void);
} SubGhzGarageProtocolPlugin;
