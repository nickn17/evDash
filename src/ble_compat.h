#pragma once

// BLE stack selection.
//
// CoreS3 (ESP32-S3) builds with -DEVDASH_USE_NIMBLE and uses NimBLE-Arduino. The
// prebuilt Bluedroid host on the S3 has only a 4 KB BTU task stack, which the GATT
// service-discovery path overflows right after connect (Guru Meditation "Stack
// canary watchpoint triggered (BTU_TASK)", GitHub issue #159). NimBLE compiles
// from source so its host stack is configurable and the footprint is far smaller.
//
// Core2 (plain ESP32) keeps Bluedroid: it has an 8 KB BTU stack and never hits
// #159, and adding NimBLE on top overflows its IRAM. The two stacks cannot coexist
// in one binary, so the include and the few API-divergent call sites are switched
// on EVDASH_USE_NIMBLE.

#ifdef EVDASH_USE_NIMBLE
#include <NimBLEDevice.h>
#else
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#endif
