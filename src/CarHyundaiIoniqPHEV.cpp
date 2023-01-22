
#include "LiveData.h"
#include "CarHyundaiIoniqPHEV.h"
#include "CommInterface.h"
#include <vector>

#define commandQueueLoopFromHyundaiIoniq 8

/**
   activateliveData->commandQueue
*/
void CarHyundaiIoniqPHEV::activateCommandQueue()
{

  // Optimizer
  lastAllowTpms = 0;
  lastAllowOdo = 0;
  lastAllowAircon = 0;
  lastAllowDriveMode = 0;

  // Command queue
  std::vector<String> commandQueueHyundaiIoniq = {
      "AT Z",    // Reset all
      "AT I",    // Print the version ID
      "AT E0",   // Echo off
      "AT L0",   // Linefeeds off
      "AT S0",   // Printing of spaces on
      "AT SP 6", // Select protocol to ISO 15765-4 CAN (11 bit ID, 500 kbit/s)
      //"AT AL",     // Allow Long (>7 byte) messages
      //"AT AR",     // Automatically receive
      //"AT H1",     // Headers on (debug only)
      //"AT D1",     // Display of the DLC on
      //"AT CAF0",   // Automatic formatting off
      "AT DP",
      "AT ST16",

      // Loop from (HYUNDAI IONIQ)
      // BMS
      "ATSH7E4",
      "2101", // power kw, ...
      "2102", // cell voltages, screen 3 only
      "2103", // cell voltages, screen 3 only
      "2104", // cell voltages, screen 3 only
      "2105", // soh, soc, ..

      // VMCU
      "ATSH7E2",
      "2101", // speed, ...
      "2102", // aux, ...

      // IGPM
      "ATSH770",
      "22BC03", // low beam
      "22BC06", // brake light

      // ABS / ESP + AHB
      "ATSH7D1",
      "22C101", // brake, park/drive mode

      // Aircondition
      // IONIQ OK
      "ATSH7B3",
      "220100", // in/out temp
      "220102", // coolant temp1, 2

      // BCM / TPMS
      // IONIQ OK
      "ATSH7A0",
      "22c00b", // tire pressure/temp

      // CLUSTER MODULE
      // IONIQ OK
      "ATSH7C6",
      "22B002", // odo
  };

  // 28kWh version
  liveData->params.batteryTotalAvailableKWh = 8.9;
  liveData->params.batModuleTempCount = 4;
  liveData->params.cellCount = 96;

  // 360V and the Capacity is 24,7 Ah, so 24,7 Ah x 360 V = 8892Wh = 8,9 kWh. So with this data, 8892 Wh / 96 cells = 96 Wh per cell

  //  Empty and fill command queue
  liveData->commandQueue.clear();
  for (auto cmd : commandQueueHyundaiIoniq)
  {
    liveData->commandQueue.push_back({0, cmd});
  }

  //
  liveData->commandQueueLoopFrom = commandQueueLoopFromHyundaiIoniq;
  liveData->commandQueueCount = commandQueueHyundaiIoniq.size();
  if (liveData->settings.carType == CAR_HYUNDAI_IONIQ_PHEV)
  {
    liveData->rxTimeoutMs = 500;            // timeout for receiving of CAN response
    liveData->delayBetweenCommandsMs = 100; // delay between commands, set to 0 if no delay is needed
  }
}

/**
   parseRowMerged
*/
void CarHyundaiIoniqPHEV::parseRowMerged()
{

  uint8_t tempByte;
  //  float tempFloat;
  String tmpStr;

  // VMCU 7E2
  if (liveData->currentAtshRequest.equals("ATSH7E2"))
  {
    if (liveData->commandRequest.equals("2101"))
    {
      liveData->params.speedKmh = liveData->hexToDecFromResponse(32, 36, 2, false) * 0.0155; // / 100.0 *1.609 = real to gps is 1.750
      if (liveData->params.speedKmh > 10)
        liveData->params.speedKmh += liveData->settings.speedCorrection;
      if (liveData->params.speedKmh < -99 || liveData->params.speedKmh > 200)
        liveData->params.speedKmh = 0;
    }
    if (liveData->commandRequest.equals("2102"))
    {
      liveData->params.auxCurrentAmp = -liveData->hexToDecFromResponse(46, 50, 2, true) / 1000.0;
    }
  }

  // IGPM
  if (liveData->currentAtshRequest.equals("ATSH770"))
  {
    if (liveData->commandRequest.equals("22BC03"))
    {
      tempByte = liveData->hexToDecFromResponse(14, 16, 1, false);
      liveData->params.hoodDoorOpen = (bitRead(tempByte, 7) == 1);
      if (liveData->settings.rightHandDrive)
      {
        liveData->params.leftFrontDoorOpen = (bitRead(tempByte, 0) == 1);
        liveData->params.rightFrontDoorOpen = (bitRead(tempByte, 5) == 1);
        liveData->params.leftRearDoorOpen = (bitRead(tempByte, 2) == 1);
        liveData->params.rightRearDoorOpen = (bitRead(tempByte, 4) == 1);
      }
      else
      {
        liveData->params.leftFrontDoorOpen = (bitRead(tempByte, 5) == 1);
        liveData->params.rightFrontDoorOpen = (bitRead(tempByte, 0) == 1);
        liveData->params.leftRearDoorOpen = (bitRead(tempByte, 4) == 1);
        liveData->params.rightRearDoorOpen = (bitRead(tempByte, 2) == 1);
      }

      tempByte = liveData->hexToDecFromResponse(16, 18, 1, false);
      liveData->params.trunkDoorOpen = (bitRead(tempByte, 0) == 1);
      liveData->params.ignitionOn = (bitRead(tempByte, 5) == 1);
      if (liveData->params.ignitionOn)
      {
        liveData->params.lastIgnitionOnTime = liveData->params.currentTime;
      }

      tempByte = liveData->hexToDecFromResponse(18, 20, 1, false);
      liveData->params.headLights = (bitRead(tempByte, 5) == 1);
      liveData->params.autoLights = (bitRead(tempByte, 4) == 1);
      liveData->params.dayLights = (bitRead(tempByte, 3) == 1);
    }
    if (liveData->commandRequest.equals("22BC06"))
    {
      tempByte = liveData->hexToDecFromResponse(14, 16, 1, false);
      liveData->params.brakeLights = (bitRead(tempByte, 5) == 1);
    }
  }

  // ABS / ESP + AHB 7D1
  if (liveData->currentAtshRequest.equals("ATSH7D1"))
  {
    if (liveData->commandRequest.equals("22C101"))
    {
      uint8_t driveMode = liveData->hexToDecFromResponse(22, 24, 1, false);
      liveData->params.forwardDriveMode = (driveMode == 4);
      liveData->params.reverseDriveMode = (driveMode == 2);
      liveData->params.parkModeOrNeutral = (driveMode == 1);
    }
  }

  // Cluster module 7c6
  if (liveData->currentAtshRequest.equals("ATSH7C6"))
  {
    if (liveData->commandRequest.equals("22B002"))
    {
      liveData->params.odoKm = liveData->decFromResponse(18, 24);
      syslog->print("KM: ");
      syslog->println(liveData->params.odoKm);
      syslog->println(liveData->responseRowMerged.substring(18, 24));
    }
  }

  // Aircon 7b3
  if (liveData->currentAtshRequest.equals("ATSH7B3"))
  {
    if (liveData->commandRequest.equals("220100"))
    {
      liveData->params.indoorTemperature = (liveData->hexToDecFromResponse(16, 18, 1, false) / 2) - 40;
      liveData->params.outdoorTemperature = (liveData->hexToDecFromResponse(18, 20, 1, false) / 2) - 40;
    }
    if (liveData->commandRequest.equals("220102") && liveData->responseRowMerged.substring(12, 14) == "00")
    {
      liveData->params.coolantTemp1C = (liveData->hexToDecFromResponse(14, 16, 1, false) / 2) - 40;
      liveData->params.coolantTemp2C = (liveData->hexToDecFromResponse(16, 18, 1, false) / 2) - 40;
    }
  }

  // BMS 7e4
  if (liveData->currentAtshRequest.equals("ATSH7E4"))
  {
    if (liveData->commandRequest.equals("2101"))
    {
      liveData->params.socPercBms = liveData->hexToDecFromResponse(12, 14, 1, false) / 2.0;
      liveData->params.cumulativeEnergyChargedKWh = liveData->decFromResponse(80, 88) / 10.0;
      liveData->params.cumulativeEnergyDischargedKWh = liveData->decFromResponse(88, 96) / 10.0;
      liveData->params.availableChargePower = liveData->decFromResponse(16, 20) / 100.0;
      liveData->params.availableDischargePower = liveData->decFromResponse(20, 24) / 100.0;
      liveData->params.isolationResistanceKOhm = liveData->hexToDecFromResponse(118, 122, 2, false);
      liveData->params.batFanStatus = liveData->hexToDecFromResponse(58, 60, 1, false);
      liveData->params.batFanFeedbackHz = liveData->hexToDecFromResponse(60, 62, 1, false);

      if (liveData->settings.voltmeterEnabled == 0)
      {
        liveData->params.auxVoltage = liveData->hexToDecFromResponse(62, 64, 1, false) / 10.0;

        float tmpAuxPerc = (float)(liveData->params.auxVoltage - 11.6) * 100 / (float)(12.8 - 11.6); // min 11.6V; max: 12.8V

        if (tmpAuxPerc > 100)
        {
          liveData->params.auxPerc = 100;
        }
        else if (tmpAuxPerc < 0)
        {
          liveData->params.auxPerc = 0;
        }
        else
        {
          liveData->params.auxPerc = tmpAuxPerc;
        }
      }

      liveData->params.getValidResponse = true;

      liveData->params.batPowerAmp = -liveData->hexToDecFromResponse(24, 28, 2, true) / 10.0;
      liveData->params.batVoltage = liveData->hexToDecFromResponse(28, 32, 2, false) / 10.0;
      liveData->params.batPowerKw = (liveData->params.batPowerAmp * liveData->params.batVoltage) / 1000.0;
      if (liveData->params.batPowerKw < 1) // Reset charging start time
        liveData->params.chargingStartTime = liveData->params.currentTime;
      liveData->params.batPowerKwh100 = liveData->params.batPowerKw / liveData->params.speedKmh * 100;
      liveData->params.batCellMaxV = liveData->hexToDecFromResponse(50, 52, 1, false) / 50.0;
      liveData->params.batCellMinV = liveData->hexToDecFromResponse(54, 56, 1, false) / 50.0;
      liveData->params.batModuleTempC[0] = liveData->hexToDecFromResponse(36, 38, 1, true);
      liveData->params.batModuleTempC[1] = liveData->hexToDecFromResponse(38, 40, 1, true);
      liveData->params.batModuleTempC[2] = liveData->hexToDecFromResponse(40, 42, 1, true);
      liveData->params.batModuleTempC[3] = liveData->hexToDecFromResponse(42, 44, 1, true);
      liveData->params.batModuleTempC[4] = liveData->hexToDecFromResponse(44, 46, 1, true);
      // liveData->params.batTempC = liveData->hexToDecFromResponse(34, 36, 1, true);
      // liveData->params.batMaxC = liveData->hexToDecFromResponse(32, 34, 1, true);
      // liveData->params.batMinC = liveData->hexToDecFromResponse(34, 36, 1, true);

      tempByte = liveData->hexToDecFromResponse(22, 24, 1, false); // bit 5 = AC; bit 6 = DC; bit 7 = charging on
      liveData->params.chargerACconnected = (bitRead(tempByte, 5) == 1);
      liveData->params.chargerDCconnected = (bitRead(tempByte, 6) == 1);
      liveData->params.chargingOn = (bitRead(tempByte, 7) == 1);

      if (liveData->params.chargingOn)
      {
        liveData->params.lastChargingOnTime = liveData->params.currentTime;
      }

      // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
      liveData->params.batInletC = liveData->hexToDecFromResponse(48, 50, 1, true);
      if (liveData->params.speedKmh < 10 && liveData->params.batPowerKw >= 1 && liveData->params.socPerc > 0 && liveData->params.socPerc <= 100)
      {
        if (liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] == -100 || liveData->params.batPowerKw < liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)])
          liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
        if (liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] == -100 || liveData->params.batPowerKw > liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)])
          liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
        liveData->params.chargingGraphBatMinTempC[int(liveData->params.socPerc)] = liveData->params.batMinC;
        liveData->params.chargingGraphBatMaxTempC[int(liveData->params.socPerc)] = liveData->params.batMaxC;
        liveData->params.chargingGraphHeaterTempC[int(liveData->params.socPerc)] = liveData->params.batHeaterC;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("2102") && liveData->responseRowMerged.substring(10, 12) == "FF")
    {
      for (int i = 0; i < 32; i++)
      {
        liveData->params.cellVoltage[i] = liveData->hexToDecFromResponse(12 + (i * 2), 12 + (i * 2) + 2, 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("2103"))
    {
      for (int i = 0; i < 32; i++)
      {
        liveData->params.cellVoltage[32 + i] = liveData->hexToDecFromResponse(12 + (i * 2), 12 + (i * 2) + 2, 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("2104"))
    {
      for (int i = 0; i < 32; i++)
      {
        liveData->params.cellVoltage[64 + i] = liveData->hexToDecFromResponse(12 + (i * 2), 12 + (i * 2) + 2, 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("2105"))
    {
      liveData->params.socPercPrevious = liveData->params.socPerc;
      liveData->params.sohPerc = liveData->hexToDecFromResponse(74, 78, 2, false) / 10.0; // PHEV
      liveData->params.socPerc = liveData->hexToDecFromResponse(80, 82, 1, false) / 2.0;  // PHEV

      // Remaining battery modules (tempC)
      liveData->params.batModuleTempC[5] = liveData->hexToDecFromResponse(22, 24, 1, true);
      liveData->params.batModuleTempC[6] = liveData->hexToDecFromResponse(24, 26, 1, true);
      liveData->params.batModuleTempC[7] = liveData->hexToDecFromResponse(26, 28, 1, true);
      liveData->params.batModuleTempC[8] = liveData->hexToDecFromResponse(28, 30, 1, true);
      liveData->params.batModuleTempC[9] = liveData->hexToDecFromResponse(30, 32, 1, true);
      liveData->params.batModuleTempC[10] = liveData->hexToDecFromResponse(32, 34, 1, true);
      liveData->params.batModuleTempC[11] = liveData->hexToDecFromResponse(34, 36, 1, true);

      liveData->params.batMinC = liveData->params.batMaxC = liveData->params.batModuleTempC[0];
      for (uint16_t i = 1; i < liveData->params.batModuleTempCount; i++)
      {
        if (liveData->params.batModuleTempC[i] < liveData->params.batMinC)
          liveData->params.batMinC = liveData->params.batModuleTempC[i];
        if (liveData->params.batModuleTempC[i] > liveData->params.batMaxC)
          liveData->params.batMaxC = liveData->params.batModuleTempC[i];
      }
      liveData->params.batTempC = liveData->params.batMinC;

      // Soc10ced table, record x0% CEC/CED table (ex. 90%->89%, 80%->79%)
      if (liveData->params.socPercPrevious - liveData->params.socPerc > 0)
      {
        byte index = (int(liveData->params.socPerc) == 4) ? 0 : (int)(liveData->params.socPerc / 10) + 1;
        if ((int(liveData->params.socPerc) % 10 == 9 || int(liveData->params.socPerc) == 4) && liveData->params.soc10ced[index] == -1)
        {
          liveData->params.soc10ced[index] = liveData->params.cumulativeEnergyDischargedKWh;
          liveData->params.soc10cec[index] = liveData->params.cumulativeEnergyChargedKWh;
          liveData->params.soc10odo[index] = liveData->params.odoKm;
          liveData->params.soc10time[index] = liveData->params.currentTime;
        }
      }
      liveData->params.batHeaterC = liveData->hexToDecFromResponse(50, 52, 1, true);
      //
      for (int i = 30; i < 32; i++)
      { // ai/aj position
        liveData->params.cellVoltage[96 - 30 + i] = -1;
      }
    }
    // BMS 7e4
    // IONIQ FAILED
    if (liveData->commandRequest.equals("2106"))
    {
      liveData->params.coolingWaterTempC = liveData->hexToDecFromResponse(14, 16, 1, false);
    }
  }

  // TPMS 7a0
  if (liveData->currentAtshRequest.equals("ATSH7A0"))
  {
    if (liveData->commandRequest.equals("22c00b"))
    {
      liveData->params.tireFrontLeftPressureBar = liveData->hexToDecFromResponse(14, 16, 2, false) / 72.51886900361;  // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireFrontRightPressureBar = liveData->hexToDecFromResponse(22, 24, 2, false) / 72.51886900361; // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireRearRightPressureBar = liveData->hexToDecFromResponse(30, 32, 2, false) / 72.51886900361;  // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireRearLeftPressureBar = liveData->hexToDecFromResponse(38, 40, 2, false) / 72.51886900361;   // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireFrontLeftTempC = liveData->hexToDecFromResponse(16, 18, 2, false) - 50;                    // === OK Valid
      liveData->params.tireFrontRightTempC = liveData->hexToDecFromResponse(24, 26, 2, false) - 50;                   // === OK Valid
      liveData->params.tireRearRightTempC = liveData->hexToDecFromResponse(32, 34, 2, false) - 50;                    // === OK Valid
      liveData->params.tireRearLeftTempC = liveData->hexToDecFromResponse(40, 42, 2, false) - 50;                     // === OK Valid
    }
  }
}

/**
   Is command from queue allowed for execute, or continue with next
*/
bool CarHyundaiIoniqPHEV::commandAllowed()
{

  /* syslog->print("Command allowed: ");
    syslog->print(liveData->currentAtshRequest);
    syslog->print(" ");
    syslog->println(liveData->commandRequest);*/

  // SleepMode Queue Filter
  if (liveData->params.sleepModeQueue)
  {
    if (liveData->commandQueueIndex < liveData->commandQueueLoopFrom)
    {
      return true;
    }
    if (liveData->commandRequest.equals("ATSH7E4"))
    {
      return true;
    }
    if (liveData->currentAtshRequest.equals("ATSH7E4") && liveData->commandRequest.equals("2101"))
    {
      return true;
    }

    return false;
  }

  // Disabled command optimizer (allows to log all car values to sdcard, but it's slow)
  if (liveData->settings.disableCommandOptimizer) {
    return true;
  }

  // TPMS (once per 120 secs.)
  if (liveData->commandRequest.equals("ATSH7A0"))
  {
    return lastAllowTpms + 120 < liveData->params.currentTime;
  }
  if (liveData->currentAtshRequest.equals("ATSH7A0") && liveData->commandRequest.equals("22c00b"))
  {
    if (lastAllowTpms + 120 < liveData->params.currentTime)
    {
      lastAllowTpms = liveData->params.currentTime;
    }
    else
    {
      return false;
    }
  }

  // ODO (onec per 22 secs.)
  if (liveData->commandRequest.equals("ATSH7C6"))
  {
    return lastAllowOdo + 22 < liveData->params.currentTime;
  }
  if (liveData->currentAtshRequest.equals("ATSH7C6") && liveData->commandRequest.equals("22B002"))
  {
    if (lastAllowOdo + 22 < liveData->params.currentTime)
    {
      lastAllowOdo = liveData->params.currentTime;
    }
    else
    {
      return false;
    }
  }

  // AirCon (once per 31 secs.)
  if (liveData->commandRequest.equals("ATSH7B3"))
  {
    return lastAllowAircon + 31 < liveData->params.currentTime;
  }
  if (liveData->currentAtshRequest.equals("ATSH7B3"))
  {
    if (liveData->commandRequest.equals("220102"))
    {
      if (lastAllowAircon + 31 < liveData->params.currentTime)
      {
        lastAllowAircon = liveData->params.currentTime;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return lastAllowAircon + 31 < liveData->params.currentTime;
    }
  }

  // DriveMode (once per 13s)
  if (liveData->commandRequest.equals("ATSH7D1"))
  {
    return lastAllowDriveMode + 13 < liveData->params.currentTime;
  }
  if (liveData->currentAtshRequest.equals("ATSH7D1") && liveData->commandRequest.equals("22C101"))
  {
    if (lastAllowDriveMode + 13 < liveData->params.currentTime)
    {
      lastAllowDriveMode = liveData->params.currentTime;
    }
    else
    {
      return false;
    }
  }

  // BMS (only for SCREEN_CELLS)
  if (liveData->currentAtshRequest.equals("ATSH7E4"))
  {
    if (liveData->commandRequest.equals("2102") || liveData->commandRequest.equals("2103") || liveData->commandRequest.equals("2104"))
    {
      if (liveData->params.displayScreen != SCREEN_CELLS && liveData->params.displayScreenAutoMode != SCREEN_CELLS)
        return false;
    }
  }

  return true;
}

/**
   loadTestData
*/
void CarHyundaiIoniqPHEV::loadTestData()
{

  // VMCU ATSH7E2
  liveData->currentAtshRequest = "ATSH7E2";
  // 2101
  liveData->commandRequest = "2101";
  liveData->responseRowMerged = "6101FFE0000009215A09061803000000000E773404200000000000";
  parseRowMerged();
  // 2102
  liveData->commandRequest = "2102";
  liveData->responseRowMerged = "6102FF80000001010000009522C570273A0F0D9199953900000000";
  parseRowMerged();

  // "ATSH7DF",
  liveData->currentAtshRequest = "ATSH7DF";

  // AIRCON / ACU ATSH7B3
  liveData->currentAtshRequest = "ATSH7B3";
  // 220100
  liveData->commandRequest = "220100";
  // liveData->responseRowMerged = "6201007E5007C8FF7A665D00A981FFFF81FF10FFFFFFFFFFFFFFFFFF44CAA7AD00FFFF01FFFF000000";
  liveData->responseRowMerged = "6201007FD0071E8FF726A6C00882EF0088EFFF10FF3FFFFFFFFFFFFFF4FF32556E6E00535FF00FFFF000000";
  parseRowMerged();
  // 220102
  liveData->commandRequest = "220102";
  liveData->responseRowMerged = "620102FF800000CA5E0101000101005100000000";
  parseRowMerged();

  // BMS ATSH7E4
  liveData->currentAtshRequest = "ATSH7E4";
  // 220101
  liveData->commandRequest = "2101";
  // liveData->responseRowMerged = "6101FFFFFFFFBD136826480300220F600B0B0B0B0B0B0B000CCD05CC0A00009100012C4A00012A1800006F37000069F700346CC30D01890000000003E800";
  liveData->responseRowMerged = "6101FFFFFFFF24125C16F883FFD10D481C191A1919191C0017B101B11100FF9000038F4100038F5C0001479F00013F2300D2714F4901540000000003E800"; // phev
  parseRowMerged();
  // 220102
  liveData->commandRequest = "2102";
  // liveData->responseRowMerged = "6102FFFFFFFFCDCDCDCDCDCDCDCDCDCCCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCCCDCDCD000000";
  liveData->responseRowMerged = "6102FFFFFFFFB2B2B2B2B2B2B2B2B1B2B2B2B2B2B2B1B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2000000";
  parseRowMerged();
  // 220103
  liveData->commandRequest = "2103";
  // liveData->responseRowMerged = "6103FFFFFFFFCDCDCDCDCDCDCCCDCDCDCDCDCDCDCDCDCCCDCDCCCDCDCDCDCDCDCDCCCDCDCDCC000000";
  liveData->responseRowMerged = "6103FFFFFFFFB2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2000000";
  parseRowMerged();
  // 220104
  liveData->commandRequest = "2104";
  // liveData->responseRowMerged = "6104FFFFFFFFCDCDCDCDCDCDCDCDCDCDCCCCCDCDCCCDCDCDCDCDCDCDCDCDCDCDCDCCCCCCCDCD000000";
  liveData->responseRowMerged = "6104FFFFFFFFB2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2000000";
  parseRowMerged();
  // 220105
  liveData->commandRequest = "2105";
  // liveData->responseRowMerged = "6105FFFFFFFF00000000000B0B0B0B0B0B0B136826480001500B0B03E80203E831C60031000000000000000000000000";
  liveData->responseRowMerged = "6105FFFFFFFF0064010064011011BA16F800110000000000000000FF0F0000000042006A1003E852C000000000000000";
  parseRowMerged();

  // BCM / TPMS ATSH7A0
  liveData->currentAtshRequest = "ATSH7A0";
  // 22c00b
  liveData->commandRequest = "22c00b";
  // liveData->responseRowMerged = "62C00BFFFF0000B73D0100B63D0100B43D0100B53C0100AAAAAAAA";
  liveData->responseRowMerged = "62C00BFFFF0000B14A0100B1450100B0450100B1470100AAAAAAAA"; // phev
  parseRowMerged();

  // ATSH7C6
  liveData->currentAtshRequest = "ATSH7C6";
  // 22b002
  liveData->commandRequest = "22B002";
  // liveData->responseRowMerged = "62B002E000000000AA003B0B0000000000000000";
  // liveData->responseRowMerged =  "62B002E00010027A90113F800200000000000000";
  liveData->responseRowMerged = "62B002E000010027A90113F800200000000000000";

  parseRowMerged();

  // ATSH770
  liveData->currentAtshRequest = "ATSH770";
  liveData->commandRequest = "22BC03";
  // liveData->responseRowMerged = "62BC03FDEE3C7300600000AAAA";
  liveData->responseRowMerged = "62BC03FFEE3C7340600000AAAA"; // phev
  parseRowMerged();
  liveData->commandRequest = "22BC06";
  // liveData->responseRowMerged = "62BC06B480000000000000AAAA";
  liveData->responseRowMerged = "62BC06B480000000000000AAAA";
  parseRowMerged();

  liveData->currentAtshRequest = "ATSH7D1";
  liveData->commandRequest = "22C101";
  liveData->responseRowMerged = "62C101FBD7E7600000001101EAFF00000000FF7FFF0030F4000000FFFF8E0307FB07FAFFFF0000FFFFFF000000000000";

  parseRowMerged();
  /*  liveData->params.batModule01TempC = 28;
    liveData->params.batModule02TempC = 29;
    liveData->params.batModule03TempC = 28;
    liveData->params.batModule04TempC = 30;
    //liveData->params.batTempC = liveData->hexToDecFromResponse(36, 38, 1, true);
    //liveData->params.batMaxC = liveData->hexToDecFromResponse(34, 36, 1, true);
    //liveData->params.batMinC = liveData->hexToDecFromResponse(36, 38, 1, true);

    // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
    liveData->params.batMinC = liveData->params.batMaxC = liveData->params.batModule01TempC;
    liveData->params.batMinC = (liveData->params.batModule02TempC < liveData->params.batMinC) ? liveData->params.batModule02TempC : liveData->params.batMinC ;
    liveData->params.batMinC = (liveData->params.batModule03TempC < liveData->params.batMinC) ? liveData->params.batModule03TempC : liveData->params.batMinC ;
    liveData->params.batMinC = (liveData->params.batModule04TempC < liveData->params.batMinC) ? liveData->params.batModule04TempC : liveData->params.batMinC ;
    liveData->params.batMaxC = (liveData->params.batModule02TempC > liveData->params.batMaxC) ? liveData->params.batModule02TempC : liveData->params.batMaxC ;
    liveData->params.batMaxC = (liveData->params.batModule03TempC > liveData->params.batMaxC) ? liveData->params.batModule03TempC : liveData->params.batMaxC ;
    liveData->params.batMaxC = (liveData->params.batModule04TempC > liveData->params.batMaxC) ? liveData->params.batModule04TempC : liveData->params.batMaxC ;
    liveData->params.batTempC = liveData->params.batMinC;

    //
    liveData->params.soc10ced[10] = 2200;
    liveData->params.soc10cec[10] = 2500;
    liveData->params.soc10odo[10] = 13000;
    liveData->params.soc10time[10] = 13000;
    liveData->params.soc10ced[9] = liveData->params.soc10ced[10] + 6.4;
    liveData->params.soc10cec[9] = liveData->params.soc10cec[10] + 0;
    liveData->params.soc10odo[9] = liveData->params.soc10odo[10] + 30;
    liveData->params.soc10time[9] = liveData->params.soc10time[10] + 900;
    liveData->params.soc10ced[8] = liveData->params.soc10ced[9] + 6.8;
    liveData->params.soc10cec[8] = liveData->params.soc10cec[9] + 0;
    liveData->params.soc10odo[8] = liveData->params.soc10odo[9] + 30;
    liveData->params.soc10time[8] = liveData->params.soc10time[9] + 900;
    liveData->params.soc10ced[7] = liveData->params.soc10ced[8] + 7.2;
    liveData->params.soc10cec[7] = liveData->params.soc10cec[8] + 0.6;
    liveData->params.soc10odo[7] = liveData->params.soc10odo[8] + 30;
    liveData->params.soc10time[7] = liveData->params.soc10time[8] + 900;
    liveData->params.soc10ced[6] = liveData->params.soc10ced[7] + 6.7;
    liveData->params.soc10cec[6] = liveData->params.soc10cec[7] + 0;
    liveData->params.soc10odo[6] = liveData->params.soc10odo[7] + 30;
    liveData->params.soc10time[6] = liveData->params.soc10time[7] + 900;
    liveData->params.soc10ced[5] = liveData->params.soc10ced[6] + 6.7;
    liveData->params.soc10cec[5] = liveData->params.soc10cec[6] + 0;
    liveData->params.soc10odo[5] = liveData->params.soc10odo[6] + 30;
    liveData->params.soc10time[5] = liveData->params.soc10time[6] + 900;
    liveData->params.soc10ced[4] = liveData->params.soc10ced[5] + 6.4;
    liveData->params.soc10cec[4] = liveData->params.soc10cec[5] + 0.3;
    liveData->params.soc10odo[4] = liveData->params.soc10odo[5] + 30;
    liveData->params.soc10time[4] = liveData->params.soc10time[5] + 900;
    liveData->params.soc10ced[3] = liveData->params.soc10ced[4] + 6.4;
    liveData->params.soc10cec[3] = liveData->params.soc10cec[4] + 0;
    liveData->params.soc10odo[3] = liveData->params.soc10odo[4] + 30;
    liveData->params.soc10time[3] = liveData->params.soc10time[4] + 900;
    liveData->params.soc10ced[2] = liveData->params.soc10ced[3] + 5.4;
    liveData->params.soc10cec[2] = liveData->params.soc10cec[3] + 0.1;
    liveData->params.soc10odo[2] = liveData->params.soc10odo[3] + 30;
    liveData->params.soc10time[2] = liveData->params.soc10time[3] + 900;
    liveData->params.soc10ced[1] = liveData->params.soc10ced[2] + 6.2;
    liveData->params.soc10cec[1] = liveData->params.soc10cec[2] + 0.1;
    liveData->params.soc10odo[1] = liveData->params.soc10odo[2] + 30;
    liveData->params.soc10time[1] = liveData->params.soc10time[2] + 900;
    liveData->params.soc10ced[0] = liveData->params.soc10ced[1] + 2.9;
    liveData->params.soc10cec[0] = liveData->params.soc10cec[1] + 0.5;
    liveData->params.soc10odo[0] = liveData->params.soc10odo[1] + 15;
    liveData->params.soc10time[0] = liveData->params.soc10time[1] + 900;
  */
}
std::vector<String> CarHyundaiIoniqPHEV::customMenu(int16_t menuId)
{
  if (menuId == MENU_CAR_COMMANDS)
    return {
        "vessOn=VESS 5sec.",
        "doorsUnlock=Unlock doors",
        "doorsLock=Lock doors",
        "chargeCableLockOff=Charge cable lock off",
        "chargeCableLockOn=Charge cable lock on",
        "roomLampOff=Room lamp off",
        "roomLampOn=Room lamp on",
        "luggageLampOff=Luggage lamp off",
        "luggageLampOn=Luggage lamp on",
        "mirrorsUnfold=Unfold mirrors",
        "mirrorsFold=Fold mirrors",
        "heatSteeringWheelOff=Heat steering wheel off",
        "heatSteeringWheelOn=Heat steering wheel on",
        "clusterIndicatorsOff=Cluster indicators off",
        "clusterIndicatorsOn=Cluster indicators on",
        "turnSignalLeftOff=Turn signal left off",
        "turnSignalLeftOn=Turn signal left on",
        "turnSignalRightOff=Turn signal right off",
        "turnSignalRightOn=Turn signal right on",
        "headLightLowOff=Head light low off",
        "headLightLowOn=Head light low on",
        "headLightHighOff=Head light high off",
        "headLightHighOn=Head light high on",
        "frontFogLightOff=Front fog light off",
        "frontFogLightOn=Front fog light on",
        "rearLightOff=Rear light off",
        "rearLightOn=Rear light on",
        "rearFogLightOff=Rear fog light off",
        "rearFogLightOn=Rear fog light on",
        "rearDefoggerOff=Rear deffoger off",
        "rearDefoggerOn=Rear deffoger on",
        "rearLeftBrakeLightOff=Left brake light off",
        "rearLeftBrakeLightOn=Left brake light on",
        "rearRightBrakeLightOff=Right brake light off",
        "rearRightBrakeLightOn=Right brake light on",
    };

  return {};
}

/**
 * Execute custom command
 */

void CarHyundaiIoniqPHEV::carCommand(const String &cmd)
{
  if (cmd.equals("vessOn"))
  {
    ioniqPhevCarControl(0x736, "2FF01103");
  }
  if (cmd.equals("doorsUnlock"))
  {
    ioniqPhevCarControl(0x770, "2FBC1103");
  }
  if (cmd.equals("doorsLock"))
  {
    ioniqPhevCarControl(0x770, "2FBC1003");
  }
  if (cmd.equals("chargeCableLockOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC4103");
  }
  if (cmd.equals("chargeCableLockOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC3F03");
  }
  if (cmd.equals("roomLampOff"))
  {
    ioniqPhevCarControl(0x7A0, "2FB01900");
  }
  if (cmd.equals("roomLampOn"))
  {
    ioniqPhevCarControl(0x7A0, "2FB01903");
  }
  if (cmd.equals("luggageLampOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC1C00");
  }
  if (cmd.equals("luggageLampOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC1C03");
  }
  if (cmd.equals("mirrorsUnfold"))
  {
    ioniqPhevCarControl(0x7A0, "2FB05C03");
  }
  if (cmd.equals("mirrorsFold"))
  {
    ioniqPhevCarControl(0x7A0, "2FB05B03");
  }
  if (cmd.equals("heatSteeringWheelOff"))
  {
    ioniqPhevCarControl(0x7A0, "2FB05900"); // heat power
    ioniqPhevCarControl(0x7A0, "2FB05A00"); // LED indicator
  }
  if (cmd.equals("heatSteeringWheelOn"))
  {
    ioniqPhevCarControl(0x7A0, "2FB05903"); // heat power
    ioniqPhevCarControl(0x7A0, "2FB05A03"); // LED indicator
  }
  if (cmd.equals("clusterIndicatorsOff"))
  {
    ioniqPhevCarControl(0x7C6, "2FB00100");
  }
  if (cmd.equals("clusterIndicatorsOn"))
  {
    ioniqPhevCarControl(0x7C6, "2FB00103");
  }
  if (cmd.equals("turnSignalLeftOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC1500");
  }
  if (cmd.equals("turnSignalLeftOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC1503");
  }
  if (cmd.equals("turnSignalRightOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC1600");
  }
  if (cmd.equals("turnSignalRightOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC1603");
  }
  if (cmd.equals("headLightLowOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC0100");
  }
  if (cmd.equals("headLightLowOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC0103");
  }
  if (cmd.equals("headLightHighOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC0200");
  }
  if (cmd.equals("headLightHighOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC0203");
  }
  if (cmd.equals("frontFogLightOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC0300");
  }
  if (cmd.equals("frontFogLightOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC0303");
  }
  if (cmd.equals("rearLightOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC0400");
  }
  if (cmd.equals("rearLightOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC0403");
  }
  if (cmd.equals("rearFogLightOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC0800");
  }
  if (cmd.equals("rearFogLightOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC0803");
  }
  if (cmd.equals("rearDefoggerOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC0C00");
  }
  if (cmd.equals("rearDefoggerOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC0C03");
  }
  if (cmd.equals("rearLeftBrakeLightOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC2B00");
  }
  if (cmd.equals("rearLeftBrakeLightOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC2B03");
  }
  if (cmd.equals("rearRightBrakeLightOff"))
  {
    ioniqPhevCarControl(0x770, "2FBC2C00");
  }
  if (cmd.equals("rearRightBrakeLightOn"))
  {
    ioniqPhevCarControl(0x770, "2FBC2C03");
  }
}

/**
 * Eniro cmds
 */
void CarHyundaiIoniqPHEV::ioniqPhevCarControl(const uint16_t pid, const String &cmd)
{
  // syslog->println("EXECUTING COMMAND");
  // syslog->println(cmd);
  commInterface->sendPID(pid, "3E"); // SET TESTER PRESENT
  delay(10);
  for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
  {
    if (commInterface->receivePID() != 0xff)
      break;
    delay(20);
  }
  delay(liveData->delayBetweenCommandsMs);

  commInterface->sendPID(pid, "1003"); // CHANGE SESSION
  delay(10);
  for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
  {
    if (commInterface->receivePID() != 0xff)
    {
      // WAIT FOR POSITIVE ANSWER
      if (liveData->responseRowMerged.equals("5003"))
      {
        break;
      }
    }
    delay(20);
  }
  delay(liveData->delayBetweenCommandsMs);

  // EXECUTE COMMAND
  commInterface->sendPID(pid, cmd);
  syslog->setDebugLevel(DEBUG_COMM);
  delay(10);
  for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
  {
    if (commInterface->receivePID() != 0xff)
      break;
    delay(20);
  }
  delay(liveData->delayBetweenCommandsMs);

  syslog->setDebugLevel(liveData->settings.debugLevel);
}