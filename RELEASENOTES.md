# RELEASE NOTES

### Next version
- new message dialog
- new commands time/setTime/ntpSync (core2 RTC only)
- Wifi menu / NTP sync off/on option
- Save/restore setting to/from SD card (/settings_backup.bin)
- INA3221 voltmeter - info item (3x voltage/current)
- FIX added PSRAM option for M5 Core2 and TTGO T4
- [DEV] OTA
- [DEV] adding Bluetooth3 OBD2 device

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

