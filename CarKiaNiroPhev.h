#pragma once

#include "CarInterface.h"

class CarKiaNiroPhev: public CarInterface {
  
  protected:
    
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    void loadTestData() override;   
};
