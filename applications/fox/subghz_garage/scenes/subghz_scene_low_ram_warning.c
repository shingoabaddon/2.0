/**
 * @file subghz_scene_low_ram_warning.c
 * @brief Fallback shown from Read/Read RAW/Decode RAW when free heap is
 *        still below SUBGHZ_LOW_RAM_FREE_HEAP (subghz_i.h) even after
 *        subghz_low_ram_mitigate() already tried soft-locking CLI/RPC and
 *        waiting for it to take effect - see that function's own comment,
 *        and the various call sites in subghz_scene_reader.c/subghz_scene_
 *        decode_raw.c. Since mitigation now handles the common case (heap
 *        tight because of qFlipper's screen-stream or a live CLI/RPC
 *        session) before ever reaching here, this screen should be rare -
 *        it means heap is genuinely too low for reasons mitigation
 *        couldn't fix.
 *
 * [< Close]
 * No manual "Continue" - the screen watches free heap itself (Tick) and
 * returns to Read automatically the moment it clears SUBGHZ_LOW_RAM_
 * RECOVER_FREE_HEAP, no user action needed. This used to soft-disable
 * USB/CLI on a manual Continue press, but that killed the CLI connection
 * needed to actually diagnose what happens next - removed in favor of
 * suppressing just qFlipper's screen-stream specifically (see
 * rpc_gui_screen_suppress.h), which is the actual cost, without losing
 * CLI visibility. Close backs out to Garage's Start menu (not a full app
 * exit - Read's own low-RAM path shouldn't be more disruptive than
 * pressing Back would be).
 *
 * The very first time this screen auto-recovers with qFlipper's screen-
 * stream already confirmed closed, new CLI/RPC sessions are blocked for
 * the rest of this app run (subghz_lock_cli_sessions(), subghz.c, via
 * cli_vcp_session_lock() - see its extensive comment on subghz_i.h's
 * cli_sessions_locked_after_recovery for why this specific API and not
 * cli_vcp_disable() or raw furi_hal_usb) - a belt-and-suspenders latch,
 * not a substitute for the heap-check fixes elsewhere: it exists because
 * qFlipper reconnecting mid-Read can still race a heap check by a few
 * instructions (its screen-stream session setup runs on its own RPC
 * thread), so closing the door after the first recovery is more reliable
 * than trying to win every individual race.
 */

#include "../subghz_i.h"
#include "../helpers/subghz_custom_event.h"
#include "../helpers/subghz_debug_log.h"
#include <furi.h>
#include <furi/core/memmgr.h>

/* Recovery debounce - require heap to sit at/above RECOVER for half a
 * second (5 ticks at this app's 100ms tick period) before actually
 * recovering, mirroring the trigger-side debounce in subghz_scene_
 * reader.c (SUBGHZ_LOW_RAM_TRIGGER_TICKS). Without this, a screen that
 * only ever appeared because of a momentary spike could clear on the very
 * next tick - fine on its own, but the CLI-session-lock step right below
 * fires unconditionally, and dismissing this fast made it look (from the
 * user's side, e.g. on lab.flipper.net) like CLI had frozen rather than
 * cleanly disconnected. Reset on every fresh entry to this screen. */
#define SUBGHZ_LOW_RAM_RECOVER_TICKS 5
static uint32_t s_low_ram_warning_recover_tick_count = 0;

/* Minimum time this screen stays up before it's allowed to auto-recover,
 * regardless of how quickly heap actually clears - the whole point of
 * showing "Uh Oh!" is for the user to read it; a heap that recovers
 * within a couple hundred ms of arriving here (a real, observed case)
 * would otherwise make it flash past almost unreadably. Tracked
 * separately from SUBGHZ_LOW_RAM_RECOVER_TICKS's own sustained-heap
 * debounce - once heap has been fine long enough, this is the second,
 * independent gate before actually leaving. */
#define SUBGHZ_LOW_RAM_WARNING_MIN_DWELL_MS 1500
static uint32_t s_low_ram_warning_shown_at_ms = 0;

static void subghz_scene_low_ram_warning_widget_cb(
    GuiButtonType result,
    InputType type,
    void* context) {
    SubGhz* subghz = context;
    if(type != InputTypeShort) return;

    if(result == GuiButtonTypeLeft) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventLowRamWarningExit);
    }
}

static void subghz_scene_low_ram_warning_exit_to_start(SubGhz* subghz) {
    /* Reset to SubGhzReceiverAutoStateStart (0) - without this, a stale
     * Listening state made the next Read press re-enter start_listening()
     * immediately instead of showing the idle Start screen. */
    scene_manager_set_scene_state(subghz->scene_manager, SubGhzSceneReceiver, 0);
    if(!scene_manager_search_and_switch_to_previous_scene(
           subghz->scene_manager, SubGhzSceneStart)) {
        scene_manager_stop(subghz->scene_manager);
        view_dispatcher_stop(subghz->view_dispatcher);
    }
}

void subghz_scene_low_ram_warning_on_enter(void* context) {
    SubGhz* subghz = context;

    subghz_debug_log_write(
        "low_ram_warning: on_enter, free heap %zu, qFlipper screen-stream active=%d",
        memmgr_get_free_heap(),
        (int)rpc_gui_screen_stream_is_active());

    s_low_ram_warning_recover_tick_count = 0;
    s_low_ram_warning_shown_at_ms = furi_get_tick();

    /* Stay suppressed while this screen is up regardless of how we got
     * here - Receiver's/ReadRAW's own on_exit already un-suppresses on the
     * way here, so this re-establishes it. The whole point of this screen
     * is "heap is critically low"; letting qFlipper resume live frames
     * right now would work against the fix it exists to apply. */
    rpc_gui_screen_stream_set_suppressed(true);

    subghz_ensure_widget(subghz);
    Widget* widget = subghz->widget;

    widget_add_string_multiline_element(
        widget, 64, 8, AlignCenter, AlignTop, FontPrimary, "Uh Oh!");
    /* FontSecondary is ~8px/line. 3 lines = 24px, starting at y=20 ends at
     * y=44 - buttons sit at y≈51, so this clears them with room to spare -
     * see subghz_scene_decode_raw_failed.c's comment for the same math.
     *
     * qFlipper's screen-stream is specifically detectable (see rpc_gui_
     * screen_suppress.h's is_active()) - a plain CLI/serial session with
     * no screen-mirroring open doesn't trigger it. Name qFlipper
     * specifically when it's the actual culprit; fall back to the generic
     * USB/CLI wording otherwise, since a bare serial connection alone can
     * also be enough to keep heap pinned this low. Suppression only stops
     * qFlipper's live frames, not its one-time session cost - fully
     * closing it (not just leaving it idle on a non-streaming tab) is
     * still what actually frees that RAM back up. */
    widget_add_string_multiline_element(
        widget,
        64,
        20,
        AlignCenter,
        AlignTop,
        FontSecondary,
        rpc_gui_screen_stream_is_active() ? "SubGhz READ needs more\nRAM - close qFlipper to\ncontinue."
                                           : "SubGhz READ needs more\nRAM - close USB/CLI to\ncontinue.");
    widget_add_button_element(
        widget, GuiButtonTypeLeft, "Close", subghz_scene_low_ram_warning_widget_cb, subghz);

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdWidget);
}

bool subghz_scene_low_ram_warning_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;

    if(event.type == SceneManagerEventTypeBack) {
        /* Same as Close now - this used to go straight to Desktop, but a
         * low-RAM condition on Read isn't a reason to be more disruptive
         * than backing out of Read normally would be. */
        subghz_scene_low_ram_warning_exit_to_start(subghz);
        return true;
    }

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubGhzCustomEventLowRamWarningExit) {
            subghz_scene_low_ram_warning_exit_to_start(subghz);
            return true;
        }
    }

    if(event.type == SceneManagerEventTypeTick) {
        size_t tick_free_heap = memmgr_get_free_heap();
        if(tick_free_heap >= SUBGHZ_LOW_RAM_RECOVER_FREE_HEAP) {
            s_low_ram_warning_recover_tick_count++;
        } else {
            s_low_ram_warning_recover_tick_count = 0;
        }

        if(s_low_ram_warning_recover_tick_count >= SUBGHZ_LOW_RAM_RECOVER_TICKS) {
            uint32_t shown_for_ms = furi_get_tick() - s_low_ram_warning_shown_at_ms;
            if(shown_for_ms < SUBGHZ_LOW_RAM_WARNING_MIN_DWELL_MS) {
                /* Heap's been fine long enough already - just not the
                 * screen's own minimum-dwell time yet. Hold here and
                 * re-check next tick without resetting the sustained-
                 * recovery count above; heap readiness isn't what we're
                 * still waiting on. */
                return true;
            }
            subghz_debug_log_write(
                "low_ram_warning: tick, free heap %zu sustained >= recover threshold for %lu ticks, recovering",
                tick_free_heap,
                (unsigned long)s_low_ram_warning_recover_tick_count);
            s_low_ram_warning_recover_tick_count = 0;
            /* Belt-and-suspenders fix for a race the RESTORE-threshold
             * checks alone can't fully close: qFlipper's screen-stream
             * session cost is paid on its own RPC thread, independent of
             * this app's Tick cadence, so it can still land in a gap
             * between one of Read's own heap checks and the allocation
             * right after it - user confirmed this could still surface as
             * "Function requires an SD card." if qFlipper reconnected
             * mid-Read. Once we've recovered AND qFlipper's screen-stream
             * is confirmed already closed (never forcibly disconnect an
             * active one - that's what broke reconnection before), lock
             * out new CLI/RPC sessions for the rest of this app run so a
             * second reconnect can never retrigger the same cycle. If
             * qFlipper somehow still shows active right at the recovery
             * instant, skip the lock this time and just let normal
             * recovery proceed - it'll be reconsidered next time this
             * screen is reached. */
            if(!rpc_gui_screen_stream_is_active()) {
                subghz_debug_log_write("low_ram_warning: locking CLI sessions before recovery");
                subghz_lock_cli_sessions(subghz);
                subghz_debug_log_write("low_ram_warning: CLI sessions locked");
            } else {
                subghz_debug_log_write(
                    "low_ram_warning: qFlipper screen-stream still active, skipping CLI lock this time");
            }

            /* Arm the grace window BEFORE resuming Listening - see subghz_
             * i.h's low_ram_grace_until_ms comment. Read's own low-RAM
             * checks (subghz_scene_reader.c) fall back to the idle Start
             * screen instead of re-showing this screen if they'd otherwise
             * trip again before this deadline, so a shortage that wasn't
             * actually fixed shows up as one clean bounce to Start - not a
             * repeat "Uh Oh!" - and a real recovery gets to hold instead of
             * being immediately re-tested by the heavier chain it's about
             * to run. */
            subghz->low_ram_grace_until_ms = furi_get_tick() + SUBGHZ_LOW_RAM_GRACE_MS;
            subghz_debug_log_write("low_ram_warning: switching to previous scene");
            scene_manager_previous_scene(subghz->scene_manager);
            subghz_debug_log_write("low_ram_warning: previous scene switch returned");
        }
        return true;
    }

    return false;
}

void subghz_scene_low_ram_warning_on_exit(void* context) {
    SubGhz* subghz = context;

    /* Un-suppress unconditionally - the auto-recovery path (Tick above)
     * lands back on Receiver/ReadRAW's on_enter, which re-suppresses
     * immediately; the Close/Back path genuinely leaves Read, where this
     * is exactly the state we want. */
    rpc_gui_screen_stream_set_suppressed(false);

    if(subghz->widget) {
        widget_reset(subghz->widget);
    }
}
