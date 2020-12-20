#pragma once

#include "LiveData.h"
#include "CommInterface.h"
#include <mcp_can.h>

class CommObd2Can : public CommInterface {

  protected:
    byte pinCanInt = 15;
    //MCP_CAN CAN(12); // Set CS to pin 10
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char rxBuf[512];
    char msgString[128];                        // Array to store serial string
  public:
    void connectDevice() override;
    void disconnectDevice() override;
    void scanDevices() override;
    void mainLoop() override;
    //
    /* void startBleScan();
      bool connectToServer(BLEAddress pAddress);
      bool doNextAtCommand();
      bool parseRow();
      bool parseRowMerged(); */
};
