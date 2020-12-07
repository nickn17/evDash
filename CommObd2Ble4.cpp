#ifndef COMMOBD2BLE4_CPP
#define COMMOBD2BLE4_CPP

#include <BLEDevice.h>
#include "CommObd2Ble4.h"
#include "LiveData.h"

/**
 * Connect ble4 adapter
 */
void CommObd2Ble4::connectDevice() {
  Serial.println("COMM connectDevice");
}

/** 
 * Disconnect device
 */
void CommObd2Ble4::disconnectDevice() {
  Serial.println("COMM disconnectDevice");
}

/**
 * Scan device list
 */
void CommObd2Ble4::scanDevices() {
  Serial.println("COMM scanDevices");
}

#endif // COMMOBD2BLE4_CPP
