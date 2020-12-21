#pragma once

#include "LiveData.h"
#include "CommInterface.h"
#include <mcp_can.h>

#include <memory>

class CommObd2Can : public CommInterface {

  protected:
    const uint8_t pinCanInt = 15;
    const uint8_t pinCanCs = 12;
	std::unique_ptr <MCP_CAN> CAN;
    long unsigned int rxId;
    unsigned char rxLen = 0;
    uint8_t rxBuf[32];
    int16_t rxRemaining; // Remaining bytes to complete message
    char msgString[128];                        // Array to store serial string
    uint16_t lastPid;
    unsigned long lastDataSent = 0;
  public:
    void connectDevice() override;
    void disconnectDevice() override;
    void scanDevices() override;
    void mainLoop() override;
    void executeCommand(String cmd) override;
    //
    void sendPID(const uint16_t pid, const String& cmd);
    void sendFlowControlFrame();
    uint8_t receivePID();
    bool processFrame();
};
