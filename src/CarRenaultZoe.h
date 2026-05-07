#pragma once

#include "CarInterface.h"

class CarRenaultZoe : public CarInterface
{

protected:
public:
  void activateCommandQueue() override;
  void parseRowMerged() override;
  void loadTestData() override;

private:
  bool zoeHasBmsEnergy = false;
  bool inRange(float value, float min, float max);
  String normalizedResponse();
  bool isTextResponse(const String &response);
  String decodeAscii(const String &response, uint16_t from);
  int canzeBits(const String &response, uint16_t start, uint16_t end);
  float zoeTempFromBits(const String &response, uint16_t start, uint16_t end, float scale = 1.0f);
  float selectedPackKWh();
  void updateZoePackSize();
  void updateZoeCells(uint16_t offset, uint16_t count);
  void updateZoeCellMinMax();
  void updateZoeModuleMinMax(uint16_t count);
  void applyZoeGear(uint8_t gear);
  void setTirePressureFromRaw(float &target, int raw);
  void updateChargingStateFromPower();
  void parseZoePhase2(const String &response);
};
