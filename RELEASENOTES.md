# RELEASE NOTES

### V4.1.0 2026-01-08
- Update platform version for M5Core2 configurations (thanks to bkralik)
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

