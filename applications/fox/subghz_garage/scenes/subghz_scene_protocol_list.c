/**
 * @file subghz_scene_protocol_list.c
 *
 * Protocol Groups list — one full-width, double-row button per RX group
 * (see protocols/protocol_groups.h): bold group name on top, a horizontally
 * scrolling "Protocols: ..." line underneath listing that group's members.
 * A 7x7 OK icon marks whichever group is currently active.
 *
 * Up/Down move the cursor to browse groups; pressing OK on a group makes
 * IT the active RX group. Mirrors Radio Settings' "Protocol Group" setting
 * (see subghz_scene_receiver_config_set_protocol_group()) - both stay in
 * sync and either can be used to switch groups.
 */

#include "../subghz_i.h"

static void protocol_list_group_activated_cb(void* context, uint8_t group_index) {
    SubGhz* subghz = context;
    subghz_txrx_set_protocol_group(subghz->txrx, (SubGhzGarageProtocolGroup)group_index);
    subghz->last_settings->protocol_group = group_index;
}

void subghz_scene_protocol_list_on_enter(void* context) {
    FURI_LOG_I("SubGhzSceneProtocolList", "on_enter");
    SubGhz* subghz = context;

    subghz_ensure_protocol_groups(subghz);
    subghz_protocol_groups_set_selected(
        subghz->protocol_groups, (uint8_t)subghz_txrx_get_protocol_group(subghz->txrx));
    subghz_protocol_groups_set_callback(
        subghz->protocol_groups, protocol_list_group_activated_cb, subghz);
    subghz_protocol_groups_resume_scroll(subghz->protocol_groups);

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdProtocolGroups);
}

bool subghz_scene_protocol_list_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;
    bool consumed = false;
    if(event.type == SceneManagerEventTypeCustom) {
        consumed = true;
    } else if(event.type == SceneManagerEventTypeBack) {
        subghz_save_all(subghz);
    }
    return consumed;
}

void subghz_scene_protocol_list_on_exit(void* context) {
    SubGhz* subghz = context;
    /* This view is kept alive (lazily allocated, never freed) for the rest
     * of the app's life - stop its scroll timer here so it doesn't keep
     * ticking (and calling view_port_update() on a view that's no longer
     * on screen) in the background after the user backs out. This was the
     * cause of a ViewPort lockup warning. */
    subghz_protocol_groups_pause_scroll(subghz->protocol_groups);
    subghz_save_all(subghz);
}
