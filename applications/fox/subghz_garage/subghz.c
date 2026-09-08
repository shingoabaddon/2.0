/* Abandon hope, all ye who enter here. */

#include <furi/core/log.h>
#include <furi/core/memmgr.h>
#include <subghz/types.h>
#include <lib/toolbox/path.h>
#include <float_tools.h>
#include "subghz_i.h"
#include "scenes/subghz_scene_start.h"
#include "helpers/subghz_debug_log.h"
#include "helpers/subghz_lib_ext_compat.h"
#include "helpers/subghz_cli_vcp_compat.h"
#include "helpers/subghz_memmgr_pool_compat.h"
#include "helpers/rpc_gui_screen_suppress_compat.h"
#include <storage/storage.h>
#include <furi_hal_usb.h>

#define TAG "SubGhzApp"

bool subghz_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    SubGhz* subghz = context;
    return scene_manager_handle_custom_event(subghz->scene_manager, event);
}

bool subghz_back_event_callback(void* context) {
    furi_assert(context);
    SubGhz* subghz = context;
    return scene_manager_handle_back_event(subghz->scene_manager);
}

void subghz_tick_event_callback(void* context) {
    furi_assert(context);
    SubGhz* subghz = context;
    scene_manager_handle_tick_event(subghz->scene_manager);
}

static void subghz_rpc_command_callback(const RpcAppSystemEvent* event, void* context) {
    furi_assert(context);
    SubGhz* subghz = context;

    furi_assert(subghz->rpc_ctx);

    if(event->type == RpcAppEventTypeSessionClose) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneRpcSessionClose);
        rpc_system_app_set_callback(subghz->rpc_ctx, NULL, NULL);
        subghz->rpc_ctx = NULL;
    } else if(event->type == RpcAppEventTypeAppExit) {
        view_dispatcher_send_custom_event(subghz->view_dispatcher, SubGhzCustomEventSceneExit);
    } else if(event->type == RpcAppEventTypeLoadFile) {
        furi_assert(event->data.type == RpcAppSystemEventDataTypeString);
        furi_string_set(subghz->file_path, event->data.string);
        view_dispatcher_send_custom_event(subghz->view_dispatcher, SubGhzCustomEventSceneRpcLoad);
    } else if(event->type == RpcAppEventTypeButtonPress) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneRpcButtonPress);
    } else if(event->type == RpcAppEventTypeButtonRelease) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneRpcButtonRelease);
    } else if(event->type == RpcAppEventTypeButtonPressRelease) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneRpcButtonPressRelease);
    } else {
        rpc_system_app_confirm(subghz->rpc_ctx, false);
    }
}
/* Writes all subghz settings (last_settings + protocol/mod filters) to one file. */
void subghz_save_all(SubGhz* subghz) {
    furi_assert(subghz);
    /* Copy current filter state into the last_settings struct */
    subghz_garage_protocol_filter_get_raw(
        subghz->protocol_filter,
        subghz->last_settings->protocol_filter_data,
        sizeof(subghz->last_settings->protocol_filter_data));
    subghz->last_settings->protocol_filter_present = true;
    subghz_modulation_filter_get_raw(
        subghz->modulation_filter,
        subghz->last_settings->mod_filter_data,
        sizeof(subghz->last_settings->mod_filter_data));
    subghz->last_settings->mod_filter_present = true;
    /* Write everything in one pass */
    subghz_garage_last_settings_save(subghz->last_settings);
}

void subghz_lock_cli_sessions(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->cli_sessions_locked_after_recovery) return;

    FURI_LOG_W(TAG, "Locking CLI/RPC sessions for the rest of this app run (see subghz_i.h)");
    subghz_debug_log_write("lock_cli_sessions: enter");
    /* cli_vcp_session_lock(), NOT cli_vcp_disable() and NOT raw furi_hal_
     * usb calls - see subghz_i.h's comment on
     * cli_sessions_locked_after_recovery for why both of those corrupted
     * persistent CLI VCP state badly enough to need a full reboot. */
    CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
    subghz_debug_log_write("lock_cli_sessions: RECORD_CLI_VCP open, %p, calling session_lock", (void*)cli_vcp);
    subghz_garage_cli_vcp_session_lock(cli_vcp);
    subghz_debug_log_write("lock_cli_sessions: session_lock returned");
    furi_record_close(RECORD_CLI_VCP);
    subghz->cli_sessions_locked_after_recovery = true;
    subghz_debug_log_write("lock_cli_sessions: done");
}

void subghz_cli_soft_lock(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->cli_sessions_soft_locked) return;

    /* Two steps, in this order, not either alone:
     *
     * 1. cli_vcp_session_lock() first - closes any currently-active CLI/
     *    RPC session (including qFlipper's screen-stream) through the
     *    SAME safe path an organic disconnect uses (signals CliVcpInternal
     *    EventDisconnected while callbacks are still live), properly
     *    freeing its shell/pipe/RPC objects. Confirmed via furi_hal_usb_
     *    cdc.c that step 2 alone does NOT do this: cdc_deinit() only
     *    unregisters USB descriptor callbacks - it never touches the
     *    separate callbacks[]/cb_ctx[] table furi_hal_cdc_set_callbacks()
     *    populated, so CLI VCP's own state (is_connected, shell, pipes)
     *    is never told anything happened. Skipping this step is exactly
     *    what caused the OOM crash after repeated Read/Read RAW visits -
     *    each cycle's still-active session got orphaned instead of freed,
     *    leaking a little more every time, surviving even a full app exit
     *    since CLI VCP is a permanent system service, not part of this
     *    app's own lifetime.
     * 2. Hard USB teardown - same mechanism the PIN lock screen and
     *    Desktop's own low-RAM watchdog already use successfully (see
     *    desktop.c's should_disconnect_usb block and desktop_ram_watchdog_
     *    trigger()). This is what actually makes qFlipper's client notice
     *    and show "Connect your Flipper" - cli_vcp_session_lock() alone
     *    (tried first) doesn't touch USB hardware at all, so qFlipper
     *    never sees anything happened and just hangs on its last frame. */
    FURI_LOG_I(TAG, "Soft-locking CLI/RPC sessions (proactive Read/Read RAW/Decode RAW entry)");
    CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
    subghz_garage_cli_vcp_session_lock(cli_vcp);
    furi_record_close(RECORD_CLI_VCP);

    FURI_LOG_I(TAG, "Hard-disconnecting USB/CLI (proactive Read/Read RAW/Decode RAW entry)");
    subghz->cli_hard_disconnect_usb_config = furi_hal_usb_get_config();
    furi_hal_usb_unlock();
    furi_hal_usb_set_config(NULL, NULL);
    subghz->cli_sessions_soft_locked = true;
}

void subghz_cli_soft_unlock(SubGhz* subghz) {
    furi_assert(subghz);
    if(!subghz->cli_sessions_soft_locked) return;

    /* Reverse order from lock: USB hardware back up first, so the DTR
     * read inside cli_vcp_session_unlock() (see cli_vcp.c's
     * CliVcpMessageTypeSessionUnlock handler) reflects reality rather
     * than a still-torn-down interface. If qFlipper hasn't reasserted DTR
     * yet at this exact instant, that's fine - the normal CDC ctrl-line
     * callback picks it up organically the moment it does, same as any
     * real reconnect. */
    FURI_LOG_I(TAG, "Restoring USB/CLI (leaving Read/Read RAW/Decode RAW)");
    furi_hal_usb_set_config((FuriHalUsbInterface*)subghz->cli_hard_disconnect_usb_config, NULL);
    subghz->cli_hard_disconnect_usb_config = NULL;

    CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
    subghz_garage_cli_vcp_session_unlock(cli_vcp);
    furi_record_close(RECORD_CLI_VCP);
    subghz->cli_sessions_soft_locked = false;
}

#define SUBGHZ_LOW_RAM_MITIGATE_WAIT_MS 300
#define SUBGHZ_LOW_RAM_MITIGATE_RECHECK_WAIT_MS 50

bool subghz_low_ram_mitigate(SubGhz* subghz, size_t threshold) {
    furi_assert(subghz);

    /* Unconditional, not RAM-gated - always soft-lock CLI/RPC (idempotent,
     * see subghz_cli_soft_lock()) the first time this runs per scene
     * visit, regardless of current free heap and regardless of whether
     * qFlipper's screen-mirror is actively streaming. By design: this
     * deliberately disconnects an active mirror too - qFlipper shows its
     * own "Connect your Flipper" prompt for the rest of this Read/Read
     * RAW/Decode RAW visit, then reconnects automatically once
     * subghz_cli_soft_unlock() runs on the way out. Deterministic
     * behavior every time beats a live heap check that only sometimes
     * disconnects qFlipper - see subghz_i.h's doc comment on this
     * function for the full reasoning. */
    if(!subghz->cli_sessions_soft_locked) {
        size_t free_heap_before_lock = memmgr_get_free_heap();
        FURI_LOG_I(
            TAG,
            "low_ram_mitigate: soft-locking CLI/RPC before heavy allocation (free heap %zu)",
            free_heap_before_lock);
        subghz_cli_soft_lock(subghz);
        furi_delay_ms(SUBGHZ_LOW_RAM_MITIGATE_WAIT_MS);
        size_t free_heap_after_lock = memmgr_get_free_heap();
        FURI_LOG_I(
            TAG,
            "low_ram_mitigate: free heap %zu after %dms wait",
            free_heap_after_lock,
            SUBGHZ_LOW_RAM_MITIGATE_WAIT_MS);
        if(free_heap_after_lock >= threshold) {
            /* Only latch the sticky "no new CLI/RPC sessions for the rest
             * of this app run" block (same one subghz_scene_low_ram_
             * warning.c's own recovery path uses) if locking CLI/RPC just
             * PROVED necessary - heap was actually below threshold before
             * the lock, and clear after it, so a live session was
             * measurably part of the shortfall. Heap that was already
             * fine before we even locked doesn't tell us anything about
             * CLI/RPC being a problem, so don't punish it - same
             * qFlipper-still-active guard as that file, for the same
             * race-avoidance reason. */
            if(free_heap_before_lock < threshold && !rpc_gui_screen_stream_is_active()) {
                subghz_lock_cli_sessions(subghz);
            }
            return false;
        }
        return true;
    }

    /* Already locked from earlier in this same scene visit - there's no
     * fresh mitigation left to apply here, so a single instant read can
     * catch a momentary dip from unrelated concurrent allocation (RX
     * worker, GUI redraw) rather than a genuine shortage. Confirmed via
     * device logs: this exact read reported "still low" with a value the
     * very next log line - just a handful of instructions later - already
     * contradicted. Settle briefly and recheck once before conceding. */
    if(memmgr_get_free_heap() >= threshold) {
        return false;
    }
    furi_delay_ms(SUBGHZ_LOW_RAM_MITIGATE_RECHECK_WAIT_MS);
    return memmgr_get_free_heap() < threshold;
}

void subghz_ensure_submenu(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->submenu) return;
    subghz->submenu = submenu_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher, SubGhzViewIdMenu, submenu_get_view(subghz->submenu));
}

void subghz_ensure_text_input(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->text_input) return;
    subghz->text_input = text_input_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher, SubGhzViewIdTextInput, text_input_get_view(subghz->text_input));
}

void subghz_ensure_byte_input(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->byte_input) return;
    subghz->byte_input = byte_input_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher, SubGhzViewIdByteInput, byte_input_get_view(subghz->byte_input));
}

void subghz_ensure_widget(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->widget) return;
    subghz->widget = widget_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher, SubGhzViewIdWidget, widget_get_view(subghz->widget));
}

void subghz_ensure_variable_item_list(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->variable_item_list) return;
    subghz->variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdVariableItemList,
        variable_item_list_get_view(subghz->variable_item_list));
}

void subghz_ensure_signal_visualizer(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->subghz_signal_visualizer) return;
    subghz->subghz_signal_visualizer = subghz_signal_visualizer_alloc(subghz->txrx);
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdSignalVisualizer,
        subghz_signal_visualizer_get_view(subghz->subghz_signal_visualizer));
}

void subghz_ensure_protocol_groups(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->protocol_groups) return;
    subghz->protocol_groups = subghz_protocol_groups_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdProtocolGroups,
        subghz_protocol_groups_get_view(subghz->protocol_groups));
}

void subghz_ensure_receiver_view(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->subghz_receiver) return;
    FURI_LOG_I(TAG, "Allocating receiver view: free heap %zu", memmgr_get_free_heap());
    subghz->subghz_receiver = subghz_view_receiver_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdReceiver,
        subghz_view_receiver_get_view(subghz->subghz_receiver));
    FURI_LOG_I(TAG, "Receiver view added: free heap %zu", memmgr_get_free_heap());
}

void subghz_ensure_popup(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->popup) return;
    subghz->popup = popup_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher, SubGhzViewIdPopup, popup_get_view(subghz->popup));
}

void subghz_ensure_transmitter_view(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->subghz_transmitter) return;
    subghz->subghz_transmitter = subghz_view_transmitter_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdTransmitter,
        subghz_view_transmitter_get_view(subghz->subghz_transmitter));
}

/* Allocated lazily (Receiver's and Decode RAW's on_enter, same as
 * ProtoPirate_FoxEdition's protopirate_history_alloc() call sites) instead
 * of unconditionally in subghz_alloc() - a session that never enters Read
 * or Decode RAW never needs it. */
void subghz_ensure_history(SubGhz* subghz) {
    furi_assert(subghz);
    if(subghz->history) return;
    subghz->history = subghz_history_alloc();
}

SubGhz* subghz_alloc(bool alloc_for_tx_only) {
    /* Pool is the SEPARATE memory region .fal plugins (protocol groups,
     * TX plugins) load into - distinct from the main heap logged just
     * below throughout this file. Logged once here, at the very start of
     * every launch, specifically so repeated-launch pool exhaustion/
     * fragmentation (invisible to every other free-heap log in this file)
     * would show up as a downward trend across consecutive device-log
     * entries, without needing a separate RAM Monitor app launch. */
    FURI_LOG_I(
        TAG,
        "subghz_alloc: enter, free heap %zu, pool free %zu, pool max block %zu",
        memmgr_get_free_heap(),
        subghz_garage_pool_get_free(),
        subghz_garage_pool_get_max_block());
    subghz_debug_log_write(
        "subghz_alloc: enter, free heap %zu, pool free %zu, pool max block %zu",
        memmgr_get_free_heap(),
        subghz_garage_pool_get_free(),
        subghz_garage_pool_get_max_block());

    SubGhz* subghz = malloc(sizeof(SubGhz));

    /* Explicitly NULL fields that subghz_free() checks but that are only
     * conditionally set during runtime. malloc() does NOT zero memory, so
     * without this an unset field containing malloc heap garbage would be
     * treated as a valid pointer and crash on free. */
    subghz->blank_transition_viewport = NULL;

    subghz->file_path = furi_string_alloc();
    subghz->file_path_tmp = furi_string_alloc();
    subghz->decoded_preview_orig_path = furi_string_alloc();
    subghz->decoded_preview_active    = false;

    // GUI
    subghz->gui = furi_record_open(RECORD_GUI);

    /* Show loading wheel immediately — covers the apps menu before
     * the heavy subghz_alloc work begins, preventing the user from
     * seeing or interacting with the apps menu during startup.
     * Removed in subghz_scene_start_on_enter() once ready. */
    if(!alloc_for_tx_only) {
        subghz->startup_loading = loading_alloc();
        subghz->startup_holder  = view_holder_alloc();
        view_holder_attach_to_gui(subghz->startup_holder, subghz->gui);
        view_holder_set_view(
            subghz->startup_holder, loading_get_view(subghz->startup_loading));
    }

    // View Dispatcher
    subghz->view_dispatcher = view_dispatcher_alloc();

    subghz->scene_manager = scene_manager_alloc(&subghz_scene_handlers, subghz);
    view_dispatcher_set_event_callback_context(subghz->view_dispatcher, subghz);
    view_dispatcher_set_custom_event_callback(
        subghz->view_dispatcher, subghz_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        subghz->view_dispatcher, subghz_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        subghz->view_dispatcher, subghz_tick_event_callback, 100);

    // Open Notification record
    subghz->notifications = furi_record_open(RECORD_NOTIFICATION);
#if SUBGHZ_MEASURE_LOADING
    uint32_t load_ticks = furi_get_tick();
#endif
    subghz->txrx = subghz_txrx_alloc();

    /* Receiver view is allocated lazily (subghz_ensure_receiver_view(),
     * called from the Receiver and Decode RAW scenes) instead of here -
     * it doesn't need to exist at all until the user actually picks Read
     * or a signal gets decoded, and its allocation was one of the last
     * boot-eager costs left. */
    subghz->subghz_receiver = NULL;
    /* Popup is allocated lazily (subghz_ensure_popup()) - only a handful
     * of confirmation/error scenes use it. */
    subghz->popup = NULL;

    /* SubMenu, Text Input, Byte Input, Custom Widget, Variable Item List,
     * and Signal Visualizer are allocated lazily on first use instead of
     * here - see subghz_ensure_submenu() etc. in subghz_i.h. Only a
     * handful of scenes ever need any one of them, and eagerly allocating
     * all of them at boot was costing ~4.6KB of heap before the RX group
     * plugin ever got a chance to load. */
    subghz->submenu = NULL;
    subghz->text_input = NULL;
    subghz->byte_input = NULL;
    subghz->widget = NULL;
    subghz->variable_item_list = NULL;
    subghz->subghz_signal_visualizer = NULL;
    subghz->protocol_groups = NULL;

    //Dialog
    subghz->dialogs = furi_record_open(RECORD_DIALOGS);

    /* Transmitter view is allocated lazily (subghz_ensure_transmitter_view())
     * - only needed once the user actually sends/emulates a signal. */
    subghz->subghz_transmitter = NULL;
    FURI_LOG_I(TAG, "Boot: core views added, free heap %zu", memmgr_get_free_heap());
    // Read RAW
    subghz->subghz_read_raw = subghz_read_raw_alloc(alloc_for_tx_only);
    FURI_LOG_I(TAG, "Boot: read raw alloc'd, free heap %zu", memmgr_get_free_heap());
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdReadRAW,
        subghz_read_raw_get_view(subghz->subghz_read_raw));
    FURI_LOG_I(TAG, "Boot: read raw view added");

    /* Fox-theme start grid — always allocated, always registered */
    subghz->start_grid = subghz_start_grid_alloc();
    FURI_LOG_I(TAG, "Boot: start grid alloc'd, free heap %zu", memmgr_get_free_heap());
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdStartGrid,
        subghz_start_grid_get_view(subghz->start_grid));
    FURI_LOG_I(TAG, "Boot: start grid view added");

    //init threshold rssi
    subghz->threshold_rssi = subghz_threshold_rssi_alloc();
    FURI_LOG_I(TAG, "Boot: threshold rssi alloc'd, free heap %zu", memmgr_get_free_heap());

    //init TxRx & Protocol & History
    // Load last used values for Read, Read RAW, etc. or default
    subghz->last_settings = subghz_garage_last_settings_alloc();
    subghz->protocol_filter = subghz_garage_protocol_filter_alloc();
    subghz->modulation_filter = subghz_modulation_filter_alloc();
    FURI_LOG_I(TAG, "Boot: filters alloc'd, free heap %zu", memmgr_get_free_heap());
    /* Load all settings (including filter data) from one file */
    subghz_garage_last_settings_load(subghz->last_settings, 0);
    FURI_LOG_I(TAG, "Boot: last settings loaded");
    /* Apply loaded filter arrays to the runtime filter objects */
    if(subghz->last_settings->protocol_filter_present) {
        subghz_garage_protocol_filter_set_raw(
            subghz->protocol_filter,
            subghz->last_settings->protocol_filter_data,
            sizeof(subghz->last_settings->protocol_filter_data));
    }
    if(subghz->last_settings->mod_filter_present) {
        subghz_modulation_filter_set_raw(
            subghz->modulation_filter,
            subghz->last_settings->mod_filter_data,
            sizeof(subghz->last_settings->mod_filter_data));
    }

    // Set LED and Amp GPIO control state
    subghz_garage_set_ext_leds_and_amp(subghz->last_settings->leds_and_amp);
    FURI_LOG_I(TAG, "Boot: leds/amp set");

    // Restore which garage/gate protocol group was active for RX (lazy -
    // doesn't force a load, just records the selection for later use)
    subghz_txrx_set_protocol_group(
        subghz->txrx, (SubGhzGarageProtocolGroup)subghz->last_settings->protocol_group);
    FURI_LOG_I(TAG, "Boot: protocol group restored");

    if(!alloc_for_tx_only) {
        subghz_txrx_set_preset_internal(
            subghz->txrx,
            subghz->last_settings->frequency,
            subghz->last_settings->preset_index,
            subghz->tx_power);
        FURI_LOG_I(TAG, "Boot: preset set");
        /* History is allocated lazily - see subghz_ensure_history(). */
        subghz->history = NULL;
    }

    subghz_rx_key_state_set(subghz, SubGhzRxKeyStateIDLE);

    if(!alloc_for_tx_only) {
        subghz->filter = subghz->last_settings->filter;
        subghz->tx_power = subghz->last_settings->tx_power;
    } else {
        subghz->filter = SubGhzProtocolFlag_Decodable;
        subghz->tx_power = 0;
    }

    subghz_txrx_receiver_set_filter(subghz->txrx, subghz->filter);
    subghz_txrx_set_need_save_callback(subghz->txrx, subghz_save_to_file, subghz);

    if(!alloc_for_tx_only) {
        if(!float_is_equal(subghz->last_settings->rssi, 0)) {
            subghz_threshold_rssi_set(subghz->threshold_rssi, subghz->last_settings->rssi);
        } else {
            subghz->last_settings->rssi = SUBGHZ_LAST_SETTING_FREQUENCY_ANALYZER_TRIGGER;
        }
    }
#if SUBGHZ_MEASURE_LOADING
    load_ticks = furi_get_tick() - load_ticks;
    FURI_LOG_I(TAG, "Loaded: %ld ms.", load_ticks);
#endif
    //Init Error_str
    subghz->error_str = furi_string_alloc();

    subghz->cli_sessions_locked_after_recovery = false;
    subghz->cli_sessions_soft_locked = false;
    subghz->cli_hard_disconnect_usb_config = NULL;
    subghz->shared_ram_warning_shown = false;

    FURI_LOG_I(TAG, "Boot: subghz_alloc complete, free heap %zu", memmgr_get_free_heap());
    return subghz;
}

void subghz_free(SubGhz* subghz, bool alloc_for_tx_only) {
    furi_assert(subghz);
    FURI_LOG_I(TAG, "subghz_free: enter, free heap %zu", memmgr_get_free_heap());

    /* Safety net for an abrupt exit out of Read/Read RAW (e.g. the
     * no-previous-scene fallback in those scenes' Back handlers, which
     * calls scene_manager_stop()/view_dispatcher_stop() directly instead
     * of a normal scene transition, skipping that scene's on_exit). This
     * flag lives in the firmware's RPC service, not this app - left stuck
     * "true" past this app's own lifetime, it would suppress qFlipper's
     * screen-stream for every other app too, until a reboot. No-op if
     * already un-suppressed. */
    rpc_gui_screen_stream_set_suppressed(false);

    /* Safety net for subghz_lock_cli_sessions() below - the session lock
     * must never stay in effect past this app's own lifetime. Goes
     * through cli_vcp_session_unlock() - see subghz_i.h. */
    if(subghz->cli_sessions_locked_after_recovery) {
        FURI_LOG_I(TAG, "subghz_free: unlocking CLI sessions (were locked this run)");
        CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
        subghz_garage_cli_vcp_session_unlock(cli_vcp);
        furi_record_close(RECORD_CLI_VCP);
        subghz->cli_sessions_locked_after_recovery = false;
    }

    /* Same safety net for the separate, reversible soft-lock used by
     * proactive Read/Read RAW/Decode RAW entry (subghz_cli_soft_lock()) -
     * an abrupt exit that skips a scene's on_exit must never leave USB
     * hard-disconnected past the app's own lifetime either. */
    subghz_cli_soft_unlock(subghz);

    if(subghz->rpc_ctx) {
        rpc_system_app_set_callback(subghz->rpc_ctx, NULL, NULL);
        rpc_system_app_send_exited(subghz->rpc_ctx);
        subghz_blink_stop(subghz);
        subghz->rpc_ctx = NULL;
    }

    subghz_txrx_speaker_off(subghz->txrx);
    subghz_txrx_stop(subghz->txrx);
    subghz_txrx_sleep(subghz->txrx);
    FURI_LOG_I(TAG, "subghz_free: txrx stopped/slept, free heap %zu", memmgr_get_free_heap());

    /* Receiver, Popup, Transmitter, TextInput, ByteInput, Custom Widget,
     * Variable Item List, Signal Visualizer, and Submenu are only ever
     * allocated lazily (see subghz_ensure_*() above) - NULL means a scene
     * needing it was never entered this run, so skip both the free and
     * the view_dispatcher
     * removal (removing a view ID that was never added would be invalid). */
    if(subghz->subghz_receiver) {
        FURI_LOG_I(TAG, "subghz_free: freeing receiver view, free heap %zu", memmgr_get_free_heap());
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdReceiver);
        subghz_view_receiver_free(subghz->subghz_receiver);
        FURI_LOG_I(TAG, "subghz_free: receiver view freed, free heap %zu", memmgr_get_free_heap());
    }
    if(subghz->text_input) {
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdTextInput);
        text_input_free(subghz->text_input);
    }
    if(subghz->byte_input) {
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdByteInput);
        byte_input_free(subghz->byte_input);
    }
    if(subghz->widget) {
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdWidget);
        widget_free(subghz->widget);
    }
    //Dialog
    furi_record_close(RECORD_DIALOGS);

    // Transmitter
    if(subghz->subghz_transmitter) {
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdTransmitter);
        subghz_view_transmitter_free(subghz->subghz_transmitter);
    }
    if(subghz->variable_item_list) {
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdVariableItemList);
        variable_item_list_free(subghz->variable_item_list);
    }
    if(subghz->subghz_signal_visualizer) {
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdSignalVisualizer);
        subghz_signal_visualizer_free(subghz->subghz_signal_visualizer);
    }
    if(subghz->protocol_groups) {
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdProtocolGroups);
        subghz_protocol_groups_free(subghz->protocol_groups);
    }
    // Read RAW
    FURI_LOG_I(TAG, "subghz_free: freeing read_raw/start_grid, free heap %zu", memmgr_get_free_heap());
    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdReadRAW);
    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdStartGrid);
    subghz_start_grid_free(subghz->start_grid);
    subghz_read_raw_free(subghz->subghz_read_raw);
    FURI_LOG_I(TAG, "subghz_free: read_raw/start_grid freed, free heap %zu", memmgr_get_free_heap());
    if(subghz->submenu) {
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdMenu);
        submenu_free(subghz->submenu);
    }
    // Popup
    if(subghz->popup) {
        view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdPopup);
        popup_free(subghz->popup);
    }

    // Scene manager
    FURI_LOG_I(TAG, "subghz_free: freeing scene_manager, free heap %zu", memmgr_get_free_heap());
    scene_manager_free(subghz->scene_manager);
    FURI_LOG_I(TAG, "subghz_free: scene_manager freed, free heap %zu", memmgr_get_free_heap());

    // View Dispatcher
    view_dispatcher_free(subghz->view_dispatcher);
    FURI_LOG_I(TAG, "subghz_free: view_dispatcher freed, free heap %zu", memmgr_get_free_heap());

    // Blank transition cover — must be freed AFTER view_dispatcher_free
    // (so its registered view is no longer live) but BEFORE the GUI record
    // closes. Missing this was the cause of the white screen hang after
    // RAW Edit returns: the viewport was registered but never removed.
    if(subghz->blank_transition_viewport) {
        gui_remove_view_port(subghz->gui, subghz->blank_transition_viewport);
        view_port_free(subghz->blank_transition_viewport);
        subghz->blank_transition_viewport = NULL;
    }

    // GUI
    /* Remove startup loading wheel if still active */
    if(subghz->startup_holder) {
        view_holder_set_view(subghz->startup_holder, NULL);
        view_holder_free(subghz->startup_holder);
        subghz->startup_holder = NULL;
    }
    if(subghz->startup_loading) {
        loading_free(subghz->startup_loading);
        subghz->startup_loading = NULL;
    }
    furi_record_close(RECORD_GUI);
    subghz->gui = NULL;

    subghz_save_all(subghz);
    subghz_garage_protocol_filter_free(subghz->protocol_filter);
    subghz_modulation_filter_free(subghz->modulation_filter);
    subghz_garage_last_settings_free(subghz->last_settings);

    // threshold rssi
    subghz_threshold_rssi_free(subghz->threshold_rssi);

    if(!alloc_for_tx_only && subghz->history) {
        subghz_history_free(subghz->history);
    }

    //TxRx
    FURI_LOG_I(TAG, "subghz_free: freeing txrx, free heap %zu", memmgr_get_free_heap());
    subghz_txrx_free(subghz->txrx);
    FURI_LOG_I(TAG, "subghz_free: txrx freed, free heap %zu", memmgr_get_free_heap());

    //Error string
    furi_string_free(subghz->error_str);

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);
    subghz->notifications = NULL;

    // Path strings
    furi_string_free(subghz->file_path);
    furi_string_free(subghz->file_path_tmp);
    furi_string_free(subghz->decoded_preview_orig_path);

    // The rest
    /* Pool reading here is the important one - if this run loaded any
     * protocol group / TX plugin .fal, everything above should have
     * unloaded it by now (subghz_txrx_free()). If pool free/max block
     * DON'T return to roughly what subghz_alloc()'s own log line showed
     * at boot, something plugin-related isn't being released - compare
     * this line against the NEXT launch's "subghz_alloc: enter" line to
     * see whether it's carrying over. */
    FURI_LOG_I(
        TAG,
        "subghz_free: complete, free heap %zu, pool free %zu, pool max block %zu",
        memmgr_get_free_heap(),
        subghz_garage_pool_get_free(),
        subghz_garage_pool_get_max_block());
    subghz_debug_log_write(
        "subghz_free: complete, free heap %zu, pool free %zu, pool max block %zu",
        memmgr_get_free_heap(),
        subghz_garage_pool_get_free(),
        subghz_garage_pool_get_max_block());
    free(subghz);
}

int32_t subghz_app(void* p) {
    /* Two marker files let SubGHz return precisely to where the user was
     * after closing an external sub-tool FAP, without depending on
     * chaining a SECOND args-based deferred launch (confirmed to fail —
     * see the detailed comment below). Both are only checked on a normal,
     * no-args launch, and both are deleted immediately after reading so
     * they can't incorrectly affect a later, unrelated launch.
     *
     *  .focus_menu  — written by the Frequency/Modulation Analyzer FAPs
     *                 on Back (no result selected): "menu:freq" or
     *                 "menu:mod", pre-selects that Start-menu item.
     *  .focus_file  — written by the RAW Edit FAP on exit: the file path
     *                 that was being edited. Reassigns `p` itself so all
     *                 the existing, already-proven file-load logic below
     *                 runs completely unchanged, exactly as if that path
     *                 had been the original launch argument.
     *
     * Why marker files instead of chained args: passing "menu:freq" as
     * args worked fine for SubGHz→FAP (outbound) but was confirmed to
     * fail for FAP→SubGHz (the return leg fell through to the Desktop
     * instead of relaunching SubGHz) — the earlier confirmed-working
     * test used NULL args for that exact leg. Rather than depend on an
     * unconfirmed detail of how the Loader's deferred-launch queue
     * handles two chained args-based launches back to back, this
     * sidesteps that mechanism entirely for the leg where it broke. */
    static char focus_file_buf[256];
    uint32_t menu_focus_index = 0; /* 0 = no focus override */
    bool focus_file_existed = false;
    char focus_menu_content[16] = {0};

    if(!p || strlen((const char*)p) == 0) {
        Storage* storage = furi_record_open(RECORD_STORAGE);

        if(storage_file_exists(storage, "/ext/subghz/.focus_menu")) {
            File* f = storage_file_alloc(storage);
            if(storage_file_open(f, "/ext/subghz/.focus_menu", FSAM_READ, FSOM_OPEN_EXISTING)) {
                char buf[16] = {0};
                uint16_t read = storage_file_read(f, buf, sizeof(buf) - 1);
                buf[read] = '\0';
                strncpy(focus_menu_content, buf, sizeof(focus_menu_content) - 1);
                if(strcmp(buf, "menu:freq") == 0) {
                    menu_focus_index = SubmenuIndexFrequencyAnalyzer;
                } else if(strcmp(buf, "menu:mod") == 0) {
                    menu_focus_index = SubmenuIndexModulationAnalyzer;
                } else if(strcmp(buf, "read") == 0) {
                    /* FA/MA OK result: open the Receiver directly. */
                    static const char read_arg[] = "read";
                    p = (void*)read_arg;
                } else if(strcmp(buf, "readraw") == 0) {
                    /* FA/MA OK result: open Read RAW (live capture mode). */
                    static const char readraw_arg[] = "readraw";
                    p = (void*)readraw_arg;
                }
            }
            storage_file_close(f);
            storage_file_free(f);
            storage_simply_remove(storage, "/ext/subghz/.focus_menu");
            /* Also delete any stale .focus_file so a leftover from a
             * failed RAW Edit run can't shadow a future menu return. */
            if(storage_file_exists(storage, "/ext/subghz/.focus_file")) {
                storage_simply_remove(storage, "/ext/subghz/.focus_file");
            }
        } else if(storage_file_exists(storage, "/ext/subghz/.focus_file")) {
            focus_file_existed = true;
            File* f = storage_file_alloc(storage);
            if(storage_file_open(f, "/ext/subghz/.focus_file", FSAM_READ, FSOM_OPEN_EXISTING)) {
                uint16_t read = storage_file_read(f, focus_file_buf, sizeof(focus_file_buf) - 1);
                focus_file_buf[read] = '\0';
                /* Marker format is "rawreturn:<filepath>" — strip the prefix
                 * so downstream gets a plain path, but we know NOT to set
                 * raw_send_only (which hides the "More" button and causes
                 * the white-screen hang when the viewport isn't covered). */
                if(read > 0) {
                    const char* prefix = "rawreturn:";
                    if(strncmp(focus_file_buf, prefix, strlen(prefix)) == 0) {
                        /* Strip the prefix — p points at the plain path.
                         * Set is_rawreturn via a dedicated flag rather than
                         * pointer arithmetic (simpler, no void* cast issues). */
                        p = focus_file_buf + strlen(prefix);
                        focus_file_existed = true; /* already true, reaffirm */
                    } else {
                        p = focus_file_buf;
                    }
                }
            }
            storage_file_close(f);
            storage_file_free(f);
            storage_simply_remove(storage, "/ext/subghz/.focus_file");
        }

        furi_record_close(RECORD_STORAGE);
    }

    bool open_receiver = (p && strcmp((const char*)p, "read") == 0);
    bool open_readraw  = (p && strcmp((const char*)p, "readraw") == 0);
    bool open_menu_focused = (menu_focus_index != 0);
    bool from_mode_picker = (p && strcmp((const char*)p, "frommode") == 0);

    FURI_LOG_I(
        TAG,
        "Launch routing: p=\"%s\" open_receiver=%d open_readraw=%d menu_focus_index=%lu focus_file_existed=%d from_mode_picker=%d",
        p ? (const char*)p : "(null)",
        (int)open_receiver,
        (int)open_readraw,
        (unsigned long)menu_focus_index,
        (int)focus_file_existed,
        (int)from_mode_picker);

    /* When returning from RAW Edit the pointer was stripped of the
     * "rawreturn:" prefix — detect by checking if it points into
     * focus_file_buf past offset 10 (length of "rawreturn:"). We
     * want full app allocation so the normal Saved-file UI appears
     * with the "More" button, not the raw-TX-only stripped mode. */
    /* Set during focus_file parsing above when "rawreturn:" prefix found. */
    bool is_rawreturn = (focus_file_existed &&
                         p != NULL &&
                         p != (void*)focus_file_buf &&
                         (const char*)p == focus_file_buf + 10);

    bool alloc_for_tx;
    if(p && strlen((const char*)p) && !open_receiver && !open_readraw && !open_menu_focused &&
       !is_rawreturn && !from_mode_picker) {
        alloc_for_tx = true;
    } else {
        alloc_for_tx = false;
    }

    SubGhz* subghz = subghz_alloc(alloc_for_tx);

    if(alloc_for_tx) {
        subghz->raw_send_only = true;
    } else {
        subghz->raw_send_only = false;
    }
    subghz->launched_from_mode_picker = from_mode_picker;

    // Check argument and run corresponding scene
    if(open_menu_focused) {
        view_dispatcher_attach_to_gui(
            subghz->view_dispatcher, subghz->gui, ViewDispatcherTypeFullscreen);
        scene_manager_set_scene_state(subghz->scene_manager, SubGhzSceneStart, menu_focus_index);
        scene_manager_next_scene(subghz->scene_manager, SubGhzSceneStart);
    } else if(open_receiver) {
        view_dispatcher_attach_to_gui(
            subghz->view_dispatcher, subghz->gui, ViewDispatcherTypeFullscreen);
        furi_string_set(subghz->file_path, SUBGHZ_APP_FOLDER);
        if(subghz_txrx_is_database_loaded(subghz->txrx)) {
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneStart);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReceiver);
        } else {
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzSceneShowError, SubGhzCustomEventManagerSet);
            furi_string_set(
                subghz->error_str,
                "No SD card or\ndatabase found.\nSome app function\nmay be reduced.");
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneShowError);
        }
    } else if(open_readraw) {
        /* Launched from FA/MA OK — open Read RAW live-capture mode,
         * already tuned to the frequency written into last_subghz.settings
         * by the FAP. Back from Read RAW returns to the Start menu. */
        view_dispatcher_attach_to_gui(
            subghz->view_dispatcher, subghz->gui, ViewDispatcherTypeFullscreen);
        furi_string_set(subghz->file_path, SUBGHZ_APP_FOLDER);
        if(subghz_txrx_is_database_loaded(subghz->txrx)) {
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneStart);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReadRAW);
        } else {
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzSceneShowError, SubGhzCustomEventManagerSet);
            furi_string_set(
                subghz->error_str,
                "No SD card or\ndatabase found.\nSome app function\nmay be reduced.");
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneShowError);
        }
    } else if(p && strlen((const char*)p) && !from_mode_picker) {
        uint32_t rpc_ctx = 0;

        if(sscanf(p, "RPC %lX", &rpc_ctx) == 1) {
            subghz->rpc_ctx = (void*)rpc_ctx;
            rpc_system_app_set_callback(subghz->rpc_ctx, subghz_rpc_command_callback, subghz);
            rpc_system_app_send_started(subghz->rpc_ctx);
            view_dispatcher_attach_to_gui(
                subghz->view_dispatcher, subghz->gui, ViewDispatcherTypeDesktop);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneRpc);
        } else {
            view_dispatcher_attach_to_gui(
                subghz->view_dispatcher, subghz->gui, ViewDispatcherTypeFullscreen);
            if(subghz_key_load(subghz, p, true)) {
                furi_string_set(subghz->file_path, (const char*)p);

                if(subghz_get_load_type_file(subghz) == SubGhzLoadTypeFileRaw) {
                    //Load Raw TX
                    subghz_rx_key_state_set(subghz, SubGhzRxKeyStateRAWLoad);
                    if(is_rawreturn) {
                        /* Returning from RAW Edit — build a proper scene stack
                         * so Back from ReadRAW returns to the Start menu with
                         * "Read RAW" highlighted, rather than exiting SubGHz. */
                        scene_manager_set_scene_state(
                            subghz->scene_manager,
                            SubGhzSceneStart,
                            SubmenuIndexReadRAW);
                        scene_manager_next_scene(subghz->scene_manager, SubGhzSceneStart);
                    }
                    scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReadRAW);
                } else {
                    //Load transmitter TX
                    scene_manager_next_scene(subghz->scene_manager, SubGhzSceneTransmitter);
                }
            } else {
                //exit app
                scene_manager_stop(subghz->scene_manager);
                view_dispatcher_stop(subghz->view_dispatcher);
            }
        }
    } else {
        view_dispatcher_attach_to_gui(
            subghz->view_dispatcher, subghz->gui, ViewDispatcherTypeFullscreen);
        furi_string_set(subghz->file_path, SUBGHZ_APP_FOLDER);
        if(subghz_txrx_is_database_loaded(subghz->txrx)) {
            FURI_LOG_I(TAG, "Routing: default branch -> SubGhzSceneStart");
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneStart);
        } else {
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzSceneShowError, SubGhzCustomEventManagerSet);
            furi_string_set(
                subghz->error_str,
                "No SD card or\ndatabase found.\nSome app function\nmay be reduced.");
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneShowError);
        }
    }

    furi_hal_power_suppress_charge_enter();

    view_dispatcher_run(subghz->view_dispatcher);

    furi_hal_power_suppress_charge_exit();

    subghz_free(subghz, alloc_for_tx);

    return 0;
}
