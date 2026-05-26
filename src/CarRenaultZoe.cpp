#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include <WString.h>
#include <sys/time.h>
#include "LiveData.h"
#include "CarRenaultZoe.h"
#include <vector>

#define commandQueueLoopFromRenaultZoe 8

/**
   Checks numeric value range
*/
bool CarRenaultZoe::inRange(float value, float min, float max)
{
  return value >= min && value <= max;
}

/**
   Normalizes ELM/CAN response and removes command echo
*/
String CarRenaultZoe::normalizedResponse()
{
  String response = liveData->responseRowMerged;
  response.replace(" ", "");
  response.replace("\r", "");
  response.replace("\n", "");
  response.replace(">", "");
  response.trim();
  response.toUpperCase();

  String command = liveData->commandRequest;
  command.replace(" ", "");
  command.toUpperCase();
  while (command.length() > 0 && response.startsWith(command) && response.length() > command.length())
    response = response.substring(command.length());

  return response;
}

/**
   Detects text-only adapter responses
*/
bool CarRenaultZoe::isTextResponse(const String &response)
{
  if (response.length() == 0 || response.equals("NODATA") || response.equals("NO DATA") || response.equals("OK") || response.equals("?"))
    return true;
  for (uint16_t i = 0; i < response.length(); i++)
  {
    const char c = response.charAt(i);
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')))
      return true;
  }
  return false;
}

/**
   Decodes ASCII bytes from a hex response
*/
String CarRenaultZoe::decodeAscii(const String &response, uint16_t from)
{
  String out = "";
  for (uint16_t i = from; i + 1 < response.length() && out.length() < 17; i += 2)
  {
    const uint8_t ch = liveData->hexToDec(response.substring(i, i + 2), 1, false);
    if (ch >= 32 && ch <= 126)
      out += char(ch);
  }
  out.trim();
  return out;
}

/**
   Reads CanZE big-endian bit fields from merged response
*/
int CarRenaultZoe::canzeBits(const String &response, uint16_t start, uint16_t end)
{
  if (end < start || ((end / 8) + 1) * 2 > response.length())
    return -1;

  uint32_t value = 0;
  for (uint16_t bit = start; bit <= end; bit++)
  {
    const uint16_t bytePos = (bit / 8) * 2;
    const uint8_t byteValue = liveData->hexToDec(response.substring(bytePos, bytePos + 2), 1, false);
    value = (value << 1) | bitRead(byteValue, 7 - (bit % 8));
  }
  return value;
}

/**
   Reads Renault temperature bit fields
*/
float CarRenaultZoe::zoeTempFromBits(const String &response, uint16_t start, uint16_t end, float scale)
{
  const int raw = canzeBits(response, start, end);
  return raw < 0 ? -100 : (raw * scale) - 40;
}

/**
   Returns selected ZOE usable pack size
*/
float CarRenaultZoe::selectedPackKWh()
{
  if (liveData->settings.carType == CAR_RENAULT_ZOE_ZE40_41)
    return 41;
  if (liveData->settings.carType == CAR_RENAULT_ZOE_ZE50_52)
    return 52;
  return 22;
}

/**
   Applies selected pack size and SOC-derived energy fallback
*/
void CarRenaultZoe::updateZoePackSize()
{
  const float pack = selectedPackKWh();
  liveData->params.batteryTotalAvailableKWh = pack;
  liveData->params.batMaxEnergyContent = pack;
  if (!zoeHasBmsEnergy && inRange(liveData->params.socPerc, 0, 100))
    liveData->params.batEnergyContent = pack * liveData->params.socPerc / 100.0;
}

/**
   Updates cell voltages from legacy LBC frames
*/
void CarRenaultZoe::updateZoeCells(uint16_t offset, uint16_t count)
{
  if (liveData->responseRowMerged.length() < 4 + (count * 4))
    return;

  for (uint16_t i = 0; i < count && offset + i < 200; i++)
  {
    const uint16_t from = 4 + (i * 4);
    const float voltage = liveData->hexToDecFromResponse(from, from + 4, 2, false) / 1000.0;
    if (inRange(voltage, 2.5, 4.5))
      liveData->params.cellVoltage[offset + i] = voltage;
  }
  liveData->params.cellCount = 96;
  updateZoeCellMinMax();
}

/**
   Updates cell min/max from available cell array
*/
void CarRenaultZoe::updateZoeCellMinMax()
{
  float minV = 9;
  float maxV = 0;
  uint8_t minNo = 255;
  uint8_t maxNo = 255;
  const uint16_t count = liveData->params.cellCount > 0 ? liveData->params.cellCount : 96;

  for (uint16_t i = 0; i < count && i < 200; i++)
  {
    const float voltage = liveData->params.cellVoltage[i];
    if (!inRange(voltage, 2.5, 4.5))
      continue;
    if (voltage < minV)
    {
      minV = voltage;
      minNo = i + 1;
    }
    if (voltage > maxV)
    {
      maxV = voltage;
      maxNo = i + 1;
    }
  }

  if (minNo != 255)
  {
    liveData->params.batCellMinV = minV;
    liveData->params.batCellMinVNo = minNo;
  }
  if (maxNo != 255)
  {
    liveData->params.batCellMaxV = maxV;
    liveData->params.batCellMaxVNo = maxNo;
  }
}

/**
   Updates module temperature min/max from parsed module array
*/
void CarRenaultZoe::updateZoeModuleMinMax(uint16_t count)
{
  float minC = 100;
  float maxC = -100;
  bool found = false;
  for (uint16_t i = 0; i < count && i < 25; i++)
  {
    const float temp = liveData->params.batModuleTempC[i];
    if (!inRange(temp, -40, 90))
      continue;
    if (!found || temp < minC)
      minC = temp;
    if (!found || temp > maxC)
      maxC = temp;
    found = true;
  }
  if (found)
  {
    liveData->params.batMinC = minC;
    liveData->params.batMaxC = maxC;
    liveData->params.batTempC = minC;
  }
}

/**
   Applies Renault gear value
*/
void CarRenaultZoe::applyZoeGear(uint8_t gear)
{
  if (gear < 1 || gear > 4)
    return;

  liveData->params.forwardDriveMode = (gear == 4);
  liveData->params.reverseDriveMode = (gear == 2);
  liveData->params.parkModeOrNeutral = (gear == 1 || gear == 3);

  const bool driving = liveData->params.forwardDriveMode || liveData->params.reverseDriveMode;
  liveData->params.ignitionOn = driving || liveData->params.chargingOn || (liveData->params.speedKmh > 1);
  if (liveData->params.ignitionOn)
    liveData->params.lastIgnitionOnTime = liveData->params.currentTime;
}

/**
   Converts CanZE TPMS raw pressure to bar
*/
void CarRenaultZoe::setTirePressureFromRaw(float &target, int raw)
{
  if (raw <= 0 || raw >= 255)
    return;
  const float pressure = raw * 13.725 / 1000.0;
  if (inRange(pressure, 0.5, 4.5))
    target = pressure;
}

/**
   Updates charging status from pack power
*/
void CarRenaultZoe::updateChargingStateFromPower()
{
  if (liveData->params.batPowerKw > 0.5)
  {
    liveData->params.chargingOn = true;
    liveData->params.lastChargingOnTime = liveData->params.currentTime;
  }
  else if (liveData->params.batPowerKw < -0.5)
  {
    liveData->params.chargingOn = false;
    liveData->params.chargerACconnected = false;
    liveData->params.chargerDCconnected = false;
  }
}

/**
   Parses ZOE Phase2 / Z.E.50 CanZE PIDs
*/
void CarRenaultZoe::parseZoePhase2(const String &response)
{
  if (liveData->currentAtshRequest.equals("ATSH18DAF1DB"))
  {
    if (liveData->commandRequest.equals("229002") && response.startsWith("629002") && response.length() >= 10)
    {
      const float soc = liveData->hexToDecFromResponse(6, 10, 2, false) / 100.0;
      if (inRange(soc, 0, 100))
      {
        liveData->params.socPercPrevious = liveData->params.socPerc;
        liveData->params.socPerc = soc;
        liveData->params.socPercBms = soc;
      }
    }
    else if (liveData->commandRequest.equals("229001") && response.startsWith("629001") && response.length() >= 10)
    {
      const float soc = (liveData->hexToDecFromResponse(6, 10, 2, false) - 300) / 100.0;
      if (inRange(soc, 0, 100))
      {
        if (!inRange(liveData->params.socPerc, 0, 100))
          liveData->params.socPerc = soc;
        liveData->params.socPercBms = soc;
      }
    }
    else if (liveData->commandRequest.equals("229003") && response.startsWith("629003") && response.length() >= 10)
    {
      const float soh = liveData->hexToDecFromResponse(6, 10, 2, false) / 100.0;
      if (inRange(soh, 0, 100))
        liveData->params.sohPerc = soh;
    }
    else if (liveData->commandRequest.equals("229005") && response.startsWith("629005") && response.length() >= 10)
    {
      const float voltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 10.0;
      if (inRange(voltage, 200, 450))
        liveData->params.batVoltage = voltage;
    }
    else if (liveData->commandRequest.equals("229012") && response.startsWith("629012") && response.length() >= 10)
    {
      const float temp = (liveData->hexToDecFromResponse(6, 10, 2, false) - 640) * 0.0625;
      if (inRange(temp, -40, 90))
      {
        liveData->params.batTempC = temp;
        liveData->params.batMinC = temp;
        liveData->params.batMaxC = temp;
        liveData->params.batModuleTempC[0] = temp;
      }
    }
    else if (liveData->commandRequest.equals("229018") && response.startsWith("629018") && response.length() >= 10)
    {
      const float power = liveData->hexToDecFromResponse(6, 10, 2, false) / 100.0;
      if (inRange(power, 0, 100))
        liveData->params.availableChargePower = power;
    }
    else if (liveData->commandRequest.equals("2291C8") && response.startsWith("6291C8") && response.length() >= 12)
    {
      const float energy = liveData->hexToDecFromResponse(6, 12, 3, false) / 1000.0;
      if (inRange(energy, 0, 80))
      {
        liveData->params.batEnergyContent = energy;
        zoeHasBmsEnergy = true;
      }
    }
    else if (liveData->commandRequest.startsWith("22913") && response.startsWith("62913") && response.length() >= 10)
    {
      const uint16_t pid = strtoul(liveData->commandRequest.substring(2).c_str(), NULL, 16);
      const uint8_t idx = pid >= 0x9131 ? pid - 0x9131 : 255;
      const float temp = (liveData->hexToDecFromResponse(6, 10, 2, false) - 640) * 0.0625;
      if (idx < 12 && inRange(temp, -40, 90))
      {
        liveData->params.batModuleTempC[idx] = temp;
        liveData->params.batModuleTempCount = 12;
        updateZoeModuleMinMax(12);
      }
    }
  }

  if (liveData->currentAtshRequest.equals("ATSH18DAF1DA"))
  {
    if (liveData->commandRequest.equals("222003") && response.startsWith("622003") && response.length() >= 10)
    {
      float speed = liveData->hexToDecFromResponse(6, 10, 2, false) / 100.0;
      if (inRange(speed, 0, 220))
      {
        if (speed > 10)
          speed += liveData->settings.speedCorrection;
        liveData->params.speedKmh = speed;
        if (!liveData->params.forwardDriveMode && !liveData->params.reverseDriveMode)
        {
          liveData->params.forwardDriveMode = speed > 1;
          liveData->params.parkModeOrNeutral = speed <= 1;
        }
        liveData->params.ignitionOn = speed > 1 || liveData->params.forwardDriveMode || liveData->params.reverseDriveMode;
      }
    }
    else if (liveData->commandRequest.equals("222005") && response.startsWith("622005") && response.length() >= 10)
    {
      const float voltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 100.0;
      if (inRange(voltage, 8, 16))
      {
        liveData->params.auxVoltage = voltage;
        const float tmpAuxPerc = liveData->params.ignitionOn ? (voltage - 12.8) * 100 / (14.8 - 12.8) : (voltage - 11.6) * 100 / (12.8 - 11.6);
        liveData->params.auxPerc = tmpAuxPerc > 100 ? 100 : (tmpAuxPerc < 0 ? 0 : tmpAuxPerc);
      }
    }
    else if (liveData->commandRequest.equals("22200F") && response.startsWith("62200F"))
    {
      const int brake = canzeBits(response, 28, 31);
      if (brake >= 0)
        liveData->params.brakeLights = (brake == 2 || brake == 4);
    }
    else if (liveData->commandRequest.equals("2220DE") && response.startsWith("6220DE") && response.length() >= 10)
    {
      const float temp = (liveData->hexToDecFromResponse(6, 10, 2, false) / 10.0) - 273;
      if (inRange(temp, -40, 80))
        liveData->params.outdoorTemperature = temp;
    }
    else if (liveData->commandRequest.equals("2221DF") && response.startsWith("6221DF") && response.length() >= 10)
    {
      const float current = liveData->hexToDecFromResponse(6, 10, 2, false) - 32767;
      if (inRange(current, -200, 200))
        liveData->params.auxCurrentAmp = current;
    }
    else if (liveData->commandRequest.equals("222B85") && response.startsWith("622B85"))
    {
      const int plugged = canzeBits(response, 31, 31);
      if (plugged >= 0)
      {
        liveData->params.chargerACconnected = plugged == 1;
        liveData->params.chargingOn = liveData->params.chargerACconnected && liveData->params.batPowerKw > 0.5;
        if (liveData->params.chargingOn)
          liveData->params.lastChargingOnTime = liveData->params.currentTime;
      }
    }
    else if (liveData->commandRequest.equals("22300F") && response.startsWith("62300F") && response.length() >= 10)
    {
      const float power = liveData->hexToDecFromResponse(6, 10, 2, false) * 0.025;
      if (inRange(power, 0, 100))
        liveData->params.availableChargePower = power;
    }
    else if (liveData->commandRequest.equals("223064") && response.startsWith("623064") && response.length() >= 10)
    {
      const float rpm = liveData->hexToDecFromResponse(6, 10, 2, true);
      if (inRange(rpm, -20000, 20000))
        liveData->params.motor1Rpm = rpm;
    }
  }

  if (liveData->currentAtshRequest.equals("ATSH764"))
  {
    if (liveData->commandRequest.equals("224009") && response.startsWith("624009") && response.length() >= 10)
    {
      const float temp = (liveData->hexToDecFromResponse(6, 10, 2, false) - 400) / 10.0;
      if (inRange(temp, -40, 80))
        liveData->params.indoorTemperature = temp;
    }
    else if (liveData->commandRequest.equals("224347") && response.startsWith("624347") && response.length() >= 10)
    {
      const float temp = (liveData->hexToDecFromResponse(6, 10, 2, false) - 200) * 0.25;
      if (inRange(temp, -40, 80))
        liveData->params.outdoorTemperature = temp;
    }
    else if (liveData->commandRequest.equals("224423") && response.startsWith("624423") && response.length() >= 10)
    {
      const float temp = (liveData->hexToDecFromResponse(6, 10, 2, false) - 400) / 10.0;
      if (inRange(temp, -40, 80))
        liveData->params.batInletC = temp;
    }
    else if (liveData->commandRequest.equals("22441C") && response.startsWith("62441C") && response.length() >= 8)
    {
      const int mode = liveData->hexToDecFromResponse(6, 8, 1, false);
      liveData->params.batteryManagementMode = mode == 0 ? BAT_MAN_MODE_OFF : BAT_MAN_MODE_COOLING;
    }
    else if (liveData->commandRequest.equals("22400A") && response.startsWith("62400A") && response.length() >= 10)
    {
      const float temp = (liveData->hexToDecFromResponse(6, 10, 2, false) - 400) / 10.0;
      if (inRange(temp, -40, 80))
        liveData->params.evaporatorTempC = temp;
    }
  }
}

/**
   activateCommandQueue
*/
void CarRenaultZoe::activateCommandQueue()
{

  std::vector<String> commandQueueRenaultZoe = {
      "AT Z",
      "AT I",
      "AT S0",
      "AT E0",
      "AT L0",
      "AT SP 6",
      "AT DP",
      "AT ST16",

      // LBC Lithium battery controller (TX 0x79B -> RX 0x7BB, Renault +0x20)
      "ATSH79B",
      "ATFCSH79B",
      "ATCRA7BB",
      "ATFCSD300010",
      "ATFCSM1",
      "2101",
      "2103",
      "2104",
      "2141",
      "2142",
      "2161",

      // CLUSTER Instrument panel (TX 0x743 -> RX 0x763, Renault +0x20)
      "ATSH743",
      "ATFCSH743",
      "ATCRA763",
      "ATFCSD300010",
      "ATFCSM1",
      "220206",

      // CLIM Climate control (TX 0x744 -> RX 0x764, Renault +0x20)
      "ATSH744",
      "ATFCSH744",
      "ATCRA764",
      "ATFCSD300010",
      "ATFCSM1",
      "2143",
      "2121",
      "2144",

      // EVC Electric vehicle controller (TX 0x7E4 -> RX 0x7EC, UDS +8)
      "ATSH7E4",
      "ATFCSH7E4",
      "ATCRA7EC",
      "ATFCSD300010",
      "ATFCSM1",
      "222003",
      "22200F",
      "222233",
      "222238",
      "222C04",
      "22331E",
      "22338E",
      "223392",
      "223397",
      "223398",
      "22339B",
      "22339D",
      "223451",
      "223456",
      "222005",
      "2220DE",

      // TPMS / VIN - clear ATCRA filter so ELM327 accepts any response ID
      "ATSH765",
      "ATFCSH765",
      "ATCRA",
      "ATFCSD300010",
      "ATFCSM1",
      "2174",
      "2181",
      "ATSH763",
      "ATFCSH763",
      "ATCRA",
      "ATFCSD300010",
      "ATFCSM1",
      "2181",
      "22F190",

      // PEB Power electronics bloc - clear ATCRA filter
      "ATSH77E",
      "ATFCSH77E",
      "ATCRA",
      "ATFCSD300010",
      "ATFCSM1",
      "22302B",
  };

  if (liveData->settings.carType == CAR_RENAULT_ZOE_ZE50_52)
  {
    const std::vector<String> commandQueueRenaultZoePhase2 = {
        // BMS Phase2
        "ATSH18DAF1DB",
        "ATFCSH18DAF1DB",
        "ATFCSD300010",
        "ATFCSM1",
        "1003",
        "229002",
        "229001",
        "229003",
        "229005",
        "229012",
        "229018",
        "2291C8",
        "229131",
        "229132",
        "229133",
        "229134",
        "229135",
        "229136",
        "229137",
        "229138",
        "229139",
        "22913A",
        "22913B",
        "22913C",

        // EVC Phase2
        "ATSH18DAF1DA",
        "ATFCSH18DAF1DA",
        "ATFCSD300010",
        "ATFCSM1",
        "222003",
        "222005",
        "22200F",
        "2220DE",
        "2221DF",
        "222B85",
        "22300F",
        "223451",
        "223064",

        // CLIM Phase2
        "ATSH764",
        "ATFCSH764",
        "ATFCSD300010",
        "ATFCSM1",
        "224009",
        "224347",
        "224423",
        "22441C",
        "22400A",
    };
    commandQueueRenaultZoe.insert(commandQueueRenaultZoe.end(), commandQueueRenaultZoePhase2.begin(), commandQueueRenaultZoePhase2.end());
  }

  liveData->params.batModuleTempCount = liveData->settings.carType == CAR_RENAULT_ZOE_ZE50_52 ? 12 : 24;
  liveData->params.cellCount = 96;
  liveData->params.batteryManagementMode = BAT_MAN_MODE_UNKNOWN;
  zoeHasBmsEnergy = false;
  updateZoePackSize();

  liveData->commandQueue.clear();
  for (auto cmd : commandQueueRenaultZoe)
  {
    liveData->commandQueue.push_back({0, cmd});
  }

  liveData->commandQueueLoopFrom = commandQueueLoopFromRenaultZoe;
  liveData->commandQueueCount = commandQueueRenaultZoe.size();
}

/**
   parseRowMerged
*/
void CarRenaultZoe::parseRowMerged()
{

  String response = normalizedResponse();
  liveData->responseRowMerged = response;
  if (isTextResponse(response))
    return;

  parseZoePhase2(response);

  if (liveData->currentAtshRequest.equals("ATSH79B"))
  {
    if (liveData->commandRequest.equals("2101") && response.startsWith("6101") && response.length() >= 88)
    {
      const float amps = (liveData->hexToDecFromResponse(4, 8, 2, false) - 5000) / 10.0;
      if (inRange(amps, -500, 500))
      {
        liveData->params.batPowerAmp = amps;
        if (inRange(liveData->params.batVoltage, 200, 450))
          liveData->params.batPowerKw = (liveData->params.batPowerAmp * liveData->params.batVoltage) / 1000.0;
        if (liveData->params.batPowerKw < 0)
          liveData->params.chargingStartTime = liveData->params.currentTime;
        if (liveData->params.speedKmh > 1)
          liveData->params.batPowerKwh100 = liveData->params.batPowerKw / liveData->params.speedKmh * 100;
        updateChargingStateFromPower();
      }

      const float auxVoltage = liveData->hexToDecFromResponse(56, 60, 2, false) / 100.0;
      if (inRange(auxVoltage, 8, 16))
      {
        liveData->params.auxVoltage = auxVoltage;
        const float tmpAuxPerc = liveData->params.ignitionOn ? (auxVoltage - 12.8) * 100 / (14.8 - 12.8) : (auxVoltage - 11.6) * 100 / (12.8 - 11.6);
        liveData->params.auxPerc = tmpAuxPerc > 100 ? 100 : (tmpAuxPerc < 0 ? 0 : tmpAuxPerc);
      }

      const float availablePower = liveData->hexToDecFromResponse(84, 88, 2, false) / 100.0;
      if (inRange(availablePower, 0, 100))
        liveData->params.availableChargePower = availablePower;
    }
    else if (liveData->commandRequest.equals("2103") && response.startsWith("6103") && response.length() >= 52)
    {
      const float cellMin = liveData->hexToDecFromResponse(24, 28, 2, false) / 100.0;
      const float cellMax = liveData->hexToDecFromResponse(28, 32, 2, false) / 100.0;
      const float soc = liveData->hexToDecFromResponse(48, 52, 2, false) / 100.0;
      if (inRange(soc, 0, 100))
      {
        liveData->params.socPercPrevious = liveData->params.socPerc;
        liveData->params.socPerc = soc;
        liveData->params.socPercBms = soc;
      }
      if (inRange(cellMin, 2.5, 4.5))
        liveData->params.batCellMinV = cellMin;
      if (inRange(cellMax, 2.5, 4.5))
        liveData->params.batCellMaxV = cellMax;
    }
    else if (liveData->commandRequest.equals("2104") && response.startsWith("6104"))
    {
      for (uint16_t i = 0; i < 12; i++)
      {
        if (response.length() >= 10 + (i * 6))
        {
          const float temp = liveData->hexToDecFromResponse(8 + (i * 6), 10 + (i * 6), 1, false) - 40;
          if (inRange(temp, -40, 90))
            liveData->params.batModuleTempC[i] = temp;
        }
      }
      for (uint16_t i = 12; i < 24; i++)
      {
        const uint16_t from = 80 + ((i - 12) * 6);
        if (response.length() >= from + 2)
        {
          const float temp = liveData->hexToDecFromResponse(from, from + 2, 1, false) - 40;
          if (inRange(temp, -40, 90))
            liveData->params.batModuleTempC[i] = temp;
        }
      }
      liveData->params.batModuleTempCount = 24;
      updateZoeModuleMinMax(24);
    }
    else if (liveData->commandRequest.equals("2141") && response.startsWith("6141"))
    {
      updateZoeCells(0, 62);
    }
    else if (liveData->commandRequest.equals("2142") && response.startsWith("6142"))
    {
      updateZoeCells(62, 34);
      const uint8_t voltageOffsets[] = {144, 136, 140};
      for (uint8_t i = 0; i < 3; i++)
      {
        const uint8_t from = voltageOffsets[i];
        if (response.length() >= from + 4)
        {
          const float voltage = liveData->hexToDecFromResponse(from, from + 4, 2, false) / 100.0;
          if (inRange(voltage, 200, 450))
          {
            liveData->params.batVoltage = voltage;
            break;
          }
        }
      }
    }
    else if (liveData->commandRequest.equals("2161") && response.startsWith("6161") && response.length() >= 20)
    {
      const float soh = liveData->hexToDecFromResponse(18, 20, 1, false) / 2.0;
      if (inRange(soh, 0, 100))
        liveData->params.sohPerc = soh;
    }
  }

  if (liveData->currentAtshRequest.equals("ATSH743"))
  {
    if (liveData->commandRequest.equals("220206") && response.startsWith("620206") && response.length() >= 14)
      liveData->params.odoKm = liveData->hexToDecFromResponse(6, 14, 4, false);
  }

  if (liveData->currentAtshRequest.equals("ATSH744"))
  {
    if (liveData->commandRequest.equals("2143") && response.startsWith("6143"))
    {
      const float temp = zoeTempFromBits(response, 110, 117);
      if (inRange(temp, -40, 80))
        liveData->params.outdoorTemperature = temp;
    }
    else if (liveData->commandRequest.equals("2121") && response.startsWith("6121"))
    {
      const float indoor = zoeTempFromBits(response, 26, 35, 0.1);
      const float batInlet = zoeTempFromBits(response, 150, 159, 0.1);
      if (inRange(indoor, -40, 80))
        liveData->params.indoorTemperature = indoor;
      if (inRange(batInlet, -40, 80))
        liveData->params.batInletC = batInlet;
    }
    else if (liveData->commandRequest.equals("2144") && response.startsWith("6144"))
    {
      const float batInlet = zoeTempFromBits(response, 170, 179, 0.1);
      if (inRange(batInlet, -40, 80))
        liveData->params.batInletC = batInlet;
    }
  }

  if (liveData->currentAtshRequest.equals("ATSH7E4"))
  {
    if (liveData->commandRequest.equals("222003") && response.startsWith("622003") && response.length() >= 8)
    {
      const bool hasTwoBytes = response.length() >= 10;
      float speed = liveData->hexToDecFromResponse(6, hasTwoBytes ? 10 : 8, hasTwoBytes ? 2 : 1, false) / 100.0;
      if (inRange(speed, 0, 220))
      {
        if (speed > 10)
          speed += liveData->settings.speedCorrection;
        liveData->params.speedKmh = speed;
        if (!liveData->params.forwardDriveMode && !liveData->params.reverseDriveMode)
        {
          liveData->params.forwardDriveMode = speed > 1;
          liveData->params.parkModeOrNeutral = speed <= 1;
        }
        liveData->params.ignitionOn = speed > 1 || liveData->params.forwardDriveMode || liveData->params.reverseDriveMode || liveData->params.chargingOn;
      }
    }
    else if (liveData->commandRequest.equals("22200F") && response.startsWith("62200F"))
    {
      const int brake = canzeBits(response, 28, 31);
      if (brake >= 0)
        liveData->params.brakeLights = (brake == 2 || brake == 4);
    }
    else if ((liveData->commandRequest.equals("222238") || liveData->commandRequest.equals("222C04")) && (response.startsWith("622238") || response.startsWith("622C04")))
    {
      const int gear = canzeBits(response, 29, 31);
      if (gear >= 0)
        applyZoeGear(gear);
    }
    else if (liveData->commandRequest.equals("222005") && response.startsWith("622005") && response.length() >= 10)
    {
      const float voltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 100.0;
      if (inRange(voltage, 8, 16))
      {
        liveData->params.auxVoltage = voltage;
        const float tmpAuxPerc = liveData->params.ignitionOn ? (voltage - 12.8) * 100 / (14.8 - 12.8) : (voltage - 11.6) * 100 / (12.8 - 11.6);
        liveData->params.auxPerc = tmpAuxPerc > 100 ? 100 : (tmpAuxPerc < 0 ? 0 : tmpAuxPerc);
      }
    }
    else if (liveData->commandRequest.equals("2220DE") && response.startsWith("6220DE") && response.length() >= 10)
    {
      const float temp = (liveData->hexToDecFromResponse(6, 10, 2, false) / 10.0) - 273;
      if (inRange(temp, -40, 80))
        liveData->params.outdoorTemperature = temp;
    }
    else if (liveData->commandRequest.equals("22338E") && response.startsWith("62338E"))
    {
      const int open = canzeBits(response, 31, 31);
      if (open >= 0)
        liveData->params.leftFrontDoorOpen = open == 1;
    }
    else if (liveData->commandRequest.equals("22339B") && response.startsWith("62339B"))
    {
      const int open = canzeBits(response, 31, 31);
      if (open >= 0)
        liveData->params.rightFrontDoorOpen = open == 1;
    }
    else if (liveData->commandRequest.equals("22339D") && response.startsWith("62339D"))
    {
      const int plugged = canzeBits(response, 31, 31);
      if (plugged >= 0)
      {
        liveData->params.chargerACconnected = plugged == 1;
        liveData->params.chargingOn = liveData->params.chargerACconnected && liveData->params.batPowerKw > 0.5;
        if (liveData->params.chargingOn)
          liveData->params.lastChargingOnTime = liveData->params.currentTime;
      }
    }
  }

  if (liveData->currentAtshRequest.equals("ATSH765"))
  {
    if (liveData->commandRequest.equals("2174") && response.startsWith("6174"))
    {
      setTirePressureFromRaw(liveData->params.tireRearRightPressureBar, canzeBits(response, 112, 119));
      setTirePressureFromRaw(liveData->params.tireRearLeftPressureBar, canzeBits(response, 120, 127));
      setTirePressureFromRaw(liveData->params.tireFrontRightPressureBar, canzeBits(response, 128, 135));
      setTirePressureFromRaw(liveData->params.tireFrontLeftPressureBar, canzeBits(response, 136, 143));
    }
    else if (liveData->commandRequest.equals("2181") && response.startsWith("6181"))
    {
      const String vin = decodeAscii(response, 4);
      if (vin.length() >= 10)
        vin.toCharArray(liveData->params.carVin, sizeof(liveData->params.carVin));
    }
  }

  if (liveData->currentAtshRequest.equals("ATSH763"))
  {
    if (liveData->commandRequest.equals("2181") && response.startsWith("6181"))
    {
      const String vin = decodeAscii(response, 4);
      if (vin.length() >= 10)
        vin.toCharArray(liveData->params.carVin, sizeof(liveData->params.carVin));
    }
    else if (liveData->commandRequest.equals("22F190") && response.startsWith("62F190"))
    {
      const String vin = decodeAscii(response, 6);
      if (vin.length() >= 10)
        vin.toCharArray(liveData->params.carVin, sizeof(liveData->params.carVin));
    }
  }

  if (liveData->currentAtshRequest.equals("ATSH77E"))
  {
    if (liveData->commandRequest.equals("22302B") && response.startsWith("62302B") && response.length() >= 10)
    {
      const float temp = liveData->hexToDecFromResponse(6, 10, 2, false) / 64.0;
      if (inRange(temp, -30, 180))
        liveData->params.motorTempC = temp;
    }
  }

  updateZoePackSize();
}

/**
   loadTestData
*/
void CarRenaultZoe::loadTestData()
{

  /// LBC 79B
  liveData->currentAtshRequest = "ATSH79B";
  liveData->commandRequest = "2101";
  liveData->responseRowMerged = "6101134D138600000000000000000000000009980D610FB120D0000005420000000000000004ECB20000074B2927100000000000000000";
  parseRowMerged();
  liveData->commandRequest = "2103";
  liveData->responseRowMerged = "610301770D010D740000000001750174000000FFFF07D0050D410000030000000000";
  parseRowMerged();
  liveData->commandRequest = "2104";
  liveData->responseRowMerged = "61040B9D290B9F290B9D290B99290B9E290B98290B8A2A0B842A0BA0290B9B290B9A290B9629FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF29292A000000000000";
  parseRowMerged();
  liveData->commandRequest = "2141";
  liveData->responseRowMerged = "61410E930E950E900E930E930E950E950E930E970E940E970E970E950E940E940E930E970E920E930E8F0E900E8E0E8C0E920E970E920E940E940E930E950E950E940E970E970E950E970E940E950E950E990E940E9A0E8E0E900E990E950E900E990E980E950E940E970E970E950E940E980E970E920E920E940E950E93000000000000";
  parseRowMerged();
  liveData->commandRequest = "2142";
  liveData->responseRowMerged = "61420E920E940E970E930E920E970E940E950E950E980E920E900E8F0E8F0E920E900E920E940E9B0E980E950E940E950E930E970E980E980E950E950E930E970E950E950E978BFA8C200000";
  parseRowMerged();
  liveData->commandRequest = "2161";
  liveData->responseRowMerged = "6161000AA820C8C8C8C2C2000153B400004669FF";
  parseRowMerged();

  // CLUSTER 743
  liveData->currentAtshRequest = "ATSH743";
  liveData->commandRequest = "220206";
  liveData->responseRowMerged = "62020600015459";
  parseRowMerged();
}
