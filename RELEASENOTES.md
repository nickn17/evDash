# RELEASE NOTES

### Next version

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

