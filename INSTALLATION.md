# INSTALLATION

## Quick installation from binary files with ESP32 flash tool

M5STACK (Many thanks to DimZen)

https://docs.google.com/document/d/17vJmeveNfN0exQy9wKC-5igU8zzNjsuOn1DPuPV_yJA/edit?usp=sharing

TTGO-T4 (older guide)

https://docs.google.com/document/d/1nEezrtXY-8X6mQ1hiZVWDjBVse1sXQg1SlnizaRmJwU/edit?usp=sharing

## Installation from sources
- install arduino IDE + ESP32 support

Required libraries

- ArduinoJson
- TFT_eSPI
- ESP32_AnalogWrite
- esp32-micro-sdcard (arduino-cli)
- TinyGPSPlus (m5stack GPS)

Configure TFT eSPI

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
