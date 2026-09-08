#pragma once

#include "helpers/subghz_types.h"
#include <lib/subghz/types.h>
#include "subghz.h"
#include "views/receiver.h"
#include "views/transmitter.h"
#include "views/subghz_signal_visualizer.h"
#include "subghz_protocol_filter.h"
#include "subghz_modulation_filter.h"
#include "views/subghz_view_start_grid.h"
#include "views/subghz_view_protocol_groups.h"
#include <gui/modules/loading.h>
#include <gui/view_holder.h>
#include "views/subghz_read_raw.h"

#include <gui/gui.h>
#include <gui/view_port.h>
#include <assets_icons.h>
#include <dialogs/dialogs.h>
#include <gui/scene_manager.h>
#include <notification/notification_messages.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <gui/modules/text_input.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/widget.h>

#include "scenes/subghz_scene.h"
#include <lib/subghz/subghz_file_encoder_worker.h>
#include <lib/subghz/subghz_setting.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/transmitter.h>

#include "subghz_history.h"
#include "subghz_last_settings.h"

#include <gui/modules/variable_item_list.h>
#include <lib/toolbox/path.h>

#include "rpc/rpc_app.h"
#include "helpers/rpc_gui_screen_suppress_compat.h"
#include <cli/cli_vcp.h>

#include "helpers/subghz_threshold_rssi.h"

#include "helpers/subghz_txrx.h"

#define SUBGHZ_MAX_LEN_NAME      64
#define SUBGHZ_EXT_PRESET_NAME   true
#define SUBGHZ_RAW_THRESHOLD_MIN (-90.0f)
#define SUBGHZ_MEASURE_LOADING   false

/* Recovery for both bands is gated on the single SUBGHZ_LOW_RAM_RECOVER_
 * FREE_HEAP bar below (subghz_scene_low_ram_warning.c), not a per-band
 * value:
 *
 *   CLI-lock-list band:   trip <8000,  recover >=16000
 *   Everything-else band: trip <14000, recover >=16000
 *
 * "CLI-lock-list" = Read, Read RAW, Decode RAW, Emulate - the four
 * operations that proactively soft-lock CLI/RPC (subghz_cli_soft_lock(),
 * via subghz_low_ram_mitigate()) before doing their heavy allocation, and
 * unlock again the moment that specific operation ends. Their RX worker's
 * own ~10.8KB allocation (subghz_txrx_ensure_worker(), lazily allocated
 * right at rx_start) brings a normal internal-CC1101 Listening session
 * down to ~14100-14800 free heap, confirmed across many device logs. An
 * external CC1101 session runs meaningfully lower again - a live device
 * log showed free heap settling at 9952, close enough to the old 10000
 * trip point to fire the watchdog on completely normal RX and bounce
 * (trip/recover/re-trip) repeatedly. 8000 keeps real margin below that
 * measured external-CC1101 floor and ProtoPirate's own proven-stable
 * ~6000 floor on this hardware.
 *
 * "Everything else" = menus and one-shot lookups that don't hold CLI/RPC
 * locked: the Start screen's pre-emptive warning, ReceiverInfo's
 * redisplay of an already-captured/saved signal, and loading a saved file
 * (e.g. for Emulate's preview). These don't share the lock-list's RX-
 * worker-driven floor, so they keep a higher, more conservative trip
 * point. */
#define SUBGHZ_LOW_RAM_FREE_HEAP_READ    8000
#define SUBGHZ_LOW_RAM_FREE_HEAP         14000
#define SUBGHZ_LOW_RAM_RECOVER_FREE_HEAP 16000

/* Known, fixed cost of allocating the RX worker (subghz_txrx_ensure_
 * worker(), ~10.8KB - see helpers/subghz_txrx.c's own comment), rounded up
 * for margin. Anywhere Read/Read RAW is about to pay this cost - a fresh
 * Start-button press, or the Tick handler's own "recovered, resume
 * Listening" decision - require free heap to clear SUBGHZ_LOW_RAM_
 * FREE_HEAP PLUS this margin first, not just the bare recover threshold.
 * Without this, a heap that had *just* cleared 14000 would drop back
 * under the 10000 trip point the instant the worker actually allocated
 * (its own cost is bigger than the entire 10000-14000 hysteresis gap),
 * and the watchdog would stop/recover/resume in an endless loop paying
 * and re-paying the same cost - see subghz_scene_reader.c's Tick handler
 * for where this is actually applied. */
#define SUBGHZ_GARAGE_WORKER_RAM_COST 11000

/* Post-recovery grace period (subghz->low_ram_grace_until_ms, set by
 * subghz_scene_low_ram_warning.c) - see that field's own comment below for
 * why this exists. 2 seconds gives the resumed Listening session's own
 * allocations (RAW decoder, RX worker, scratch file) real room to settle
 * before the watchdog is allowed to act on them again. */
#define SUBGHZ_LOW_RAM_GRACE_MS 2000

struct SubGhz {
    Gui* gui;
    NotificationApp* notifications;

    SubGhzTxRx* txrx;

    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;

    /* A standalone ViewPort (NOT managed by view_dispatcher) that paints
     * solid black over the full screen, registered right before SubGHz
     * exits to launch a sub-tool FAP. view_dispatcher_stop() fully tears
     * down SubGHz's own GUI presence, briefly revealing the Desktop/Apps
     * menu underneath before the next app's view attaches — this gives
     * our OWN content something to show during that exact gap instead,
     * since it's independent of the ViewDispatcher being torn down.
     * Cleaned up in subghz_free(), right before the app's thread ends. */
    ViewPort* blank_transition_viewport;

    Submenu* submenu;
    Popup* popup;
    TextInput* text_input;
    ByteInput* byte_input;
    Widget* widget;
    DialogsApp* dialogs;
    FuriString* file_path;
    FuriString* file_path_tmp;

    /* Decoded-protocol "Send" preview: when a decoded file's Emulate
     * action is triggered, the protocol is synthesized into a temporary
     * RAW capture and played through the RAW player (waveform/bargraph,
     * pause, seek). These track that state so the ORIGINAL file can be
     * restored when the user backs out, and the temp file cleaned up. */
    bool        decoded_preview_active;
    FuriString* decoded_preview_orig_path;
    char file_name_tmp[SUBGHZ_MAX_LEN_NAME];
    SubGhzNotificationState state_notifications;

    SubGhzViewReceiver* subghz_receiver;
    SubGhzViewTransmitter* subghz_transmitter;
    VariableItemList* variable_item_list;
    SubGhzProtocolGroups* protocol_groups;

    SubGhzSignalVisualizer*       subghz_signal_visualizer;
    SubGhzGarageProtocolFilter*         protocol_filter;
    SubGhzModulationFilter*        modulation_filter;
    SubGhzStartGrid*               start_grid;
    /* Startup loading wheel — shown immediately on launch, removed
     * when the start grid scene enters (hides apps menu + input). */
    Loading*                       startup_loading;
    ViewHolder*                    startup_holder;
    SubGhzReadRAW* subghz_read_raw;
    bool raw_send_only;
    /* True when launched from core subghz's Mode Picker screen ("frommode"
     * launch arg) - Start's Back handler relaunches the picker instead of
     * exiting straight to the Apps menu. */
    bool launched_from_mode_picker;

    bool save_datetime_set;
    DateTime save_datetime;

    SubGhzGarageLastSettings* last_settings;

    SubGhzProtocolFlag filter;
    FuriString* error_str;

    SubGhzFileEncoderWorker* decode_raw_file_worker_encoder;

    SubGhzThresholdRssi* threshold_rssi;
    SubGhzRxKeyState rx_key_state;
    SubGhzHistory* history;

    uint16_t idx_menu_chosen;
    SubGhzLoadTypeFile load_type_file;
    uint8_t tx_power;
    void* rpc_ctx;

    /* Set once the Low RAM Warning screen's own auto-recovery has fired
     * with qFlipper's screen-stream confirmed already closed (see
     * subghz_scene_low_ram_warning.c's Tick handler and
     * subghz_lock_cli_sessions() in subghz.c) - from that point on, no new
     * CLI/RPC (qFlipper) session can be established for the rest of THIS
     * app run, so a second reconnect can never re-trigger the same
     * low-RAM cycle again. Restored (new sessions allowed again) only in
     * subghz_free().
     *
     * Goes through cli_vcp_session_lock()/_unlock() (<cli/cli_vcp.h>) -
     * NOT cli_vcp_disable()/_enable(), and NOT raw furi_hal_usb calls.
     * Two earlier attempts both corrupted persistent, system-wide state
     * that outlives this app (CLI VCP is a permanent service, never torn
     * down when Garage exits):
     *   1. Raw furi_hal_usb_set_config(NULL, NULL) left CLI VCP's own
     *      internal bookkeeping (its previous_interface, its registered
     *      CDC callbacks) pointing at a torn-down interface, crashing the
     *      next time CLI's own code touched it.
     *   2. cli_vcp_disable() itself only flips is_enabled and touches the
     *      raw USB config/CDC callbacks - it does NOT close an already-
     *      active session (is_connected/shell/own_pipe/shell_pipe are
     *      untouched - see cli_vcp.c's CliVcpMessageTypeDisable handler).
     *      Worse, it unregisters the CDC callbacks BEFORE any pending
     *      disconnect could still signal through them, so a session that
     *      was still active (or mid-disconnect) at the moment we disabled
     *      got orphaned - shell/pipe never freed, is_connected stuck true
     *      forever, since the only callbacks that could ever flip it back
     *      were just nulled out. This survived app exit and even a
     *      relaunch (CLI VCP itself is never re-initialized) - only a
     *      full device reboot cleared it. Confirmed via the RAM Monitor
     *      app during the stuck state: 129KB free of 188KB total, 85KB
     *      max contiguous block - conclusively not a heap problem.
     * cli_vcp_session_lock() avoids both: it never touches furi_hal_usb
     * or the CDC callback registration at all, and it closes any active
     * session through the SAME safe path an organic disconnect uses
     * (signals CliVcpInternalEventDisconnected while callbacks are still
     * live, so shell/pipe get freed properly) - then blocks any new
     * session from starting until unlocked. qFlipper stays enumerated at
     * the USB descriptor level throughout (no hard disconnect/reconnect
     * flicker), it just can't open a new CLI/RPC session while locked. */
    bool cli_sessions_locked_after_recovery;

    /* Separate, genuinely reversible sibling of the latch above - locked
     * live, on-the-spot, right where Read/Read RAW/Decode RAW commit to
     * their heavy allocation chain (see subghz_low_ram_mitigate()), and
     * unlocked again on leaving, instead of staying locked for the rest
     * of the app run.
     *
     * Two mechanisms combined, in this order (see subghz_cli_soft_lock()/
     * _unlock() in subghz.c) - neither alone was enough, confirmed
     * on-device both times:
     *   1. cli_vcp_session_lock()/_unlock() - closes/reopens any active
     *      CLI/RPC session through the SAME safe path an organic
     *      disconnect uses, properly freeing its shell/pipe/RPC objects.
     *      Tried alone first: doesn't touch USB hardware at all, so
     *      qFlipper's client never notices anything happened - its
     *      screen-mirror just hung frozen on its last frame forever,
     *      never showing "Connect your Flipper."
     *   2. A hard USB teardown (furi_hal_usb_set_config(NULL, NULL), then
     *      restore) - modeled on the identical, already-proven sequence
     *      Desktop's own PIN lock screen and low-RAM watchdog both use
     *      (desktop.c's should_disconnect_usb block and desktop_ram_
     *      watchdog_trigger()). This is what actually makes qFlipper's
     *      client notice and show "Connect your Flipper." Tried alone
     *      (without step 1 first): confirmed via furi_hal_usb_cdc.c that
     *      cdc_deinit() only unregisters USB descriptor callbacks - it
     *      never touches the separate callbacks[]/cb_ctx[] table furi_
     *      hal_cdc_set_callbacks() populated, so CLI VCP's own state
     *      (is_connected, shell, pipes) is never told a disconnect
     *      happened. Any session that was active at that exact moment got
     *      orphaned instead of freed - confirmed on-device as the cause
     *      of an OOM crash after several Read/Read RAW visits (each
     *      cycle's still-active session leaked a little more, in CLI
     *      VCP's own permanent service state, surviving even a full app
     *      exit).
     * Doing step 1 before step 2 means there's no active session left for
     * step 2 to orphan - properly freed first, then the hardware comes
     * down to actually notify the client. Restored (unlocked)
     * unconditionally in subghz_free() too, same abrupt-exit safety net
     * as the latch above. */
    bool cli_sessions_soft_locked;

    /* Saved by subghz_cli_soft_lock() right before tearing USB down (see
     * above), restored by subghz_cli_soft_unlock() - void* rather than
     * FuriHalUsbInterface* so this header doesn't need to pull in
     * furi_hal_usb.h; cast back to FuriHalUsbInterface* in subghz.c where
     * it's actually used. NULL whenever cli_sessions_soft_locked is
     * false. */
    void* cli_hard_disconnect_usb_config;

    /* Set true the first time subghz_scene_start_on_enter() runs in this
     * app session (guards against re-checking/re-showing on every return
     * to Start from a sub-scene) - see subghz_scene_start.c. */
    bool shared_ram_warning_shown;

    /* Set true by subghz_scene_receiver_on_enter() just before delegating
     * into subghz_scene_read_raw_on_enter() (see subghz_scene_reader.c) -
     * tells the shared on_enter/on_event/on_exit which of the two modes
     * (auto capture+decode "Read", vs manual record/load/send "Read RAW")
     * to run. Cleared in subghz_scene_receiver_on_exit(), which always
     * fires when leaving that scene regardless of direction, so a later
     * direct entry into SubGhzSceneReadRAW never sees a stale true. */
    bool reader_read_mode;

    /* Set by subghz_scene_low_ram_warning.c's Tick handler the moment it
     * auto-recovers (heap sustained >= SUBGHZ_LOW_RAM_RECOVER_FREE_HEAP for
     * SUBGHZ_LOW_RAM_RECOVER_TICKS already) - a furi_get_tick() deadline a
     * short grace period later. Read's own low-RAM checks (subghz_scene_
     * reader.c's Tick watchdog and start_listening()'s immediate bail) both
     * skip bailing back to the warning screen while furi_get_tick() is
     * still before this deadline, even if their own instantaneous heap
     * reading looks low again. Recovery already proved real headroom a
     * moment ago via its own debounce; without this, resuming Listening
     * immediately re-runs those same zero-debounce checks and can bounce
     * straight back into the warning screen before the recovered heap gets
     * a real chance to hold - reads as the screen flashing rather than
     * settling. 0 (the default) means no grace in effect. */
    uint32_t low_ram_grace_until_ms;
};

/* Submenu, TextInput, ByteInput, Widget, VariableItemList, and
 * SignalVisualizer are allocated lazily on first use rather than eagerly
 * at boot - each of these just allocates + registers the view with
 * view_dispatcher if not already done, and is a no-op otherwise. Any
 * scene that references the matching SubGhzViewId or struct field must
 * call the matching ensure function first. */
void subghz_ensure_submenu(SubGhz* subghz);
void subghz_ensure_text_input(SubGhz* subghz);
void subghz_ensure_byte_input(SubGhz* subghz);
void subghz_ensure_widget(SubGhz* subghz);
void subghz_ensure_variable_item_list(SubGhz* subghz);
void subghz_ensure_signal_visualizer(SubGhz* subghz);
void subghz_ensure_protocol_groups(SubGhz* subghz);
void subghz_ensure_receiver_view(SubGhz* subghz);
void subghz_ensure_popup(SubGhz* subghz);
void subghz_ensure_transmitter_view(SubGhz* subghz);
void subghz_ensure_history(SubGhz* subghz);

void subghz_blink_start(SubGhz* subghz);
void subghz_blink_stop(SubGhz* subghz);

bool subghz_tx_start(SubGhz* subghz, FlipperFormat* flipper_format);
void subghz_dialog_message_freq_error(SubGhz* subghz, bool only_rx);

bool subghz_key_load(SubGhz* subghz, const char* file_path, bool show_dialog);
bool subghz_get_next_name_file(SubGhz* subghz, uint8_t max_len);
bool subghz_save_protocol_to_file(
    SubGhz* subghz,
    FlipperFormat* flipper_format,
    const char* dev_file_name);
void subghz_save_to_file(void* context);
bool subghz_load_protocol_from_file(SubGhz* subghz);
bool subghz_rename_file(SubGhz* subghz);
bool subghz_file_available(SubGhz* subghz);
bool subghz_delete_file(SubGhz* subghz);
void subghz_file_name_clear(SubGhz* subghz);
bool subghz_path_is_file(FuriString* path);
SubGhzLoadTypeFile subghz_get_load_type_file(SubGhz* subghz);

void subghz_rx_key_state_set(SubGhz* subghz, SubGhzRxKeyState state);
SubGhzRxKeyState subghz_rx_key_state_get(SubGhz* subghz);

extern const NotificationSequence subghz_sequence_rx;
void subghz_save_all(SubGhz* subghz);

/**
 * Block any new CLI/RPC (qFlipper) session from being established for the
 * rest of this app run (idempotent - a no-op if already locked). Only call
 * once qFlipper's screen-stream is confirmed already closed
 * (rpc_gui_screen_stream_is_active() == false) - meant to latch AFTER the
 * user has voluntarily disconnected, never to forcibly kick an active
 * session. See cli_sessions_locked_after_recovery's doc comment above for
 * why this goes through cli_vcp_session_lock() specifically, not
 * cli_vcp_disable() or raw furi_hal_usb calls - both of those were tried
 * and both corrupted persistent CLI VCP state badly enough to need a full
 * device reboot to clear. See subghz_scene_low_ram_warning.c's Tick
 * handler, the only caller.
 */
void subghz_lock_cli_sessions(SubGhz* subghz);

/**
 * Reversible sibling of subghz_lock_cli_sessions() above - blocks new
 * CLI/RPC (qFlipper) sessions the same safe way (cli_vcp_session_lock(),
 * see cli_sessions_locked_after_recovery's doc comment for why), but meant
 * to be toggled per scene visit rather than latched for the rest of the
 * app run. Idempotent. Not called directly by scenes any more - invoked
 * live, on-the-spot, by subghz_low_ram_mitigate() only when free heap
 * actually needs it; call subghz_cli_soft_unlock() unconditionally on
 * leaving Read/Read RAW/Decode RAW (also idempotent - a no-op if never
 * locked this visit). If qFlipper is still connected while locked, it
 * shows its own "Connect Flipper" screen (nothing extra to build for that
 * - inherent behavior of a blocked CLI/RPC session, same as it already is
 * for the existing one-way latch above).
 */
void subghz_cli_soft_lock(SubGhz* subghz);
void subghz_cli_soft_unlock(SubGhz* subghz);

/**
 * Deliberately disconnects CLI/RPC (including an actively-streaming
 * qFlipper screen-mirror) before Read/Read RAW/Decode RAW run their
 * allocation-heavy startup chain (worker alloc, radio/rx start) -
 * unconditional, not gated on current free heap. Call exactly at the
 * point the user has committed to that heavy chain (pressing Start/Rec,
 * or the equivalent programmatic restart after Config/silence-timeout/
 * decode-failure) - not on mere scene entry, so browsing to the Start
 * screen and backing out again never touches qFlipper.
 *
 * By design, this always soft-locks (idempotent, no-op if already locked
 * this visit - see subghz_cli_soft_lock()) rather than only locking when
 * heap already happens to be tight: deterministic beats "sometimes
 * qFlipper gets kicked, sometimes it doesn't" for something this visible
 * to the user. qFlipper shows its own "Connect your Flipper" prompt for
 * the rest of this Read/Read RAW/Decode RAW visit and reconnects
 * automatically once subghz_cli_soft_unlock() runs on the way out.
 *
 * Waits briefly after locking before re-checking heap: the RAM that
 * frees isn't reclaimed synchronously with the lock call - closing the
 * CLI VCP pipe just breaks the blocking read loop the RPC session (and
 * anything running over it, including qFlipper's screen-stream) is
 * parked in; that session's own thread needs a scheduling turn to
 * actually notice, unwind, and free its buffers (confirmed via rpc_cli.c/
 * rpc_gui.c - cli_vcp_session_lock() does cascade to a real
 * rpc_system_gui_free() eventually, just not inside this same call).
 *
 * Returns true if free heap is STILL below `threshold` even after
 * disconnecting CLI/RPC and waiting - treat that as "this genuinely isn't
 * fixable by freeing CLI/RPC" (heap is tight for some other reason) and
 * fall back to SubGhzSceneLowRamWarning. Returns false once heap clears
 * the threshold - proceed normally. Callers pass SUBGHZ_LOW_RAM_FREE_
 * HEAP_READ (Read/Read RAW, subghz_scene_reader.c) or SUBGHZ_LOW_RAM_
 * FREE_HEAP (Decode RAW) - see those constants' own comments for why
 * they differ.
 */
bool subghz_low_ram_mitigate(SubGhz* subghz, size_t threshold);
