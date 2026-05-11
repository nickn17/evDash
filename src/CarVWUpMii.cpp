#include <Arduino.h>
#include <stdint.h>
#include <WString.h>
#include "LiveData.h"
#include "CarVWUpMii.h"
#include <vector>

#define commandQueueLoopFromVWUpMii 6

static String didFromRequest(String request)
{
  request.replace(" ", "");
  request.toUpperCase();
  if (request.startsWith("22") && request.length() >= 6)
    return request.substring(2, 6);
  if (request.startsWith("0322") && request.length() >= 8)
    return request.substring(4, 8);
  return "";
}

static bool inRange(const float value, const float min, const float max)
{
  return value >= min && value <= max;
}

static void addDid(std::vector<String> &queue, uint16_t did)
{
  String didHex = String(did, HEX);
  didHex.toUpperCase();
  while (didHex.length() < 4)
    didHex = "0" + didHex;
  queue.push_back(String("22") + didHex);
}

static void updateCellMinMax(LiveData *liveData)
{
  bool found = false;
  float minV = 9;
  float maxV = 0;
  uint8_t minNo = 255;
  uint8_t maxNo = 255;
  for (uint8_t i = 0; i < 84; i++)
  {
    const float value = liveData->params.cellVoltage[i];
    if (!inRange(value, 2.5, 4.4))
      continue;
    if (!found || value < minV)
    {
      minV = value;
      minNo = i + 1;
    }
    if (!found || value > maxV)
    {
      maxV = value;
      maxNo = i + 1;
    }
    found = true;
  }
  if (!found)
    return;
  liveData->params.cellCount = 84;
  liveData->params.batCellMinV = minV;
  liveData->params.batCellMaxV = maxV;
  liveData->params.batCellMinVNo = minNo;
  liveData->params.batCellMaxVNo = maxNo;
}

static void updateModuleTempMinMax(LiveData *liveData)
{
  bool found = false;
  float minC = 100;
  float maxC = -100;
  for (uint8_t i = 0; i < 14; i++)
  {
    const float value = liveData->params.batModuleTempC[i];
    if (!inRange(value, -30, 90))
      continue;
    if (!found || value < minC)
      minC = value;
    if (!found || value > maxC)
      maxC = value;
    found = true;
  }
  if (!found)
    return;
  liveData->params.batMinC = minC;
  liveData->params.batMaxC = maxC;
  liveData->params.batTempC = maxC;
}

/**
   activateCommandQueue
*/
void CarVWUpMii::activateCommandQueue()
{
  std::vector<String> commandQueueVwUpMii = {
      "ATZ",
      "ATE0",
      "ATL0",
      "ATS0",
      "ATSP6",
      "ATST96",

      // Loop from (VW e-Up / Skoda Citigo-e / Seat Mii)
      "ATCAF1",
      "ATSH7E5",
      "ATCRA7ED",
      "22028C",
      "221E3B",
      "221E3D",
      "227448",
      "221E0E",
      "221E0F",
      "221E33",
      "221E34",
      "222A0B",
      "2274CB",
  };

  if (liveData->settings.commType == COMM_TYPE_OBD2_BLE4)
  {
    const std::vector<String> elmFallback = {
        "ATCAF0",
        "ATSH7E5",
        "ATCRA7ED",
        "0322028C55555555",
        "03221E3B55555555",
        "03221E3D55555555",
        "0322744855555555",
        "03221E0E55555555",
        "03221E0F55555555",
        "03221E3355555555",
        "03221E3455555555",
        "032274CB55555555",
        "ATSH765",
        "ATCRA7CF",
        "03221DD055555555",
        "ATSH7E0",
        "ATCRA7E8",
        "0322F45B55555555",
        "ATCAF1",
    };
    commandQueueVwUpMii.insert(commandQueueVwUpMii.end(), elmFallback.begin(), elmFallback.end());
  }

  const std::vector<String> extraCommands = {
      "ATSH765",
      "ATCRA7CF",
      "221DD0",
      "221DD6",
      "ATSH7E0",
      "ATCRA7E8",
      "22F802",
      "22F40D",
      "22F446",
      "22F45B",
      "ATSH7E6",
      "ATCRA7EE",
      "223E94",
      "22465C",
      "22465B",
      "ATSH714",
      "ATCRA77E",
      "2222E0",
      "2222E4",
      // Switch back to BMS before cell voltage / module temperature DIDs below
      "ATSH7E5",
      "ATCRA7ED",
  };
  commandQueueVwUpMii.insert(commandQueueVwUpMii.end(), extraCommands.begin(), extraCommands.end());

  for (uint8_t vi = 0; vi < 84; vi++)
  {
    const uint16_t pollIndex = (vi % 6) * 14 + (vi / 6);
    addDid(commandQueueVwUpMii, 0x1E40 + pollIndex);
  }
  for (uint8_t index = 0; index < 14; index++)
    addDid(commandQueueVwUpMii, 0x1EAE + index);

  liveData->params.batModuleTempCount = 14;
  liveData->params.batteryTotalAvailableKWh = 36.8;
  liveData->params.cellCount = 84;
  liveData->rxTimeoutMs = 800;
  liveData->delayBetweenCommandsMs = 20;

  liveData->commandQueue.clear();
  for (auto cmd : commandQueueVwUpMii)
  {
    liveData->commandQueue.push_back({0, cmd});
  }

  liveData->commandQueueLoopFrom = commandQueueLoopFromVWUpMii;
  liveData->commandQueueCount = commandQueueVwUpMii.size();
}

/**
   parseRowMerged
*/
void CarVWUpMii::parseRowMerged()
{
  if (liveData->responseRowMerged.equals("NO DATA") || liveData->responseRowMerged.indexOf("ERROR") >= 0)
    return;

  const String did = didFromRequest(liveData->commandRequest);
  if (did.length() != 4 || !liveData->responseRowMerged.startsWith(String("62") + did) || liveData->responseRowMerged.length() < 8)
    return;

  if (did == "028C" || did == "1DD0" || did == "F45B")
  {
    if (did != "028C" && inRange(liveData->params.socPerc, 0, 100))
      return;
    const float raw = liveData->hexToDecFromResponse(6, 8, 1, false);
    const float soc = (did == "1DD0") ? raw / 2.0 : raw / 2.55;
    if (!inRange(soc, 0, 100))
      return;
    liveData->params.socPercPrevious = liveData->params.socPerc;
    liveData->params.socPerc = soc;
    liveData->params.socPercBms = soc;
    liveData->params.batEnergyContent = liveData->params.batteryTotalAvailableKWh * soc / 100.0;
    return;
  }

  if (did == "1E3B")
  {
    const float voltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 4.0;
    if (inRange(voltage, 180, 450))
      liveData->params.batVoltage = voltage;
    return;
  }

  if (did == "1E3D")
  {
    const float raw = liveData->hexToDecFromResponse(6, 10, 2, false);
    const float amp = -1.0 * (raw - 2044.0) / 4.0;
    if (!inRange(amp, -300, 300))
      return;
    liveData->params.batPowerAmp = amp;
    if (inRange(liveData->params.batVoltage, 180, 450))
    {
      liveData->params.batPowerKw = liveData->params.batPowerAmp * liveData->params.batVoltage / 1000.0;
      if (liveData->params.speedKmh > 1)
        liveData->params.batPowerKwh100 = liveData->params.batPowerKw / liveData->params.speedKmh * 100.0;
      if (liveData->params.speedKmh < 10 && liveData->params.batPowerKw >= 1 && inRange(liveData->params.socPerc, 0, 100))
      {
        const uint8_t socIndex = static_cast<uint8_t>(liveData->params.socPerc);
        if (liveData->params.chargingGraphMinKw[socIndex] < 0 || liveData->params.batPowerKw < liveData->params.chargingGraphMinKw[socIndex])
          liveData->params.chargingGraphMinKw[socIndex] = liveData->params.batPowerKw;
        if (liveData->params.chargingGraphMaxKw[socIndex] < 0 || liveData->params.batPowerKw > liveData->params.chargingGraphMaxKw[socIndex])
          liveData->params.chargingGraphMaxKw[socIndex] = liveData->params.batPowerKw;
        liveData->params.chargingGraphBatMinTempC[socIndex] = liveData->params.batMinC;
        liveData->params.chargingGraphBatMaxTempC[socIndex] = liveData->params.batMaxC;
      }
    }
    return;
  }

  if (did == "7448" || did == "1DD6")
  {
    const uint8_t mode = static_cast<uint8_t>(liveData->hexToDecFromResponse(6, 8, 1, false));
    const bool driving = did == "7448" && mode == 1;
    const bool dcCharging = (did == "7448" && mode == 6) || (did == "1DD6" && mode >= 4);
    const bool acCharging = (did == "7448" && mode == 4) || (did == "1DD6" && mode >= 1 && !dcCharging);
    liveData->params.chargerACconnected = acCharging;
    liveData->params.chargerDCconnected = dcCharging;
    liveData->params.chargingOn = acCharging || dcCharging;
    liveData->params.forwardDriveMode = driving || (did == "1DD6" && liveData->params.forwardDriveMode);
    liveData->params.parkModeOrNeutral = !liveData->params.forwardDriveMode && !liveData->params.chargingOn;
    liveData->params.ignitionOn = liveData->params.forwardDriveMode || liveData->params.chargingOn;
    if (liveData->params.chargingOn)
      liveData->params.lastChargingOnTime = liveData->params.currentTime;
    return;
  }

  if (did == "1E0E" || did == "1E0F" || did == "2A0B")
  {
    const float temp = liveData->hexToDecFromResponse(6, 10, 2, true) / 64.0;
    if (!inRange(temp, -30, 90))
      return;
    if (did == "1E0E")
      liveData->params.batMaxC = temp;
    else if (did == "1E0F")
      liveData->params.batMinC = temp;
    else
      liveData->params.batMinC = liveData->params.batMaxC = temp;
    liveData->params.batTempC = liveData->params.batMaxC;
    return;
  }

  if (did == "1E33" || did == "1E34")
  {
    const float voltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 4096.0;
    if (!inRange(voltage, 2.5, 4.4))
      return;
    if (did == "1E33")
      liveData->params.batCellMaxV = voltage;
    else
      liveData->params.batCellMinV = voltage;
    return;
  }

  if (did == "74CB")
  {
    const float soh = liveData->hexToDecFromResponse(6, 10, 2, false) / 120.0;
    if (inRange(soh, 0, 105))
      liveData->params.sohPerc = soh;
    return;
  }

  if (did == "F446")
  {
    const float temp = liveData->hexToDecFromResponse(6, 8, 1, false) - 40.0;
    if (inRange(temp, -40, 80))
      liveData->params.outdoorTemperature = temp;
    return;
  }

  if (did == "3E94")
  {
    const float temp = liveData->hexToDecFromResponse(6, 10, 2, true) / 64.0;
    if (inRange(temp, -30, 180))
      liveData->params.motorTempC = temp;
    return;
  }

  if (did == "465C")
  {
    const float voltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 512.0;
    if (inRange(voltage, 8, 16))
      liveData->params.auxVoltage = voltage;
    return;
  }

  if (did == "465B")
  {
    const float current = liveData->hexToDecFromResponse(6, 10, 2, true) / 16.0;
    if (inRange(current, -200, 200))
      liveData->params.auxCurrentAmp = current;
    return;
  }

  if (did == "22E0" || did == "22E4")
  {
    const float raw = liveData->hexToDecFromResponse(6, 10, 2, false);
    if (did == "22E4" && inRange(raw / 10.0, 1, 100))
      liveData->params.batteryTotalAvailableKWh = raw / 10.0;
    return;
  }

  const uint16_t didInt = liveData->hexToDec(did, 2, false);
  if (didInt >= 0x1E40 && didInt <= 0x1EA5)
  {
    const uint16_t pollIndex = didInt - 0x1E40;
    const uint8_t cellIndex = (pollIndex % 14) * 6 + (pollIndex / 14);
    const float voltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 4096.0;
    if (cellIndex < 84 && inRange(voltage, 2.5, 4.4))
    {
      liveData->params.cellVoltage[cellIndex] = voltage;
      updateCellMinMax(liveData);
    }
    return;
  }

  if (didInt >= 0x1EAE && didInt <= 0x1EBB)
  {
    const uint8_t tempIndex = didInt - 0x1EAE;
    const float temp = liveData->hexToDecFromResponse(6, 10, 2, true) / 64.0;
    if (tempIndex < 14 && inRange(temp, -30, 90))
    {
      liveData->params.batModuleTempC[tempIndex] = temp;
      updateModuleTempMinMax(liveData);
    }
  }
}

/**
   loadTestData
*/
void CarVWUpMii::loadTestData()
{
  liveData->commandRequest = "22028C";
  liveData->responseRowMerged = "62028CD9";
  parseRowMerged();

  liveData->commandRequest = "221E3B";
  liveData->responseRowMerged = "621E3B054E";
  parseRowMerged();

  liveData->commandRequest = "221E3D";
  liveData->responseRowMerged = "621E3D07F0";
  parseRowMerged();

  liveData->commandRequest = "227448";
  liveData->responseRowMerged = "62744801";
  parseRowMerged();

  liveData->commandRequest = "221E0E";
  liveData->responseRowMerged = "621E0E05C40001";
  parseRowMerged();

  liveData->commandRequest = "221E0F";
  liveData->responseRowMerged = "621E0F05920005";
  parseRowMerged();
}
