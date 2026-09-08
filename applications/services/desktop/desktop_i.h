#pragma once

#include "desktop.h"
#include "desktop_settings.h"

#include "animations/animation_manager.h"
#include "views/desktop_view_pin_timeout.h"
#include "views/desktop_view_pin_input.h"
#include "views/desktop_view_locked.h"
#include "views/desktop_view_main.h"
#include "views/desktop_view_lock_menu.h"
#include "views/desktop_view_debug.h"
#include "views/desktop_view_slideshow.h"
#include "views/desktop_view_clock_lock.h"

#include <gui/gui.h>
#include <gui/view_stack.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/popup.h>
#include <gui/scene_manager.h>

#include <loader/loader.h>
#include <notification/notification_app.h>

#define STATUS_BAR_Y_SHIFT 13

typedef enum {
    DesktopViewIdMain,
    DesktopViewIdLockMenu,
    DesktopViewIdLocked,
    DesktopViewIdDebug,
    DesktopViewIdPopup,
    DesktopViewIdPinInput,
    DesktopViewIdPinTimeout,
    DesktopViewIdSlideshow,
    DesktopViewIdClockLock,
    DesktopViewIdTotal,
} DesktopViewId;

typedef struct {
    uint8_t hour;
    uint8_t minute;
    bool format_12;
} DesktopClock;

struct Desktop {
    FuriThread* scene_thread;

    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    Popup* popup;
    DesktopLockMenuView* lock_menu;
    DesktopDebugView* debug_view;
    DesktopViewLocked* locked_view;
    DesktopMainView* main_view;
    DesktopViewPinTimeout* pin_timeout_view;
    DesktopSlideshowView* slideshow_view;
    DesktopViewPinInput* pin_input_view;
    DesktopClockLockView* clock_lock_view;

    ViewStack* main_view_stack;
    ViewStack* locked_view_stack;

    ViewPort* lock_icon_viewport;
    ViewPort* clock_viewport;
    ViewPort* stealth_mode_icon_viewport;
    ViewPort* no_sd_viewport;  // Shown when SD card is ejected mid-session

    // Fox ESP32 WiFi / CC1101 status icon - always visible (unlike
    // lock_icon_viewport, which toggles on/off), shows one of three states:
    // WiFi connected, CC1101 external module connected (only checked/shown
    // if WiFi isn't), or neither. The icon itself just reads two small flag
    // files on the SD card every update_wifi_timer tick (cheap, frequent,
    // never touches the UART or the subghz device registry directly) -
    // FOX_ESP32_WIFI_STATUS_PATH (kept fresh by whichever Fox ESP32 app the
    // user has open, or by wifi_recheck_thread below when nothing is) and
    // CC1101_EXT_STATUS_PATH (kept fresh by wifi_recheck_thread too - see
    // desktop_cc1101_ext_check() in desktop.c). wifi_recheck_thread runs
    // both the WiFi UART probe and the CC1101 probe-app launch on the same
    // cadence (fast once an ESP32's ever answered, slow discovery cadence
    // until then), and only while nothing else is running and the device
    // isn't locked - see desktop_wifi_recheck_thread()'s header comment in
    // desktop.c for the full reasoning.
    ViewPort* wifi_icon_viewport;
    FuriTimer* update_wifi_timer;
    FuriThread* wifi_recheck_thread;
    bool wifi_connected;
    bool cc1101_connected;
    bool pending_slideshow;  // Set at boot when fox_setup needs to run before the
                              // slideshow; consumed in DesktopGlobalAfterAppFinished
                              // once fox_setup exits — no timer guessing involved.

    View* wallpaper_view;
    uint8_t* wallpaper_data;
    FuriMutex* wallpaper_mutex; // guards wallpaper_data - written from the check timer /
                                 // settings-save event, read from the draw callback, different
                                 // threads
    FuriTimer* wallpaper_check_timer; // polls for a pending web-install activation + edited file

    Loader* loader;
    Storage* storage;
    NotificationApp* notification;

    FuriPubSub* status_pubsub;
    FuriPubSub* input_events_pubsub;
    FuriPubSubSubscription* input_events_subscription;

    FuriTimer* auto_lock_timer;
    FuriTimer* update_clock_timer;

    AnimationManager* animation_manager;
    FuriSemaphore* animation_semaphore;

    DesktopClock clock;
    DesktopSettings settings;

    bool in_transition;
    bool app_running;
    bool locked;

    // Fox Alarm Clock - see desktop_check_alarms()/desktop_trigger_alarm_ring()
    // in desktop.c. Runs regardless of which app is in the foreground, since
    // this timer lives on the always-running Desktop service.
    FuriTimer* alarm_check_timer;
    uint16_t alarm_last_checked_stamp; // hour*60+minute of the last scan, so a
                                        // match only ever fires once per minute
    bool alarm_ringing;
    uint8_t alarm_ringing_index; // which settings.alarms[] entry is ringing
    bool on_clock_lock_scene;    // tracked by desktop_scene_clock_lock.c
    bool clock_lock_backlight_manually_off; // Left-arrow override on the Fox
                                             // Clock screen - see
                                             // desktop_scene_clock_lock.c

    // Low-RAM watchdog - see desktop_ram_watchdog_trigger()/_timer_callback()
    // in desktop.c. Runs regardless of which app is in the foreground or
    // which desktop scene is active, same as alarm_check_timer above.
    FuriTimer* ram_watchdog_timer;
    bool ram_watchdog_tripped; // true from trigger until free heap recovers
};

void desktop_lock(Desktop* desktop);
void desktop_unlock(Desktop* desktop);
void desktop_set_stealth_mode_state(Desktop* desktop, bool enabled);
void desktop_alarm_dismiss(Desktop* desktop);
void desktop_cycle_wallpaper(Desktop* desktop);
