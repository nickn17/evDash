# INSTALLATION

## Quick installation from binary files with ESP32 flash tool

M5STACK (Many thanks to DimZen)

https://docs.google.com/document/d/17vJmeveNfN0exQy9wKC-5igU8zzNjsuOn1DPuPV_yJA/edit?usp=sharing

TTGO-T4 (older guide)

https://docs.google.com/document/d/1nEezrtXY-8X6mQ1hiZVWDjBVse1sXQg1SlnizaRmJwU/edit?usp=sharing

## Installation from sources
- install VS code + Platformio
- import project to platformio (uncheck import libraries from Arduino, select board: m5stack core)
- install libraries

Required libraries

- ArduinoJson
- TFT_eSPI
- ESP32_AnalogWrite
- esp32-micro-sdcard (arduino-cli)
- TinyGPSPlus (m5stack GPS) https://github.com/mikalhart/TinyGPSPlus
- INA3221 https://github.com/switchdoclabs/SDL_Arduino_INA3221
- CAN library https://github.com/coryjfowler/MCP_CAN_lib

Configure TFT eSPI

w:\Documents\PlatformIO\Projects\210121-183512-m5stack-core-esp32\.pio\libdeps\m5stack-core-esp32\TFT_eSPI\
```  
// Comment
//#include <User_Setup.h>           // Default setup is root library folder
// And uncomment
#include <User_Setups/Setup12_M5Stack.h>           // Setup file for the ESP32 based M5Stack
```  

plaformio.ini
```  
[env:m5stack-core-esp32]
platform = espressif32
board = m5stack-core-esp32
framework = arduino
lib_deps = 
	bodmer/TFT_eSPI@^2.3.58
	mikalhart/TinyGPSPlus@^1.0.2
	erropix/ESP32 AnalogWrite@^0.2.0
	bblanchon/ArduinoJson@^6.17.2

targets = upload
upload_protocol = esptool
upload_port = COM8
upload_speed = 921600
monitor_port = COM8
monitor_speed = 115200
```  

CPU config example
- Board ESP32 Dev module
- Upload speed 921600
- CPU freq: 240MHz (Wifi/BT)
- Flash freq: 80MHz
- Flash mode: QIO
- Flash size 4MB (32mb)
- Partion scheme: default 4MB with spiffs
- Core debug level: none
- PSRAM: disable
