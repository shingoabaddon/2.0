# Unleashed Firmware & ARF Firmware — Feature Gap Analysis

## Checkpoint — last reviewed up to here

| Repo | Branch | Commit reviewed through | Date | Local checkout |
|---|---|---|---|---|
| Unleashed (`DarkFlippers/unleashed-firmware`) | `dev` | `92c4fdd9cab1754e8c1e16d7ad1fc236c28b3e83` ("bump apps tag") | 2026-09-04 | `D:\FZ\FIRMWARE\unleashed` |
| ARF (`D4C1-Labs/Flipper-ARF`) | `main` | `56701a815b54121a4b4d102e361add5468ffa92b` | 2026-07-25 | `D:\FZ\FIRMWARE\ARF` |

Every item below was assessed and given a decision (ported, declined, or
tracked as a pending task) as of this checkpoint. Next time this
assessment is redone: `git log <checkpoint-commit>..HEAD` (or re-pull and
diff) against each repo's local checkout above to see only what's new
since this pass, rather than re-reviewing the full history again. Update
this table (and re-run the two "recent commits" reports, tasks #347/#348's
pattern) once the next pass completes.

Research doc, no code changes — per explicit instruction, nothing here has
been ported. Unleashed's history was read live from GitHub
(`DarkFlippers/unleashed-firmware`, `dev` branch, commits/PRs/CHANGELOG
covering roughly Jun 6 - Sep 4, 2026). ARF's history was read the same way
against `D4C1-Labs/Flipper-ARF` (confirmed as the right repo by matching
its last commit hash/date/message against our own local ARF checkout's
`git log` and `git remote -v`) — its `main` branch has had zero commits
since Jul 25, 2026, so "recent" for ARF really means Jun 1 - Jul 25.
Excluded from both: bug fixes, renames, CI/docs/typo changes, and routine
upstream-merge noise. Items FoxFW already covers (often more extensively)
are also left out — see `PROJECT_HANDOVER.md` for what FoxFW already has.

Also folded in here: the separate API-level research task (why Unleashed
moved from 88.2 to 88.4, where FoxFW still sits). Sourced from Unleashed
PR #1073's commit message (Aug 20, 2026): upstream official firmware added
`fap_exclude_libs=["gcc"]` manifest support and exported the AEABI
soft-float helper routines through `api_symbols.csv`, so an app doing
double-precision float math no longer needs to statically link libgcc's
soft-float routines into its own `.fap` — a real per-app size win. Worth
remembering if FoxFW ever bumps past 88.2: TPMS's pack()/unpack() math and
any other float-heavy app code could drop that linkage. Unleashed's `dev`
tip has since moved further to 88.6 via routine merges with no distinct
version-bump commit found by message search (GitHub's commit search only
indexes messages, not diffs).

## 1. Unleashed Firmware

1. **NFC resident-RAM cut via per-protocol plugins (~54%).** Moved NFC
   protocol scenes out of the main app image into loadable per-protocol
   plugins — the same architectural pattern FoxFW already uses for SubGHz
   Garage's protocol groups. FoxFW's NFC support is largely stock this
   session, so this is the single most relevant architectural idea in the
   window if NFC RAM/flash ever becomes a constraint.
   [PR #1073](https://github.com/DarkFlippers/unleashed-firmware/pull/1073)
   (commit `2b4411f`, Aug 20)

2. **Native MIFARE Plus SL3 support.** MIFARE Plus promoted from
   detection-only to a first-class protocol: AES-auth read, dictionary
   attack with a per-UID key cache, full SL3 emulation with
   shadow-writeback, write/update-to-card, and an "Add Manually" flow for
   18 Plus variants.
   [PR #1032](https://github.com/DarkFlippers/unleashed-firmware/pull/1032)
   (commit `45c3762`, Jul 7; refined by #1014, #1016, #1035)

3. **NFC Magic 2.0.** Magic Ultralight/NTAG (USCUID-UL) write/clone/wipe
   with automatic transport selection (direct CUID/ATS vs. backdoor),
   PWD-AUTH for protected tags, resumable per-page Partial Write, and
   family-first detection across UL11/UL21, NTAG213/215/216, UL-C, UL-5.
   [CHANGELOG at build tag 27jul2026](https://raw.githubusercontent.com/DarkFlippers/unleashed-firmware/28a95c9/CHANGELOG.md)

4. **RPC Network + GPS services.** New RPC service classes exposing
   network and GPS to external tooling — directly relevant to FoxFW Lab's
   own CLI/RPC-driven WebUI. Could plug in as a new panel (real GPS
   location, HTTP-style bridging) without needing the ESP32 companion.
   [PR #1013](https://github.com/DarkFlippers/unleashed-firmware/pull/1013)
   (Jun 28); [PR #1047](https://github.com/DarkFlippers/unleashed-firmware/pull/1047)
   (Jul 28)

5. **JS Runner externalized to a `.fap`.** Moved out of the firmware image
   into `apps/assets/js_app.fap` to free internal flash/RAM; the `js` CLI
   command became a CLI plugin. Same externalization strategy FoxFW
   already used for TPMS/Frequency Analyzer/Modulation Analyzer — worth a
   quick check for any FoxFW stock app that could still get the same
   treatment.
   [Commit `1a402a8`](https://github.com/DarkFlippers/unleashed-firmware/commit/1a402a8)
   "sorry no ram for js scripts anymore" (Jul 30)

6. **NightStand Clock**, a new default Clock app: overnight display with
   Up/Down brightness control, a red LED nightlight, a stopwatch, and a
   locale-aware 12h/24h daily alarm.
   [Commit `be05e0c`](https://github.com/DarkFlippers/unleashed-firmware/commit/be05e0c)
   (Jul 25)

7. **Pluggable main-menu style system + new "Grid" style.** Unleashed
   generalized its menu styling into a pluggable system and shipped Grid
   as the first style built on it — worth a look alongside FoxFW's own
   Fox Theme/Carousel/Classic system, either as a 4th style or as
   inspiration for making the style system itself more pluggable.
   [PR #1119](https://github.com/DarkFlippers/unleashed-firmware/pull/1119)
   (Sep 3); [PR #1126](https://github.com/DarkFlippers/unleashed-firmware/pull/1126)
   (Grid, Sep 4)

8. **LFRFID: Hitag Micro chip support (8265/8210/H5.5)**, read/write.
   [PR #1002](https://github.com/DarkFlippers/unleashed-firmware/pull/1002)
   (Jun 9)

9. **LFRFID: Wipe T5577** — reset a T5577 to blank with read-back
   verification.
   [PR #1003](https://github.com/DarkFlippers/unleashed-firmware/pull/1003)
   (Jun 9)

10. **NFC: auto-save recovered MIFARE Classic keys to the user
    dictionary**, so a cracked key persists for future reads instead of
    being one-shot.
    [PR #1118](https://github.com/DarkFlippers/unleashed-firmware/pull/1118)
    (Sep 3)

11. **GUI: apps can cover their own startup, and the Loader shows a
    loading animation while a large `.fap` is read from SD** — relevant
    context for FoxFW's own loading-wheel/Installing-screen work.
    [PR #1125](https://github.com/DarkFlippers/unleashed-firmware/pull/1125)
    (Sep 4); [PR #1101](https://github.com/DarkFlippers/unleashed-firmware/pull/1101)
    (Aug 21)

12. **New GUI primitives**: a public `canvas_get_buffer` API (could
    simplify future screenshot/frame-capture work) and a reusable
    date/time input widget.
    [PR #4399](https://github.com/DarkFlippers/unleashed-firmware/pull/4399),
    [PR #4261](https://github.com/DarkFlippers/unleashed-firmware/pull/4261)
    (both Jun 30, upstream OFW)

13. **New SubGHz protocols**: Telcoma/Cardin EDGE
    ([PR #1001](https://github.com/DarkFlippers/unleashed-firmware/pull/1001)),
    Elplast (PR #4309), Cardin S449 + FSK12Kdev modulation (PR #4328),
    Superrollo GW60 roller-shutter KeeLoq HCS361
    ([PR #1068](https://github.com/DarkFlippers/unleashed-firmware/pull/1068),
    Aug 26). Pure protocol-coverage work rather than architecture — worth
    a quick registry diff against FoxFW's Automotive/Garage lists, lowest
    priority of this section.

14. **NFC: Bambu Lab filament-spool parser** (type/color/code/temps),
    ported from an external open-source project. Fun, low-effort parity
    item.
    [PR #1012](https://github.com/DarkFlippers/unleashed-firmware/pull/1012)
    (Jun 16)

15. **Infrared: save universal-remote buttons** into a profile — minor UX
    addition to the universal IR remote.
    Commit dated Sep 1, message "infrared allow saving universal remote
    buttons" (see `dev` branch history).

## 2. ARF Firmware

Repo: [`D4C1-Labs/Flipper-ARF`](https://github.com/D4C1-Labs/Flipper-ARF)
("Flipper-ARF — Automotive Research Firmware," described in its own
README as "based on Unleashed Firmware but heavily modified... focuses
exclusively on automotive research and experimentation"). `main` branch,
last commit `56701a8` on Jul 25, 2026 — matches our local ARF checkout
exactly, so this is confirmed as the same fork we already track.

1. **RollJam attack app** — a standalone SubGHz app implementing the
   classic jam-and-capture-two-codes rolling-code attack, shipped
   alongside a rewritten Garage Door Remote (ProtoPirate-based). This is a
   distinct technique from anything in FoxFW's Automotive/RF
   Jammer/ProtoPirate suite: jam while recording two consecutive rolling
   codes, then replay the one the receiver never saw, rather than blind
   jamming or simple capture-replay.
   [Commit `3a63e14`](https://github.com/D4C1-Labs/Flipper-ARF/commit/3a63e14)
   (Jun 25)

2. **SubGHz modulation hopping** — automatically cycles RX through a
   table of modulation profiles to catch signals sent on non-default
   modulations, with a script to add new hopping tables. Could improve
   unknown-signal capture odds across Automotive/Garage/Read generally.
   [Commit `a3698f9`](https://github.com/D4C1-Labs/Flipper-ARF/commit/a3698f9),
   `99ac826` (both Jun 13)

3. **Full-d-pad live Emulate on the Receiver screen, no save required** —
   maps all four d-pad directions to different repeat/emulate behaviors
   directly off a decoded-but-unsaved signal, including "keep protocol
   encoders running while held" and "repeat car-emulate TX while held."
   FoxFW already has its own hold-OK-to-repeat-TX feature, so this reads
   as a possible enhancement (4-direction mapping, no save step) rather
   than a flat gap.
   [Commit `833c9ad`](https://github.com/D4C1-Labs/Flipper-ARF/commit/833c9ad)
   (Jul 24); also `f1422cb` (Jul 14), `b34a73b`/`069a42d` (Jul 12)

4. **Saved-signal editor + duplicate-capture detection** — a scene to edit
   a saved SubGHz signal's metadata directly, plus a warning when a new
   capture duplicates one already saved. FoxFW deliberately kept Garage's
   saved-file management lean (browse + Details + Emulate only, no
   editor) as a considered design choice (see task history) — flagging
   this so it's a conscious decision to revisit or not, not an oversight.
   [Commit `e606c5b`](https://github.com/D4C1-Labs/Flipper-ARF/commit/e606c5b)
   "Add saved signal editor and duplicate detection" (Jul 12)

5. **FlipperGB — Game Boy emulator app.** Added Jul 3, stability pass Jul
   15. A fun, high-visibility capability FoxFW has no equivalent of.
   [Commit `0d598b8`](https://github.com/D4C1-Labs/Flipper-ARF/commit/0d598b8)
   (Jul 3)

6. **FlipperDoom — Doom port.** Added Jul 2 "for fun," exit-freeze fix the
   next day. Same category as FlipperGB.
   [Commit `a0c53e3`](https://github.com/D4C1-Labs/Flipper-ARF/commit/a0c53e3)
   (Jul 2)

7. **Native Android companion app pairing built into the firmware.**
   ARF is building an "Official Flipper Mobile APP" connection layer
   directly into the firmware (still unfinished as of its last commit:
   "the connection with the mobile app isn't working... I'll take care of
   that tomorrow"), backed by its own
   [Android companion repo](https://github.com/D4C1-Labs/arf-android-companion).
   A different strategic approach from FoxFW Lab's browser-based
   CLI/RPC WebUI — worth being aware of as an alternative architecture,
   not necessarily something to copy outright.
   [Commit `94dcc82`](https://github.com/D4C1-Labs/Flipper-ARF/commit/94dcc82)
   (Jun 13)

8. **Protocol name allowlist ("Proto Filter")** — a Receiver Config option
   restricting RX decode to a chosen list of protocols by name, persisted
   in settings, to cut RAM use and improve capture odds for a known
   target. Lowest priority here: FoxFW's 11-group RX/TX plugin system
   with on/off filtering already achieves a similar goal at group
   granularity, so this is a refinement (per-protocol vs. per-group)
   rather than a clean gap.
   [Commit `f4fe2d4`](https://github.com/D4C1-Labs/Flipper-ARF/commit/f4fe2d4)
   (Jul 15)

No further ARF coverage was pursued past Jul 25, 2026 — the repo has had
zero commits since.
