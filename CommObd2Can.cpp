#ifndef COMMINTERFACE_CPP
#define COMMINTERFACE_CPP

#include "CommObd2Can.h"
#include "BoardInterface.h"
#include "LiveData.h"

/**
   Connect CAN adapter
*/
void CommObd2Can::connectDevice() {

  line = "";
 
  Serial.println("CAN connectDevice");
}

/**
   Disconnect device
*/
void CommObd2Can::disconnectDevice() {

  Serial.println("COMM disconnectDevice");
}

/**
   Scan device list, from menu
*/
void CommObd2Can::scanDevices() {
  
  Serial.println("COMM scanDevices");
}

/**
 * Main loop
 */
void CommObd2Can::mainLoop() {
  
}

#endif // COMMINTERFACE_CPP
