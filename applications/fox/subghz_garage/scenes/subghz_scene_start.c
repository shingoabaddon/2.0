#include "../subghz_i.h"
#include "../views/subghz_view_start_grid.h"
#include "../helpers/fox_theme_compat.h"
#include "subghz_scene_start.h"
#include <dolphin/dolphin.h>
#include <loader/loader.h>
#include <storage/storage.h>
#include <furi/core/memmgr.h>

#include <lib/subghz/protocols/raw.h>

/* Uses loader_enqueue_launch() (not loader_start() — locked while any app runs).
 * Passes full FAP path — custom FAPs are not in the loader name catalog. */

#define SUBGHZ_MOD_ANALYZER_FAP_PATH  EXT_PATH("apps/Sub-GHz/subghz_modulation_analyzer.fap")
#define SUBGHZ_FREQ_ANALYZER_FAP_PATH EXT_PATH("apps/Sub-GHz/subghz_frequency_analyzer.fap")

void subghz_blank_transition_draw_cb(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
}

void subghz_scene_start_launch_and_exit(
    SubGhz* subghz, const char* fap_path, const char* args) {
    Loader* loader = furi_record_open(RECORD_LOADER);
    loader_enqueue_launch(loader, fap_path, args, LoaderDeferredLaunchFlagNone);
    furi_record_close(RECORD_LOADER);

    /* Cover the screen with a blank page BEFORE tearing down SubGHz's own
     * GUI presence. view_dispatcher_stop() fully detaches SubGHz's view
     * from the GUI, which briefly reveals the Desktop/Apps menu
     * underneath before the next app's own view attaches — this
     * standalone viewport (independent of the ViewDispatcher being torn
     * down) gives the screen something of ours to show during that exact
     * gap instead. Freed in subghz_free(), right before the app's thread
     * truly ends — see the struct field comment in subghz_i.h. */
    subghz->blank_transition_viewport = view_port_alloc();
    view_port_draw_callback_set(
        subghz->blank_transition_viewport, subghz_blank_transition_draw_cb, NULL);
    gui_add_view_port(subghz->gui, subghz->blank_transition_viewport, GuiLayerFullscreen);
    view_port_update(subghz->blank_transition_viewport);

    scene_manager_stop(subghz->scene_manager);
    view_dispatcher_stop(subghz->view_dispatcher);
}

void subghz_scene_start_submenu_callback(void* context, uint32_t index) {
    SubGhz* subghz = context;
    view_dispatcher_send_custom_event(subghz->view_dispatcher, index);
}

void subghz_scene_start_on_enter(void* context) {
    FURI_LOG_I("SubGhzSceneStart", "on_enter");
    SubGhz* subghz = context;

    /* Dismiss the startup loading wheel now that the start grid is
     * ready to display.  SubGhz's own viewport takes over immediately. */
    if(subghz->startup_holder) {
        view_holder_set_view(subghz->startup_holder, NULL);
        view_holder_free(subghz->startup_holder);
        subghz->startup_holder = NULL;
    }
    if(subghz->startup_loading) {
        loading_free(subghz->startup_loading);
        subghz->startup_loading = NULL;
    }
    if(subghz->state_notifications == SubGhzNotificationStateStarting) {
        subghz->state_notifications = SubGhzNotificationStateIDLE;
    }

    /* Flipper RAM heads-up - checked once, the first time Start is reached
     * this app session (not on every return from a sub-scene). Purely
     * informational: it doesn't lock anything itself. The actual RAM
     * protection is live and conditional, applied only at the point of
     * real need (Read/Read RAW/Decode RAW/Emulate's own on_enter/Tick
     * handlers - see subghz_low_ram_mitigate() in subghz_i.h) so
     * qFlipper's screen-mirror keeps working normally everywhere else,
     * including here at Start. This screen just warns up front rather
     * than only surfacing the first time the user happens to hit Read.
     *
     * Uses SUBGHZ_LOW_RAM_FREE_HEAP (the "everything else" tier -
     * ReceiverInfo/saved-file-load's own checks use the same one). Read/
     * Read RAW/Decode RAW/Emulate use the lower SUBGHZ_LOW_RAM_FREE_HEAP_
     * READ tier instead (see that constant's own comment) - their RX
     * worker alone normally sits below this screen's threshold, so this
     * pre-flight warning stays deliberately conservative rather than
     * chasing that lower number and going quiet for the one case where
     * heap is actually tightest.
     *
     * qFlipper's OR clause uses rpc_gui_screen_stream_is_active() - true
     * only while qFlipper's screen-mirror view is actively pulling frames
     * (rpc_gui.c's StartScreenStream handler), not merely "qFlipper is
     * connected." A qFlipper connected but sitting on a different tab
     * doesn't set this - the heap check above is what catches that case
     * (mirroring costs real RAM the instant it starts, whenever that is). */
    if(!subghz->shared_ram_warning_shown) {
        subghz->shared_ram_warning_shown = true;
        bool qflipper_active = rpc_gui_screen_stream_is_active();
        bool low_ram = memmgr_get_free_heap() < SUBGHZ_LOW_RAM_FREE_HEAP;
        if(qflipper_active || low_ram) {
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneSharedRamWarning);
            return;
        }
    }

    /* Check which FAPs are installed — FA/MA are conditional */
    Storage* storage = furi_record_open(RECORD_STORAGE);
    bool has_freq_analyzer = storage_file_exists(storage, SUBGHZ_FREQ_ANALYZER_FAP_PATH);
    bool has_mod_analyzer  = storage_file_exists(storage, SUBGHZ_MOD_ANALYZER_FAP_PATH);
    furi_record_close(RECORD_STORAGE);

    /* Configure the Fox-theme grid — show/hide conditional buttons */
    subghz_start_grid_set_visible(subghz->start_grid, SGRID_IDX_FREQANA, has_freq_analyzer);
    subghz_start_grid_set_visible(subghz->start_grid, SGRID_IDX_MODANA,  has_mod_analyzer);

    /* Wire callback so grid button presses fire scene custom events */
    subghz_start_grid_set_callback(
        subghz->start_grid,
        subghz_scene_start_submenu_callback,
        subghz);

    /* Restore focus when returning from FA/MA or other scenes.
     * scene_manager_get_scene_state returns the SubmenuIndex value that
     * was last active — map it to the grid button index. */
    {
        uint32_t focus =
            scene_manager_get_scene_state(subghz->scene_manager, SubGhzSceneStart);
        uint8_t grid_btn = 0; /* default: Read */
        if     (focus == SubmenuIndexSaved)                grid_btn = SGRID_IDX_SAVED;
        else if(focus == SubmenuIndexReadRAW)              grid_btn = SGRID_IDX_READRAW;
        else if(focus == SubmenuIndexFrequencyAnalyzer)    grid_btn = SGRID_IDX_FREQANA;
        else if(focus == SubmenuIndexModulationAnalyzer)   grid_btn = SGRID_IDX_MODANA;
        else if(focus == SubmenuIndexProtocolList)         grid_btn = SGRID_IDX_PROTOCOLS;
        subghz_start_grid_set_selected(subghz->start_grid, grid_btn);
    }

    if(fox_theme_is_active()) {
        /* Fox Theme: show the custom button grid */
        view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdStartGrid);
    } else {
        /* Classic Theme: fall back to the standard vertical submenu.
         * Build the classic menu items matching the grid's SubmenuIndex values. */
        uint32_t focus =
            scene_manager_get_scene_state(subghz->scene_manager, SubGhzSceneStart);
        subghz_ensure_submenu(subghz);
        submenu_reset(subghz->submenu);
        submenu_add_item(subghz->submenu, "Read", SubmenuIndexRead,
            subghz_scene_start_submenu_callback, subghz);
        submenu_add_item(subghz->submenu, "Read RAW", SubmenuIndexReadRAW,
            subghz_scene_start_submenu_callback, subghz);
        submenu_add_item(subghz->submenu, "Saved", SubmenuIndexSaved,
            subghz_scene_start_submenu_callback, subghz);
        if(has_freq_analyzer)
            submenu_add_item(subghz->submenu, "Freq. Analyzer", SubmenuIndexFrequencyAnalyzer,
                subghz_scene_start_submenu_callback, subghz);
        if(has_mod_analyzer)
            submenu_add_item(subghz->submenu, "Mod. Analyzer", SubmenuIndexModulationAnalyzer,
                subghz_scene_start_submenu_callback, subghz);
        submenu_add_item(subghz->submenu, "Protocol Group", SubmenuIndexProtocolList,
            subghz_scene_start_submenu_callback, subghz);
        submenu_set_selected_item(subghz->submenu, focus);
        view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdMenu);
    }
}

bool subghz_scene_start_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;
    FURI_LOG_I(
        "SubGhzSceneStart", "on_event: type=%d event=%lu", (int)event.type, (unsigned long)event.event);
    if(event.type == SceneManagerEventTypeBack) {
        if(subghz->launched_from_mode_picker) {
            /* Launched from core subghz's Mode Picker - return there
             * instead of exiting straight to the Apps menu. "subghz" is
             * the Loader's catalog name for the built-in app (it's not an
             * external FAP, so there's no path to pass). Same .focus_menu
             * marker mechanism FA/MA use to reopen with a specific item
             * focused - "menu:garage" tells the Mode Picker to pre-select
             * Garage instead of defaulting to Automotive. */
            Storage* storage = furi_record_open(RECORD_STORAGE);
            File* f = storage_file_alloc(storage);
            if(storage_file_open(f, "/ext/subghz/.focus_menu", FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
                storage_file_write(f, "menu:garage", strlen("menu:garage"));
            }
            storage_file_close(f);
            storage_file_free(f);
            furi_record_close(RECORD_STORAGE);
            subghz_scene_start_launch_and_exit(subghz, "subghz", NULL);
        } else {
            //exit app
            scene_manager_stop(subghz->scene_manager);
            view_dispatcher_stop(subghz->view_dispatcher);
        }
        return true;
    } else if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexReadRAW) {
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzSceneStart, SubmenuIndexReadRAW);
            subghz_rx_key_state_set(subghz, SubGhzRxKeyStateIDLE);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReadRAW);
            return true;
        } else if(event.event == SubmenuIndexRead) {
            subghz_rx_key_state_set(subghz, SubGhzRxKeyStateIDLE);
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzSceneStart, SubmenuIndexRead);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReceiver);
            return true;
        } else if(event.event == SubmenuIndexSaved) {
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzSceneStart, SubmenuIndexSaved);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneSaved);
            return true;
        } else if(event.event == SubmenuIndexFrequencyAnalyzer) {
            /* Moved to an external FAP to save firmware .text space — see
             * applications/main/subghz_frequency_analyzer/. SubGHz exits
             * cleanly and the Loader launches the FAP right after — see
             * the comment above subghz_scene_start_launch_and_exit(). The
             * "garage:" prefix tells the FAP which app to relaunch on
             * Back (it's shared with core Automotive too); "menu:freq"
             * is what lets it send us back to this exact menu item
             * instead of the Desktop. */
            dolphin_deed(DolphinDeedSubGhzFrequencyAnalyzer);
            subghz_scene_start_launch_and_exit(
                subghz, SUBGHZ_FREQ_ANALYZER_FAP_PATH, "garage:menu:freq");
            return true;
        } else if(event.event == SubmenuIndexModulationAnalyzer) {
            /* Moved to an external FAP to save firmware .text space — see
             * applications/main/subghz_modulation_analyzer/. */
            subghz_scene_start_launch_and_exit(
                subghz, SUBGHZ_MOD_ANALYZER_FAP_PATH, "garage:menu:mod");
            return true;
        } else if(event.event == SubmenuIndexProtocolList) {
            scene_manager_set_scene_state(
                subghz->scene_manager, SubGhzSceneStart, SubmenuIndexProtocolList);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneProtocolList);
        }
    }
    return false;
}

void subghz_scene_start_on_exit(void* context) {
    SubGhz* subghz = context;
    /* Reset the shared submenu widget, if it's actually been allocated.
     * Fox Theme uses the grid view (not submenu) and may never have
     * touched it at all; Classic Theme allocates it in on_enter above.
     * Either way, any stale items from a prior Classic-mode run would
     * otherwise bleed into scenes like SubGhzSceneSavedMenu that append
     * to the same widget without resetting. */
    if(subghz->submenu) {
        submenu_reset(subghz->submenu);
    }
}
