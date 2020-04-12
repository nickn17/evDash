# KIA ENIRO DASHBOARD

OBDII dashboard for TTGO-T4 module (ESP32) + OBD BLE4.0 adapter. Developed for my KIA ENIRO 2020, but could work on Hyundai Kona EV. Contact me if not. 

Author: nick.n17@gmail.com (Lubos Petrovic)

## Hardware and software
- TTGO-T4. I used this from banggood (T4 v1.3) ~ USD $30 https://www.banggood.com/LILYGO-TTGO-T-Watcher-BTC-Ticker-ESP32-For-Bitcoin-Price-Program-4M-SPI-Flash-Psram-LCD-Display-Module-p-1345292.html
- OBD BLE4.0 adapter. Ex. Vgate iCar Pro Bluetooth 4.0 (BLE) OBD2 ~ USD $30
- Software is written for Arduino IDE (ESP32).

## Supporting 

- Buy Me a Beer via paypal https://www.paypal.me/nickn17
- EU companies can support me via IBAN/Invoice (my company is non-VAT payer in Slovakia).

Thank you for supporting me.

![image](https://github.com/nickn17/enirodashboard/blob/master/screenshots/v1.jpg)

[![Watch the video](https://github.com/nickn17/enirodashboard/blob/master/screenshots/v0.9.jpg)](https://www.youtube.com/watch?v=Jg5VP2P58Yg&)

## Hardware and software
- TTGO-T4. I used this from banggood (T4 v1.3) ~ USD $30 https://www.banggood.com/LILYGO-TTGO-T-Watcher-BTC-Ticker-ESP32-For-Bitcoin-Price-Program-4M-SPI-Flash-Psram-LCD-Display-Module-p-1345292.html
- OBD BLE4.0 adapter. Ex. Vgate iCar Pro Bluetooth 4.0 (BLE) OBD2 ~ USD $30
- Software is written for Arduino IDE (ESP32).

## Release notes
    
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

## About T4
ESP32-TTGO-T4
https://github.com/fdufnews/ESP32-TTGO-T4

## Installation guide
- install arduino IDE + ESP32 support
- https://github.com/Bodmer/TFT_eSPI  - display library
- Configure TFT eSPI
  W:\Documents\Arduino\libraries\TFT_eSP\User_Setup_Select.h  
```  
// Comment
//#include <User_Setup.h>           // Default setup is root library folder
// And uncomment
#include <User_Setups/Setup22_TTGO_T4_v1.3.h>      // Setup file for ESP32 and TTGO T4 version 1.3
```  

My configuration
- Board ESP32 Dev module
- Upload speed 921600
- CPU freq: 240MHz (Wifi/BT)
- Flash freq: 80MHz
- Flash mode: QIO
- Flash size 4MB (32mb)
- Partion scheme: default 4MB with spiffs
- Core debug level: none
- PSRAM: disable
