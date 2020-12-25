#pragma once

#include "LiveData.h"
#include "CommInterface.h"
#include <mcp_can.h>

#include <memory>
#include <vector>
#include <unordered_map>

class CommObd2Can : public CommInterface {

  protected:
    const uint8_t pinCanInt = 15;
    const uint8_t pinCanCs = 12;
  	std::unique_ptr <MCP_CAN> CAN;
    long unsigned int rxId;
    unsigned char rxLen = 0;
    uint8_t rxBuf[32];
    int16_t rxRemaining; // Remaining bytes to complete message, signed is ok
    uint8_t requestFramesCount;
    char msgString[128];                        // Array to store serial string
    uint16_t lastPid;
    unsigned long lastDataSent = 0;
    uint8_t txStartChar = 0; // set when sending PID, used when sending FlowControl
    
    std::vector<uint8_t> mergedData;
    std::unordered_map<uint16_t, std::vector<uint8_t>> dataRows;
    
    enum class enFrame_t
    {
      single = 0,
      first = 1,
      consecutive = 2,
      unknown = 9
    };
    
  public:
    void connectDevice() override;
    void disconnectDevice() override;
    void scanDevices() override;
    void mainLoop() override;
    void executeCommand(String cmd) override;
  
  private:
    void sendPID(const uint16_t pid, const String& cmd);
    void sendFlowControlFrame();
    uint8_t receivePID();
    enFrame_t getFrameType(const uint8_t firstByte);
    bool processFrameBytes();
    bool processFrame();
    void processMergedResponse();
  
};
