#pragma once

#include "CarInterface.h"

class CarVWID3 : public CarInterface
{

protected:
  time_t lastAllowTpms;
  void VWID3CarControl(const uint16_t pid, const String &cmd);

public:
  void activateCommandQueue() override;
  void parseRowMerged() override;
  bool commandAllowed() override;
  void loadTestData() override;
  //
  void testHandler(const String &cmd) override;
  //
  std::vector<String> customMenu(int16_t menuId) override;
  void carCommand(const String &cmd) override;
  //
  float convertLatLonToDecimal(String orig);
};
