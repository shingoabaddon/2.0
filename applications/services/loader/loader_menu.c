#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/menu.h>
#include <gui/modules/submenu.h>
#include <assets_icons.h>
#include <applications.h>
#include <archive/helpers/archive_favorites.h>
#include <storage/storage.h>
#include <flipper_application/flipper_application.h>
#include <string.h>

#include "loader.h"
#include "loader_menu.h"
#include "loader_main_menu_pins.h"
#include "../desktop/desktop_settings.h"

#define TAG "LoaderMenu"

struct LoaderMenu {
    FuriThread* thread;
    void (*closed_cb)(void*);
    void* context;
};

static int32_t loader_menu_thread(void* p);

LoaderMenu* loader_menu_alloc(void (*closed_cb)(void*), void* context) {
    LoaderMenu* loader_menu = malloc(sizeof(LoaderMenu));
    loader_menu->closed_cb = closed_cb;
    loader_menu->context = context;
    loader_menu->thread = furi_thread_alloc_ex(TAG, 1024, loader_menu_thread, loader_menu);
    furi_thread_start(loader_menu->thread);
    return loader_menu;
}

void loader_menu_free(LoaderMenu* loader_menu) {
    furi_assert(loader_menu);
    furi_thread_join(loader_menu->thread);
    furi_thread_free(loader_menu->thread);
    free(loader_menu);
}

typedef enum {
    LoaderMenuViewPrimary,
    LoaderMenuViewSettings,
} LoaderMenuView;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Menu* primary_menu;
    Submenu* settings_menu;
    MainMenuPins pins;
    // menu_add_item() stores the label pointer, not a copy - these must
    // outlive the menu, so they can't be per-iteration stack locals.
    char pin_labels[MAIN_MENU_PINS_MAX][MAIN_MENU_PINS_PATH_LEN];

    // Needed to rebuild the primary menu on LoaderMenuCustomEventRefreshPins
    // - see that event's own comment below for why this exists.
    LoaderMenu* loader_menu;
    Loader* loader;
    FuriPubSubSubscription* loader_sub;
} LoaderMenuApp;

// Pins are only read from disk into app->pins at alloc time, but this menu
// instance stays alive (just hidden) for as long as the Apps menu itself
// stays open - launching Fox Settings via loader_menu_fox_settings_callback
// doesn't tear it down, it just runs on top. So a pin added/removed via Fox
// Settings' "Main Menu Apps" and saved to disk never reaches this already-
// built menu until the whole LoaderMenuApp is freed and reallocated (i.e.
// backing all the way out to the Desktop and reopening the Apps menu).
// Fixed by subscribing to the Loader's own pubsub for
// LoaderEventTypeApplicationStopped (fires whenever the app running on top
// closes, regardless of which one) and reloading+rebuilding the primary
// menu at that point - cheap even when nothing changed, and catches every
// path back to this menu, not just Fox Settings specifically.
#define LoaderMenuCustomEventRefreshPins 0

static void loader_menu_start(const char* name) {
    Loader* loader = furi_record_open(RECORD_LOADER);
    loader_start_with_gui_error(loader, name, NULL);
    furi_record_close(RECORD_LOADER);
}

static void loader_menu_apps_callback(void* context, uint32_t index) {
    UNUSED(context);
    const char* name = FLIPPER_APPS[index].name;
    loader_menu_start(name);
}

static void loader_menu_external_apps_callback(void* context, uint32_t index) {
    UNUSED(context);
    const char* path = FLIPPER_EXTERNAL_APPS[index].name;
    loader_menu_start(path);
}

static void loader_menu_pinned_callback(void* context, uint32_t index) {
    UNUSED(context);
    const char* path = (const char*)index;
    loader_menu_start(path);
}

// Fallback display label from a .fap path: strips directory and extension.
// Writes into `out` (caller-owned, at least `out_size` bytes).
static void loader_menu_pin_label_from_filename(const char* path, char* out, size_t out_size) {
    const char* slash = strrchr(path, '/');
    const char* base = slash ? slash + 1 : path;
    strlcpy(out, base, out_size);
    size_t len = strlen(out);
    if(len > 4 && strcmp(out + len - 4, ".fap") == 0) {
        out[len - 4] = '\0';
    }
}

// Prefers the user's rename, then the .fap manifest name, then the filename.
static void loader_menu_pin_label(
    Storage* storage,
    const char* path,
    const char* custom_name,
    char* out,
    size_t out_size) {
    if(custom_name && custom_name[0] != '\0') {
        strlcpy(out, custom_name, out_size);
        return;
    }

    FuriString* path_str = furi_string_alloc_set_str(path);
    FuriString* name_str = furi_string_alloc();
    uint8_t icon_buf[FAP_MANIFEST_MAX_ICON_SIZE];
    uint8_t* icon_ptr = icon_buf;

    bool loaded = flipper_application_load_name_and_icon(path_str, storage, &icon_ptr, name_str);
    if(loaded && !furi_string_empty(name_str)) {
        strlcpy(out, furi_string_get_cstr(name_str), out_size);
    } else {
        loader_menu_pin_label_from_filename(path, out, out_size);
    }

    furi_string_free(path_str);
    furi_string_free(name_str);
}

static void loader_menu_applications_callback(void* context, uint32_t index) {
    UNUSED(index);
    UNUSED(context);
    const char* name = LOADER_APPLICATIONS_NAME;
    loader_menu_start(name);
}

static void
    loader_menu_settings_menu_callback(void* context, InputType input_type, uint32_t index) {
    UNUSED(context);
    if(input_type == InputTypeShort) {
        loader_menu_start((const char*)index);
    } else if(input_type == InputTypeLong) {
        archive_favorites_handle_setting_pin_unpin((const char*)index, NULL);
    }
}

static void loader_menu_fox_settings_callback(void* context, uint32_t index) {
    UNUSED(context);
    UNUSED(index);
    loader_menu_start("/ext/apps/Fox/desktop_settings.fap");
}

static void loader_menu_switch_to_settings(void* context, uint32_t index) {
    UNUSED(index);
    LoaderMenuApp* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, LoaderMenuViewSettings);
}

static uint32_t loader_menu_switch_to_primary(void* context) {
    UNUSED(context);
    return LoaderMenuViewPrimary;
}

static uint32_t loader_menu_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static void loader_menu_build_menu(LoaderMenuApp* app, LoaderMenu* menu) {
    size_t i = 0;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    for(size_t p = 0; p < app->pins.count; p++) {
        loader_menu_pin_label(
            storage,
            app->pins.paths[p],
            app->pins.names[p],
            app->pin_labels[p],
            sizeof(app->pin_labels[p]));
        menu_add_item(
            app->primary_menu,
            app->pin_labels[p],
            &A_Star_14,
            (uint32_t)app->pins.paths[p],
            loader_menu_pinned_callback,
            (void*)menu);
    }
    furi_record_close(RECORD_STORAGE);

    menu_add_item(
        app->primary_menu,
        LOADER_APPLICATIONS_NAME,
        &A_Plugins_14,
        i++,
        loader_menu_applications_callback,
        (void*)menu);

    for(i = 0; i < FLIPPER_APPS_COUNT; i++) {
        menu_add_item(
            app->primary_menu,
            FLIPPER_APPS[i].name,
            FLIPPER_APPS[i].icon,
            i,
            loader_menu_apps_callback,
            (void*)menu);
    }

    for(i = 0; i < FLIPPER_EXTERNAL_APPS_COUNT; i++) {
        menu_add_item(
            app->primary_menu,
            FLIPPER_EXTERNAL_APPS[i].name,
            FLIPPER_EXTERNAL_APPS[i].icon,
            i,
            loader_menu_external_apps_callback,
            (void*)menu);
    }

    menu_add_item(
        app->primary_menu,
        "Fox Settings",
        &A_Settings_14,
        i++,
        loader_menu_fox_settings_callback,
        (void*)menu);

    menu_add_item(
        app->primary_menu, "Settings", &A_Settings_14, i++, loader_menu_switch_to_settings, app);
}

static void loader_menu_build_submenu(LoaderMenuApp* app, LoaderMenu* loader_menu) {
    for(size_t i = 0; i < FLIPPER_EXTSETTINGS_APPS_COUNT; i++) {
        submenu_add_item_ex(
            app->settings_menu,
            FLIPPER_EXTSETTINGS_APPS[i].name,
            (uint32_t)FLIPPER_EXTSETTINGS_APPS[i].name,
            loader_menu_settings_menu_callback,
            loader_menu);
    }
    for(size_t i = 0; i < FLIPPER_SETTINGS_APPS_COUNT; i++) {
        submenu_add_item_ex(
            app->settings_menu,
            FLIPPER_SETTINGS_APPS[i].name,
            (uint32_t)FLIPPER_SETTINGS_APPS[i].name,
            loader_menu_settings_menu_callback,
            loader_menu);
    }
}

static bool loader_menu_custom_event_callback(void* context, uint32_t event) {
    LoaderMenuApp* app = context;
    if(event == LoaderMenuCustomEventRefreshPins) {
        main_menu_pins_load(&app->pins);
        menu_reset(app->primary_menu);
        loader_menu_build_menu(app, app->loader_menu);
    }
    return true;
}

// Runs on the Loader service's own thread (furi_pubsub_publish's caller),
// not this app's - just marshal onto our own ViewDispatcher via a custom
// event instead of touching app->primary_menu here directly.
static void loader_menu_loader_pubsub_callback(const void* message, void* context) {
    LoaderMenuApp* app = context;
    const LoaderEvent* event = message;
    if(event->type == LoaderEventTypeApplicationStopped) {
        view_dispatcher_send_custom_event(app->view_dispatcher, LoaderMenuCustomEventRefreshPins);
    }
}

static LoaderMenuApp* loader_menu_app_alloc(LoaderMenu* loader_menu) {
    LoaderMenuApp* app = malloc(sizeof(LoaderMenuApp));
    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    app->primary_menu = menu_alloc();
    app->settings_menu = submenu_alloc();
    app->loader_menu = loader_menu;
    main_menu_pins_load(&app->pins);

    app->loader = furi_record_open(RECORD_LOADER);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, loader_menu_custom_event_callback);
    app->loader_sub = furi_pubsub_subscribe(
        loader_get_pubsub(app->loader), loader_menu_loader_pubsub_callback, app);

    // Only the primary Apps menu picks up the user's Fox Theme/Carousel
    // choice - menu_alloc() defaults every Menu instance to Classic so any
    // other app pulling in this shared widget for its own internal menu
    // isn't affected by that setting. Heap-allocated, not stack: this
    // thread only has a 1024-byte stack and DesktopSettings is sizable.
    DesktopSettings* settings = malloc(sizeof(DesktopSettings));
    if(settings) {
        desktop_settings_load(settings);
        menu_set_theme(app->primary_menu, settings->menu_theme);
        free(settings);
    }

    loader_menu_build_menu(app, loader_menu);
    loader_menu_build_submenu(app, loader_menu);

    // Primary menu
    View* primary_view = menu_get_view(app->primary_menu);
    view_set_context(primary_view, app->primary_menu);
    view_set_previous_callback(primary_view, loader_menu_exit);
    view_dispatcher_add_view(app->view_dispatcher, LoaderMenuViewPrimary, primary_view);

    // Settings menu
    View* settings_view = submenu_get_view(app->settings_menu);
    view_set_context(settings_view, app->settings_menu);
    view_set_previous_callback(settings_view, loader_menu_switch_to_primary);
    view_dispatcher_add_view(app->view_dispatcher, LoaderMenuViewSettings, settings_view);
    view_dispatcher_switch_to_view(app->view_dispatcher, LoaderMenuViewPrimary);

    return app;
}

static void loader_menu_app_free(LoaderMenuApp* app) {
    furi_pubsub_unsubscribe(loader_get_pubsub(app->loader), app->loader_sub);
    furi_record_close(RECORD_LOADER);

    view_dispatcher_remove_view(app->view_dispatcher, LoaderMenuViewPrimary);
    view_dispatcher_remove_view(app->view_dispatcher, LoaderMenuViewSettings);
    view_dispatcher_free(app->view_dispatcher);

    menu_free(app->primary_menu);
    submenu_free(app->settings_menu);
    furi_record_close(RECORD_GUI);
    free(app);
}

static int32_t loader_menu_thread(void* p) {
    LoaderMenu* loader_menu = p;
    furi_assert(loader_menu);

    LoaderMenuApp* app = loader_menu_app_alloc(loader_menu);

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_run(app->view_dispatcher);

    if(loader_menu->closed_cb) {
        loader_menu->closed_cb(loader_menu->context);
    }

    loader_menu_app_free(app);

    return 0;
}