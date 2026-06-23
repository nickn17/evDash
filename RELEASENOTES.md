# RELEASE NOTES

### V5.0.5 2026-06-23
- Peugeot e-208 direct-CAN now reads the VIN and no longer wastes a poll cycle on it — ISO-TP flow-control addressing fix:
  - The mode-09 VIN request (`0902`) is sent on the functional broadcast header `0x7DF` and the BMU answers from its physical response ID `0x7E8`. `sendFlowControlFrame()` addressed the ISO-TP Flow Control frame to `lastPid` (the `0x7DF` broadcast), but an ECU only accepts an FC on its physical request ID (`0x7E0`). The First Frame arrived (`49 02 01 "VR3…"`, the Peugeot WMI) but the BMU never sent the consecutive frames, so the VIN read timed out (~1.2 s) on every poll cycle. Because `carVin` stayed empty, `CarPeugeotE208::commandAllowed()` never suppressed `0902`, so the timeout repeated indefinitely and dragged down the whole loop's responsiveness.
  - Flow Control is now sent to the responder's physical request ID: on a functional broadcast (`lastPid == 0x7DF`) answered from `0x7E8..0x7EF`, the FC target is `rxId - 8` (`0x7E0`); every other request keeps `lastPid` (manufacturer ECUs such as PSA `0x6B4`→`0x694` already use the request ID directly). The VIN now reassembles on the first cycle, after which `commandAllowed()` permanently skips `0902` and the recurring timeout is gone. Gated on the broadcast ID, so no other car's reception changes.

### V5.0.4 2026-06-20
- Peugeot e-208 now actually goes to sleep (Sentry auto-stop) after parking:
  - The e-208 parser latched `forwardDriveMode` to `true` the first time speed exceeded 1 km/h and never cleared it (the only reset ran once at boot). `applyIgnitionState()` derives `ignitionOn` from the drive mode, so after the first drive `ignitionOn` stayed permanently `true`, `parkingLikely` was never satisfied and the command-queue auto-stop never armed — the device kept scanning CAN and draining the 12 V AUX battery indefinitely. Selecting Park or switching the car off made no difference, and an INA3221 voltmeter would not have helped (the e-208 reads its AUX voltage from CAN, and the only voltage-based stop triggers below 11.5 V, an emergency-discharge level).
  - Drive mode now uses a stationary debounce: it enters on movement (`speed > 1`) immediately, and leaves only after the car has stood still (`speed <= 1`) for 120 s. A normal traffic-light stop (< 2 min) stays in drive; a genuine park releases the latch, `ignitionOn` goes false and Sentry arms as designed (~2 min debounce + 60 s grace before SENTRY ON). Gated entirely inside `CarPeugeotE208`, so no other car changes.

### V5.0.3 2026-06-19
- Peugeot e-208 direct-CAN now reads battery data on the busy OBD bus — MCP2515 hardware acceptance filtering:
  - V5.0.2 got the command queue running again, but on the e-208 the OBD port is the live broadcast CAN (hundreds of frames/s). With the MCP2515 masks left open, its two RX buffers overflowed with broadcast traffic and the battery ECU's reply was dropped before the firmware could read it — every value still read `n/a` / "Packet filtered". A plugged-in ELM327 works on the same port because it programs a hardware receive filter via `ATCRA`; the direct-CAN path left the masks open and filtered only in software.
  - The Peugeot car type now programs the MCP2515 to accept only the four e-208 diagnostic response IDs (`0x58F` charger, `0x682` VCU, `0x694` BMS, `0x7E8` VIN). These IDs never appear in the e-208's broadcast traffic (verified against ~35k captured frames), so the controller stays idle until a genuine reply arrives and can no longer overflow. Mirrors the existing BMW i3 filter block and is gated on `CAR_PEUGEOT_E208`, so no other car's reception changes — their masks stay at the accept-all default. ISO-TP multi-frame is unaffected: consecutive frames arrive on the accepted response ID.

### V5.0.2 2026-06-18
- Direct-CAN command queue no longer stalls on a busy bus (Peugeot e-208 / PSA e-CMP, but affects any car):
  - `receivePID()` refreshed the rx-timeout (`lastDataSent = millis()`) on **every** received frame, including the broadcast traffic it filters out. On a quiet diagnostic bus this is harmless, but on a car whose OBD port is the live high-speed CAN (the e-208 emits hundreds of broadcast frames/s), `lastDataSent` was permanently "now", so the `rxTimeoutMs` check never fired. The command queue hung on the very first PID that got no answer (`0902` VIN) and never advanced to the battery PIDs — every value stayed `n/a` with a constant "Packet filtered" status, even though the adapter was transmitting and reading the bus correctly.
  - The timeout is now refreshed only when a frame passes the response-ID filter (i.e. is actually part of our reply). Multi-frame consecutive frames arrive on the response ID and still extend the timeout, so ISO-TP reception is unchanged; a genuinely lost frame now correctly times out instead of being masked by unrelated broadcast traffic. Behavior for accepted single-frame responses is byte-identical, so quiet-bus cars (Korean/VW/BMW) are unaffected.

### V5.0.1 2026-06-13
- Build config — the CoreS3 #159 fix now actually ships: added the CoreS3 NimBLE build config (`-D EVDASH_USE_NIMBLE`, `CONFIG_BT_NIMBLE_HOST_TASK_STACK_SIZE=8192`, `NimBLE-Arduino`, `lib_ignore = BLE, SimpleBLE`) to `platformio.ini.example`. V5.0.0 shipped the NimBLE source guards but not the build flag in the example, so a fresh clone still built CoreS3 on Bluedroid and hit #159.
- TLS / OTA security (from project audit):
  - OTA firmware download now validates against a pinned root CA again. The bundled root was a stale DigiCert cert, but GitHub raw moved to Let's Encrypt, so validation silently failed and every OTA ran unverified (MITM → arbitrary firmware = remote code execution). Now pinned to ISRG Root X1 (the current chain), with the existing insecure fallback kept and logged so a future CA change still can't brick OTA.
  - Added `evdash_certs.h` with verified root CAs (ISRG Root X1 for evdash.eu/GitHub, Amazon Root CA 1 for ABRP). The drive-time telemetry paths (ABRP, contribute, SD-log) deliberately stay on unvalidated TLS — loading a CA chain raises the handshake's internal-heap use, the exact pressure behind the contribute TLS-memory failures on Core2, and their leak risk is a telemetry token, not RCE. This is documented at each call site.
- Single source of truth for per-vehicle ids (maintainability, from project audit):
  - The ABRP model string and the mobile-app relay vehicle id were two parallel `switch` statements (37 cars each) that had to be kept in sync with the app by hand. Both now read one `kCarModels[]` table in `CarModelUtils` — adding a car is one row. Behavior is byte-identical for all 37 car types (verified).
- BLE / stability hardening (from project audit):
  - Added a BLE command-queue watchdog: if the ELM327 `>` prompt is lost (packet loss / frozen adapter), the queue used to hang until reboot — it now flushes the stale partial response and re-arms after 4 s.
  - BLE connect/disconnect callbacks no longer render to the display (TFT/sprite work must not run in BLE host/controller task context); the message is now drawn from the main loop via a flag.
  - On BLE disconnect the stale characteristic pointers are cleared and a reconnect is re-armed, fixing a possible null-dereference in `executeCommand` and making auto-reconnect reliable.
  - NTP sync no longer blocks the main loop for up to 10 s (watchdog risk when NTP is unreachable); it does a single short poll since `netLoop` already retries every 5 s.
  - `commConnected` / `obd2ready` / `canSendNextAtCommand` marked `volatile` (written from the BLE callback task, read in the loop task).
- Parser correctness fixes (from project audit):
  - VW ID.3: fixed the HMI-SOC formula — `* 0, 4425 - 6, 1947` used comma operators, so SOC was always reported as 1947. Now `* 0.4425 - 6.1947` and the result is range-checked 0..100.
  - Consumption (kWh/100km) no longer divides by zero at standstill on Kia eNiro, Hyundai Ioniq, Ioniq PHEV, BMW i3 and VW ID.3 — these now use the same `speed > 20` guard as eGMP/EV9 (fall back to instantaneous kW below that).
  - SOC is range-checked (0..100) before use on Hyundai Ioniq, Ioniq PHEV, Kia eNiro and BMW i3, preventing a malformed CAN frame (e.g. a raw byte decoding to >100 %) from writing past the end of the 11-slot `soc10*` history tables and corrupting adjacent fields.

### V5.0.0 2026-06-13
- CoreS3 BLE stack migrated to NimBLE (see below) — major version bump.
- Hyundai Ioniq 6 53 kWh (Standard Range) — dedicated vehicle type (issue #107):
  - Added a separate "Ioniq6 53kWh" car type. Previously the only option was "Ioniq6 58/63kWh", whose constants (144 cells, 8 temp sensors, 58 kWh) are wrong for the SR pack — and the owner-contributed capture the 58/63 profile was tuned on actually came from this 53 kWh car.
  - The 53 kWh profile uses 132 cells, 14 battery module temperature sensors and 53 kWh, and reads module temps through the standard eGMP mapping (`220101` sensors 1-5, `220105` sensors 6-14). Tail sensors 15/16 don't exist on this pack and are no longer decoded, so they can't be read as phantom 0 °C.
  - Added a generic guard for short packs: a contiguous run of exactly 0 °C module temps at the array tail is dropped when the warmest module is ≥ 15 °C (thermally-coupled pack ⇒ a 0 °C slot behind a warm module is a missing sensor, not ice), so battery min temp is no longer dragged to 0.
  - `22010C` (cells 161-180) is skipped for the 132-cell pack; ABRP id `hyundai:ioniq6:23:58:rwd`; mobile-relay id `hyundai_ioniq6_53` (matches the evDash app).
  - After updating, select Vehicle → Hyundai → Ioniq6 53kWh, then Save settings and restart.
- eGMP frozen battery power / kWh-per-100 on BLE4 (Ioniq 6, issue #107):
  - The BLE4 notification handler reset the line buffer on every notification, so an ELM327 line split across two BLE packets was discarded. The longest response (`620101`, which carries pack power/voltage/aux) is the one most likely to span a packet boundary, so power could read once after connect and then stay frozen for the whole drive while shorter PIDs (speed, SOC, temps) kept updating.
  - The line buffer now persists across notifications and is cleared before each command; the off-by-one buffer over-read in the notification loop was fixed.
  - The BLE response normalizer now strips a leading `7F xx 78` ("response pending") prefix before parsing (the direct-CAN path already did this), so a pending+payload frame no longer fails the decode.
  - Fixed the Speed-screen kWh/100 sentinel check (`-1` vs `-1000`) so "n/a" shows correctly before the first reading.
- CoreS3 BLE boot-loop fixed — migrated to NimBLE (issue #159):
  - On M5Stack CoreS3 (ESP32-S3) the prebuilt Bluedroid host has only a 4 KB BTU task stack, which the GATT service-discovery path overflowed right after connecting to the OBD adapter ("Guru Meditation Error … Stack canary watchpoint triggered (BTU_TASK)", reboot loop). CoreS3 now uses NimBLE, whose host task stack is configurable (set to 8 KB) and whose footprint is much smaller (CoreS3 flash 60.9% → 50.0%).
  - Core2 (plain ESP32) keeps Bluedroid: it has an 8 KB BTU stack, is not affected by #159, and the two BLE stacks cannot coexist in one binary.
- Settings robustness (issue #123):
  - Serial-console setters (`wifiSsid=`, `wifiPassword=`, `remoteApiUrl=`, `mqtt*=`, …) are now bounded to the destination field size, so an over-length value can no longer overflow into adjacent settings.
  - All string settings are force-NUL-terminated after loading from flash, so a blob written by older firmware can never be read past its buffer (a source of the garbage-hostname DNS queries reported in #123).

### V4.6.15 2026-06-10
- Peugeot e-208 / PSA e-CMP:
  - Replaced the old Renault-ZOE-style `79B/21xx` polling with the confirmed PSA e-CMP topology: TBMU `6B4→694`, VCU `6A2→682`, and charger `590→58F`.
  - Added BLE4 and direct-CAN compatible `ATSH/ATCRA/ATFCSH` queue sections for M5-visible values: SOC, SOH, pack voltage/current/power, speed, outdoor temperature, 12V voltage, available charge/discharge power, isolation, battery temperatures, 108 cell voltages, module temperatures, charge-active flag, VIN, and front motor RPM.
  - Fixed the e-208 loop header ordering so each loop re-arms the correct PSA ECU header instead of inheriting the previous `ATSH`.
  - Initialized direct-CAN ISO-TP flow-control block size to `0` for long multi-frame replies, so COMMU mode can reliably receive full cell/SOH/temp arrays.
  - Added the Peugeot e-208 ABRP model id and removed the DEV suffix from the vehicle menu entry.

### V4.6.14 2026-05-26
- Renault ZOE direct CAN fix:
  - ZOE Phase 1 (Z.E. 20/22 and Z.E. 40/41) ECUs respond with a `+0x20` offset, not the default UDS `+8`. With BLE4 this worked because the ELM327 auto-receives any response ID, but the direct-CAN driver discarded the valid frames as `[Filtered packet]` (e.g. CLIM first frame `Standard ID: 0x764 Data: 0x10 0x1B 0x61 0x44 …` for request `2144` on TX `0x744`), and the M5Stack screen showed only dashes.
  - Added explicit `ATCRA` filter values after each `ATFCSH` in the ZOE command queue: `ATCRA7BB` (LBC `0x79B→0x7BB`), `ATCRA763` (Cluster `0x743→0x763`), `ATCRA764` (CLIM `0x744→0x764`), `ATCRA7EC` (EVC `0x7E4→0x7EC`, UDS standard). The TPMS / VIN / PEB sections (`ATSH765`, `ATSH763`, `ATSH77E`) use a bare `ATCRA` to clear the previous filter so the ELM327 does not inherit a stale receive address from the previous section.

### V4.6.13 2026-05-20
- Maintenance:
  - Extracted 320x240 UI primitive methods from `Board320_240.cpp` into `Board320_240_ui.cpp`, keeping display helpers separate from board/network/GPS logic.
  - Added `SETTINGS_VERSION_CURRENT` for persisted settings schema version and reused it in default settings and upgrades.
  - Reworked repeated MQTT topic formatting to use bounded `snprintf` and checked remote JSON payload size before sending.
  - Reduced PlatformIO config duplication by sharing common dependencies, build flags and Core2 board settings across environments.
  - Made `update_bin.sh` compatible with `sh update_bin.sh` for copying built firmware files from `.pio/build` into `dist`.
  - Removed the on-device CAN car command menu and its vehicle-specific action lists.

### V4.6.11 2026-05-12
- CAN driver:
  - The direct-CAN driver previously ignored `ATCRA…` commands and always expected the response ID to be `TX+8`. For VW-family vehicles the gateway routes some responses with a `+0x6A` offset (e.g. `0x714→0x77E`, `0x765→0x7CF`), so valid replies for `2222E0/E4` (climate) and `221DD0/DD6` (gateway) were marked `[Filtered packet]` and discarded.
  - The driver now tracks the last `ATCRAxxx` value and accepts that RX ID in addition to the default `TX+8` mapping. `ATSH` resets the tracked value so it must be re-armed by the next `ATCRA`.

### V4.6.10 2026-05-11
- Skoda Citigo-e / VW e-Up / Seat Mii:
  - Fixed cell voltage and module temperature polling — DIDs `221E40-221EA5` (84 cells) and `221EAE-221EBB` (14 module temps) were being sent to the climate ECU (`0x714`) instead of the BMS (`0x7E5`) because the CAN header was not restored after polling the climate DIDs `2222E0/2222E4`. The ECU answered with `7F 22 31` ("request out of range"), shown as `[Filtered packet]` in the log, so per-cell voltages and module temperatures never populated.
  - Added `ATSH7E5 / ATCRA7ED` after the climate block so subsequent cell/module requests go to the BMS.

### V4.6.9 2026-05-09
- Mobile relay snapshot:
  - Added TPMS fields to the `snapshot` JSON: `flTireKpa`, `frTireKpa`, `rlTireKpa`, `rrTireKpa` (bar*100 → kPa) and `flTireC`, `frTireC`, `rlTireC`, `rrTireC`. Tyre pressures and temperatures now appear on the iPhone evDash relay screen instead of `--`.
  - Sentinels (no data yet) are emitted as JSON `null` so the iPhone shows `--` rather than a bogus 0.

### V4.6.8 2026-05-07
- Mobile relay / HTTPS upload:
  - Contribute upload is no longer deferred just because the evDash mobile relay app is connected.
  - BLE relay stays active during Contribute TLS uploads; mbedTLS now prefers PSRAM for larger TLS allocations instead of freeing BLE memory.
  - Remote terminal capture now queues serial lines and flushes them from the relay loop, so ABRP/Contribute/Traccar HTTP logs are not lost or sent synchronously inside TLS code.

### V4.6.7 2026-05-07
- Renault ZOE support:
  - Reworked ZOE Z.E. 20/40/50 polling from the evDash app profile instead of only changing pack capacity.
  - Added legacy ZOE CLIM/EVC/TPMS/PEB/VIN requests for cabin/outdoor temperatures, 12V voltage, brake/gear/doors, plug state, tyre pressure and motor temperature.
  - Added Z.E. 50 Phase2 29-bit BMS/EVC/CLIM polling for SOC, SOH, pack voltage, available energy, battery temperatures, 12V DCDC current and cabin/outdoor temperatures.
  - Validates ZOE SOC/SOH/voltage/temperature values before storing them, avoiding bad placeholder values in the UI.
  - Grouped ZOE variants under Renault in the vehicle menu.

### V4.6.6 2026-05-06
- Mobile relay terminal capture:
  - Added relay control messages `serialCaptureStart` / `serialCaptureStop`.
  - Serial console output can now be mirrored to the paired evDash mobile app as
    `type:"serial"` JSON lines while capture is enabled.
  - Bumped firmware version for M5Stack terminal-capture builds.

### V4.6.5 2026-05-05
- Mobile relay stability:
  - Online contribute upload is deferred while a phone is actively connected to the BLE relay, avoiding repeated BLE disconnects during HTTPS/TLS uploads.
  - SD `v2` minute logging stays active while contribute upload is deferred by an active relay connection.

### V4.6.4 2026-05-05
- Renault ZOE support:
  - Added selectable ZOE Z.E. 20/40/50 variants.
  - ZOE Z.E. 40 now uses a 41kWh pack instead of falling back to 22kWh.
- VW legacy city EV support:
  - Added shared Skoda Citigo-e iV / VW e-Up / Seat Mii Electric profile.
  - Reads SOC, pack voltage/current/power, charge state, battery temperatures, SOH, aux voltage/current and cell min/max where the ECU replies.
  - Keeps fallback SOC PIDs as backup only, so primary SOC from `028C` is not overwritten by noisier fallback values.

### V4.6.3 2026-05-04
- Web flasher release refresh:
  - Bumped firmware version so `evdash.eu/m5flash` and firmware version check pick up the contribute TLS/raw HTTPS fixes.

### V4.6.2 2026-05-03
- Contribute/relay v2 fix:
  - Disabled legacy contribute/SD JSON `v1`; saved `v1` settings are forced back to `v2` on load.
  - Online contribute now always builds the `v2` payload with 5-second motion/charging samples.
  - Online contribute POST now moves the JSON payload to PSRAM, uses raw TLS with SNI first, and temporarily deinitializes the mobile BLE relay stack during HTTPS when CAN adapter is used.
  - SD logging no longer offers the JSON type switch and records only `v2` logs.
  - Mobile relay protocol messages now report `ver:2`; the live stream stays on compact `snapshot`/`cells`/`temps`/`raw` messages and full `v2` contribute is only sent when the app explicitly requests it.
  - Contribute `v2` JSON numbers are serialized with fixed precision to avoid float artifacts like `3.980000019`.
  - Contribute HTTPS POST now sends `User-Agent` and `Accept` headers so hosting WAF does not reject raw TLS uploads with `403 Forbidden`.
- Removed the unused local hotspot/webadmin server and ESP32WebServer dependency.

### V4.6.1 2026-05-02
- evDash mobile relay v1:
  - Added `Relay for iOS/Android app`, `Pair mobile app`, and `Forget mobile app` under the adapter menu.
  - Added BLE peripheral service `evDash Relay` for the mobile Android/iPhone app.
  - Added 6-digit pairing flow with stored relay token and paired mobile id.
  - Pair mobile app menu suffix now shows only the 6-digit pairing code.
  - Streams live snapshots, cell voltages, module temperatures, and recent raw CAN/OBD frames to the mobile app.
  - Settings upgraded to version 25 with relay defaults disabled.
- evDash mobile relay snapshot v2:
  - Added indoor/outdoor temperature, BMS mode, doors/hood/trunk, lights, brake
    lights, front/rear motor RPM, trip, average speed, aux voltage, and kWh
    deltas for the iOS/Android dashboard.
- Adapter cleanup:
  - Removed BT3 and OBD2 WiFi adapter support, including menu/Web Interface selection.
  - Renamed `Search BT3/4/WiFi adapter` to `Search BLE4 adapter`.
  - Renamed `BT3/4 name` to `BLE4 name`.
  - Legacy saved BT3/WiFi adapter mode is normalized to BLE4 on load.

### V4.5.25 2026-05-02
- GPS v2.1 GNSS LED:
  - Blue PPS LED is now disabled during normal driving/standing.
  - PPS LED is enabled only while the CAN queue is stopped/suspended in Sentry.
- Contribute/SD `v2` raw frame export:
  - Increased raw frame upload limit from `24` to `32`.
  - Allows large eGMP packs to include all BMS cell-voltage blocks (`220102/103/104/10A/10B/10C`) in contribute/SD snapshots.
- Traccar Web UI/menu config:
  - Added Web UI fields for Traccar enabled state, server host and server port.
  - Added on-device menu editors for Traccar server host and port.
  - Traccar upload now uses saved settings instead of hard-coded `demo3.traccar.org`.
  - Settings upgraded to version 24 with default Traccar server `demo3.traccar.org:5055`.

### V4.5.24 2026-05-01
- Traccar server upload support (thanks to Conny/spot2000):
  - Added optional Traccar GPS position upload, enabled from the on-device menu.
  - Upload sends device id, timestamp, latitude/longitude, speed, heading, altitude, SoC and charging state.
  - Server address/port are compile-time constants, so they must be adjusted before building custom firmware.
  - Settings upgraded to version 23 with default Traccar upload disabled.

### V4.5.23 2026-03-25
- Menu idle auto-exit:
  - When the on-device menu stays open without touch/button activity for `60s`, it now closes automatically and returns to the active screen.

### V4.5.22 2026-03-23
- WiFi transfer activity icon:
  - While WiFi data transfer is active, the top WiFi status icon on speed screen now switches to transfer arrows.
  - Arrows keep the same WiFi state color logic (`red/yellow/green`) as the original icon.
  - This also makes `Contribute` and offline `SD v2` background upload activity visible on-screen.
- Sentry touch wake responsiveness:
  - While the CAN queue is stopped in `SENTRY ON`, the idle wait is now sliced into short `50ms` steps instead of one hard `1s` sleep.
  - This keeps the low-power/Sentry pacing, but touch/button wake is polled much more often, so waking the device after entering the car should react noticeably faster.

### V4.5.21 2026-03-21
- Contribute/SD `v2` offline fallback and manual upload fix:
  - SD `v2` offline snapshots no longer require valid `odoKm`; when `Contribute data` is `OFF`, non-empty `v2` snapshots continue to be recorded to SD card.
  - Restores delayed/offline fallback logging for cars or startup phases where odometer is temporarily unavailable.
  - Manual `Upload logs to evdash server` now flushes the pending in-memory SD buffer before scanning files.
  - Manual upload now also includes the currently active JSON log after flush, instead of skipping it completely as the active file.
  - Fixes cases where the server had no recent data and manual upload reported effectively `No files found` even though current drive data existed only in the open SD log/buffer.
- Driving stats auto-clear after long parking/sentry:
  - If the car stays parked or in Sentry for at least `2 hours`, the next shift into `Drive` triggers `Clear driving stats` automatically.
- EV9 parked BMS keep-alive reduction:
  - EV9 now skips `ATSH7E4 021003` and `ATSH7E4 3E00` while ignition is off and no AC/DC charging is active.
  - Helps avoid holding the BMS in diagnostic session during idle/Sentry and reduces the chance that evDash itself keeps the parked car awake.
- IGPM ignition parser hardening:
  - EV9 and shared eGMP parser now ignore `ATSH770 22BC03` placeholder replies where the ignition/light state bytes are only `AA` padding.
  - Prevents false `ignitionOn=1` from padded IGPM frames, which could block CAN autostop and keep the device out of Sentry.

### V4.5.19 2026-03-05
- Changes from recent `spot2000` merges into `master` (range `3567e57..2a7b584`):
  - EV9 charging and drive parser update:
    - Added explicit AC/DC charging detection via `ATSH744 / 22E001`, including transition and heartbeat debug logs.
    - `chargingOn` is now derived from `chargerACconnected || chargerDCconnected` (with discharge guard for `batPowerKw < -0.5`).
    - `ChargingOn` stale auto-reset timeout was increased from `10s` to `30s`.
    - EV9 queue now includes BMS diagnostic keep-alive (`021003`, `3E00`) for more stable charging data.
    - Gear parsing moved to `ATSH7E2 / 22E004`; speed refresh now reads from `ATSH7B3 / 220100`.
  - ABRP telemetry and upload interval update:
    - ABRP interval now uses `0.5s` steps (`off`, `0.5` ... `5.0s`) in both menu and Web UI.
    - ABRP scheduler now uses millisecond timing (`lastAbrpSendAtMs`) for reliable sub-second intervals.
    - ABRP payload now includes `heading` and tire pressures `tire_pressure_fl/fr/rl/rr`.
    - Added detailed ABRP diagnostics (payload/body length, HTTP status, response body).
  - GPS/NTP robustness:
    - For GPS `v2.1`, heading can now be computed from movement between fixes (minimum `3m`) with module `course` fallback.
    - NTP sync now uses `pool.ntp.org` with fallbacks (`time.cloudflare.com`, `129.6.15.28`) and retries every `5s` for `60s` before GPS-time fallback.
    - Debug time-source label was normalized to `NTP INTERNET` / `GPS` / `NONE`.
  - GPS UART init now accepts hardware serial port `1` and `2` for `SERIAL2_RX/TX` mapping.

### V4.5.18 2026-02-21
- GPS `v2.1` (ATGM336H/AT6668) startup init added:
  - Added CASIC PCAS init sequence during GPS init (`PCAS02`, `PCAS03`, `PCAS00`).
  - Sets output interval to `200ms`, keeps only `GGA+RMC`, and saves settings to the module.
  - Keeps `v2.1` on AT commands only (no u-blox UBX config).
- GPS jump-filter reacquire tuning for `v2.1`:
  - Reacquire timeout after large jump reject is now `120s` for `GPS v2.1` (previously effectively `900s` for all modules).
  - Helps avoid several-minute stale coordinates after startup while preserving conservative behavior for other GPS module types.

### V4.5.17 2026-02-21
- Contribute/SD `v2` GPS motion stability update:
  - `motion` sampling now accepts a recent valid GPS fix for a short grace window (`15s`) even if the current parser tick marks fix as invalid.
  - Helps prevent empty/missing 5-second motion rows on GPS `v2.1` during transient validity drops.
  - GPS coordinate rounding for `lat/lon` in contribute `v2` (top-level and `motion`) was increased from `5` to `6` decimals.
- eGMP charging state false-positive fix (preheat without cable):
  - For eGMP `220106` charging-bit parsing, `chg` is no longer set when battery power is clearly discharging (`batPowerKw < -0.5`).
  - Prevents `chg=1` with `chgAc=0`/`chgDc=0` during cabin preheat or auxiliary load without plugged charger.
  - Applied to both shared eGMP parser (Ioniq 5/6, EV6) and EV9 parser.

### V4.5.16 2026-02-19
- ABRP telemetry sign alignment:
  - ABRP payload now explicitly uses reversed sign against internal evDash battery power convention.
  - `power/current`: driving consumption (negative in evDash) is sent as positive to ABRP, while charging/regen is sent as negative.

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
