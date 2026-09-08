# SD-Card Crash Logger — Design Doc (not yet implemented)

Status: design only, pending review. No firmware code has been written for
this yet. This doc exists because the original idea named `RTC_NOINIT_ATTR`
(an ESP-IDF/ESP32 attribute) as the retained-RAM mechanism, but this repo's
actual hardware is STM32WB55 — the two platforms need genuinely different
mechanisms, and the STM32 side already has a relevant piece of
infrastructure that should be extended rather than duplicated. This doc
resolves that ambiguity and proposes a concrete design for both repos
before any code gets written.

## The problem

FoxFW2.0 can't show symbolized crash backtraces (file/line/resolved
symbols) — that was tried before and pushed firmware size over the flash
budget, so it's off the table (see `CLAUDE.md`). The result: a
`furi_check()`/`furi_assert()`/`furi_crash()` failure today shows at most an
optional message string on the next boot, no file/line, no task name, no
memory watermarks, nothing that survives past that one popup screen.

The idea: capture a compact crash record at the moment of the crash, write
it to a persistent SD-card log once it's safe to do file I/O (next boot,
after storage is mounted), so real diagnostic history accumulates instead
of a single popup that's gone the moment you press through it.

The crash handler runs with interrupts disabled, a few instructions before
the actual reset — writing to SD card directly from there is unsafe
(storage driver isn't guaranteed reentrant/available in that context, and
a wedged crash handler that itself hangs trying to do I/O is worse than the
original crash). So the record has to land somewhere that (a) a few
instructions can write to safely in that exact context, and (b) survives
the reset that follows.

## Target resolution

This repo (FoxFW2.0) and Fox_ESP32_FW are different hardware with
different retained-memory mechanisms. Both need this feature, and both are
in scope, but as two separate implementations sharing only the log-file
*format* (see below), not shared code.

### FoxFW2.0 / STM32WB55

**`RTC_NOINIT_ATTR` doesn't exist on this platform.** STM32 has no
equivalent linker attribute in this firmware today, and a plain SRAM
`.noinit` region isn't safe to add: `Reset_Handler`
(`targets/f7/src/stm32wb55_startup.c`) and `furi_hal_memory_init()`
(`targets/f7/furi_hal/furi_hal_memory.c`) both memset every SRAM region
this firmware currently knows about on *every* reset, warm or cold — a new
`.noinit` section would need its own linker region carved out and
permanent discipline that nothing else ever claims that memory. Fragile,
and it's solving a problem this firmware already has a real answer to.

**Use RTC backup registers instead — already proven for exactly this.**
`furi_hal_rtc_set_fault_data()`/`get_fault_data()`
(`targets/f7/furi_hal/furi_hal_rtc.c`) already write/read one 32-bit value
through `RTC->BKPxR` (STM32WB55 has 20 of these, `RTC_BKP_NUMBER` in
`stm32wb55xx.h`), and `furi/core/check.c`'s `__furi_crash_implementation()`
already calls it — with interrupts already disabled — right before
`furi_hal_power_reset()` (a plain `NVIC_SystemReset()`, confirmed, no
watchdog involved). On the next boot, `desktop.c` checks
`furi_hal_rtc_get_fault_data()` and shows `DesktopSceneFault` if it's
non-zero, then clears it. This is the *entire* crash → reboot → next-boot
→ display → clear pipeline the SD logger needs, already built and battle
tested — it currently just displays a popup and stores one flash pointer
instead of writing a file and storing a real record.

`FuriHalRtcRegister` currently uses 8 of the 20 backup registers
(`furi_hal_rtc.h`), leaving 12 free (48 bytes) — enough for a compact
packed record (see Record Format below). These registers live in the RTC
backup domain, powered by `VBAT` (tied to the main battery on this
hardware, no separate coin cell) — they survive warm software resets and
ordinary power cycles, and are only lost on a full battery disconnect.
That's an acceptable/expected limitation, not a bug: a battery pull is a
different failure mode than a `furi_check()` crash, and losing an
in-flight crash record to it is fine.

**Known gap, not solved by this design**: `furi_check()`/`furi_assert()`
don't auto-capture `__FILE__`/`__LINE__` today — only whatever message
string a call site optionally passes (`furi/core/check.h`). This logger
makes better use of a crash record than exists today, but it doesn't
retroactively add file/line capture to every check site — that's a
separate, larger change (would need every call site updated or the
macros redefined to stringify `__FILE__`/`__LINE__` automatically, which
has its own flash-size cost from all the embedded path strings) and is out
of scope here unless requested separately.

### Fox_ESP32_FW / ESP32

**`RTC_NOINIT_ATTR` is correct for this repo** — green-field, no
conflicting existing code. Confirmed: no crash/panic-handling code exists
in this project at all today (grepped the whole source tree), and the
build's inherited `sdkconfig` already has
`CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT=y` with a 0-second reboot delay —
panics already go through a software reset (`esp_restart()`), which is
exactly the reset class `RTC_NOINIT_ATTR` is designed to survive.

Caveat worth designing around: `RTC_NOINIT_ATTR` survives software/
watchdog resets and deep sleep, but is cleared on power-on reset and, on
some chip revisions, brownout reset — so (same as the STM32 side) a record
can silently not be there even though a crash happened, if the crash
coincided with a brown-out. The design must treat "no valid record found
at boot" as the normal/expected case, never an error.

This project currently targets six ESP32 variants (`esp32`, `esp32c3`,
`esp32c5`, `esp32c6`, `esp32s2`, `esp32s3`, per the `build/` directory) —
`RTC_NOINIT_ATTR` is available on all of them via arduino-esp32/ESP-IDF,
no per-variant branching needed for the attribute itself.

## Record format (shared field list, not shared code)

Both platforms should log the same *fields*, so the two firmwares' log
files read the same way even though the capture mechanism differs:

- Magic/version byte(s) — so a stale or corrupt record from a firmware
  update (struct layout changed) is detected and discarded instead of
  misread.
- Timestamp — actual wall-clock isn't available at crash time on either
  platform typically (RTC calendar may not be set / no network time on the
  ESP32 side without WiFi); log the crash-side uptime tick count as
  captured, and stamp the *file* with wall-clock time at flush-to-SD time
  (next boot) instead of trying to get real time into the tiny retained
  record.
- Message — whatever string the crash site provided (FoxFW2.0: already a
  flash pointer today, validate range same as now; ESP32: a short fixed
  buffer, no flash-pointer trick needed/available).
- Task/thread name — FreeRTOS on both platforms exposes the current task
  name; capture it at crash time (`pcTaskGetName(NULL)` equivalent).
- Stack/heap watermarks — same idea, different APIs
  (`uxTaskGetStackHighWaterMark` on both is realistic since both are
  FreeRTOS; heap free bytes via each platform's own heap API).
- Reset reason, if cheaply available at the point of capture (STM32: not
  distinguishing much beyond "we crashed"; ESP32: `esp_reset_reason()` if
  callable safely in a panic context — needs checking, not assumed here).

FoxFW2.0's 12 free backup registers (48 bytes) are the hard ceiling on the
STM32 side — the record needs to fit that, packed (bitfields / fixed-width
truncated fields), not a free-form struct. The ESP32 side has much more
generous RTC memory available, so its struct can be roomier; keeping the
STM32 struct as the "lowest common denominator" field list (same field
*names and meaning*, ESP32 can carry a slightly longer message string,
say) keeps the two log files readable the same way without forcing the
richer platform down to the constrained one's byte budget.

## Where the file goes

Reuse this project's existing on-disk conventions rather than inventing a
new one:

- FoxFW2.0: the user's own spec named `/System/Logs/`, one timestamped
  file per crash flush. `subghz_debug_log.c`
  (`applications/fox/subghz_garage/helpers/`) is the closest existing
  precedent for "open in append mode, write a line, close every time" —
  worth reusing that exact write pattern, but note its own header
  explicitly says it's a temporary, delete-when-done diagnostic aid, not a
  permanent facility — a real crash logger needs its own module (own
  directory, no "delete me" lifecycle), just borrowing the proven
  open-write-close idiom.
- Fox_ESP32_FW: no equivalent SD-card-writing convention exists in that
  project today (it's a WiFi/BLE/HTTP/scripting tool, not historically a
  file-logging one) — this would be a first for that repo. Needs its own
  small storage-write helper; check what filesystem that firmware already
  mounts (LittleFS/SD, whichever `Fox_ESP32_FW.ino` already initializes)
  before assuming SD-card semantics carry over directly.

## What this design doc does NOT cover yet (deliberately)

Given the crash-handler write happens in an interrupts-disabled context
right before a reset — a section of code where a mistake produces a worse
failure than the one being diagnosed, and where nothing here can be
verified by a real compile this session — the actual code for both sides
should go through its own focused review before being written, rather than
being bolted on as a fast follow to this doc. Specifically still open:

- Exact bit-packing of the FoxFW2.0 48-byte record (which fields, how many
  bits each, what gets truncated).
- Whether `esp_reset_reason()` (or equivalent) is safe to call from
  wherever Fox_ESP32_FW's crash record actually gets written — needs
  checking against that specific call context, not assumed.
- The next-boot flush code on both sides: exact scene/screen (FoxFW2.0
  already has `DesktopSceneFault` as a natural hook point; whether the SD
  write happens there or earlier, before that scene, needs deciding) and
  the exact file-naming/rotation scheme under `/System/Logs/` (cap on
  number of retained files? Per the release process's general spirit of
  not silently filling a user's SD card).
- Whether FoxFW2.0's `furi_check`/`furi_assert` macros should be extended
  to auto-capture `__FILE__`/`__LINE__` as part of this work or left as a
  separate, explicitly-scoped follow-up (flash-size cost either way, needs
  its own sizing pass like the earlier symbolized-backtraces attempt got).
