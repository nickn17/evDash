#pragma once

#include "CarInterface.h"

class CarPeugeotE208 : public CarInterface {
  
  protected:
    
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    void loadTestData() override;  
};
