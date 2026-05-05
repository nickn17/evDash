#pragma once

#include "CarInterface.h"

class CarVWUpMii : public CarInterface
{

protected:
public:
  void activateCommandQueue() override;
  void parseRowMerged() override;
  void loadTestData() override;
};
