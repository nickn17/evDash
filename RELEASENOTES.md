# RELEASE NOTES

### V4.5.15 2026-02-17
- SD `v2` log file size cap (`256KB`) with automatic rollover:
  - Active `*_v2.json` file now rotates to indexed file (`..._1_v2.json`, `..._2_v2.json`, ...) before flush if pending write would exceed `256KB`.
  - Rotation is applied during regular flush and also when recording is stopped with buffered data pending.
  - Helps background upload keep pace on slower links by producing smaller upload units.

### V4.5.14 2026-02-17
- Sentry/charging edge-case fix (eGMP):
  - Fixed case where command queue could still fall into Sentry/autostop during charging start sequence (unlock -> open charge port -> start charge -> lock).
  - Added charging hold window (`180s`) based on recent charging activity to avoid false autostop from short charging-state transitions.
  - If autostop preparation timer is already running and charging becomes active, pending stop is cancelled and queue stays/resumes active.
  - Queue autostop stop conditions now explicitly ignore low-voltage/autostop triggers while charging is active/recent.

### V4.5.13 2026-02-17
- SD log manual upload tuning and stability fix:
  - Manual upload chunk size was increased from `4KB` to `8KB` for faster transfer while keeping stability on slower links.
  - If an `8KB` chunk fails, upload now automatically falls back to `4KB` chunking and retries the same part.
  - Manual upload now uses dedicated HTTPS timeouts (`connect 6s`, `I/O 12s`) to avoid premature failures on larger files.
  - Manual upload uses per-chunk SD reopen/seek/read for better long-run stability on device (avoids stale file-handle behavior across HTTPS calls).
  - Upload endpoint fallback was removed; SD log upload now uses only `https://api.evdash.eu/v1/upload`.
  - Fixes regression where manual upload could finish with `Uploaded 0 of 1` on larger logs.
- Background SD `v2` upload behavior remains unchanged (still conservative chunking/timing).

### V4.5.12 2026-02-17
- Contribute/SD `v2` anti-duplicate tuning:
  - While online contribute is active (`WiFi connected` + net available), minute `v2` SD snapshots are now skipped.
  - SD `v2` snapshots are still recorded when contribute is offline/unavailable, so delayed upload fallback remains functional.
  - Helps prevent mixed parallel streams (`live` + delayed SD upload) that can create shifted/duplicated track points for the same drive period.
- CoreS3/CoreS3 SE BLE startup responsiveness fix:
  - Removed blocking BLE auto-scan during boot; connection now starts non-blocking from the main loop.
  - BLE reconnect retries now use backoff instead of tight repeated attempts with blocking delay.
  - Improved BLE runtime state initialization/reuse for more stable reconnect behavior.
  - Helps prevent long post-boot periods where touch/UI appears unresponsive before adapter reconnect.

### V4.5.11 2026-02-14
- README cleanup and positioning updates:
  - `Contribute data` section was rewritten to focus on secure access to user-owned vehicle history (default `ON` with explicit opt-out path).
  - `UI controls` and `Screens` sections were moved below `Contribute data`.
  - Hardware comparison was expanded to `CoreS3 vs Core2 v1.0 vs Core2 v1.1` (targets, UI responsiveness, OBD BLE4, CAN COMMU, CPU/speed, PMIC, RTC backup).
- SD `v2` background upload visual indicator:
  - During active background upload of closed `_v2.json` logs, the SD status circle is rendered with `+2px` larger radius.
  - Makes ongoing silent upload during driving more visible without popups.
- evdash API identity update:
  - Removed board prefix in API `deviceId/id` (`c2.` / `c3.` no longer sent to `api.evdash.eu`; only clean hardware token is sent).
  - Added `dev` marker with compiled board type (`core2` or `coreS3`) to online contribute payloads (`v1` and `v2`) and SD `v2` upload query.

### V4.5.10 2026-02-14
- Debug screen was redesigned into paged view (`2` pages) with larger and clearer text.
- Added debug screen paging controls identical to Battery cells screen (`top-left` previous page, `top-right` next page).
- Debug Page 1 now shows compact operational status lines (WiFi/IP, comm/remote mode, ignition/charging, `AC/DC`, drive direction, SD, sleep/queue, net state).
- Debug Page 2 now focuses on GPS details (module/UART/speed, fix/sat/age, speed/heading, altitude, lat/lon, wake counters, time source/date/time).
- Charging state summary is now condensed to practical one-line diagnostics (for example `CHG AC ... DC ...`) to improve readability.

### V4.5.9 2026-02-13
- SD `v2` log upload now runs quietly in background during normal app use (no popups or user messages).
- Upload starts after a `5 minute` grace period and retries later if network/server is unavailable.
- Only closed log files are uploaded (currently active file is skipped).
- Successful upload renames file from `_v2.json` to `_v2_uploaded.json`; failed upload keeps original file for retry.
- Uploaded files are automatically cleaned after `30` days.
- Improved manual upload stability for larger files.
- Server upload handling was updated for better compatibility and reliable device verification.
- Contribute/SD `v2` GPS source marker for `M5 GNSS (NEO-M9N)` is now sent as `m9n` (instead of `gnss`).

### V4.5.8 2026-02-13
- Touch keyboard input hardening:
  - While on-screen keyboard is active, bottom touch buttons under display are now fully ignored.
  - Prevents accidental `BtnA/BtnB/BtnC` actions during text input (SSID/password/API fields).
  - Bottom-button touch events are swallowed until keyboard is closed, then normal behavior resumes.
  - Fixed keyboard/menu rendering collision: while keyboard is open, background menu touch handlers are paused, so screen no longer flickers between menu and keyboard.

### V4.5.7 2026-02-13
- WiFi status indicator tuning for `Contribute data`:
  - In `contribute-only` mode (without ABRP and without Remote API/MQTT), green `WiFi OK` window is now `75s`.
  - Existing behavior remains unchanged for ABRP/Remote API/MQTT workflows (`15s` green window after successful upload).
- Contribute JSON `v2` / SD card `v2` metadata update:
  - `carVin` remains included in every `v2` snapshot payload.
  - Added `sd` flag (`0/1`) to indicate active SD logging state (`mounted+recording`).
  - Added `gps` source marker: `m8n`, `m9n`, `v21` (or `none` when unavailable).
  - Added `comm` source marker: `can`, `ble4`, `bt4`, `wifi` (fallback `unknown`).
  - Since SD `v2` logging uses the same payload builder, these fields are now present in both online contribute uploads and SD `v2` logs.
- Speed screen unit rendering polish:
  - Cell min/max voltage keeps large numeric value and renders unit `V` in small font.
  - Cell min/max temperature keeps large numeric value and renders small unit suffix (`°C`/`°F`).
  - `out` temperature line now shows small `°C`/`°F` suffix after the value.
  - Motor speed line now renders `kRPM` as a small unit suffix.
  - Small unit labels were nudged down by `+3px`; `cell min` voltage unit `V` was additionally moved `16px` left for cleaner alignment.
- CoreS3 touch/menu drag stability:
  - Fixed extra menu offset jump after drag release on CoreS3 (`touch drag_end` no longer applies one more scroll step).
  - Menu drag/swipe updates are now processed only while touch is actively pressed, and drag deltas are reset on release.

### V4.5.6 2026-02-13
- Command optimizer menu wording cleanup:
  - `Disable CMD optimizer` was renamed to `Command optimizer`.
  - Status display now shows positive state (`[on]` when optimizer is active, `[off]` when disabled).
- Sentry/autostop safety fallback:
  - If no valid CAN response is available for a longer time (adapter/config issue), command-queue autostop can still enter Sentry after a grace period.
  - This removes dependency on GPS/CAN wake data for Sentry activation and helps protect AUX 12V battery.
- SD settings restore hardening:
  - Restoring `/settings_backup.bin` now validates/limits read size to `SETTINGS_STRUC` and prevents overflow on size mismatch.
  - Partial restore from older backup sizes now keeps current defaults for new fields instead of corrupting settings.

### V4.5.5 2026-02-13
- Adapter search/menu updates:
  - `Select BLE4 Obd2 adapter` renamed to `Search BT3/4/WiFi adapter`.
  - Search item was moved above `Obd2 Bluetooth4 (BLE4)` in adapter menu order.
  - Search action now works in both modes:
    - `Obd2 Bluetooth4 (BLE4)` -> BLE4 scan (existing flow).
    - `Obd2 Bluetooth3 classic` -> BT3 device discovery and selection list.
    - `Obd2 WiFi adapter [DEV]` -> WiFi AP scan/list and connect flow.
  - When adapter mode is not BT3/BLE4/WiFi, menu shows clear hint: `Use BT3/BLE4/WiFi mode`.
- Added on-device keyboard editing for:
  - `Obd2 -> WiFi IP`
  - `Obd2 -> WiFi port` (stored as numeric value, clamped to `0..65535`)
- Menu text casing cleanup:
  - Adapter/menu labels now consistently use `Obd2` instead of `OBD2`.
  - `WIFI` labels were normalized to `WiFi`.
- Documentation update:
  - Recommended BLE4 adapter is now explicitly listed as `OBDLink CX BLE4` in README.

### V4.5.4 2026-02-13
- Menu updates:
  - `Clear driving stats` renamed to `Clear realtime driving stats`.
  - `Clear realtime driving stats` moved under `Others` as the first item.
  - Adapter labels unified: `BT3/4 name`, `WiFi ip`, `WiFi port` (consistent `WiFi` casing).
- Selected item marker in menu is now visual (green check icon) instead of the text prefix `> `.
- Touch keyboard fix: closing keyboard with `ENTER`/`EXIT` now suppresses the next touch release briefly, preventing accidental click-through (e.g. menu `page down`).
- Message dialog fix: closing a message popup now consumes that touch event, so it no longer activates the menu item underneath.
- Pairing UX update:
  - Pairing instructions are shown in a single persistent 3-line popup:
    - `Open evdash.eu`
    - `Settings / cars -> pair`
    - `Pin/Code xxxxxx`
  - Added generic 3-line `displayMessage` support for on-device dialogs.
  - The pairing popup stays visible until user dismisses it or pairing state changes (paired/expired/error).
- Boot UX update: startup now shows explicit text progress steps on screen (Display/WiFi/GPS/SD/Adapter/Finalize), including `GPS initialization...` for easier freeze diagnostics.

### V4.5.3 2026-02-13
- Added OBD2 Bluetooth Classic (`BT3`) adapter mode with command-queue compatible AT flow (targeting adapters such as OBDLink MX/MX+ on ESP32 boards with Classic BT support).
- Boards/chips without Classic BT (e.g. BLE-only variants) report BT3 mode as unsupported at runtime.
- New adapter selection in menu and Web Interface: `OBD2 Bluetooth3 classic`.
- BT3 connection logic supports MAC-first connect (if set) with fallback to adapter name (`OBD2 name` setting).
- Boot-time BT memory release now keeps BT stack enabled when `BT3` mode is selected.
- BLE adapter scan action is now explicitly limited to BLE4 mode to avoid wrong-mode pairing flow.
- `OBDII not connected` status box now covers BT3 mode too.

### V4.5.2 2026-02-13
- Core3/CoreS3 GPS now uses `Serial2` pins in build config: `-D SERIAL2_RX=44` and `-D SERIAL2_TX=43`.

### V4.5.1 2026-02-13
- Added new GPS module option `GPS v2.1 GNSS` in both on-device menu and Web Interface.
- GPS v2.1 selection now auto-sets GPS serial speed to `115200` and uses baud auto-detect order `115200 -> 38400 -> 9600`.
- GPS v2.1 path runs without the u-blox UBX init sequence used for `NEO-M8N`.
- Updated `platformio.ini.example` for CoreS3 defaults: `SERIAL2_RX=44`, `SERIAL2_TX=43`, plus clearer `upload_port`/`monitor_port` placeholders.

### V4.5.0 2026-02-12
- New WiFi menu action `Pair with evdash.eu`: generates a 6-digit pairing code and shows live countdown in menu suffix.
- Added cloud pairing flow against `pair/start` + `pair/status` endpoints with board-specific device ID (`c2.`/`c3.` prefix), response validation, and auto-poll every 8s.
- Pairing UX now reports clear states on-device: `WiFi not connected`, `Pairing failed`, `Paired with evdash.eu`, and `Pair code expired`.

### V4.3.6 2026-02-10
- eGMP parser hardening for Ioniq 53/58 (58/63 profile): invalid responses (`NO DATA`, negative UDS reply, short/truncated frame) are ignored.
- Added stricter value validation before decode/overwrite: `SOC 0..100`, `SOH 0..100`, battery/module temps `-30..80 C`, AUX voltage `9.0..16.5 V`, and speed/ODO sanity checks.
- BMS cell frame parser now skips placeholder-heavy blocks (`C8/FF/00` flood) and keeps last valid cell values instead of propagating garbage.
- Response normalizer now strips framing separators/noise from binary payloads (e.g. `:` in split lines) before car parsing.
- 58/63 kWh queue optimization: skipped `ATSH7E4 22010C` requests (not needed for 144-cell packs).
- Ioniq6 58/63 `loadTestData` demo payloads refreshed from contributed capture.

### V4.3.5 2026-02-10
- eGMP `loadTestData`: added dedicated demo config/profile for Hyundai Ioniq6 AWD 77/84 with full demo PID set.
- eGMP `loadTestData`: added dedicated demo config/profile for Hyundai Ioniq6 53 kWh contributed data (mapped to Ioniq6 58/63 car type).
- Ioniq6 demo payloads now include compatible BMS cell-frame generation for `220102/220103/220104/22010A/22010B/22010C` and corrected VIN demo response parsing.

### V4.3.4 2026-02-10
- Added automatic firmware version check after WiFi connects (online check against server latest build).
- Firmware version check request now includes `id` and `v` query params (`id=c2|c3.<hwid>`, `v=<current app version>`).
- If a newer build is available, device now shows: `New version`, `available <version>`, and update URL `evdash.eu/m5flash`.
- Web flasher short URL path standardized to `/m5flash` in firmware constants and docs.

### V4.3.0 2026-02-09
- API endpoints migrated: contribute posts to `https://api.evdash.eu/v1/contribute`, SD log upload posts to `https://api.evdash.eu/v1/contribute/upload`.
- Web Flasher links updated to `https://www.evdash.eu/m5flash`.
- Contribute JSON `v2` timestamp cleanup: `ts` uses `YYMMDDHHIISS` (e.g. `200509093307`); removed `tsUnix` and `tsDate`.
- Contribute JSON `v2` identity cleanup: payload now uses `key` only (duplicate `token` removed).
- Contribute JSON `v2` device identity upgraded: `deviceId` is now a stable UUID-like HW ID derived from efuse MAC.
- Contribute JSON `v2` payload compacting: `stoppedCan`, `ign`, `chg`, `chgAc`, `chgDc` are encoded as `0/1`.
- Contribute JSON `v2` motion cleanup: samples without valid GPS coordinates are skipped; `motion` is omitted when no valid samples exist.
- SD log upload reliability: fixed log discovery when active filename is empty and changed active-log exclusion to exact filename match (for both `v1` and `v2` logs).
- Network scheduler cleanup: `Remote send tick` / `ABRP send tick` run only when target upload is configured, reducing false `Net temporarily unavailable`.
- Contribute stability on low-memory devices: tuned HTTPS timeouts, capped `v2` raw-frame upload, reduced retry/fallback pressure, and optional BLE memory release on boot when BLE4 is not used.
- Top-level menu `App version` no longer triggers OTA.
- CAN reconnect policy relaxed: no-response threshold increased to 20s with an additional 20s grace period after `init_can()`.

### V4.2.3 2026-02-08
- Contribute upload reliability fixes: increased HTTPS timeouts, redirect handling, payload sanity checks, and better HTTP error logging to diagnose `WiFi OK / Net temporarily unavailable` cases.
- Contribute identity compatibility finalized: backend is token-first again (legacy devices stay compatible), while `key`/`deviceKey` are accepted as aliases; API response remains legacy `status + token`.
- Stable hardware ID support kept: `deviceId` in payload is parsed and stored as `hwDeviceId` on backend (fallback pairing when useful).
- Contribute `v2` still includes `token` (primary ID) and also carries `key` + `deviceId` for cross-version/client compatibility.
- `Contribute once` behavior improved:
  - In `No MCP2515` state it sends immediately (no CAN wait).
  - With active OBD2/CAN it keeps original behavior and sends only after full CAN queue collection.
- Manual `Contribute once` now bypasses temporary network backoff, and HTTP error diagnostics now show real negative `HTTPClient` codes (instead of wrapped `65535`) plus WiFi/IP/GW/DNS context.
- Net status popup UI updated to match CAN status style (rounded box) and can now be dismissed by touch in the same way as CAN status messages.

### V4.2.2 2026-02-08
- Contribute JSON `v2`: new compact payload with shorter keys (`soh`, `powKw`, `batV`, `auxA`, ...), plus `motion` and `charging` sections sampled every 5 seconds (up to 12 samples / minute).
- Added charging transition events in contribute `v2`: `chargingStart` and `chargingEnd` now include SOC, battery V/A, min/max cell values, min cell index, battery min/max temperature, `cecKWh`, `cedKWh`.
- Contribute JSON `v2` naming cleanup for backend alignment: `stoppedCan`, `spd`/`gpsSpd`/`hdg`, `cMinV`/`cMaxV`/`cMinNo`; `apikey` removed from `v2`, and raw ECU keys (`<ecu>_<did>` + `_ms`) are appended in the same payload.
- SD card logging now supports JSON mode switch:
  - `v1`: legacy per-loop logging (existing behavior).
  - `v2`: minute snapshots using the new contribute `v2` payload; filenames use `_v2.json` suffix.
- Menu updates (`Others -> SD card`):
  - Added `Json type [v1/v2]` (default `v2` for new devices and after factory reset).
  - Removed `Save console to SD card`.
- Settings upgraded to version 22 with migration and validation for the new JSON mode field.
- OBD2 WiFi adapter (`[DEV]`) communication hardened: better prompt parsing (`>`), command latency tracking, and timeout recovery so command queue does not get stuck waiting for prompt forever.
- OTA update over WiFi fixed: OTA now downloads board-specific binary path (`core2 v1.0`, `core2 v1.1`, `CoreS3`), follows HTTPS redirects, and supports missing `Content-Length` responses.
- Menu (`Others`): `Rem. Upload` renamed to `Remote upload`, and adapter-type suffix (`[WIFI]`) removed from this item.
- Menu (`Remote upload`): fixed `Contribute data` toggle so it reliably switches `on/off` without unintended reboot.
- Menu (`OBD2/CAN adapter`): `Select OBD2 adapter` renamed/moved to `Select BLE4 OBD2 adapter` after `OBD2 Bluetooth4 (BLE4)`.
- Speed screen battery separators: top dashed line animation remains for `PTC`; bottom dashed line now animates for `LTR`, `COOL`, and `LTRCOOL` battery-management modes.
- Net status: fixed stale `Net temporarily unavailable` message when WiFi stays connected but no internet upload task is active (or last failure is old).
- Added browser flashing page (`https://www.evdash.eu/m5flash`) for all supported boards.

### V4.2.0 2026-02-07
- GPS: faster first fix via robust GNSS baud auto-detect/confirm; module type change now auto-sets default baud (NEO-M8N `9600`, M5 GNSS `38400`) and logs are clearer for troubleshooting.
- Refactor/UI baseline: all `drawScene*` rendering moved to `Board320_240_display.cpp`, while original font styles (bitmap/digital) were preserved after the split.
- Menu overhaul: smooth pixel drag scrolling, refreshed row styling/spacing, partial last-row rendering, right-side scrollbar, and cleaner back navigation icons.
- Menu touch reliability: reserved 64x64 zones (`exit/parent`, `page up/down`) now consume taps, drag release no longer triggers accidental item click, and menu cursor/offset handling no longer jumps to stale positions.
- Dynamic scan-list return: leaving BLE/WiFi scan lists now returns to the correct parent item (`Select OBD2 adapter` / `Scan WiFi networks`).
- Screen carousel: bounded swipe carousel (`Main`..`Debug`) with live preview, ~30% release threshold, corrected swipe direction (left=next, right=previous), and reduced drag flicker.
- Screen switching: classic left/right tap zones still cycle full screen set (including `SCREEN_BLANK`), while swipe carousel remains bounded.
- Blank screen UX: switching to `SCREEN_BLANK` now shows black for 5s before LCD power-off.
- CAN status UX: CAN status box is rounded (8px), tappable-to-dismiss, and dismiss touch no longer propagates to menu actions.
- CAN communication stability: CAN TX now retries up to 3x (PID + flow-control) before reporting send error; helps reduce transient `Err sending frame` popups with threading enabled.
- Dialogs: `Confirm action` and related modal flow were redesigned (rounded dialog + styled `YES/NO`) and fixed to prevent touch-release click-through to underlying menu items.
- Cell screen: added paging for long packs (e.g., eGMP 192 cells) via top-left/top-right taps with `PG x/y` indicator.
- Speed screen status icons: WiFi icon redesign (larger, with backup badge) plus cleaned top-bar spacing with GPS/SD/satellite indicators.
- Speed screen vehicle/status visuals: top-view car is rounded and now shows door/hood/trunk + light/charging-port states; bottom brake indicator is redesigned as rear-car with integrated brake lights.
- Speed screen battery indicators: min/max separator lines are now dashed; top dashed line animates when PTC is active; top-right BMS mode text is color-coded (`PTC`=red, `COOL`/`LTR`/`LTRCOOL`=cyan).
- WiFi flow: added `Scan WiFi networks` with SSID list and on-device touch keyboard for connect/password entry.
- WiFi keyboard: improved readability, reusable editor for `SSID`/`Passwd`/backup fields, hold-drag magnifier preview, delayed key preview hold, `ENTER` bottom-right, and `123`/`ABC` layout toggle.
- WiFi/OBD2 behavior fixes: WiFi scan list now follows BLE-style list behavior and exits back to menu (no reboot); `OBD2 WIFI adapter [DEV]` no longer causes reboot loops when WiFi link is down.
- Performance: while anonymous `Contribute` data is collected, UI now gets periodic redraw yields to avoid frozen-looking speed/GPS values.
- Menu naming/structure cleanup: top-level `WiFi network` moved near top (`#2` after `Clear driving stats?`), `Others` renamed to `Others (SD card, GPS,...)`, `Adapter (CAN/OBD2)` to `OBD2/CAN adapter`, and `Board setup` to `M5stack board setup`.

### V4.1.15 2026-02-06
- Sentry: autostop no longer blocked by stale door state when CAN responses stop.
- Auto brightness: periodic recalculation during parking so sunrise updates without new GPS fix.
- Sentry: AUX voltage shown only when data is fresh (INA <= 15s, CAN <= 5s).
- Sentry: reduce loop rate and redraw to lower idle load (1s loop delay, 2s redraw).
- Sentry: stop ABRP SD card logging while suspended.

### V4.1.14 2026-02-05
- Contribute uploads now send when WiFi is connected, regardless of "Remote upload type".

### V4.1.13 2026-02-05
- eGMP: add detailed UDS logging for 3E/1003/command with NRC decode.
- eGMP: add safe test mode for single ECU/DID (UDS 22 only).
- eGMP 58/63 packs: prefer 220105 temp bytes for module temps + sanity-filter temps (reject < -40 or > 120) to prevent spikes in min/max and inlet/heater.

### V4.1.12 2026-02-04
- Sentry wake tuning: higher gyro wake limit plus periodic reset while parked.
- Sentry GPS wake: relaxed thresholds (valid fix, >= 5 km/h, 4+ satellites).
- First touch now wakes display and resumes queue (no extra tap needed).
- Sentry screen now shows GPS speed/satellites and AUX voltage.
- CoreS3: enabled IMU gyro motion detection for wake.

### V4.1.11 2026-02-04
- eGMP 58/63 packs: clamp temp sensor count to 8 when parsing 220101 to avoid bogus negative readings.

### V4.1.10 2026-02-02
- eGMP 58/63 packs: parse only 8 BMS temp sensors from 220101 to avoid null (-40°C) readings.

### V4.1.9 2026-02-01
- Sentry/queue stop now hard-blocks OBD2/CAN sends (BLE/WiFi included) and pauses comm loop while suspended.
- BLE/WiFi adapters stop reconnect/scanning during Sentry; suspend/resume now cleanly toggles OBD2 client state.
- Sentry wake logic: when voltmeter is enabled, GPS/gyro wake triggers are ignored and GPS processing is paused to avoid false wakes.
- Sentry: GPS/Gyro motion wake is limited to one per parking session; further motion wakes require a touch to resume the queue.
- Sentry screen now shows GPS/Gyro wake counters.
- eGMP: duplicate brake light/speed/power requests early in the queue for faster display refresh while keeping full ECU scan.

### V4.1.8 2026-02-01
- ABRP upload: avoid stacking back-to-back sends when the network task is busy (helps prevent lockups with short ABRP intervals).
- ABRP upload: refresh HTTPS client per request to reduce resource pressure after repeated posts.
- eGMP: add Ioniq6 58/63 option and update Ioniq5/EV6 labels to 58/63 and 77/84 where cell counts match.

### V4.1.7 2026-01-25
- Added per-PID latency tracking and upload metadata so the anonymous `Contribute data` stream can show how long each request took and help spot slow ECUs.
- Once the VIN is cached we stop sending the Mode 09 PID 02 (`0902`), preventing repeated “Packet filtered” noise while keeping VIN detection stable.

### V4.1.6 2026-01-23
- WiFi fallback now triggers on prolonged internet outage even when WiFi stays connected (counts failed uploads and switches to backup).
- eGMP: stop querying unsupported VIN/MCU requests (770/22F190, 7E3/2102) to avoid negative responses.
- eGMP: once VIN is loaded (e.g., via 0902), skip further 22F190 polling.

### V4.1.5 2026-01-21
- Network status: avoid "Net temporarily unavailable" when remote API/ABRP is not configured; skip sends if URL/token is missing.
- Contribute uploads only run when enabled in settings.
- Charging screen: charging time now advances even before RTC/NTP/GPS time is available; reset start time on charging transition.
- eGMP VIN: added Mode 09 PID 02 (7DF/0902) and expanded UDS 22F190 queries for more reliable VIN detection.

### V4.1.4 2026-01-12
- GPS sanity filter: reject zero/invalid coords and implausible jumps; re-accept after long gaps to recover after tunnels/reboots.
- ABRP/remote/MQTT/contribute GPS payloads only send filtered fixes to avoid bad locations.

### V4.1.3 2026-01-11
- WiFi internet outage handling: detect failed uploads, show "WiFi OK / Net temporarily unavailable" status, and back off network requests to keep CAN/SD logging responsive.

### V4.1.2 2026-01-10
- Web interface: editable settings (wifi, adapter, board, units, GPS, remote upload, etc.) with auto-save + status message.
- Web interface: BLE4 scan + adapter selection, plus Reboot/Shutdown actions.

### V4.1.1 2026-01-09
Several fixes
- Threading: CAN communication moved to a dedicated background FreeRTOS task (xTaskCreate / pinned-to-core where available) with proper synchronization (task handle + mutex) to keep UI responsive and reduce sporadic CAN dropouts.
- Bootup & CAN: Fixed CAN command-queue auto-stop so it only becomes eligible after the first valid CAN response (checks params.getValidResponse). Prevents the queue from stopping during initial “silent” boot phase.
- eGMP / VIN loading: Fixed missing VIN initialization on eGMP platforms.
- Bootup & UI: Avoid blocking on `getLocalTime()` during cold start by using non-blocking time reads with cached fallback; keeps menu/UI responsive even before GPS/NTP time is set.
- UI / Architecture: Menu logic was separated out of Board320_240.cpp.

### V4.1.0 2026-01-08
- Updated platform version for M5Core2 configurations (thanks to bkralik)
- Adopted spot2000 changes - Kia EV9 support, gps heading for ABRP and optimized SD card buffer and flush

### V4.0.4 2025-01-21
- CAN queue can stop, suspend, or resume the CAN chip

### V4.0.3 2025-01-11
- CarHyundaiIoniq5 renamed to CarHyundaiEgmp
- refactoring in src/Board320_240.cpp

### V4.0.2 2024-11-11
- New: Automatic detection of service, RX, and TX UUIDs for BLE4 adapters!

### V4.0.0 2024-11-08
!! Note !! This is an initial support release. Known issues include:
- Core2 v1.0: When powered via external CAN, the module does not start. You need to briefly connect USB power, and after disconnecting, it runs fine. Tested for a few months with this configuration.
- Core2 v1.1, CoreS3: Support is not thoroughly tested yet. Basic USB and OBD2 functionality should work. The community will need to help complete other features.
- CoreS3: Using COMMU CAN power with GNSS settings for C2, and connecting USB caused smoke and resulted in a burnt CoreS3. Please proceed with caution and at your own risk.
Changelog: 
- New: M5Stack CoreS3 support
  - LogSerial via HWCDC instead HardwareSerial
  - M5GFX, M5Unified
  - Rewritten ESprite to LGFX_Sprite, fonts::xx
  - CAN CS/INT pins
- New: M5Stack Core2 v1.1 board 
- Core2 v1.0 rewriten to latest m5core2 branch
- No longer support for SLEEP MODE - deep sleep and shutdown

### v3.0.6 2024-04-11
- motor1/2 rpm instead out temperature

### v3.0.5 2024-04-05
- upload logs to evdash server fixes
- factory reset console command

### v3.0.4 2024-03-24
- Menu / others / Remote upload / Upload logs to evdash server
  - upload all JSON files from the SD card to the evdash server. After successful upload files will be deleted.
  - If you want to use this feature for easier downloading instead using SD card reader. Let me know (nick.n17@gmail.com). I will send you a token to your logs on the evdash server. 

### v3.0.3 2024-03-23
- eGMP: detection of AC/DC based on power kw
- contributing data: added battery current amps (for abrp support)
- gps altitude fix
- speed screen: outdoor temperature instead inverter temperature

### v3.0.2 2024-03-20
- Improved logic for stop CAN queue - not only car off but not charging too (works with DC charger)
- Average charging speed in kW
- Gyro motion sensor support (orange icon), wake queue with gyro sensor motion
  Note: Core2 contains gyro-chip on small board with battery (bottom module)

### v3.0.1 2024-03-19
- "stop CAN command queue" optimizations
- "sentry ON" message when car is parked (sleep mode = off)
- Car speed type (auto, only from car, only from gps)

### v3.0.0 2024-03-16
- No longer supported boards - TTGO-T4 and M5Stack Core1
- New gps M5 module GNSS works with UART2 / 38400 settings
- [DEV] added support for new board M5Stack CoreS3
- Removed SIM800L module support
- Removed feature: Headlight reminder
- Removed feature: Pre-drawn charging graphs
- Menu Board/Power mode (external/USB)
- Menu Others/Gps/Module type - init sequence (none, M5 NEO-M8N, M5 GNSS)
- Source code comments by Spotzify and Code AI extension
- Colored booting sequence
- Display status - No CAN response, CAN failed
- Speed screen now displays charging info (time, HV voltage/current) instead speed 0 km/h
- Init sdcard (saved 500ms)
- GPS handling (removed infinity loop)
- eGMP - When "ignition" is off evDash scan only 1 ECU (770/22BC03) to check ignitionOn state. 

### v2.8.3 2023-12-25
- removed debugData1/2 structures. Replaced with contribute anonymous data feature.
- e-GMP - added bms mode (none/LTR/PTC/cooling)
- webinterface only for Core2 (memory problems with core1)
- ABRP switched to https (thanks to Spotzify)

### v2.8.2 2023-12-23
- GPS serial speed (default 9600)
- command queue autostop option (recommended for eGMP)

### v2.8.1 2023-11-20
- MQTT client (remote upload)

### v2.8.0 2023-11-17
- Menu item - Clear driving & charging stats
- Some speedKmh / speedKmhGPS fixes
- Sleep mode = OFF | SCREEN ONLY (prevent AUX 12V battery drain)
  - Automatically turn off CAN scanning when 2 minute inactive 
    - car stopped, not in D/R drive mode
    - car is not charging
    - voltage is under 14V (dcdc is not running)
    - TODO: BMS state - HV battery was disconnected
  - Wake up CAN scanning when 
    - gps speed > 10-20kmh
    - voltage is >= 14V (dcdc is runinng)
    - any touch on display

### v2.7.10 2023-09-21
- Speed screen: Show drive mode (neutral), drive, reverse, and charging type AC/DC
- Automatic shutdown to protect AUX 12V in sleep mode / screen only
- eGMP - In/Out temperature

### v2.7.9 2023-08-30
- fix: eGMP Ioniq6 dc2dc is sometimes not active in forward driving mode and evDash then turn off screen 
- fix: abrp sdcard records are saved when the switch in the menu is off

### v2.7.8 2023-08-23
- Added reboot menu item
- Fixed gps 0 longitude

### v2.7.7 2023-06-17
- Initial code for BT3 and OBD2 WIFI adapters
- Code cleaning
- Switch for turn off backup wifi, additional icon indicating backup wifi
- Setting for BT3 adapter name, OBD2 WIFI ip/port

### v2.7.6 2023-06-10
- Wifi AP mode + web interface http://192.168.1.2
- Removed Kia DEBUG vehicle

### v2.7.5 2023-06-03
- Log ABRP json to sdcard

### v2.7.4 2023-06-01
- MR from spot2000 https://github.com/nickn17/evDash/pull/74
- Added new values to "debug" screen
- Ioniq5/eGmp improvements

### v2.7.3 2023-05-30
- Touch screen, better response for single tap, long press supoort, btnA-C handling
- Removed modal dialog for CAN initialization. Better info about errors
- CAN max.limit (512 chars) for mergedResponse. Prevents corrupt setting variables with invalid CAN responses.

### v2.7.2 2023-02-04
- Fixed ABRP live data (increased timeout from 500 to 1000)

### v2.7.1 2023-01-22
- Menu Adapter type / Disable optimizer  (this log all obd2 values to sdcard, for example all cell voltages)
- Fixed touch screen
- Added new screen 6 (debug screen)

### v2.7.0 2023-01-22
- m5core library 1.2.0 -> 1.5.0

### v2.6.9 2023-01-16
- Speed screen: added Aux%

### v2.6.8 2023-01-12
- m5core2 is working again (thanks to ayasystems & spot2000)
- added Ioniq 28 PHEV (ayasystems)

### v2.6.6 2022-10-11
- speed screen - added aux voltage
- speed screen - added trip distance in km
- automatic clear of stats after longer standing (half hour)
- automatic wake up from "sleep mode only" when external voltmeter (INA3221) detects DC2DC charging

### v2.6.5 2022-08-30
- improved wake up from "sleep mode only" mode
- automatic clear of stats between drive / charge mode (avg.kwh/100km, charging graph, etc.)

### v2.6.4 2022-03-24
- SD card usage in %
- new confirm message dialog
- Memory usage menu item
- Adapter type / Threading off/on [experimental]
- SpeedKmh correction (-5 to +5kmh)
- MEB plaform (VW). Added GPS support from car (no GPS module requiredgi)
- EGMP (Ioniq5) - read 180 cell votages, 16 bat.temp.modules
- hexToDec function - improved speed. Thanks to mrubioroy, fix by ilachEU
- Ioniq5 several fixes, 180 cells, tire pressure/temperature
- MEB cell voltages - refactoring, 77kWh (96 cells) support
- added car submenus (by manufacturer)
- added support for Kia EV6, Skoda Enyaq iV, VW ID.4
- added support for Audi Q4 (patrick)

### v2.6.0 2021-12-27
- FIX added PSRAM option for M5 Core2 and TTGO T4 (BLE4+SD+Wifi is now working)
- OTA update for M5Core2 (now VS code build only)
- Wifi menu / NTP sync (Core1/Core2)
- Save/restore settings to SD card (/settings_backup.bin)
- INA3221 voltmeter - info item (3x voltage/current)
- new commands time/setTime/ntpSync (core2 RTC only)
- new message dialog
- [DEV] adding Bluetooth3 OBD2 device (not working yet)

### v2.5.2 2021-11-19
- MEB optimizations (cell screen, min/max voltage)
- removed Niro PHEV support (not completed and longer without maintainer)
- display status for ABRP live via Wifi
- bugfix prevent blank screen (sleep mode) with touch panel (core2) 
- new display low brightness limit (based on gps, 20% was too bright).
- Added ascii art and help to serial console output

### v2.5.1 2021-07-27
- dynamic scale for charging graph
- speed screen - added calculated kWh/100km
- basic support for Hyudani Ioniq5
- ABRP Live Data / API over Wifi

### v2.5.0 2021-04-19
- Volkswagen ID3 45/58/77 support by spot2000 (CAN only 29bit)
- menu refactoring
- touch screen support for m5stack core2
- aux voltage sdcard logging (by kolaCZek)

### v2.2.0 2020-12-29
- Direct CAN support with m5 COMMU module (instead obd2 BLE4 adapter). RECOMMENDED
- EvDash deep sleep & wake up for Hyundai Ioniq/Kona & Kia e-Niro (kolaCZek).
- Send data via GPRS to own server (kolaCZek). Simple web api project https://github.com/kolaCZek/evDash_serverapi)
- Better support for Hyundai Ioniq (kolaCZek).
- Kia e-niro - added support for open doors/hood/trunk.
- Serial console off/on and improved logging & debug level setting
- Avoid GPS on UART0 collision with serial console.
- DEV initial support for Bmw i3 (Janulo)
- Command queue refactoring (Janulo)
- Sdcard is working only with m5stack
- Removed debug screen
- M5 mute speaker fix

### v2.1.1 2020-12-14
- tech refactoring: `hexToDecFromResponse`, `decFromResponse`
- added support for GPS module on HW UART (user HWUART=2 for m5stack NEO-M8N)
- sd card logging - added gps sat/lat/lot/alt + SD filename + time is synchronized from GPS
- small improvements: menu cycling, shutdown timer
- added Kia Niro PHEV 8.9kWh support

### v2.1.0 2020-12-06
- m5stack mute speaker
- Settings v4 (wifi/gprs/sdcard/ntp/..)
- BLE skipped if mac is not set (00:00:00:00:00:00)
- Improved menu (L&R buttons hides menu, parent menu now keep position)
- SD card car params logging (json format)
  (note: m5stack supports max 16GB FAT16/32 microSD)
- Supported serial console commands
    serviceUUID=xxx
    charTxUUID=xxx
    charRxUUID=xxx
    wifiSsid=xxx
    wifiPassword=xxx
    gprsApn=xxx
    remoteApiUrl=xxx
    remoteApiKey=xxx

### v2.0.0 2020-12-02
- Project renamed from eNiroDashboard to evDash

### v1.9.0 2020-11-30
- Refactoring (classes)
- SIM800L (m5stack) code from https://github.com/kolaCZek

### v1.8.3 2020-11-28
- Automatic shutdown when car goes off
- Fixed M5stack speaker noise
- Fixed menu, added scroll support

### v1.8.2 2020-11-25
- Removed screen flickering. (via Sprites, esp32 with SRAM is now required!)
- Code cleaning. Removed force no/yes redraw mode. Not required with sprites
- Arrow for current (based on bat.temperature) pre-drawn charging graph 

### v1.8.1 2020-11-23
- Pre-drawn charging graphs (based on coldgates)
- Show version in menu

### v1.8.0 2020-11-20
- Support for new device m5stack core1 iot development kit
- TTGO T4 is still supported device!

### v1.7.5 2020-11-17
- Settings: Debug screen off/on 
- Settings: LCD brightness (auto, 20, 50, 100%)
- Speed screen: added motor rpm, brake lights indicator
- Soc% to kWh is now calibrated for NiroEV/KonaEV 2020
- eNiroDashboard speed improvements

### v1.7.4 2020-11-12
- Added default screen option to settings
- Initial config for Renault ZOE 22kWh
- ODB response analyzer. Please help community to decode unknown values like BMS valves, heater ON switch,...
  https://docs.google.com/spreadsheets/d/1eT2R8hmsD1hC__9LtnkZ3eDjLcdib9JR-3Myc97jy8M/edit?usp=sharing

### v1.7.3 2020-11-11
- Headlights reminder (if drive mode & headlights are off)

### v1.7.2 2020-11-10
- improved charging graph

### v1.7.1 2020-10-20
- added new screen 1 - auto mode
  - automatically shows screen 3 - speed when speed is >5kph
  - screen 5 chargin graph when power kw > 1kW
- added bat.fan status and fan feedback in Hz for Ioniq

### v1.7 2020-09-16
- added support for 39.2kWh Hyundai Kona and Kia e-Niro
- added initial support for Hyundai Ioniq 28kWh (not working yet)

### v1.6 2020-06-30
- fixed ble device pairing
- added command to set protocol ISO 15765-4 CAN (11 bit ID, 500 kbit/s) - some vgate adapters freezes during "init at command" phase

### v1.5 2020-06-03
- added support for different units (miles, fahrenheits, psi)

### v1.4 2020-05-29
- added menu 
- Pairing with VGATE iCar Pro BLE4 adapter via menu!
- Installation with flash tool. You don't have to install Arduino and compile sources :)
- New screen 5. Conpumption... Can be used to measure available battery capacity!
- Load/Save settings 
- Flip screen vertical option
- Several different improvements

### v1.1 2020-04-12
- added new screens (switch via left button)
- screen 0. (blank screen, lcd off)
- screen 1. (default) summary info
- screen 2. speed kmh + kwh/100km (or kw for discharge)
- screen 3. battery cells + battery module temperatures
- screen 4. charging graph
- added low batery temperature detection for slow charging on 50kW DC (15°C) and UFC >70kW (25°C).

### v1.0 2020-03-23
- first release
- basic dashboard
