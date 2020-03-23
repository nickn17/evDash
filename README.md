# KIA ENIRO DASHBOARD v1.0

OBDII dashboard for TTGO-T4 module (ESP32) + OBD BLE4.0 adapter. I programmed it for my KIA ENIRO 2020, but could work on Hyundai Kona EV. Contact me if not. 

Author: nick.n17@gmail.com (Lubos Petrovic)

## Supporting 

- Buy Me a Beer via paypal https://www.paypal.me/nickn17
- EU companies could support me via IBAN/Invoice (my company is non-VAT payer in Slovakia). Contact me via nick.n17@gmail.com

Thank you for supporting me.

![image](https://github.com/nickn17/enirodashboard/blob/master/screenshots/v1.jpg)

Older video from alpha version 
[![Watch the video](https://github.com/nickn17/enirodashboard/blob/master/screenshots/v0.9.jpg)](https://www.youtube.com/watch?v=q0yqRzKuuWI)


## Recommended hardware and software
- TTGO-T4. I used this one (T4 v1.3) ~ USD $30 https://www.banggood.com/LILYGO-TTGO-T-Watcher-BTC-Ticker-ESP32-For-Bitcoin-Price-Program-4M-SPI-Flash-Psram-LCD-Display-Module-p-1345292.html
- OBD BLE4.0 adapter. Ex. Vgate iCar Pro Bluetooth 4.0 (BLE) OBD2 ~ USD $30
- software is written for Arduino IDE (ESP32).

## Roadmap
- connect to BLE function and deploy HEX file for common users (simplest installation)
- 2.screen with eNiro battery cells and temperature sensors
- 3.screen with charging graph
- ext.relay control for power dashboard and sleep CPU to save AUX during parking
- etc. 

## Release notes
    
### v1.0
- first release
- basic dashboard

## About T4
ESP32-TTGO-T4
https://github.com/fdufnews/ESP32-TTGO-T4

## Installation guide
- install arduino IDE + ESP32 support
- https://github.com/Bodmer/TFT_eSPI  - display library

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



