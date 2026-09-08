#include "../lfrfid_i.h"
#include "../fuzzer/fuzzer.h"

void lfrfid_scene_fuzzer_on_enter(void* context) {
    LfRfid* app = context;

    fuzzer_rfid_run();

    view_dispatcher_send_custom_event(app->view_dispatcher, LfRfidEventNext);
}

bool lfrfid_scene_fuzzer_on_event(void* context, SceneManagerEvent event) {
    LfRfid* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == LfRfidEventNext) {
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
        }
    }

    return consumed;
}

void lfrfid_scene_fuzzer_on_exit(void* context) {
    UNUSED(context);
}
