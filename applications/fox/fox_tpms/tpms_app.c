#include "tpms_app_i.h"

#include <furi.h>
#include <furi_hal.h>
#include <string.h>
#include <storage/storage.h>
#include <loader/loader.h>
#include "protocols/protocol_items.h"

#define TAG "TPMS"

static bool tpms_app_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    TPMSApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool tpms_app_back_event_callback(void* context) {
    furi_assert(context);
    TPMSApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void tpms_app_tick_event_callback(void* context) {
    furi_assert(context);
    TPMSApp* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

TPMSApp* tpms_app_alloc() {
    FURI_LOG_I(TAG, "tpms_app_alloc: enter, free heap %zu", memmgr_get_free_heap());

    TPMSApp* app = malloc(sizeof(TPMSApp));

    // GUI
    app->gui = furi_record_open(RECORD_GUI);

    // View Dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&tpms_scene_handlers, app);
    

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, tpms_app_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, tpms_app_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        app->view_dispatcher, tpms_app_tick_event_callback, 100);

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Open Notification record
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    // Variable Item List
    app->variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        TPMSViewVariableItemList,
        variable_item_list_get_view(app->variable_item_list));

    // SubMenu
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, TPMSViewSubmenu, submenu_get_view(app->submenu));

    // Widget
    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, TPMSViewWidget, widget_get_view(app->widget));

    // Receiver
    app->tpms_receiver = tpms_view_receiver_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, TPMSViewReceiver, tpms_view_receiver_get_view(app->tpms_receiver));

    // Receiver Info
    app->tpms_receiver_info = tpms_view_receiver_info_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        TPMSViewReceiverInfo,
        tpms_view_receiver_info_get_view(app->tpms_receiver_info));

    // Number Input / Byte Input (field editing)
    app->number_input = number_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, TPMSViewNumberInput, number_input_get_view(app->number_input));
    app->byte_input = byte_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, TPMSViewByteInput, byte_input_get_view(app->byte_input));

    //init setting
    app->setting = subghz_setting_alloc();

    //ToDo FIX  file name setting
    subghz_setting_load(app->setting, EXT_PATH("subghz/assets/setting_user"));

    //init Worker & Protocol & History
    app->lock = TPMSLockOff;
    app->txrx = malloc(sizeof(TPMSTxRx));
    app->txrx->preset = malloc(sizeof(SubGhzRadioPreset));
    app->txrx->preset->name = furi_string_alloc();
    tpms_preset_init(app, "AM650", subghz_setting_get_default_frequency(app->setting), NULL, 0);

    app->txrx->hopper_state = TPMSHopperStateOFF;
    app->txrx->history = tpms_history_alloc();
    app->txrx->worker = subghz_worker_alloc();
    app->txrx->environment = subghz_environment_alloc();
    subghz_environment_set_protocol_registry(
        app->txrx->environment, (void*)&tpms_protocol_registry);
    app->txrx->receiver = subghz_receiver_alloc_init(app->txrx->environment);

    subghz_devices_init();

    app->txrx->radio_device =
        radio_device_loader_set(app->txrx->radio_device, SubGhzRadioDeviceTypeExternalCC1101);

    subghz_devices_reset(app->txrx->radio_device);
    subghz_devices_idle(app->txrx->radio_device);

    subghz_receiver_set_filter(app->txrx->receiver, SubGhzProtocolFlag_Decodable);
    subghz_worker_set_overrun_callback(
        app->txrx->worker, (SubGhzWorkerOverrunCallback)subghz_receiver_reset);
    subghz_worker_set_pair_callback(
        app->txrx->worker, (SubGhzWorkerPairCallback)subghz_receiver_decode);
    subghz_worker_set_context(app->txrx->worker, app->txrx->receiver);

    furi_hal_power_suppress_charge_enter();

    scene_manager_next_scene(app->scene_manager, TPMSSceneReceiver);

    return app;
}

void tpms_app_free(TPMSApp* app) {
    furi_assert(app);

    subghz_devices_sleep(app->txrx->radio_device);
    radio_device_loader_end(app->txrx->radio_device);

    subghz_devices_deinit();

    // Submenu
    view_dispatcher_remove_view(app->view_dispatcher, TPMSViewSubmenu);
    submenu_free(app->submenu);

    // Variable Item List
    view_dispatcher_remove_view(app->view_dispatcher, TPMSViewVariableItemList);
    variable_item_list_free(app->variable_item_list);

    //  Widget
    view_dispatcher_remove_view(app->view_dispatcher, TPMSViewWidget);
    widget_free(app->widget);

    // Receiver
    view_dispatcher_remove_view(app->view_dispatcher, TPMSViewReceiver);
    tpms_view_receiver_free(app->tpms_receiver);

    // Receiver Info
    view_dispatcher_remove_view(app->view_dispatcher, TPMSViewReceiverInfo);
    tpms_view_receiver_info_free(app->tpms_receiver_info);

    // Number Input / Byte Input
    view_dispatcher_remove_view(app->view_dispatcher, TPMSViewNumberInput);
    number_input_free(app->number_input);
    view_dispatcher_remove_view(app->view_dispatcher, TPMSViewByteInput);
    byte_input_free(app->byte_input);

    //setting
    subghz_setting_free(app->setting);

    //Worker & Protocol & History
    subghz_receiver_free(app->txrx->receiver);
    subghz_environment_free(app->txrx->environment);
    tpms_history_free(app->txrx->history);
    subghz_worker_free(app->txrx->worker);
    furi_string_free(app->txrx->preset->name);
    free(app->txrx->preset);
    free(app->txrx);

    // View dispatcher
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);
    app->notifications = NULL;

    // Close records
    furi_record_close(RECORD_GUI);

    furi_hal_power_suppress_charge_exit();

    free(app);

    // Logged after every resource above is freed, so the next app's own boot-time free-heap log
    // (Garage's "subghz_alloc: enter" line, for example) can be compared against this one - a
    // large gap between the two despite this app's own alloc/free pairing looking complete would
    // point at heap fragmentation rather than a genuine leak in this app.
    FURI_LOG_I(TAG, "tpms_app_free: exit, free heap %zu", memmgr_get_free_heap());
}

// Launched with "core:<marker>" from subghz's Start Grid (see
// subghz_scene_start.c), the same convention used by the Frequency/
// Modulation Analyzer FAPs. On exit, writes <marker> to /ext/subghz/
// .focus_menu and relaunches subghz, which reads it on_enter to reopen
// with this app's button selected - matching F.A/M.A's return-to-caller
// behavior. Launched standalone from the Apps menu (no arg), it just exits.
int32_t tpms_app(void* p) {
    char return_marker[24] = {0};
    if(p && ((const char*)p)[0]) {
        const char* arg = (const char*)p;
        if(strncmp(arg, "core:", 5) == 0) {
            arg += 5;
        }
        strncpy(return_marker, arg, sizeof(return_marker) - 1);
    }

    TPMSApp* tpms_app = tpms_app_alloc();

    view_dispatcher_run(tpms_app->view_dispatcher);

    tpms_app_free(tpms_app);

    if(return_marker[0]) {
        Storage* storage = furi_record_open(RECORD_STORAGE);
        File* file = storage_file_alloc(storage);
        if(storage_file_open(file, "/ext/subghz/.focus_menu", FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            storage_file_write(file, return_marker, strlen(return_marker));
        }
        storage_file_close(file);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);

        Loader* loader = furi_record_open(RECORD_LOADER);
        loader_enqueue_launch(loader, "subghz", NULL, LoaderDeferredLaunchFlagNone);
        furi_record_close(RECORD_LOADER);
    }

    return 0;
}
