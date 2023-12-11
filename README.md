# evDash (old enirodashboard)

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!! Use it at your own risk !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

- evDash Discord server: https://discord.gg/rfAvH7xzTr
- Project is maintained by EV owners car community

Supported hardware

1. M5STACK CORE2 (best option)
2. M5STACK CORE1 IOT Development Kit (limited support)
3. DEPRECATED!!! LILYGO TTGO T4 v1.3 (limited support/no SDcard/GSM/GPS/CAN module)

Working only with electric vehicles
Fully supported: Kia e-NIRO/e-Soul (EV), Hyundai Kona EV, Hyundai Ioniq EV. 
Community supported: Niro PHEV, Renault ZOE 28, BMW i3, VW ID3 45/58/77.
See Release notes, quick installation via flash tool bellow.

## Required hardware

### Board

- M5STACK CORE2 IOT Development Kit (K010)
  https://shop.m5stack.com/products/m5stack-core2-esp32-iot-development-kit
- optional CAN module COMMU (M011) - RS485, TTL and CAN
- optional GPS module (M003) - NEO-M8N (with external atenna)
- optional GSM module (M004) - SIM800L

### CAN connection

- For nonstop use we !strongly! recommend to use direct connection to CAN (2 pins on OBD2 connector). It's due to security! Anybody can control your car via OBD2 and BT/BLE adapters are not secured well (unlock doors, ACC off/on, etc).
- OBD2 connector can provide power to the M5 stack with a 12V to 5V converter (e.g. Recom R-785.0-1.0).
- The optional INA3221A circuit allows to auto shutdown evDash when the car is off.

### OBD2 adapter

- Only this model is supported: Vgate iCar Pro Bluetooth 4.0 (BLE4) OBD2. We can add another BLE adapter if you provide 3x UUID (service/notify,read/write)

## Hardware configuration

The M5 Core 2 uses UART0 for serial communication and flashing (USB port).

The COMMU module is wired with SMD jumpers to use UART0 for TTL and UART2 for RS485. CAN doesn't need UART. Both 0 and 2 can be unwired.

The GSM module uses UART2.

The GPS module is wired with SMD jumpers to use UART2. This can be easily changed to UART0 in order to use it stacked with the GSM module. The counterside is this conflicts with the USB connection of Core2, so flashing will not be possible.If it's only stacked with COMMU, it can stay on UART2, but COMMU needs to be unwired from UART2.

Check the documentations of the modules for more details:
Core2 - https://docs.m5stack.com/en/core/core2
GPS - https://docs.m5stack.com/en/module/gps
COMMU - https://docs.m5stack.com/en/module/commu
GSM - https://docs.m5stack.com/en/module/sim800

## Quick installation with ESP32 flash tool

See [INSTALLATION.md](INSTALLATION.md)

## RELEASE NOTES

see. [RELEASENOTES.md](RELEASENOTES.md) file

## Installation from sources

See [INSTALLATION.md](INSTALLATION.md)

## Screens and shortcuts

- Middle button - menu
- Left button - toggle screens

Screen list

- no0. blank screen, lcd off
- no1. automatic mode (summary info / speed kmh / charging graph)
- no2. summary info
- no3. speed kmh + kwh/100km
- no4. battery cells + battery module temperatures
- no5. charging graph
- no6. consumption table. Can be used to measure available battery capacity.

![image](https://github.com/nickn17/evDash/blob/master/screenshots/v2_ioniq6.png)

