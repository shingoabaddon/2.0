#include "../subghz_i.h"

enum SubmenuIndex {
    SubmenuIndexRecover,
    SubmenuIndexManual,
};

static void subghz_scene_fiat_v1_key_method_submenu_callback(void* context, uint32_t index) {
    SubGhz* subghz = context;
    view_dispatcher_send_custom_event(subghz->view_dispatcher, index);
}

void subghz_scene_fiat_v1_key_method_on_enter(void* context) {
    SubGhz* subghz = context;

    submenu_add_item(
        subghz->submenu,
        "Recover Key (Auto)",
        SubmenuIndexRecover,
        subghz_scene_fiat_v1_key_method_submenu_callback,
        subghz);
    submenu_add_item(
        subghz->submenu,
        "Enter Key Manually",
        SubmenuIndexManual,
        subghz_scene_fiat_v1_key_method_submenu_callback,
        subghz);

    submenu_set_selected_item(
        subghz->submenu,
        scene_manager_get_scene_state(subghz->scene_manager, SubGhzSceneFiatV1KeyMethod));

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdMenu);
}

bool subghz_scene_fiat_v1_key_method_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexRecover) {
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzSceneFiatV1KeyMethod, SubmenuIndexRecover);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneFiatV1Recover);
            return true;
        } else if(event.event == SubmenuIndexManual) {
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzSceneFiatV1KeyMethod, SubmenuIndexManual);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneFiatV1Key);
            return true;
        }
    }

    return false;
}

void subghz_scene_fiat_v1_key_method_on_exit(void* context) {
    SubGhz* subghz = context;
    submenu_reset(subghz->submenu);
}
