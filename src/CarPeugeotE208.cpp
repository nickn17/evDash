#include <Arduino.h>
#include <stdint.h>
#include <WString.h>
#include <string.h>
#include <sys/time.h>
#include "LiveData.h"
#include "CarPeugeotE208.h"
#include <vector>

#define commandQueueLoopFromPeugeotE208 11

namespace
{
  const uint8_t kPsaCellCount = 108;
  const uint8_t kPsaModuleTempCount = 24; // M5 LiveData stores max 25 module temps.
  const time_t kParkDebounceSec = 120;    // stand still this long (speed <= 1) before leaving drive mode

  bool inRange(float value, float min, float max)
  {
    return value >= min && value <= max;
  }

  String didFromRequest(String request)
  {
    request.replace(" ", "");
    request.toUpperCase();
    if (request.startsWith("22") && request.length() >= 6)
      return request.substring(2, 6);
    return "";
  }

  double hexToDecPart(const String &response, uint16_t from, uint16_t to, uint8_t bytes, bool signedNum)
  {
    if (bytes < 1 || bytes > 4 || to > response.length() || from >= to)
      return -1;

    uint64_t value = strtoull(response.substring(from, to).c_str(), NULL, 16);
    if (signedNum)
    {
      const uint64_t range = ((uint64_t)1) << (bytes * 8);
      if (value > (range - 1) / 2)
        return (double)((int64_t)value - (int64_t)range);
    }
    return (double)value;
  }

  void updateAuxPercent(LiveData *liveData, float voltage)
  {
    const float minV = liveData->params.ignitionOn ? 12.8 : 11.6;
    const float maxV = liveData->params.ignitionOn ? 14.8 : 12.8;
    const float pct = (voltage - minV) * 100.0 / (maxV - minV);
    liveData->params.auxPerc = (pct > 100) ? 100 : ((pct < 0) ? 0 : pct);
  }

  void updateCellMinMax(LiveData *liveData)
  {
    bool found = false;
    float minV = 9;
    float maxV = 0;
    float sumV = 0;
    uint8_t minNo = 255;
    uint8_t maxNo = 255;
    uint8_t validCount = 0;

    for (uint8_t i = 0; i < kPsaCellCount; i++)
    {
      const float value = liveData->params.cellVoltage[i];
      if (!inRange(value, 2.5, 4.4))
        continue;
      sumV += value;
      validCount++;
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

    liveData->params.cellCount = kPsaCellCount;
    liveData->params.batCellMinV = minV;
    liveData->params.batCellMaxV = maxV;
    liveData->params.batCellMinVNo = minNo;
    liveData->params.batCellMaxVNo = maxNo;
    if (validCount >= kPsaCellCount - 4 && inRange(sumV, 200, 470))
      liveData->params.batVoltage = sumV;
  }

  void updateModuleTempMinMax(LiveData *liveData)
  {
    bool found = false;
    float minC = 100;
    float maxC = -100;

    for (uint8_t i = 0; i < kPsaModuleTempCount; i++)
    {
      const float value = liveData->params.batModuleTempC[i];
      if (!inRange(value, -40, 90))
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

  void updateChargeGraph(LiveData *liveData)
  {
    if (liveData->params.speedKmh >= 10 || liveData->params.batPowerKw < 1 || !inRange(liveData->params.socPerc, 0, 100))
      return;

    const uint8_t socIndex = static_cast<uint8_t>(liveData->params.socPerc);
    if (liveData->params.chargingGraphMinKw[socIndex] < 0 || liveData->params.batPowerKw < liveData->params.chargingGraphMinKw[socIndex])
      liveData->params.chargingGraphMinKw[socIndex] = liveData->params.batPowerKw;
    if (liveData->params.chargingGraphMaxKw[socIndex] < 0 || liveData->params.batPowerKw > liveData->params.chargingGraphMaxKw[socIndex])
      liveData->params.chargingGraphMaxKw[socIndex] = liveData->params.batPowerKw;
    liveData->params.chargingGraphBatMinTempC[socIndex] = liveData->params.batMinC;
    liveData->params.chargingGraphBatMaxTempC[socIndex] = liveData->params.batMaxC;
  }

  void applyIgnitionState(LiveData *liveData)
  {
    liveData->params.ignitionOn = liveData->params.forwardDriveMode ||
                                  liveData->params.reverseDriveMode ||
                                  liveData->params.chargingOn ||
                                  (liveData->params.speedKmh > 1);
    liveData->params.parkModeOrNeutral = !liveData->params.forwardDriveMode &&
                                         !liveData->params.reverseDriveMode &&
                                         !liveData->params.chargingOn;
    if (liveData->params.ignitionOn)
      liveData->params.lastIgnitionOnTime = liveData->params.currentTime;
  }

  void decodeMode09Vin(LiveData *liveData, const String &response)
  {
    if (liveData->params.carVin[0] != 0)
      return;

    int start = response.indexOf("4902");
    if (start < 0)
      return;

    start += 4;
    if (response.length() >= (uint16_t)(start + 2) && response.substring(start, start + 2) == "01")
      start += 2;

    char vin[18] = {0};
    uint8_t vinLen = 0;
    for (uint16_t i = start; i + 1 < response.length() && vinLen < 17; i += 2)
    {
      const char c = static_cast<char>(hexToDecPart(response, i, i + 2, 1, false));
      if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
        vin[vinLen++] = c;
    }

    if (vinLen == 17)
    {
      strncpy(liveData->params.carVin, vin, sizeof(liveData->params.carVin) - 1);
      liveData->params.carVin[sizeof(liveData->params.carVin) - 1] = '\0';
    }
  }
} // namespace

/**
   activateCommandQueue
*/
void CarPeugeotE208::activateCommandQueue()
{
  std::vector<String> commandQueuePeugeotE208 = {
      "AT Z",    // Reset all
      "AT I",    // Print the version ID
      "AT S0",   // Printing of spaces off
      "AT E0",   // Echo off
      "AT L0",   // Linefeeds off
      "AT SP 6", // ISO 15765-4 CAN (11 bit ID, 500 kbit/s)
      "AT DP",
      "AT ST16",
      "ATCAF1",
      "ATAL",
      "ATFCSM1",

      // Loop from (PSA e-CMP)
      "ATSH7DF",
      "ATCRA7E8",
      "0902", // VIN, e-208 exposes it on OBD-II mode 09 broadcast.

      "ATSH6A2",
      "ATFCSH6A2",
      "ATCRA682",
      "22D4021", // speed
      "22D8EF1", // outdoor temperature
      "22D8CF1", // front motor RPM

      "ATSH6B4",
      "ATFCSH6B4",
      "ATCRA694",
      "22D4101", // SOC
      "22D8161", // HV current
      "22D8151", // pack voltage
      "22D86F1", // cell min V
      "22D8701", // cell max V
      "22D43C1", // lowest cell index
      "22D8601", // SOH avg fallback
      "22D4261", // SOH weakest block
      "22D8651", // remaining usable energy
      "22D8221", // 12V aux
      "22D8711", // max charge / regen power
      "22D8731", // max discharge power
      "22D87B1", // HV isolation positive
      "22D87C1", // HV isolation negative
      "22D87D1", // pack min temp
      "22D8771", // pack avg temp
      "22D8781", // pack temp delta
      "22D4401", // 108 cell voltages
      "22D4421", // module temperatures

      "ATSH590",
      "ATFCSH590",
      "ATCRA58F",
      "22D8501", // charge active
  };

  liveData->params.batModuleTempCount = kPsaModuleTempCount;
  liveData->params.batteryTotalAvailableKWh = 46.0;
  liveData->params.batMaxEnergyContent = 50.0;
  liveData->params.cellCount = kPsaCellCount;
  liveData->rxTimeoutMs = 1200;
  liveData->delayBetweenCommandsMs = 20;

  liveData->commandQueue.clear();
  for (auto cmd : commandQueuePeugeotE208)
  {
    liveData->commandQueue.push_back({0, cmd});
  }

  liveData->commandQueueLoopFrom = commandQueueLoopFromPeugeotE208;
  liveData->commandQueueCount = commandQueuePeugeotE208.size();
}

/**
   Is command from queue allowed for execute, or continue with next
*/
bool CarPeugeotE208::commandAllowed()
{
  if (liveData->commandRequest.equals("0902") && liveData->params.carVin[0] != 0)
    return false;

  return true;
}

/**
   parseRowMerged
*/
void CarPeugeotE208::parseRowMerged()
{
  String response = liveData->responseRowMerged;
  response.trim();
  response.toUpperCase();
  if (response.length() == 0 ||
      response.indexOf("NO DATA") >= 0 ||
      response.indexOf("ERROR") >= 0 ||
      response.indexOf("UNABLE") >= 0 ||
      response.startsWith("7F"))
  {
    return;
  }

  if (liveData->commandRequest.equals("0902"))
  {
    decodeMode09Vin(liveData, response);
    return;
  }

  const String did = didFromRequest(liveData->commandRequest);
  if (did.length() != 4 || !response.startsWith(String("62") + did))
    return;

  // VCU - request 0x6A2, response 0x682.
  if (liveData->currentAtshRequest.equals("ATSH6A2"))
  {
    if (did == "D402")
    {
      float speed = hexToDecPart(response, 6, 10, 2, false) * 0.00794;
      if (speed > 10)
        speed += liveData->settings.speedCorrection;
      if (!inRange(speed, 0, 220))
        speed = 0;
      liveData->params.speedKmh = speed;
      // Drive-mode latch with a stationary debounce. Enter drive mode immediately on
      // movement, but leave it only after the car has stood still (speed <= 1) for
      // kParkDebounceSec, so a traffic-light stop does not bounce out of drive while a
      // genuine park still releases the latch. Without this clear, forwardDriveMode stayed
      // true forever, ignitionOn never went false and the Sentry auto-stop never armed.
      static time_t lastMovingTime = 0;
      if (speed > 1)
      {
        lastMovingTime = liveData->params.currentTime;
        liveData->params.forwardDriveMode = true;
      }
      else if (liveData->params.forwardDriveMode && lastMovingTime != 0 &&
               liveData->params.currentTime - lastMovingTime >= kParkDebounceSec)
      {
        liveData->params.forwardDriveMode = false;
      }
      applyIgnitionState(liveData);
      return;
    }

    if (did == "D8EF")
    {
      const float temp = hexToDecPart(response, 6, 8, 1, false);
      if (inRange(temp, -40, 60))
        liveData->params.outdoorTemperature = temp;
      return;
    }

    if (did == "D8CF")
    {
      const float rpm = hexToDecPart(response, 6, 10, 2, false);
      if (inRange(rpm, 0, 18000))
        liveData->params.motor1Rpm = rpm;
      return;
    }
  }

  // TBMU/BMS - request 0x6B4, response 0x694.
  if (liveData->currentAtshRequest.equals("ATSH6B4"))
  {
    if (did == "D410")
    {
      const float soc = hexToDecPart(response, 6, 10, 2, false) / 512.0;
      if (inRange(soc, 0, 100))
      {
        liveData->params.socPercPrevious = liveData->params.socPerc;
        liveData->params.socPerc = soc;
        liveData->params.socPercBms = soc;
        if (!inRange(liveData->params.batEnergyContent, 0, 60))
          liveData->params.batEnergyContent = liveData->params.batteryTotalAvailableKWh * soc / 100.0;
      }
      return;
    }

    if (did == "D816")
    {
      const float raw = hexToDecPart(response, 6, 14, 4, false);
      const float amp = (76800.0 - raw) * 0.018; // M5 convention: charge/regen positive, drive negative.
      if (!inRange(amp, -500, 500))
        return;

      liveData->params.batPowerAmp = amp;
      if (inRange(liveData->params.batVoltage, 200, 470))
      {
        liveData->params.batPowerKw = liveData->params.batPowerAmp * liveData->params.batVoltage / 1000.0;
        if (liveData->params.speedKmh > 5)
          liveData->params.batPowerKwh100 = liveData->params.batPowerKw / liveData->params.speedKmh * 100.0;
        if (liveData->params.speedKmh < 5 && liveData->params.batPowerKw > 0.5)
        {
          liveData->params.chargingOn = true;
          liveData->params.lastChargingOnTime = liveData->params.currentTime;
        }
        updateChargeGraph(liveData);
      }
      applyIgnitionState(liveData);
      return;
    }

    if (did == "D815")
    {
      const float voltage = hexToDecPart(response, 6, 10, 2, false) / 16.0;
      if (inRange(voltage, 200, 470))
        liveData->params.batVoltage = voltage;
      return;
    }

    if (did == "D86F" || did == "D870")
    {
      const float voltage = hexToDecPart(response, 6, 10, 2, false) / 1000.0;
      if (!inRange(voltage, 2.5, 4.4))
        return;
      if (did == "D86F")
        liveData->params.batCellMinV = voltage;
      else
        liveData->params.batCellMaxV = voltage;
      return;
    }

    if (did == "D43C")
    {
      const uint8_t cellNo = static_cast<uint8_t>(hexToDecPart(response, 6, 8, 1, false)) + 1;
      if (cellNo >= 1 && cellNo <= kPsaCellCount)
        liveData->params.batCellMinVNo = cellNo;
      return;
    }

    if (did == "D860")
    {
      const float soh = hexToDecPart(response, 8, 12, 2, false) / 16.0;
      if (inRange(soh, 0, 100) && !inRange(liveData->params.sohPerc, 0, 100))
        liveData->params.sohPerc = soh;
      return;
    }

    if (did == "D426")
    {
      bool found = false;
      float minSoh = 100;
      for (uint16_t from = 6; from + 4 <= response.length(); from += 4)
      {
        float soh = hexToDecPart(response, from, from + 4, 2, false) / 16.0;
        if (soh > 100 && soh <= 110)
          soh = 100;
        if (inRange(soh, 50, 100) && (!found || soh < minSoh))
        {
          minSoh = soh;
          found = true;
        }
      }
      if (found)
        liveData->params.sohPerc = minSoh;
      return;
    }

    if (did == "D865")
    {
      const float kWh = hexToDecPart(response, 6, 10, 2, false) / 64.0;
      if (inRange(kWh, 0, 60))
        liveData->params.batEnergyContent = kWh;
      return;
    }

    if (did == "D822")
    {
      const float voltage = hexToDecPart(response, 6, 10, 2, false) / 1000.0;
      if (inRange(voltage, 8, 16))
      {
        liveData->params.auxVoltage = voltage;
        updateAuxPercent(liveData, voltage);
      }
      return;
    }

    if (did == "D871" || did == "D873")
    {
      const float kW = hexToDecPart(response, 6, 14, 4, false) / 1000.0;
      if (!inRange(kW, 0, 600))
        return;
      if (did == "D871")
        liveData->params.availableChargePower = kW;
      else
        liveData->params.availableDischargePower = kW;
      return;
    }

    if (did == "D87B" || did == "D87C")
    {
      const float kOhm = hexToDecPart(response, 6, 14, 4, false);
      if (!inRange(kOhm, 0, 100000))
        return;
      if (did == "D87B" || !inRange(liveData->params.isolationResistanceKOhm, 0, 100000))
        liveData->params.isolationResistanceKOhm = kOhm;
      else if (kOhm < liveData->params.isolationResistanceKOhm)
        liveData->params.isolationResistanceKOhm = kOhm;
      return;
    }

    if (did == "D87D")
    {
      const float temp = hexToDecPart(response, 6, 8, 1, false) - 40.0;
      if (inRange(temp, -40, 90))
      {
        liveData->params.batMinC = temp;
        if (liveData->params.batMaxC < temp)
          liveData->params.batMaxC = temp;
        liveData->params.batTempC = temp;
      }
      return;
    }

    if (did == "D877")
    {
      const float temp = hexToDecPart(response, 6, 8, 1, false) - 40.0;
      if (inRange(temp, -40, 90))
        liveData->params.batTempC = temp;
      return;
    }

    if (did == "D878")
    {
      const float delta = hexToDecPart(response, 6, 8, 1, false);
      if (inRange(delta, 0, 60) && inRange(liveData->params.batMinC, -40, 90))
        liveData->params.batMaxC = liveData->params.batMinC + delta;
      return;
    }

    if (did == "D440")
    {
      bool changed = false;
      for (uint8_t i = 0; i < kPsaCellCount; i++)
      {
        const uint16_t from = 6 + (i * 4);
        const float voltage = hexToDecPart(response, from, from + 4, 2, false) / 1000.0;
        if (inRange(voltage, 2.5, 4.4))
        {
          liveData->params.cellVoltage[i] = voltage;
          changed = true;
        }
      }
      if (changed)
        updateCellMinMax(liveData);
      return;
    }

    if (did == "D442")
    {
      bool changed = false;
      for (uint8_t i = 0; i < kPsaModuleTempCount; i++)
      {
        const uint16_t from = 6 + (i * 2);
        const float temp = hexToDecPart(response, from, from + 2, 1, false) - 40.0;
        if (inRange(temp, -40, 90))
        {
          liveData->params.batModuleTempC[i] = temp;
          changed = true;
        }
      }
      if (changed)
        updateModuleTempMinMax(liveData);
      return;
    }
  }

  // OBC/DC-DC charger - request 0x590, response 0x58F.
  if (liveData->currentAtshRequest.equals("ATSH590") && did == "D850")
  {
    const bool charging = hexToDecPart(response, 6, 8, 1, false) > 0;
    liveData->params.chargingOn = charging;
    liveData->params.chargerACconnected = charging;
    liveData->params.chargerDCconnected = false;
    if (charging)
      liveData->params.lastChargingOnTime = liveData->params.currentTime;
    applyIgnitionState(liveData);
  }
}

/**
   loadTestData
*/
void CarPeugeotE208::loadTestData()
{
  auto applyDemoResponse = [&](const char *atsh, const char *command, const String &response)
  {
    liveData->currentAtshRequest = atsh;
    liveData->commandRequest = command;
    liveData->responseRowMerged = response;
    parseRowMerged();
  };

  applyDemoResponse("ATSH6A2", "22D4021", "62D402385E");
  applyDemoResponse("ATSH6A2", "22D8EF1", "62D8EF1E");
  applyDemoResponse("ATSH6A2", "22D8CF1", "62D8CF1814");
  applyDemoResponse("ATSH6B4", "22D4101", "62D410A90C");
  applyDemoResponse("ATSH6B4", "22D8151", "62D8151803");
  applyDemoResponse("ATSH6B4", "22D8161", "62D816000132D0");
  applyDemoResponse("ATSH6B4", "22D86F1", "62D86F0F44");
  applyDemoResponse("ATSH6B4", "22D8701", "62D8700F54");
  applyDemoResponse("ATSH6B4", "22D43C1", "62D43C6A");
  applyDemoResponse("ATSH6B4", "22D8601", "62D860010634");
  applyDemoResponse("ATSH6B4", "22D4261", "62D4260640064005B406400640");
  applyDemoResponse("ATSH6B4", "22D8651", "62D8650916");
  applyDemoResponse("ATSH6B4", "22D8221", "62D8223873");
  applyDemoResponse("ATSH6B4", "22D8711", "62D8710000AA84");
  applyDemoResponse("ATSH6B4", "22D8731", "62D873000148ED");
  applyDemoResponse("ATSH6B4", "22D87D1", "62D87D39");
  applyDemoResponse("ATSH6B4", "22D8771", "62D8773A");
  applyDemoResponse("ATSH6B4", "22D8781", "62D87802");
  applyDemoResponse("ATSH590", "22D8501", "62D85000");

  String cells = "62D440";
  for (uint8_t i = 0; i < kPsaCellCount; i++)
    cells += (i == 106) ? "0F44" : ((i == 18) ? "0F54" : "0F4C");
  applyDemoResponse("ATSH6B4", "22D4401", cells);

  String temps = "62D442";
  for (uint8_t i = 0; i < kPsaModuleTempCount; i++)
    temps += (i % 3 == 0) ? "3B" : ((i % 3 == 1) ? "3C" : "3D");
  applyDemoResponse("ATSH6B4", "22D4421", temps);
}
