#include "../lfrfid_i.h"

typedef enum {
    T5577MwSubmenuIndexWriteFirstKey,
    T5577MwSubmenuIndexWriteSecondKey,
    T5577MwSubmenuIndexWriteThirdKey,
} T5577MwSubmenuIndex;

static void lfrfid_scene_t5577mw_menu_submenu_callback(void* context, uint32_t index) {
    LfRfid* app = context;

    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void lfrfid_scene_t5577mw_menu_on_enter(void* context) {
    LfRfid* app = context;
    Submenu* submenu = app->submenu;

    submenu_add_item(
        submenu,
        "Write first key",
        T5577MwSubmenuIndexWriteFirstKey,
        lfrfid_scene_t5577mw_menu_submenu_callback,
        app);
    submenu_add_item(
        submenu,
        "Write second key",
        T5577MwSubmenuIndexWriteSecondKey,
        lfrfid_scene_t5577mw_menu_submenu_callback,
        app);
    submenu_add_item(
        submenu,
        "Write third key",
        T5577MwSubmenuIndexWriteThirdKey,
        lfrfid_scene_t5577mw_menu_submenu_callback,
        app);

    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, LfRfidSceneT5577MwMenu));

    furi_string_reset(app->file_name);
    app->protocol_id = PROTOCOL_NO;

    view_dispatcher_switch_to_view(app->view_dispatcher, LfRfidViewSubmenu);
}

bool lfrfid_scene_t5577mw_menu_on_event(void* context, SceneManagerEvent event) {
    LfRfid* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == T5577MwSubmenuIndexWriteFirstKey) {
            scene_manager_set_scene_state(
                app->scene_manager, LfRfidSceneT5577MwMenu, T5577MwSubmenuIndexWriteFirstKey);
            scene_manager_next_scene(app->scene_manager, LfRfidSceneT5577MwSelectFirstKey);
            consumed = true;
        } else if(event.event == T5577MwSubmenuIndexWriteSecondKey) {
            scene_manager_set_scene_state(
                app->scene_manager, LfRfidSceneT5577MwMenu, T5577MwSubmenuIndexWriteSecondKey);
            scene_manager_next_scene(app->scene_manager, LfRfidSceneT5577MwSelectSecondKey);
            consumed = true;
        } else if(event.event == T5577MwSubmenuIndexWriteThirdKey) {
            scene_manager_set_scene_state(
                app->scene_manager, LfRfidSceneT5577MwMenu, T5577MwSubmenuIndexWriteThirdKey);
            scene_manager_next_scene(app->scene_manager, LfRfidSceneT5577MwSelectThirdKey);
            consumed = true;
        }
    }

    return consumed;
}

void lfrfid_scene_t5577mw_menu_on_exit(void* context) {
    LfRfid* app = context;

    submenu_reset(app->submenu);
}
