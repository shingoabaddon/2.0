# ProtoPirate-base Garage Rewrite — Migration Plan (tasks #83-91)

## 1. What already exists

`applications/fox/ProtoPirate_FoxEdition/` is not a stub — it's a complete,
previously-compiling external app (build artifacts for all its RX/TX/tool
plugins already exist under `build/f7-firmware-C/.extapps/protopirate_*`,
and the same tree is mirrored byte-for-byte in `FOX_APPS/applications_user/`
as the prebuilt-.fap source of truth). It already has:

- FoxFW's full car-protocol set ported into `protocols/` (chrysler, fiat
  v0-v2, ford v0-v3, honda static/v1/v2, kia v0-v7, mazda, mitsubishi,
  porsche_touareg, psa, renault, scher_khan, star_line, subaru, vag, aut64) —
  the same protocols FoxFW's own `subghz_garage`/core `subghz` trees carry.
- A `ProtoPirateTxProtocolPlugin()` helper in `application.fam` that
  generates one TX-only `.fal` plugin per protocol, plus grouped RX-only
  `.fal` plugins (`am`, `am_vag`, `fm`, `fm_f4`, `fm_honda1`) — this is the
  RX_ONLY/TX_ONLY split pattern FoxFW's own Garage app's plugin
  architecture was built by studying.
- PSA Brute Force, Emulate, Timing Tuner, and Sub-file-Decode as separate
  optional plugins gated by `defines.h` (`ENABLE_EMULATE_FEATURE`, etc.).

## 2. What's still stock upstream ProtoPirate

`scenes/protopirate_scene_config.h` lists 10 scenes: Start, SubDecode,
About, Receiver, ReceiverConfig, ReceiverInfo, NeedSaving, Saved,
SavedInfo, Emulate, TimingTuner. `protopirate_app_i.h`'s `ProtoPirateApp`
struct is the plain upstream shape (submenu-driven UI, a single
`protocol_registry_route`, a generic `ProtoPirateLock`) — none of it has
been touched to bring in FoxFW's own layer.

By contrast, `applications/fox/subghz_garage/scenes/` has 33 scene files.
The 23 with no ProtoPirate counterpart are all FoxFW-specific and each
exists because of a real bug or feature fixed earlier in this project:

- **RAM safety** — `low_ram_warning`, `shared_ram_warning`,
  `protocol_load_error` (+ the 2s-delay/retry backoff logic behind it),
  the low-RAM watchdog-bypass fix. None of this exists in PP.
- **Lazy loading** — PP allocates its radio/environment/receiver/history
  and all its views eagerly at boot, the way FoxFW's Garage app used to
  before it was reworked to lazy-init (PP-style, ironically) and to
  lazy-load protocol groups/presets/frequencies. PP's own current RAM
  footprint here hasn't been re-measured against Garage's tuned one.
- **Merged Read flow** — `reader.c` (Read + Read RAW unified,
  silence-triggered auto-capture, RSSI-gated activity detection, qFlipper
  screen-stream suppression, CLI soft-lock) replaces PP's plain
  `receiver.c`/`receiver_config.c` scenes entirely.
- **Decode RAW** — `decode_raw.c`/`decode_raw_failed.c` (no PP equivalent;
  PP's `sub_decode` plugin is a different, narrower feature).
- **Protocol groups** — `protocol_list.c` (double-row group buttons,
  OK-to-confirm switching, 11 user-defined groups) replaces PP's flat
  single `protocol_registry_route`.
- **Saved-file management** — `saved_menu.c`, `delete.c`, `delete_raw.c`,
  `delete_success.c`, `save_name.c`, `save_success.c`, `file_prefix.c`,
  `details.c` are all more built-out than PP's plain `saved.c`/
  `saved_info.c`.
- **Everything else** — `custom_freq`, `signal_settings`,
  `signal_visualizer`, `set_button`/`set_type`/`set_key`/`set_seed`/
  `set_serial`/`set_counter` (custom-button-remap system), `counter_bf`,
  `transmitter`, `rpc.c` (RPC screen-stream suppression), `show_error`/
  `show_error_sub`. Fox-theme menu styling touches all of them.
- CC1101 external-probe integration and the Mode Picker entry flow live
  outside the scenes folder (in the app's boot path / the shared Sub-Ghz
  Mode picker) and have no PP equivalent either.

## 3. Bottom line

The "rewrite" isn't building a skeleton from scratch — that skeleton
(app.c/app_i.h/scene manager, protocol tree, RX/TX plugin split) already
exists and last compiled successfully. What's actually left is porting
roughly 20+ scenes' worth of FoxFW-specific safety and UX work — most of
it written to fix specific crashes/OOMs discovered by on-device testing —
onto that base. That's a large, structurally invasive change with no way
to compile-check it in this environment, on top of an already-unverified
backlog (nothing's been compiled since your last long message). Attempting
all of #85-90 blind in one sitting risks shipping something that doesn't
boot, with no way for me to catch that before you flash it.

## 4. Recommended order for #84-91

Rather than one big splice, port FoxFW's layer onto ProtoPirate_FoxEdition
in independently compile-testable stages, safety-critical pieces first:

1. **#84 (skeleton)** — mostly done already; just needs `application.fam`'s
   `appid`/menu placement decided (replace `subghz_garage` outright, or
   ship alongside it until parity is proven).
2. **#87 RAM safety first** — port `low_ram_warning`, `shared_ram_warning`,
   `protocol_load_error` + lazy-init before anything else touches the app,
   since every later stage depends on not OOMing while you build it out.
3. **#85 Protocol groups** — replace `protocol_registry_route` with
   Garage's group system; this is the piece most other scenes reference.
4. **#86 Fox-theme menu** — reskin Start/Receiver/Saved once the
   underlying data flow is stable, so it's a UI-only pass.
5. **#87/#88 Read/Read RAW/Decode RAW** — port `reader.c` and
   `decode_raw.c` onto PP's `receiver.c`/`sub_decode` plugin.
6. **#89 Saved-file management** — port delete/rename/prefix/details.
7. **#90 Remaining scenes/fixes** — custom-button-remap, CC1101 probe,
   RPC suppression, everything else in section 2.
8. **#91 Cutover** — swap `application.fam` registration, remove/retire
   `subghz_garage`, verify structural completeness.

Each stage should get its own compile-and-device-test pass before the
next starts, rather than reviewing 91 tasks' worth of changes at once.

## 5. Revision — 2026-09-03

Reordered and corrected after actually starting the staged work.

**Priority reorder, RAM safety pushed back.** Reading `subghz_i.h` in
full before porting anything showed the RAM-safety subsystem is not a
portable module - it's the single most hardware-tuned part of Garage,
built from two documented failed on-device attempts (raw `furi_hal_usb`
teardown and `cli_vcp_disable()` both corrupted persistent CLI VCP state
badly enough to need a full device reboot) before the current
`cli_vcp_session_lock()`-based sequencing was found to work. Porting that
blind, with no compile verification and no way to catch a bad sequencing
change before it ships, risks reintroducing exactly that class of bug -
for a problem PP may not even have to the same degree (task #80 already
found PP's stock boot-RAM footprint comparable to or better than
Garage's). Moved it to the back of the queue; do it only once on-device
testing on PP's own base shows it's actually needed, and only with real
device-log iteration behind it, the same way it took to get right the
first time.

**#85 "protocol groups" was the wrong framing - PP already solves this,
differently.** Garage's Protocol Group picker exists because Garage lets
the user manually choose which of several RX-protocol buckets to load, to
fit a huge protocol set into limited RAM. Reading
`protopirate_protocol_plugin_host.c` shows PP already has its own answer
to the identical RAM problem: `protopirate_get_protocol_registry_route()`
derives which RX plugin bucket to load (`am_default`/`am_vag`/
`fm_default`/`fm_f4`/`fm_honda1`) automatically from the currently-
selected modulation preset - no user picker needed at all, and it already
correctly buckets every protocol FoxFW added (confirmed against
`application.fam`'s plugin source lists). There's nothing to splice here;
#85 is effectively already done by PP's own architecture. What's
genuinely still open is a follow-up verification pass (not started this
session) confirming every FoxFW-added protocol decodes correctly through
its assigned bucket on real hardware - a testing task, not a porting one.

**Stage 1 shipped and promoted.** Forked `subghz_garage`'s
`views/subghz_view_start_grid.c/.h` (double-row Fox-theme button grid -
fully generic aside from its button-table) into
`views/protopirate_start_grid.c/.h` with PP's own 6 Start-menu items,
text-only (no icon assets exist yet for PP's menu labels), wired into
`protopirate_scene_start.c` with the same `fox_theme_is_active()`
dispatch Garage's own Start scene uses. Staged in `D:\TEMP\pp_staging\`,
verified (brace/paren balance across every touched file), then copied
into the real `applications/fox/ProtoPirate_FoxEdition/` tree - ready to
compile tonight. `subghz_garage` itself was not touched by any of this
and remains fully intact/separately registered as a fallback, per your
explicit ask to keep it available in case the PP replacement ends up
bigger than Garage on RAM or flash.

**Stage 1 confirmed compiling and linking clean** on the 2026-09-03
14:31 build (`proto_pirate_d.elf` linked, all RX/TX/tool `.fal` plugins
installed, `.free_flash` 199.30K, no new warnings). Promoted for real.

**#86 turns out to already be fully done by Stage 1 - no separate reskin
needed.** Grepped every scene in `subghz_garage/scenes/` for
`fox_theme_is_active()`: it appears exactly once, in `subghz_scene_start.c`
only. Fox-theme customization in this codebase has never touched Receiver,
Saved, or any other scene - it's scoped entirely to the Start menu. Stage 1
*is* task #86 in full, not a first step toward it.

**#87/#88 (Read/Read RAW/Decode RAW) is a much bigger port than framed
above - not the natural next stage.** Read `subghz_scene_reader.c` in full
(1795 lines) to scope it before writing anything. It isn't a UI port onto
PP's `receiver.c` - the silence-detection tick loop, decode-sample
batching, and every `subghz_low_ram_mitigate()` call site are the RAM-
safety subsystem *in its live form*, not a separable concern from it. This
is the same subsystem section 5 above already deferred pending on-device
proof it's needed on PP's base - it can't be ported halfway. Attempting
this blind, in one sitting, with no compile-test loop, risks recreating
exactly the class of bug documented in section 5 (the two failed
CLI-lock attempts) with no way to catch it before the user flashes it.
Deferred until a session with real device-log iteration behind it, same as
the original.

**#89 (Saved-file management) shipped - turned out to be a small, safe
add, not a 7-scene port.** Reading PP's own `saved_info.c` first showed it
already combines Details (inline scroll widget) + Delete (inline confirm
dialog) + Emulate into one compact scene - Garage needs separate
`saved_menu.c`/`details.c`/`delete.c` scenes for the same coverage only
because its older design routes through a full submenu first. The only
real gap was Rename. Added it by reusing PP's own already-established
text_input pattern (the exact same flow `receiver_info.c`'s Save button
already uses - header text, `save_filename[64]` buffer, result-callback-
fires-custom-event) rather than porting Garage's separate `save_name.c`
scene, plus one new `protopirate_storage_rename_file()` helper
(`storage_common_rename` wrapper, same one-liner Garage's own
`subghz_rename_file()` uses). SavedInfo now has Left/Center/Right =
Emulate/Rename/Delete, matching the L/C/R widget-button convention this
app already uses on ReceiverInfo - not a new UI pattern.

Explicitly not ported, and not considered gaps: Signal Settings (Garage
only shows it with `FuriHalRtcFlagDebug` set - a debug build flag, not a
real user feature), Counter BruteForce (protocol-specific advanced action,
not core file management), File Prefix (a separate global auto-naming
setting, orthogonal to managing files that already exist).

**#90 scoped in full - most of it is dead weight, not a porting backlog.**
Read every remaining Garage scene against PP before writing anything:
`signal_visualizer.c` calls `subghz_txrx_rx_start()` internally - it's part
of the deferred RX/RAM cluster, not standalone. `counter_bf.c` is TX-only
but Garage's `subghz_tx_start()` routes through `subghz_txrx_tx_start()`'s
TX-protocol-plugin loading (a Garage-specific architecture) - PP's
equivalent capability lives inside its Emulate plugin
(`protopirate_emulate_plugin.c`, 1000+ lines, its own context/state
machine), not a reusable function - a real port means extending that
plugin's boundary, not writing a new scene. `transmitter.c` is superseded
by PP's own `emulate.c`. `show_error.c`/`show_error_sub.c` are only ever
reached from the deferred cluster and the already-superseded
`save_name.c`. `signal_settings.c` is gated behind `FuriHalRtcFlagDebug` -
a debug build flag, not a real user feature. `protocol_list.c` was already
confirmed moot (see #85 above). `set_button.c`/`set_type.c`/`set_key.c`/
`set_seed.c`/`set_serial.c`/`set_counter.c`/`custom_freq.c` are 0-byte
empty files, not even registered in `scene_config.h` - dead, nothing to
port. `file_prefix.c` is small and safe, but grepping the whole app for
`scene_manager_next_scene(..., SubGhzSceneFilePrefix)` turned up nothing -
Garage itself never wired a menu entry to it. It's registered in
`scene_config.h` and compiles, but no button anywhere reaches it - a
half-finished feature in the source app too.

**File Prefix shipped anyway - and PP's version is more complete than
Garage's own.** Since it needed no RX/TX/plugin work, just settings
storage + a menu entry, built it properly: added `file_prefix[12]` to
`ProtoPirateSettings` (PP already has a real settings file mechanism -
`protopirate_settings.c`, FlipperFormat-based, versioned, atomic
temp-file-then-rename commit - reused as-is) and to `ProtoPirateApp`,
bridged at boot/exit in `protopirate_app.c` (matching how every other
setting - auto_save, tx_power, etc. - already round-trips there), and
added a "File Prefix:" entry to `protopirate_scene_receiver_config.c`
that opens PP's own text_input pattern (the same one Rename uses, see
#89 above) rather than any Garage code. Applied in
`protopirate_scene_receiver_info.c`'s auto-generated Save filename by
prepending the prefix before the protocol-based name. Known minor gap:
the settings-screen value text for File Prefix doesn't live-refresh after
an edit within the same visit to Receiver Config (shows the value from
when the scene was entered) - cosmetic only, the actual setting is fully
functional and persisted; would need per-item state tracking to fix,
judged not worth the complexity for a cosmetic refresh.

**Reversed 2026-09-03: Saved-file management is intentionally lean, not
feature-complete.** User's direction after seeing #89: the device's own
core subghz app already has full file management (rename/delete/details) -
PP duplicating that costs RAM/flash for zero real benefit, since anyone
who wants to rename or delete a capture already has a place to do it.
Removed Rename (built earlier this session) and the pre-existing Delete
button from `protopirate_scene_saved_info.c`, along with the now-dead
`protopirate_storage_rename_file()` and the `SavedInfoDelete`/
`SavedInfoRename`/`SavedInfoRenameConfirm` custom events. Kept Details
(the inline scroll widget - effectively free, built from data already
read off the file) and Emulate (not "file management" - it's PP's core
operational purpose, using a saved capture, not managing the file it
lives in). Final shape: Saved = file browser to pick a `.psf`, SavedInfo =
preview details + optionally Emulate. This is now PP's real Saved-file
scope going forward, not a stepping stone to something bigger.

## 6. Migration abandoned, PP reverted to stock — 2026-09-03

Device-log evidence killed the premise. A user device test of PP's
Receiver hit reproducible OOM crashes; static review found no leak, but
`free`/heap-delta logging showed entering Receive drops free heap from
~123,600 bytes to ~13,000 (min 12,600) out of a 188,040-byte total heap -
before qFlipper is even involved. Garage's own low-RAM guard trips at
18,000 bytes free specifically because that's comfortably above Garage's
tuned steady state; PP's normal Receive operating point already sits
*below* that number. qFlipper's screen-stream cost (4-8KB) was just the
final straw on an already near-exhausted stack, not the root cause.

This closed off the benefit that would have justified finishing the
migration: meaningfully more free RAM, ideally enough to run qFlipper
while receiving. That was never realistic even before this test - Garage's
actual fix for qFlipper isn't RAM headroom, it's active suppression of
qFlipper's screen-stream while receiving (`rpc_gui_screen_suppress`,
built and tuned specifically for this); PP has no equivalent, and would
need that exact mechanism ported regardless of how much its own footprint
shrank. Reaching real parity with Garage's current Receive reliability
would also mean redoing the RAM-safety subsystem this plan's own section 5
already flagged as "the single most hardware-tuned part of Garage" -
built from two separate failed on-device attempts before the current
approach worked, not a portable module - on a codebase that, per today's
numbers, needs it *more* than Garage does, not less.

Separately, PP's automatic modulation-based protocol routing
(`protopirate_get_protocol_registry_route()`, section 5 above) turns out
not to generalize to Garage at all: it works for PP only because PP's own
protocol list is small enough that modulation happens to double as a
reasonable bucket boundary. Garage's much larger overall protocol set is
~90% AM650, so routing by modulation would collapse nearly all of Garage's
11 hand-tuned RAM-budget groups back into one mega-bucket - exactly the
problem the group system exists to prevent.

**Decision:** stay on Garage as the shipping app. PP's
`ProtoPirate_FoxEdition/` has been reverted to stock upstream (`git
checkout` against HEAD, which had never carried FoxFW-specific PP changes
beyond one incidental 1-line build fix in `kia_v1.c` - Stage 1's Fox-theme
Start-grid, File Prefix, the SavedInfo Rename/Delete work, Counter
BruteForce, and today's heap-delta logging were all still-uncommitted
working-tree changes, so the revert was clean). This plan is closed.

What's still worth carrying forward, now targeting Garage/the main
Automotive subghz app instead of PP: PP's per-protocol button remapping,
rolling-counter tracking, and Counter BruteForce (adapted to Garage's own
`subghz_tx_start()`/TX-protocol-plugin architecture) where Garage's actual
garage/gate protocols have multi-action or rolling-counter signals; the
Fiat V1 HITAG2 key-entry flow specifically belongs on `applications/main/
subghz` (already Automotive-only per tasks #26/#27), not Garage, since
Garage carries no automotive protocols. Tracked as new tasks rather than
in this file, since this plan is PP-specific and PP is no longer the
target.

**Counter BruteForce shipped 2026-09-03, as a mode inside the existing
Emulate plugin rather than a new scene.** `protopirate_emulate_plugin.c`
already had everything a brute-force needs: per-protocol TX routing through
PP's protocol registry, preset resolution/tx-power patching, and - inside
`emulate_input_callback`'s manual press handler - the exact
increment-counter-then-retransmit cycle a brute-force automates, just
gated on a button press instead of a timer. Garage's `counter_bf.c` used a
different mechanism entirely (`furi_hal_subghz_set_rolling_counter_mult()` +
a global counter override through `subghz_tx_start()`), which only works
because Garage's TX goes through `subghz_txrx_tx_start()`'s TX-protocol-
plugin loading - not a fit for PP's architecture, confirming the #90 note
above that a real port here meant extending the plugin's boundary, not
writing a parallel scene.

Extracted the manual-transmit case body into `emulate_perform_transmit()`
and the counter-increment logic into `emulate_advance_counter()` so both
the existing manual (button-press) path and the new auto path call the
same code. Added `bruteforce_mode`/`bruteforce_running`/`bruteforce_sent`/
`bruteforce_next_ready_time` to `EmulateContext`, a second draw callback
(`emulate_bruteforce_draw_callback`, showing current/start counter and
packets-sent), and BF-specific input handling (OK toggles Start/Stop, Back
exits - the normal per-protocol button-to-action mapping and hold-to-
transmit logic are bypassed entirely in this mode, since brute-force always
resends the original captured button with only the counter changing). The
Tick handler now drives repeated sends on a timer (a fixed 300ms gap
between transmissions, reusing the existing per-protocol minimum-TX-time
gate for how long each individual send lasts) instead of relying on
button-release. New: counter write-back to the actual saved file's `Cnt`
field on Stop and on exit (`emulate_bruteforce_save_counter()`, mirroring
Garage's `counter_bf_save()`) - Emulate itself never persisted `Cnt` back
to disk before, since manual use didn't need to.

Entry point is a second SavedInfo button, "Cnt BF" (`GuiButtonTypeRight`,
alongside the existing "Emulate" on `GuiButtonTypeLeft`), shown only when
the loaded file both supports TX and actually has a `Cnt` field (fixed-code
protocols never show it). Wired through a new `app->emulate_start_bruteforce`
flag set before entering `ProtoPirateSceneEmulate` - the plugin's
`plugin_on_enter` reads and clears it to decide which draw/behavior mode to
start in, since the host scene (`protopirate_scene_emulate.c`) is a pure
pass-through and `ProtoPirateApp` is the only channel into the plugin.

With this landed, #90's real backlog is empty. #87/#88 stay deferred
pending a device-log session; #91 stays blocked on a RAM/flash comparison
once those land. Not yet compile-checked on-device - next step once the
user's current device-test pass wraps up.
