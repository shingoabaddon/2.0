# Fox_ESP32_FW vs GhostESP — Feature Gap Analysis

Research doc, no code changes. Source: FoxFW2.0 already bundles
`GhostESP-FlipperCompanion-1.8.0` as a Flipper app (`applications/system/`)
— its full command set was read directly from that app's
`src/menu.c`/`docs/menu.md` (the companion's UART command list, which
mirrors what the actual GhostESP ESP32 firmware implements). Fox's own
command set was read directly from `Fox_ESP32_FW`'s `.cpp` files
(`wifi_recon`, `wifi_attack`, `ble_tags`, `ble_attack`, `gps`, `ir`,
`subghz`, `fox_portal`, `gemini`, `http_bridge`). No internet access this
session, so this compares against GhostESP's *companion app's* command
list specifically, not GhostESP's own upstream source/docs directly — if
the actual GhostESP firmware has commands the companion app doesn't
expose, they're not captured here.

## 1. Bottom line

Fox_ESP32_FW covers the same core ground as GhostESP (WiFi/BLE recon +
attack, GPS, IR, captive portal) and has four things GhostESP's companion
doesn't: a SubGHz radio bridge, an AI chat relay, a scripting language,
and CSI-based mesh/presence sensing. GhostESP has more *breadth* within
WiFi/BLE specifically (deeper network-scanning toolset, a drone detector,
onboard WebUI, LED control) and an existing crash-coredump command pair
that's directly relevant to the crash-logger work in
`CRASH_LOGGER_DESIGN.md`.

## 2. Gaps — GhostESP has it, Fox doesn't (or unconfirmed)

### 2.1 Network-scanning toolset
GhostESP: NetBIOS scan, HTTP banner scan, SNMP probe, dedicated SSH scan,
WPA3 compliance check, local-network device scan (printers/cast/smart
devices), DIAL/Cast video casting, printer power control, TP-Link smart
plug control, a "Full Environment Sweep" that runs AP+station+BLE and
saves one CSV report.
Fox has: `WIFIPINGSCAN`, `WIFIARPSCAN`, `WIFIMACTRACK`, `WIFISIGMON`,
`WIFIPORTSCAN:`. No NetBIOS/HTTP-banner/SNMP/SSH-specific scanners, no
WPA3 checker, no IoT device control, no combined sweep-to-CSV.

### 2.2 WiFi attack breadth
GhostESP: DHCP starvation, SAE handshake flood with a supplied PSK, Karma
rogue-AP with a managed custom-SSID list (add/remove/clear/show/spam),
EAPOL logoff attack as a separate primitive from deauth.
Fox has: `WIFIATTACK:DEAUTH[:target]`, `:BEACON:`, `:PROBE`, `:RICKROLL`,
`:BADPACKET`, `:CSA`, `:SLEEP`, `:SAE`, `:QUIET`, `:KARMA`. Fox's `:SAE`
and `:KARMA` exist but their exact behavior wasn't verified against
GhostESP's fuller versions (custom PSK for SAE, custom SSID list for
Karma) — worth a closer read of `wifi_attack.cpp` before assuming parity.
No DHCP starvation, no separate EAPOL-logoff primitive.

### 2.3 Onboard WebUI + its own AP
GhostESP's ESP32 hosts its own HTTP server and AP (`webuiap`, `apcred`,
`set_evil_portal_html` upload a file to it). Fox's `http_bridge.cpp` only
runs a `WebSocketsClient` — it connects *out* to a relay, it doesn't serve
anything locally. See `FOXFW_LAB_WEBUI_DESIGN.md` for a proposed
browser-side alternative that doesn't require porting this.

### 2.4 BLE depth
GhostESP: full GATT workflow (scan/list/select/enumerate/track a specific
device's services+characteristics), a raw "all BLE traffic" view, an
advertiser scan/list pair, a device-to-device "GhostLink" BLE bridge
(start/stop/status/pair).
Fox has: `BLETAGSCAN:FINDMY/SMARTTAG/TILE/FLOCK/META` (tracker-focused,
by tag type) and `SPOOFAT`/`BLESPAM:`. No GATT enumeration, no raw
advertiser browsing, no device-to-device bridge (Fox's `fox_csi.cpp` mesh
is a different, presence-sensing-focused mechanism, not a comparable
BLE-bridge primitive).

### 2.5 Aerial drone detector
GhostESP: `aerialscan`/`aeriallist`/`aerialtrack`/`aerialspoof` —
OpenDroneID + DJI detection over WiFi+BLE, with position/altitude/speed
and RemoteID spoofing. Fox has no equivalent subsystem at all.

### 2.6 Evil Portal — correction, this is NOT a gap
First pass assumed Fox's `fox_portal.cpp` was thinner than GhostESP's
Evil Portal. Direct comparison shows the opposite: GhostESP's version is
upload-a-whole-HTML-file only (`set_evil_portal_html`). Fox's
`WIFIFOXPORTAL:` command set has structured field editing
(`SETTITLE:`/`SETINTRO:`/`SETNOTE:`/`SETPAGE:START:`/`SETPAGE:THANKS:`),
default-content getters, a QR-code command GhostESP has no equivalent
for, and a captured-credential export/confirm pair. Flagging this
explicitly since it's the one place the obvious assumption (GhostESP is
more complete) was wrong.

### 2.7 Cosmetics / hardware-dependent
GhostESP: RGB/NeoPixel LED control (`rgbmode`, pin/count/brightness
config) and nine idle-screen animations (`statusidle set ...`), SD-card
pin remapping for MMC/SPI modes. These depend on GhostESP's specific
reference-board wiring; whether they're worth porting depends on whether
Fox's target boards expose the same LED/SD hardware the same way — not
evaluated here, flagging only that they exist.

### 2.8 Crash coredump — directly relevant to other in-flight work
GhostESP: `coredump dump` / `coredump erase` / `crash` (forces a test
crash to generate a coredump), backed by ESP-IDF's coredump-to-flash-
partition mechanism. This is functionally the same problem
`CRASH_LOGGER_DESIGN.md` scoped for Fox_ESP32_FW via `RTC_NOINIT_ATTR` —
worth a follow-up look at GhostESP's coredump partition approach as a
second candidate mechanism before implementing that design, since it's a
different (partition-based, not RTC-backup-register-based) solution to a
sibling problem and ESP-IDF ships it as a built-in feature rather than
something to hand-roll.

### 2.9 Generic settings interface
GhostESP: `settings list/help/get/set/reset` — a generic key-value config
interface over the wire. Fox has "persistent settings" per
`PROJECT_HANDOVER.md` §4.3 but no confirmed generic get/set command was
found in this pass; settings may currently be per-feature and hardcoded
rather than a uniform store. Worth confirming before assuming this is a
real gap rather than just a different (also valid) design.

## 3. Fox has, GhostESP's companion doesn't

- **SubGHz radio bridge** (`subghz.cpp`): `SUBGHZINIT`, `SUBGHZFREQ:`,
  `SUBGHZRX`, `SUBGHZTX:` — an external CC1101 wired to the ESP32,
  independent of the Flipper's own radio. GhostESP is WiFi/BLE/IR/GPS
  only; no sub-1GHz story at all.
- **AI chat relay** (`gemini.cpp`): `AIASK:` / `AICOOLDOWN` — no
  equivalent surfaced anywhere in GhostESP's companion.
- **FoxScript** scripting interpreter with its own builtin function set
  (`script_engine.cpp`, per `PROJECT_HANDOVER.md` §4.3) — no scripting
  language exposed via GhostESP's companion commands.
- **CSI-based mesh/presence sensing** (`fox_csi.cpp`): role assignment,
  peer positions, a tunable gamma parameter — a different category of
  feature from GhostESP's BLE bridge; GhostESP has nothing CSI-based.
- **Discord relay** (`discord.cpp`) for remote notifications — no
  equivalent.
- **FoxDeFlock is BLE-side** (`BLETAGSCAN:FLOCK`) where GhostESP's
  equivalent (`flockscan`/`flocklist`/`flockstop`) is WiFi-side (OUI +
  wildcard-probe + SSID keyword matching on 2.4 GHz). Same threat
  category, different detection surface — genuinely complementary rather
  than a straight duplicate; a device running both could catch cases
  either alone would miss.

## 4. Suggested next steps (not started)

1. Read `wifi_attack.cpp`'s `runSae()`/`runKarma()` bodies to confirm or
   correct the §2.2 "unverified parity" note.
2. Look at ESP-IDF's coredump-partition API as a second candidate for
   `CRASH_LOGGER_DESIGN.md` before committing to the RTC-backup-register
   approach (§2.8).
3. Decide whether §2.1's network-scanning toolset (NetBIOS/SNMP/SSH/WPA3)
   and §2.5's drone detector are worth porting, given they're the biggest
   single feature blocks GhostESP has that Fox doesn't.
4. If a browser WebUI moves forward (`FOXFW_LAB_WEBUI_DESIGN.md`), §2.3's
   "Fox has no onboard web server" fact is what pushed that design toward
   a Web-Serial browser page instead of porting GhostESP's onboard-AP
   approach.
