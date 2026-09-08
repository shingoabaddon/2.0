#pragma once

/* Enqueues fap_path (with args) to launch once this app exits, then tears
 * this app down cleanly - covering the screen with a blank page first so
 * the Desktop/Apps menu doesn't flash underneath during the handoff. Used
 * both for SubGHz's own menu launches (Frequency/Modulation Analyzer, the
 * Mode Picker round trip) and, from subghz_scene_protocol_load_error.c, to
 * fully close and relaunch this app itself after a protocol-load failure. */
void subghz_scene_start_launch_and_exit(SubGhz* subghz, const char* fap_path, const char* args);

enum SubmenuIndex {
    SubmenuIndexRead = 10,
    SubmenuIndexSaved = 11,
    SubmenuIndexFrequencyAnalyzer = 13,
    SubmenuIndexModulationAnalyzer = 14,
    SubmenuIndexReadRAW = 15,
    SubmenuIndexProtocolList = 17,
};
