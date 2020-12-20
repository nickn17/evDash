#pragma once

#include "CarInterface.h"

class CarKiaEniro : public CarInterface {
  
  protected:
    
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    void loadTestData() override;  
};
