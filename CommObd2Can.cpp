#include "CommObd2Can.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include <mcp_can.h>

/**
   Connect CAN adapter
*/
void CommObd2Can::connectDevice() {

  Serial.println("CAN connectDevice");

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  /*  if (CAN.begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
      Serial.println("MCP2515 Initialized Successfully!");
      board->displayMessage(" > CAN init OK", "");
    } else {
      Serial.println("Error Initializing MCP2515...");
      board->displayMessage(" > CAN init failed", "");
      return;
    }

    CAN.setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.
    pinMode(pinCanInt, INPUT);                    // Configuring pin for /INT input

          // Serve first command (ATZ)
        doNextQueueCommand();
  */
  Serial.println("init_can() done");
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
   Main loop
*/
void CommObd2Can::mainLoop() {

  CommInterface::mainLoop();
}

/**
   Send command to CAN bus
*/
void CommObd2Can::executeCommand(String cmd) {

  /**
       Serial.println("BMS 220101");
    sendPID(0x00, 0x00);
    delay(delayTxRx);
    byte b = receivePID(0);
    if (b == 0x10) {
    Serial.println("CONTINUE");
    sendPID(0x01, 0x00);
    delay(10);
    for (byte i = 0; i < 20; i++) {
      receivePID(0);
      delay(10);
    }
    }*/
}
