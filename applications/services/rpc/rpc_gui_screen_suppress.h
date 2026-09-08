/**
 * @file rpc_gui_screen_suppress.h
 * @brief Lets an app suppress qFlipper's live screen-stream (the Screen
 *        Preview panel) without touching USB/CLI at all.
 *
 * qFlipper's screen preview works by sending a StartScreenStream RPC
 * request the moment its panel is open - the firmware then pushes a fresh
 * frame over that RPC session every single time the real screen redraws
 * (rpc_system_gui_screen_stream_frame_callback() in rpc_gui.c, driven by
 * gui_add_framebuffer_callback()). On a screen that redraws often (a
 * blinking RX indicator, a live RSSI graph, ...) that's a lot of RPC
 * traffic, plus a persistent worker thread + frame buffer for as long as
 * the stream is open - real RAM/CPU cost on top of just the fixed cost of
 * qFlipper being connected at all.
 *
 * This lets an app that knows it's about to be RAM-sensitive (SubGhz
 * Read, for instance) tell the RPC layer to stop pushing live frames
 * while suppressed - one static placeholder frame goes out the moment
 * suppression engages, then nothing further until it's lifted, at which
 * point the next real screen redraw resumes live streaming exactly as
 * before. The physical display is completely unaffected either way -
 * this only changes what gets pushed to a remote screen-stream viewer.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Suppress (or un-suppress) qFlipper's live screen-stream. Safe to call
 * with no RPC session connected, or none currently streaming - has no
 * effect until/unless one is. Safe to call repeatedly with the same
 * value.
 *
 * @param suppressed true to suppress (one placeholder frame, then
 *                    silence), false to resume normal live streaming
 *                    from the next real screen redraw.
 */
void rpc_gui_screen_stream_set_suppressed(bool suppressed);

/**
 * @return true if screen-stream suppression is currently requested.
 */
bool rpc_gui_screen_stream_is_suppressed(void);

/**
 * @return true if any RPC session currently has a screen-stream open
 *         (StartScreenStream was received and StopScreenStream hasn't
 *         been yet) - regardless of whether it's currently suppressed.
 *         Lets a caller notice "something just connected and asked for
 *         the screen" even while already suppressed, since suppression
 *         only stops the frame traffic - it doesn't stop a brand new
 *         client's session-level overhead (its own worker thread, frame
 *         buffer, ...) from being paid the moment it connects.
 */
bool rpc_gui_screen_stream_is_active(void);

/**
 * rpc_gui.c's own hook - call with true when a screen-stream session
 * starts (StartScreenStream) and false when it ends (StopScreenStream or
 * the RPC session itself closing while still streaming). Tracks a count,
 * not a bool, since more than one RPC session (e.g. USB + BLE) could
 * plausibly be streaming at once.
 */
void rpc_gui_screen_stream_mark_active(bool active);

/**
 * Fills `out_buffer` (must be exactly `buffer_size` bytes, matching the
 * real screen-stream frame size - see gui_get_framebuffer_size()) with
 * the suppressed-state placeholder frame (fox icon + "SubGhz & qFlipper
 * combined need too much RAM!"). Built once on first use and cached from
 * then on. Returns
 * false (buffer left untouched) if it couldn't be built - callers should
 * fall back to sending nothing rather than garbage.
 *
 * Not meant to be called directly by apps - this is rpc_gui.c's own hook
 * into the suppressed-frame content, exposed here only because building
 * it needs the same private offscreen-rendering machinery as the rest of
 * this module.
 */
bool rpc_gui_screen_stream_get_placeholder_frame(uint8_t* out_buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif
