#include "../ibutton_i.h"
#include "../fuzzer/fuzzer.h"

void ibutton_scene_fuzzer_on_enter(void* context) {
    iButton* ibutton = context;

    fuzzer_ibtn_run();

    view_dispatcher_send_custom_event(ibutton->view_dispatcher, iButtonCustomEventFuzzerExit);
}

bool ibutton_scene_fuzzer_on_event(void* context, SceneManagerEvent event) {
    iButton* ibutton = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == iButtonCustomEventFuzzerExit) {
            scene_manager_previous_scene(ibutton->scene_manager);
            consumed = true;
        }
    }

    return consumed;
}

void ibutton_scene_fuzzer_on_exit(void* context) {
    UNUSED(context);
}
