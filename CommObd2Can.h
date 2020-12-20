#pragma once

#include "LiveData.h"
#include "CommInterface.h"
#include <mcp_can.h>

class CommObd2Can : public CommInterface {

  protected:
    byte pinCanInt = 15;
    byte pinCanCs = 12;
    MCP_CAN* CAN;
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char rxBuf[512];
    char msgString[128];                        // Array to store serial string
  public:
    void connectDevice() override;
    void disconnectDevice() override;
    void scanDevices() override;
    void mainLoop() override;
    void executeCommand(String cmd) override;
    //
    void sendPID(uint16_t pid, String cmd);
    void sendFlowControlFrame();
    /* void startBleScan();
      bool connectToServer(BLEAddress pAddress);
      bool doNextAtCommand();
      bool parseRow();
      bool parseRowMerged(); */
};
