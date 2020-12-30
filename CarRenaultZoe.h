#pragma once

#include "CarInterface.h"

class CarRenaultZoe : public CarInterface {
  
  protected:
    
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    void loadTestData() override;  
};
