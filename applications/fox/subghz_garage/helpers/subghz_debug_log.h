#pragma once

/**
 * Temporary diagnostic aid for the repeated-app-launch NULL deref
 * investigation - appends one line per call to a plain text file on the SD
 * card (/ext/apps_data/subghz_garage/ram_debug.log), so the same trail the
 * matching FURI_LOG_I calls already produce survives a crash/reboot and can
 * be read back afterward (via qFlipper's file browser or a card reader)
 * without needing a live CLI session during the actual repro - the low-RAM
 * watchdog itself forces CLI closed once heap gets tight, which is exactly
 * the situation this exists to work around.
 *
 * Opens, writes, and closes the file on every call rather than holding one
 * handle open for the app's lifetime, so a line already written survives
 * even if the very next thing that happens is the crash itself.
 *
 * Delete this file and its call sites once the leak is found and fixed -
 * not meant to be permanent.
 */
void subghz_debug_log_write(const char* format, ...);
