#include "../subghz_i.h"
#include <lib/subghz/protocols/fiat_v1.h>

#define FIAT_V1_KEY_EV_DONE 200u

static void subghz_scene_fiat_v1_key_byte_input_cb(void* context) {
    SubGhz* subghz = context;
    view_dispatcher_send_custom_event(subghz->view_dispatcher, FIAT_V1_KEY_EV_DONE);
}

void subghz_scene_fiat_v1_key_on_enter(void* context) {
    SubGhz* subghz = context;

    byte_input_set_header_text(subghz->byte_input, "Enter Hitag2 key (hex)");
    byte_input_set_result_callback(
        subghz->byte_input,
        subghz_scene_fiat_v1_key_byte_input_cb,
        NULL,
        subghz,
        subghz->fiat_v1_key_edit.key_bytes,
        sizeof(subghz->fiat_v1_key_edit.key_bytes));

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdByteInput);
}

bool subghz_scene_fiat_v1_key_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;

    if(event.type == SceneManagerEventTypeCustom && event.event == FIAT_V1_KEY_EV_DONE) {
        FlipperFormat* fff = subghz_txrx_get_fff_data(subghz->txrx);
        uint32_t epoch = 0U;
        if(fff) {
            flipper_format_insert_or_update_hex(
                fff,
                FIAT_V1_HITAG2_KEY_FIELD,
                subghz->fiat_v1_key_edit.key_bytes,
                sizeof(subghz->fiat_v1_key_edit.key_bytes));
            flipper_format_insert_or_update_uint32(fff, FIAT_V1_HITAG2_EPOCH_FIELD, &epoch, 1U);
        }
        scene_manager_next_scene(subghz->scene_manager, SubGhzSceneTransmitter);
        return true;
    }

    return false;
}

void subghz_scene_fiat_v1_key_on_exit(void* context) {
    SubGhz* subghz = context;
    byte_input_set_result_callback(subghz->byte_input, NULL, NULL, NULL, NULL, 0);
    byte_input_set_header_text(subghz->byte_input, "");
}
