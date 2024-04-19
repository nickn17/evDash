# evDash

!!! Use it at your own risk !!!

- evDash Discord server: https://discord.gg/rfAvH7xzTr
- Project is maintained by EV owners car community

Supported hardware

- Only boards M5Stack Core2 or M5Stack CoreS3
- M5 GPS modules - GNSS NEO-M9N (38400bps), older GPS U-BLOX NEO-M8N (9600bps)

Deprecated
- INA3221A voltage meter

No longer supported hardware
- M5STACK CORE1
- LILYGO TTGO T4 v1.3
- SIM800L GPRS module

Working only with electric vehicles
Fully supported: Hyundai Ioniq5/6, Kia EV6
Community supported: Kia e-Niro/Hyundai Kona EV, Kia Niro PHEV, Renault ZOE 28, BMW i3, VW ID3 45/58/77.
See Release notes, quick installation via flash tool bellow.

## Required hardware

### BOARD
- M5STACK CORE2 IOT Development Kit (K010)
  https://shop.m5stack.com/products/m5stack-core2-esp32-iot-development-kit
- older M5STACK CORE2 IOT Development Kit (K010)
  https://shop.m5stack.com/products/m5stack-core2-esp32-iot-development-kit

### CAN vs OBD2 adapter
- optional CAN module COMMU (M011) - RS485, TTL and CAN
  https://shop.m5stack.com/products/commu-module
- OBD2 connector can provide power to the M5 stack with a 12V to 5V converter (e.g. Recom R-785.0-1.0).

- OBD2 adapters - supported is only Vgate iCar Pro Bluetooth 4.0 (BLE4) OBD2. We can add another BLE adapter if you provide 3x UUID (service/notify,read/write)
- For nonstop use we strongly recommend to use direct CAN onnection (via OBD2 connector)). It's due to security! 

### GPS
- GNSS Module with Barometric Pressure, IMU, Magnetometer Sensors (NEO-M9N, BMP280, BMI270, BMM150)
  https://shop.m5stack.com/products/gnss-module-with-barometric-pressure-imu-magnetometer-sensors
- older module (M003) - NEO-M8N (with external atenna)
  https://shop.m5stack.com/products/gps-module

## Hardware configuration

The M5 Core 2 uses UART0 for serial communication and flashing (USB port).

The COMMU module is wired with SMD jumpers to use UART0 for TTL and UART2 for RS485. CAN doesn't need UART. Both 0 and 2 can be unwired.

The GPS module is wired with SMD jumpers to use UART2. This can be easily changed to UART0 in order to use it stacked with the GSM module. The counterside is this conflicts with the USB connection of Core2, so flashing will not be possible.If it's only stacked with COMMU, it can stay on UART2, but COMMU needs to be unwired from UART2.

Check the documentations of the modules for more details:
Core2 - https://docs.m5stack.com/en/core/core2
GPS - https://docs.m5stack.com/en/module/gps
COMMU - https://docs.m5stack.com/en/module/commu

## Quick installation with ESP32 flash tool

See [INSTALLATION.md](INSTALLATION.md)

## Installation from sources (VS code)

See [INSTALLATION.md](INSTALLATION.md)

## RELEASE NOTES

see. [RELEASENOTES.md](RELEASENOTES.md) file

## Screens and shortcuts

Touch screen zones
![image](https://github.com/nickn17/evDash/blob/master/docs/core2_touch_zones.jpg)

- Middle button - menu
- Left button - toggle screens

Touch screen
- left 1/3 of screen - toggle screen left
- right 1/3 of screen - toggle screen right
- middle 1/3 of screen - menu

In the menu 
- top left corner (64x64px) - exit menu
- top right corner (64x64x) - page up
- bottom right corner (64x64px) - page down
- rest of the screen - select item

Screen list
- 0 blank screen, lcd off
- 1 automatic mode (summary info / speed kmh / charging graph)
- 2 summary info
- 3 speed kmh + kwh/100km, charging data (V/A/kW)
- 4 battery cells + battery module temperatures
- 5 charging graph
- 6 consumption table. Can be used to measure available battery capacity.
- 7 debug info

![image](https://github.com/nickn17/evDash/blob/master/screenshots/v2_ioniq6.png)

Top Info

- yellow - icon - upload data. 
- circle - outer gps 
- inner - queue loop (flashing).
- lines under it -- headlights (still not working on egmp)


![image](https://github.com/nickn17/evDash/assets/7864168/0a936f1a-fd46-49fd-926b-9716ca7ba007)

