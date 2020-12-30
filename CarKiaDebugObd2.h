#pragma once

#include "CarInterface.h"

class CarKiaDebugObd2 : public CarInterface {
  
  protected:
    
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    void loadTestData() override;  
};
