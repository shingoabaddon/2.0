#include "../lfrfid_i.h"

static bool lfrfid_scene_t5577mw_load_em4100_key(LfRfid* app) {
    if(!lfrfid_load_key_from_file_select(app)) return false;
    return app->protocol_id == LFRFIDProtocolEM4100;
}

void lfrfid_scene_t5577mw_select_second_key_on_enter(void* context) {
    LfRfid* app = context;

    if(lfrfid_scene_t5577mw_load_em4100_key(app)) {
        scene_manager_next_scene(app->scene_manager, LfRfidSceneT5577MwWriteSecondKey);
    } else {
        scene_manager_previous_scene(app->scene_manager);
    }
}

bool lfrfid_scene_t5577mw_select_second_key_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void lfrfid_scene_t5577mw_select_second_key_on_exit(void* context) {
    UNUSED(context);
}
