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
    unsigned char rxLen = 0;
    unsigned char rxBuf[32];
    int16_t rxRemaining; // Remaining bytes to complete message
    char msgString[128];                        // Array to store serial string
    uint16_t lastPid;
  public:
    void connectDevice() override;
    void disconnectDevice() override;
    void scanDevices() override;
    void mainLoop() override;
    void executeCommand(String cmd) override;
    //
    void sendPID(uint16_t pid, String cmd);
    void sendFlowControlFrame();
    byte receivePID();
    bool processFrame();
};
