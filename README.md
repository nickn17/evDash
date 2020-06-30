# KIA ENIRO DASHBOARD

OBDII dashboard for TTGO-T4 module (ESP32) + OBD BLE4.0 adapter. Working on Kia e-NIRO (EV) and Hyundai Kona.
Use it at your own risk!
Author: nick.n17@gmail.com (Lubos Petrovic / Slovakia)

## Hardware 
- LILYGO TTGO T4 v1.3 
  ~USD $30 https://www.banggood.com/LILYGO-TTGO-T-Watcher-BTC-Ticker-ESP32-For-Bitcoin-Price-Program-4M-SPI-Flash-Psram-LCD-Display-Module-p-1345292.html
  I RECOMEND TO REMOVE LION BATTERY IF INCLUDED! But be very very carefull. I will make video when I get new TTGO
- OBD BLE4.0 adapter. 
  Officialy supported is only this model...  
  Vgate iCar Pro Bluetooth 4.0 (BLE) OBD2 ~USD $30    

## Quick installation with ESP32 flash tool

Guide is here
https://docs.google.com/document/d/1nEezrtXY-8X6mQ1hiZVWDjBVse1sXQg1SlnizaRmJwU/edit?usp=sharing

## Supporting me

- Buy Me a Beer via paypal https://www.paypal.me/nickn17
- EU companies can support me via IBAN/Invoice (my company is non-VAT payer in Slovakia).

Thank you for supporting me.

Many thanks to Blas, Jens, Калин and others for help.

## Screens and shortcuts
- Middle button - menu 
- Left button - toggle screens

Screen list
- no0. blank screen, lcd off
- no1. summary info (default)
- no2. speed kmh + kwh/100km (or kw for discharge)
- no3. battery cells + battery module temperatures
- no4. charging graph
- no5. consumption table. Can be used to measure available battery capacity! 

![image](https://github.com/nickn17/enirodashboard/blob/master/screenshots/v1.jpg)

[![Watch the video](https://github.com/nickn17/enirodashboard/blob/master/screenshots/v0.9.jpg)](https://www.youtube.com/watch?v=Jg5VP2P58Yg&)

## Release notes
    
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

## About T4
ESP32-TTGO-T4
https://github.com/fdufnews/ESP32-TTGO-T4

## Installation from sources
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
