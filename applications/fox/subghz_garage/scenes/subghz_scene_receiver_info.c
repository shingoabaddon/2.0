#include "../subghz_i.h"

#include "../helpers/subghz_custom_btn_compat.h"
#include "../helpers/subghz_debug_log.h"

#include "../helpers/subghz_txrx_i.h"
#include <lib/subghz/blocks/generic.h>
#include "../helpers/subghz_generic_global_compat.h"
#include "../protocols/faac_slh.h"
#include "../protocols/beninca_arc.h"
#include "../protocols/jarolift.h"
#include "../protocols/kinggates_stylo_4k.h"

#define TAG "SubGhzSceneReceiverInfo"

/* Only these 4 protocols' get_string() actually reads the keystore for
 * display enrichment - everyone else's captured-signal info screen never
 * touches it. The ~27KB parse is real money on a heap this tight, so only
 * pay it when the captured signal is actually one of these 4, not on every
 * visit to this scene regardless of protocol. */
static bool subghz_scene_receiver_info_protocol_needs_keystore(const char* protocol_name) {
    return strcmp(protocol_name, SUBGHZ_PROTOCOL_FAAC_SLH_NAME) == 0 ||
           strcmp(protocol_name, SUBGHZ_PROTOCOL_BENINCA_ARC_NAME) == 0 ||
           strcmp(protocol_name, SUBGHZ_PROTOCOL_JAROLIFT_NAME) == 0 ||
           strcmp(protocol_name, SUBGHZ_PROTOCOL_KINGGATES_STYLO_4K_NAME) == 0;
}

void subghz_scene_receiver_info_callback(GuiButtonType result, InputType type, void* context) {
    furi_assert(context);
    SubGhz* subghz = context;

    if((result == GuiButtonTypeCenter) && (type == InputTypePress)) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneReceiverInfoTxStart);
    } else if((result == GuiButtonTypeCenter) && (type == InputTypeRelease)) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneReceiverInfoTxStop);
    } else if((result == GuiButtonTypeRight) && (type == InputTypeShort)) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneReceiverInfoSave);
    }
}

static bool subghz_scene_receiver_info_update_parser(void* context) {
    SubGhz* subghz = context;

    if(subghz_txrx_load_decoder_by_name_protocol(
           subghz->txrx,
           subghz_history_get_protocol_name(subghz->history, subghz->idx_menu_chosen))) {
        // we are trying to deserialize without checking for errors, since it is assumed that we just received this chignal
        subghz_protocol_decoder_base_deserialize(
            subghz_txrx_get_decoder(subghz->txrx),
            subghz_history_get_raw_data(subghz->history, subghz->idx_menu_chosen));

        SubGhzRadioPreset* preset =
            subghz_history_get_radio_preset(subghz->history, subghz->idx_menu_chosen);

        //Edit TX power, if necessary.
        subghz_txrx_set_tx_power(preset->data, preset->data_size, subghz->tx_power);

        subghz_txrx_set_preset(
            subghz->txrx,
            furi_string_get_cstr(preset->name),
            preset->frequency,
            preset->data,
            preset->data_size);

        return true;
    }
    return false;
}

void subghz_scene_receiver_info_draw_widget(SubGhz* subghz) {
    subghz_ensure_widget(subghz);
    if(subghz_scene_receiver_info_update_parser(subghz)) {
        FuriString* frequency_str = furi_string_alloc();
        FuriString* modulation_str = furi_string_alloc();
        FuriString* text = furi_string_alloc();

        subghz_txrx_get_frequency_and_modulation(
            subghz->txrx, frequency_str, modulation_str, false);
        widget_add_string_element(
            subghz->widget,
            78,
            0,
            AlignLeft,
            AlignTop,
            FontSecondary,
            furi_string_get_cstr(frequency_str));

        widget_add_string_element(
            subghz->widget,
            113,
            0,
            AlignLeft,
            AlignTop,
            FontSecondary,
            furi_string_get_cstr(modulation_str));
        subghz_protocol_decoder_base_get_string(subghz_txrx_get_decoder(subghz->txrx), text);
        widget_add_string_multiline_element(
            subghz->widget, 0, 0, AlignLeft, AlignTop, FontSecondary, furi_string_get_cstr(text));

        furi_string_free(frequency_str);
        furi_string_free(modulation_str);
        furi_string_free(text);

        if(subghz_txrx_protocol_is_serializable(subghz->txrx)) {
            widget_add_button_element(
                subghz->widget,
                GuiButtonTypeRight,
                "Save",
                subghz_scene_receiver_info_callback,
                subghz);
        }
        // Removed static check
        if(subghz_txrx_protocol_is_transmittable(subghz->txrx, false)) {
            widget_add_button_element(
                subghz->widget,
                GuiButtonTypeCenter,
                "Send",
                subghz_scene_receiver_info_callback,
                subghz);
        }
    } else {
        // [NO_DOLPHIN] widget_add_icon_element(subghz->widget, 83, 22, &I_WarningDolphinFlip_45x42);
        widget_add_string_element(
            subghz->widget, 13, 8, AlignLeft, AlignBottom, FontSecondary, "Error history parse.");
    }

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdWidget);
}

void subghz_scene_receiver_info_on_enter(void* context) {
    FURI_LOG_I(TAG, "on_enter");
    SubGhz* subghz = context;

    subghz_custom_btns_reset();

    /* Only load the keystore (~27KB parse) if the captured signal's own
     * protocol actually reads it for display enrichment - see
     * subghz_scene_receiver_info_protocol_needs_keystore() above. Calling
     * this unconditionally for every captured signal was paying that cost
     * on a heap that's often only a few KB free right after RX starts. */
    const char* protocol_name =
        subghz_history_get_protocol_name(subghz->history, subghz->idx_menu_chosen);
    if(subghz_scene_receiver_info_protocol_needs_keystore(protocol_name)) {
        subghz_txrx_ensure_keystore(subghz->txrx);
    }

    subghz_scene_receiver_info_draw_widget(subghz);

    if(!subghz_history_get_text_space_left(subghz->history, NULL, SUBGHZ_LOW_RAM_FREE_HEAP) &&
       !scene_manager_has_previous_scene(subghz->scene_manager, SubGhzSceneDecodeRAW)) {
        subghz->state_notifications = SubGhzNotificationStateRx;
    }
}

bool subghz_scene_receiver_info_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubGhzCustomEventSceneReceiverInfoTxStart) {
            /* Emulate is in the CLI-lock list too (see subghz_i.h's
             * threshold comment) - proactively lock CLI/RPC and gate on the
             * same lenient threshold Read/Read RAW/Decode RAW use BEFORE
             * subghz_scene_receiver_info_update_parser() below, not after -
             * that call can itself trigger a real protocol-group
             * (re)load (subghz_txrx_load_decoder_by_name_protocol()),
             * which has its own threshold check and needs CLI/qFlipper
             * disconnected first for the same reason start_listening()
             * does (see that function's comment, subghz_scene_reader.c) -
             * otherwise a still-connected qFlipper can fail that load
             * before ever getting a chance to be kicked off here.
             * subghz_cli_soft_unlock() runs once TX genuinely finishes -
             * the TxWait tick case below, or immediately here if
             * subghz_tx_start() itself fails. */
            if(subghz_low_ram_mitigate(subghz, SUBGHZ_LOW_RAM_FREE_HEAP_READ)) {
                FURI_LOG_W(
                    TAG,
                    "tx_start: free heap %zu still low after mitigation, bailing to warning",
                    memmgr_get_free_heap());
                subghz_debug_log_write(
                    "tx_start: free heap %zu still low after mitigation, bailing to warning",
                    memmgr_get_free_heap());
                scene_manager_next_scene(subghz->scene_manager, SubGhzSceneLowRamWarning);
                return true;
            }
            if(!subghz_scene_receiver_info_update_parser(subghz)) {
                return false;
            }
            //CC1101 Stop RX -> Start TX
            subghz_txrx_hopper_pause(subghz->txrx);
            // key concept: we start endless TX until user release OK button, and after this we send last
            // protocols repeats - this guarantee that one press OK will
            // be guarantee send the required minimum protocol data packets
            // for all of this we use subghz_block_generic_global.endless_tx in protocols _yield function.
            subghz->state_notifications = SubGhzNotificationStateTx;
            subghz_block_generic_global.endless_tx = true;
            if(!subghz_tx_start(
                   subghz,
                   subghz_history_get_raw_data(subghz->history, subghz->idx_menu_chosen))) {
                subghz_cli_soft_unlock(subghz);
                subghz_txrx_rx_start(subghz->txrx);
                subghz_txrx_hopper_unpause(subghz->txrx);
                subghz->state_notifications = SubGhzNotificationStateRx;
                subghz_block_generic_global.endless_tx = false;
                return true;
            }
        } else if(event.event == SubGhzCustomEventSceneReceiverInfoTxStop) {
            //CC1101 Stop Tx -> next tick event Start RX
            // user release OK
            // we switch off endless_tx - that mean protocols yield finish endless transmission,
            // send upload "repeat=xx" times, and after will be stoped by the tick event down in this code
            subghz->state_notifications = SubGhzNotificationStateTxWait;
            subghz_block_generic_global.endless_tx = false;

            return true;
        } else if(event.event == SubGhzCustomEventSceneReceiverInfoSave) {
            //CC1101 Stop RX -> Save
            subghz->state_notifications = SubGhzNotificationStateIDLE;
            subghz_txrx_hopper_set_state(subghz->txrx, SubGhzHopperStateOFF);

            subghz_txrx_stop(subghz->txrx);
            if(!subghz_scene_receiver_info_update_parser(subghz)) {
                return false;
            }

            if(subghz_txrx_protocol_is_serializable(subghz->txrx)) {
                subghz_file_name_clear(subghz);

                subghz->save_datetime =
                    subghz_history_get_datetime(subghz->history, subghz->idx_menu_chosen);
                subghz->save_datetime_set = true;
                scene_manager_next_scene(subghz->scene_manager, SubGhzSceneSaveName);
            }
            return true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        if(subghz_txrx_hopper_get_state(subghz->txrx) != SubGhzHopperStateOFF) {
            subghz_txrx_hopper_update(subghz->txrx, subghz->last_settings->hopping_threshold);
        }
        switch(subghz->state_notifications) {
        case SubGhzNotificationStateTx:
            notification_message(subghz->notifications, &sequence_blink_magenta_10);
            break;
        case SubGhzNotificationStateRx:
            notification_message(subghz->notifications, &sequence_blink_cyan_10);
            break;
        case SubGhzNotificationStateRxDone:
            notification_message(subghz->notifications, &sequence_blink_green_100);
            subghz->state_notifications = SubGhzNotificationStateRx;
            break;
        case SubGhzNotificationStateTxWait:
            // we wait until hardware TX finished and after stop TX and start RX, else just blink led
            if(!subghz_devices_is_async_complete_tx(subghz->txrx->radio_device)) {
                notification_message(subghz->notifications, &sequence_blink_magenta_10);
            } else {
                subghz_txrx_stop(subghz->txrx);
                subghz_cli_soft_unlock(subghz);
                // update screen
                widget_reset(subghz->widget);
                subghz_scene_receiver_info_draw_widget(subghz);

                subghz->state_notifications = SubGhzNotificationStateIDLE;

                if(!scene_manager_has_previous_scene(subghz->scene_manager, SubGhzSceneDecodeRAW)) {
                    subghz_txrx_rx_start(subghz->txrx);
                    subghz_txrx_hopper_unpause(subghz->txrx);
                    if(!subghz_history_get_text_space_left(
                           subghz->history, NULL, SUBGHZ_LOW_RAM_FREE_HEAP)) {
                        subghz->state_notifications = SubGhzNotificationStateRx;
                    }
                }
            }
            break;
        default:
            break;
        }
    }
    return false;
}

void subghz_scene_receiver_info_on_exit(void* context) {
    SubGhz* subghz = context;

    widget_reset(subghz->widget);
    subghz_txrx_reset_dynamic_and_custom_btns(subghz->txrx);
}
