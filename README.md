# evDash

⚠️ **Use at your own risk!**

- evDash Discord server: [Join the community](https://discord.gg/rfAvH7xzTr)
- Project maintained by the EV owners community

## Supported Hardware

- M5Stack Core2 v1.0
- In development: M5Stack Core2 v1.1
- In development: M5Stack CoreS3
- M5 GPS modules: GNSS NEO-M9N (38400bps), older GPS U-BLOX NEO-M8N (9600bps)

## Deprecated Hardware

- INA3221A voltage meter

## No Longer Supported

- M5STACK CORE1
- LILYGO TTGO T4 v1.3
- SIM800L GPRS module

**Note:** This app works only with electric vehicles.

**Fully supported cars:**
- Hyundai Ioniq5/6
- Kia EV6

**Community supported:**
- Kia e-Niro
- Hyundai Kona EV
- Kia Niro PHEV
- Renault ZOE 28
- BMW i3
- VW ID3 45/58/77

See release notes for more details and quick installation via the flash tool below. When installed from source, OTA updates over Wi-Fi are also available.

## Required Hardware

### Board

- M5STACK CORE2 IOT Development Kit (K010)  
  [M5Stack Core2 Product Link](https://shop.m5stack.com/products/m5stack-core2-esp32-iot-development-kit)

### CAN vs OBD2 Adapter

- Optional CAN module COMMU (M011) - RS485, TTL, and CAN  
  [COMMU Module Product Link](https://shop.m5stack.com/products/commu-module)

- OBD2 connector can provide power to the M5Stack with a 12V to 5V converter (e.g., Recom R-785.0-1.0).

- Supported OBD2 adapters: Vgate iCar Pro Bluetooth 4.0 (BLE4).  
  For other BLE adapters, please provide the 3 UUIDs (service/notify, read/write).

- For continuous use, we strongly recommend using a direct CAN connection via the OBD2 connector due to security concerns.

### GPS Module

- GNSS Module with Barometric Pressure, IMU, Magnetometer Sensors (NEO-M9N, BMP280, BMI270, BMM150)  
  [GNSS Module Product Link](https://shop.m5stack.com/products/gnss-module-with-barometric-pressure-imu-magnetometer-sensors)

- Older module (M003) - NEO-M8N (with external antenna)  
  [Older GPS Module Product Link](https://shop.m5stack.com/products/gps-module)

## Hardware Configuration

The M5 Core 2 uses UART0 for serial communication and flashing (USB port).

- The COMMU module is wired with SMD jumpers to use UART0 for TTL and UART2 for RS485. CAN does not require UART. Both UART0 and UART2 can be unwired if necessary.

- The GPS module is wired with SMD jumpers to use UART2. This can be easily switched to UART0 for stacking with the GSM module. However, this configuration conflicts with the USB connection of Core2, making flashing impossible. If only stacked with COMMU, it can remain on UART2, but COMMU must be unwired from UART2.

Check the documentation of the modules for more details:  
- [Core2 Documentation](https://docs.m5stack.com/en/core/core2)  
- [GPS Module Documentation](https://docs.m5stack.com/en/module/gps)  
- [COMMU Module Documentation](https://docs.m5stack.com/en/module/commu)

## Quick Installation with ESP32 Flash Tool

See [INSTALLATION.md](INSTALLATION.md) for instructions.

## Installation from Sources (VS Code)

See [INSTALLATION.md](INSTALLATION.md) for detailed steps.

## Release Notes

Check the [RELEASENOTES.md](RELEASENOTES.md) file for the latest updates.

## Screens and Shortcuts

### Touch Screen Zones

![Touch Zones](https://github.com/nickn17/evDash/blob/master/docs/core2_touch_zones.jpg)

- Middle button: Open menu
- Left button: Toggle screens

### Touch Screen Actions

- Left 1/3 of screen: Toggle screen left
- Right 1/3 of screen: Toggle screen right
- Middle 1/3 of screen: Open menu

### In the Menu

- Top left corner (64x64px): Exit menu
- Top right corner (64x64px): Page up
- Bottom right corner (64x64px): Page down
- Rest of the screen: Select item

### Screen List

1. Blank screen (LCD off)
2. Automatic mode (summary info / speed km/h / charging graph)
3. Summary info
4. Speed (km/h) + kWh/100km, charging data (V/A/kW)
5. Battery cells + battery module temperatures
6. Charging graph
7. Consumption table (used to measure available battery capacity)
8. Debug info

![Ioniq 6 Screenshot](https://github.com/nickn17/evDash/blob/master/screenshots/v2_ioniq6.png)

## Top Info Indicators

- Yellow icon: Upload data
- Circle (outer): GPS status
- Circle (inner): Queue loop (flashing)
- Lines under it: Headlights status (currently not working on eGMP vehicles)

![Top Info Indicators](https://github.com/nickn17/evDash/assets/7864168/0a936f1a-fd46-49fd-926b-9716ca7ba007)