#pragma once

#include "CarInterface.h"

class CarHyundaiIoniqPHEV : public CarInterface {
  
  protected:
    time_t lastAllowTpms;
    time_t lastAllowOdo;
    time_t lastAllowAircon;
    time_t lastAllowDriveMode;
    void ioniqPhevCarControl(const uint16_t pid, const String& cmd);
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    bool commandAllowed() override;
    void loadTestData() override;   
    std::vector<String> customMenu(int16_t menuId) override;
    void carCommand(const String& cmd) override;    
};

 