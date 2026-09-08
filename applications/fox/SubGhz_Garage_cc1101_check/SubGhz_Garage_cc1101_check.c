/* Probes for an external CC1101 module, caches the result to a flag file
 * so Garage/Gate/Other can trust it instead of paying the plugin-load
 * cost to check itself. Launched from Mode Picker (marker file carries
 * Garage's own launch args) or directly from the Apps menu - both open
 * Garage/Gate/Other after the probe. Desktop's own periodic background
 * check runs the same probe inline instead of launching this app. */

#include <furi.h>
#include <storage/storage.h>
#include <loader/loader.h>
#include <lib/subghz/devices/devices.h>
#include <applications/drivers/subghz/cc1101_ext/cc1101_ext_interconnect.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/canvas.h>

#define TAG "SubGhzGarageCC1101Check"
#define CC1101_EXT_STATUS_PATH     EXT_PATH("subghz/.cc1101_ext_status")
#define CC1101_PROBE_RELAUNCH_PATH EXT_PATH("subghz/.cc1101_probe_relaunch")
#define SUBGHZ_GARAGE_FAP_PATH     EXT_PATH("apps/Sub-GHz/subghz_garage.fap")

/* Returns true if a Mode-Picker-style relaunch marker was found and acted
 * on (Garage already enqueued), false otherwise. */
static bool subghz_garage_cc1101_check_relaunch_garage_if_requested(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage_file_exists(storage, CC1101_PROBE_RELAUNCH_PATH)) {
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    char args_buf[32] = {0};
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, CC1101_PROBE_RELAUNCH_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        uint16_t read = storage_file_read(file, args_buf, sizeof(args_buf) - 1);
        args_buf[read] = '\0';
    }
    storage_file_close(file);
    storage_file_free(file);
    storage_simply_remove(storage, CC1101_PROBE_RELAUNCH_PATH);
    furi_record_close(RECORD_STORAGE);

    Loader* loader = furi_record_open(RECORD_LOADER);
    loader_enqueue_launch(
        loader,
        SUBGHZ_GARAGE_FAP_PATH,
        args_buf[0] ? args_buf : NULL,
        LoaderDeferredLaunchFlagNone);
    furi_record_close(RECORD_LOADER);
    FURI_LOG_I(TAG, "Relaunching Garage/Gate/Other with args \"%s\"", args_buf);
    return true;
}

static void subghz_garage_cc1101_check_blank_draw_cb(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
}

int32_t subghz_garage_cc1101_check_app(void* p) {
    UNUSED(p);

    /* Cover the screen for this app's whole (brief) runtime, same trick as
     * subghz_scene_start_launch_and_exit() - without it, this being
     * headless let the Apps menu flash through underneath. */
    Gui* gui = furi_record_open(RECORD_GUI);
    ViewPort* blank_viewport = view_port_alloc();
    view_port_draw_callback_set(blank_viewport, subghz_garage_cc1101_check_blank_draw_cb, NULL);
    gui_add_view_port(gui, blank_viewport, GuiLayerFullscreen);
    view_port_update(blank_viewport);

    bool connected = false;

    subghz_devices_init_internal_only();
    if(subghz_devices_load_external()) {
        const SubGhzDevice* device = subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_EXT_NAME);
        if(device) {
            connected = subghz_devices_is_connect(device);
        }
    }
    subghz_devices_deinit();

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, CC1101_EXT_STATUS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        char c = connected ? '1' : '0';
        storage_file_write(file, &c, 1);
        storage_file_close(file);
    } else {
        FURI_LOG_W(TAG, "Couldn't write %s", CC1101_EXT_STATUS_PATH);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    FURI_LOG_I(TAG, "External CC1101 %s", connected ? "detected" : "not detected");

    if(!subghz_garage_cc1101_check_relaunch_garage_if_requested()) {
        /* No Mode-Picker marker was pending - the only other caller is a
         * manual launch from the Apps menu, so open Garage/Gate/Other now. */
        FURI_LOG_I(TAG, "Launched from Apps menu - opening Garage/Gate/Other");
        Loader* loader = furi_record_open(RECORD_LOADER);
        loader_enqueue_launch(loader, SUBGHZ_GARAGE_FAP_PATH, NULL, LoaderDeferredLaunchFlagNone);
        furi_record_close(RECORD_LOADER);
    }

    gui_remove_view_port(gui, blank_viewport);
    view_port_free(blank_viewport);
    furi_record_close(RECORD_GUI);

    return 0;
}
