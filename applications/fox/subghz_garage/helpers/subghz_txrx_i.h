#pragma once

#include "subghz_txrx.h"
#include <lib/flipper_application/plugins/plugin_manager.h>
#include <lib/flipper_application/plugins/composite_resolver.h>

struct SubGhzTxRx {
    SubGhzGarageWorker* worker;

    SubGhzEnvironment* environment;
    SubGhzReceiver* receiver;
    SubGhzTransmitter* transmitter;
    SubGhzProtocolDecoderBase* decoder_result;
    FlipperFormat* fff_data;

    SubGhzRadioPreset* preset;
    SubGhzSetting* setting;

    uint8_t hopper_timeout;
    uint8_t hopper_idx_frequency;
    bool is_database_loaded;
    /* is_database_loaded reflects whether the keystore file exists (checked
     * cheaply at alloc time, so the app's launch-time "No SD card or
     * database found" gate still works) - it does NOT mean the ~27KB
     * KeeLoq manufacturer-code table has actually been parsed into RAM.
     * That real load is deferred to first RX/TX/protocol-list use, same
     * as the protocol group plugins - see subghz_txrx_ensure_keystore(). */
    bool keystore_loaded;
    /* subghz_devices_init_internal_only() (called in subghz_txrx_alloc)
     * skips scanning /ext/apps_data/subghz/plugins for external radio
     * device .fal plugins - that scan alone costs ~25KB, unaffordable at
     * boot alongside everything else. Deferred to first actual need - see
     * subghz_txrx_ensure_external_device() in subghz_txrx.c. */
    bool external_device_loaded;
    /* Radio device (CC1101 internal), environment, and receiver are ALSO
     * deferred - not allocated until something actually needs them (Read,
     * Read RAW, TX, protocol list, Radio Settings' module toggle/frequency
     * check, ...), matching ProtoPirate's radio_init()/radio_initialized
     * gate. environment/receiver/radio_device are all NULL until this
     * flips true - see subghz_txrx_ensure_radio_init() in subghz_txrx.c. */
    bool radio_initialized;
    SubGhzHopperState hopper_state;

    SubGhzTxRxState txrx_state;
    SubGhzSpeakerState speaker_state;
    const SubGhzDevice* radio_device;
    SubGhzRadioDeviceType radio_device_type;

    SubGhzTxRxNeedSaveCallback need_save_callback;
    void* need_save_context;

    bool debug_pin_state;

    /* Garage/gate protocol registry: normally empty at alloc time and
     * lazily loaded from whichever group's protocol plugin is currently
     * selected, the first time something actually needs to decode,
     * transmit, or list protocols - see subghz_txrx_ensure_protocol_plugin()
     * / subghz_txrx_ensure_protocol_group() in subghz_txrx.c. Only one
     * group's plugin is ever resident at once. */
    SubGhzGarageProtocolGroup active_protocol_group;
    const SubGhzProtocolRegistry* protocol_registry;
    const SubGhzGarageProtocolPlugin* protocol_plugin;
    PluginManager* protocol_plugin_manager;
    CompositeApiResolver* protocol_plugin_resolver;
    /* Set when active_protocol_group's .fal failed to load (out of memory).
     * Distinguishes "tried and failed" from "not attempted yet" so
     * subghz_txrx_ensure_protocol_group() doesn't immediately retry the
     * exact same load - a failed attempt already fragments the heap further
     * on its way out, so an unconditional retry is close to guaranteed to
     * fail again, worse, for nothing. Cleared by subghz_txrx_set_protocol_group()
     * whenever the group actually changes. */
    bool protocol_plugin_load_failed;

    /* A handful of protocols (Alutech AT-4N, Somfy Telis, Jarolift - see
     * protocol_groups.h's SubGhzGarageTxProtocol) have a real dedicated
     * encoder split into their own tiny single-protocol TX plugin instead
     * of being bundled into the group plugin above. Only ever one loaded
     * at a time, entirely separate from the RX group plugin (both can be
     * loaded simultaneously) - see subghz_txrx_ensure_tx_protocol_plugin(). */
    bool tx_protocol_plugin_loaded;
    SubGhzGarageTxProtocol active_tx_protocol;
    const SubGhzGarageProtocolPlugin* tx_protocol_plugin;
    PluginManager* tx_protocol_plugin_manager;
    CompositeApiResolver* tx_protocol_plugin_resolver;

    /* subghz_alloc() configures the receiver (filter, rx callback) up
     * front, before any scene runs and before the protocol plugin has any
     * reason to be loaded yet. These are cached here and re-applied to the
     * receiver in subghz_txrx_ensure_protocol_plugin() once it actually
     * (re)allocates a plugin-backed receiver, so setting them doesn't
     * itself force the plugin to load at app boot. */
    SubGhzProtocolFlag receiver_filter;
    bool receiver_filter_set;
    SubGhzReceiverCallback rx_callback;
    void* rx_callback_context;
};
