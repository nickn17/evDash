# evDash (old enirodashboard)

Supported devices
1. M5STACK CORE1 IOT Development Kit (best option)
2. LILYGO TTGO T4 v1.3 (!!! limited support, no SDcard/GSM/GPS/CAN module)

Working only with electric vehicles 
Kia e-NIRO (EV), Hyundai Kona EV, Hyundai Ioniq EV, Kia Niro Phev 8.9kWh
Vgate iCar Pro Bluetooth 4.0 (BLE4) OBD2 adapter is required. See Release notes, quick installation via flash tool bellow. 

Use it at your own risk!
Author: nick.n17@gmail.com (Lubos Petrovic / Slovakia)

## Supporting me

- Buy Me a Beer via paypal https://www.paypal.me/nickn17
- EU companies can support me via IBAN/Invoice (my company is non-VAT payer in Slovakia).

Many thanks to Blas, Jens, Калин, Aleš Dokupil and others for help. Thank you for supporting me. 

## Required hardware
Board
- M5STACK CORE1 IOT Development Kit(~EUR 35)
  https://rlx.sk/sk/m5stack/7285-esp32-basic-core-iot-development-kit-m5-k001-m5stack.html
- optional M5 COMMU (CAN) module
- optional M5 GPS NEO-M8N (with external atenna)
- optional M5 SIM800L GPS module (dev)
  
or 
- older device LILYGO TTGO T4 v1.3 (~USD $30) 
  https://www.banggood.com/LILYGO-TTGO-T-Watcher-BTC-Ticker-ESP32-For-Bitcoin-Price-Program-4M-SPI-Flash-Psram-LCD-Display-Module-p-1345292.html
  I RECOMMEND TO REMOVE LION BATTERY DUE TO HIGH SUMMER TEMPERATURES

OBD2 adapter
- Supported is only this model... Vgate iCar Pro Bluetooth 4.0 (BLE4) OBD2 (~USD $30)

Others
- 3D printed case for TTGO-T4
  https://www.thingiverse.com/thing:3099913

## Quick installation with ESP32 flash tool

See INSTALLATION.md

## RELEASE NOTES
see. RELEASENOTES.md file

## Installation from sources

See INSTALLATION.md

## Screens and shortcuts
- Middle button - menu 
- Left button - toggle screens

Screen list
- no0. blank screen, lcd off
- no1. auto mode (summary info / speed kmh / charging graph)
- no2. summary info (default)
- no3. speed kmh + kwh/100km (or kw for discharge)
- no4. battery cells + battery module temperatures
- no5. charging graph
- no6. consumption table. Can be used to measure available battery capacity! 
- no7. debug screen (default off in the menu)

![image](https://github.com/nickn17/evDash/blob/master/screenshots/v1.jpg)

[![Watch the video](https://github.com/nickn17/evDash/blob/master/screenshots/v0.9.jpg)](https://www.youtube.com/watch?v=Jg5VP2P58Yg&)

