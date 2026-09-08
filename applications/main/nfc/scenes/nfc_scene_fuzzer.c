#include "../nfc_app_i.h"
#include "../fuzzer/fuzzer.h"

void nfc_scene_fuzzer_on_enter(void* context) {
    NfcApp* nfc = context;

    fuzzer_mifare_run(nfc->nfc);

    view_dispatcher_send_custom_event(nfc->view_dispatcher, NfcCustomEventFuzzerExit);
}

bool nfc_scene_fuzzer_on_event(void* context, SceneManagerEvent event) {
    NfcApp* nfc = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcCustomEventFuzzerExit) {
            scene_manager_previous_scene(nfc->scene_manager);
            consumed = true;
        }
    }

    return consumed;
}

void nfc_scene_fuzzer_on_exit(void* context) {
    UNUSED(context);
}
