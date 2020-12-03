#ifndef CARKIAENIRO_CPP
#define CARKIAENIRO_CPP

/* 
 *    eNiro/Kona chargings limits depending on battery temperature (min.value of 01-04 battery module)
  >= 35°C BMS allows max 180A
  >= 25°C without limit (200A)
  >= 15°C BMS allows max 120A
  >= 5°C BMS allows max 90A
  >= 1°C BMS allows max 60A
  <= 0°C BMS allows max 40A
 */

#include <Arduino.h>
#include <stdint.h>
#include <WString.h>
#include <String.h>
#include <sys/time.h>
#include "LiveData.h"
#include "CarKiaEniro.h"

#define commandQueueCountKiaENiro 30
#define commandQueueLoopFromKiaENiro 10

/**
 * activateCommandQueue
 */
void CarKiaEniro::activateCommandQueue() {

  String commandQueueKiaENiro[commandQueueCountKiaENiro] = {
    "AT Z",      // Reset all
    "AT I",      // Print the version ID
    "AT S0",     // Printing of spaces on
    "AT E0",     // Echo off
    "AT L0",     // Linefeeds off
    "AT SP 6",   // Select protocol to ISO 15765-4 CAN (11 bit ID, 500 kbit/s)
    //"AT AL",     // Allow Long (>7 byte) messages
    //"AT AR",     // Automatically receive
    //"AT H1",     // Headers on (debug only)
    //"AT D1",     // Display of the DLC on
    //"AT CAF0",   // Automatic formatting off
    ////"AT AT0",     // disabled adaptive timing
    "AT DP",
    "AT ST16",    // reduced timeout to 1, orig.16

    // Loop from (KIA ENIRO)

    // ABS / ESP + AHB
    "ATSH7D1",
    "22C101",     // brake, park/drive mode

    // IGPM
    "ATSH770",
    "22BC03",     // low beam
    "22BC06",     // brake light

    // VMCU
    "ATSH7E2",
    "2101",     // speed, ...
    "2102",     // aux, ...

    // BMS
    "ATSH7E4",
    "220101",   // power kw, ...
    "220102",   // cell voltages
    "220103",   // cell voltages
    "220104",   // cell voltages
    "220105",   // soh, soc, ..
    "220106",   // cooling water temp

    // Aircondition
    "ATSH7B3",
    "220100",   // in/out temp
    "220102",   // coolant temp1, 2

    // BCM / TPMS
    "ATSH7A0",
    "22c00b",   // tire pressure/temp

    // CLUSTER MODULE
    "ATSH7C6",
    "22B002",   // odo

  };

  // 39 or 64 kWh model?
  liveData->params.batModuleTempCount = 4;
  liveData->params.batteryTotalAvailableKWh = 64;
  // =(I18*0,615)*(1+(I18*0,0008)) soc to kwh niro ev 2020
  if (liveData->settings.carType == CAR_KIA_ENIRO_2020_39 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_39) {
    liveData->params.batteryTotalAvailableKWh = 39.2;
  }

  //  Empty and fill command queue
  for (int i = 0; i < 300; i++) {
    liveData->commandQueue[i] = "";
  }
  for (int i = 0; i < commandQueueCountKiaENiro; i++) {
    liveData->commandQueue[i] = commandQueueKiaENiro[i];
  }

  liveData->commandQueueLoopFrom = commandQueueLoopFromKiaENiro;
  liveData->commandQueueCount = commandQueueCountKiaENiro;
}

/**
 * parseRowMerged
 */
void CarKiaEniro::parseRowMerged() {

  bool tempByte;

  // ABS / ESP + AHB 7D1
  if (liveData->currentAtshRequest.equals("ATSH7D1")) {
    if (liveData->commandRequest.equals("22C101")) {
      uint8_t driveMode = liveData->hexToDec(liveData->responseRowMerged.substring(22, 24).c_str(), 1, false);
      liveData->params.forwardDriveMode = (driveMode == 4);
      liveData->params.reverseDriveMode = (driveMode == 2);
      liveData->params.parkModeOrNeutral  = (driveMode == 1);
    }
  }

  // IGPM
  if (liveData->currentAtshRequest.equals("ATSH770")) {
    if (liveData->commandRequest.equals("22BC03")) {
      tempByte = liveData->hexToDec(liveData->responseRowMerged.substring(16, 18).c_str(), 1, false);
      liveData->params.ignitionOnPrevious = liveData->params.ignitionOn;
      liveData->params.ignitionOn = (bitRead(tempByte, 5) == 1);
      if (liveData->params.ignitionOnPrevious && !liveData->params.ignitionOn)
        liveData->params.automaticShutdownTimer = liveData->params.currentTime;

      liveData->params.lightInfo = liveData->hexToDec(liveData->responseRowMerged.substring(18, 20).c_str(), 1, false);
      liveData->params.headLights = (bitRead(liveData->params.lightInfo, 5) == 1);
      liveData->params.dayLights = (bitRead(liveData->params.lightInfo, 3) == 1);
    }
    if (liveData->commandRequest.equals("22BC06")) {
      liveData->params.brakeLightInfo = liveData->hexToDec(liveData->responseRowMerged.substring(14, 16).c_str(), 1, false);
      liveData->params.brakeLights = (bitRead(liveData->params.brakeLightInfo, 5) == 1);
    }
  }

  // VMCU 7E2
  if (liveData->currentAtshRequest.equals("ATSH7E2")) {
    if (liveData->commandRequest.equals("2101")) {
      liveData->params.speedKmh = liveData->hexToDec(liveData->responseRowMerged.substring(32, 36).c_str(), 2, false) * 0.0155; // / 100.0 *1.609 = real to gps is 1.750
      if (liveData->params.speedKmh < -99 || liveData->params.speedKmh > 200)
        liveData->params.speedKmh = 0;
    }
    if (liveData->commandRequest.equals("2102")) {
      liveData->params.auxPerc = liveData->hexToDec(liveData->responseRowMerged.substring(50, 52).c_str(), 1, false);
      liveData->params.auxCurrentAmp = - liveData->hexToDec(liveData->responseRowMerged.substring(46, 50).c_str(), 2, true) / 1000.0;
    }
  }

  // Cluster module 7c6
  if (liveData->currentAtshRequest.equals("ATSH7C6")) {
    if (liveData->commandRequest.equals("22B002")) {
      liveData->params.odoKm = float(strtol(liveData->responseRowMerged.substring(18, 24).c_str(), 0, 16));
    }
  }

  // Aircon 7b3
  if (liveData->currentAtshRequest.equals("ATSH7B3")) {
    if (liveData->commandRequest.equals("220100")) {
      liveData->params.indoorTemperature = (liveData->hexToDec(liveData->responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
      liveData->params.outdoorTemperature = (liveData->hexToDec(liveData->responseRowMerged.substring(18, 20).c_str(), 1, false) / 2) - 40;
    }
    if (liveData->commandRequest.equals("220102") && liveData->responseRowMerged.substring(12, 14) == "00") {
      liveData->params.coolantTemp1C = (liveData->hexToDec(liveData->responseRowMerged.substring(14, 16).c_str(), 1, false) / 2) - 40;
      liveData->params.coolantTemp2C = (liveData->hexToDec(liveData->responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
    }
  }

  // BMS 7e4
  if (liveData->currentAtshRequest.equals("ATSH7E4")) {
    if (liveData->commandRequest.equals("220101")) {
      liveData->params.cumulativeEnergyChargedKWh = float(strtol(liveData->responseRowMerged.substring(82, 90).c_str(), 0, 16)) / 10.0;
      if (liveData->params.cumulativeEnergyChargedKWhStart == -1)
        liveData->params.cumulativeEnergyChargedKWhStart = liveData->params.cumulativeEnergyChargedKWh;
      liveData->params.cumulativeEnergyDischargedKWh = float(strtol(liveData->responseRowMerged.substring(90, 98).c_str(), 0, 16)) / 10.0;
      if (liveData->params.cumulativeEnergyDischargedKWhStart == -1)
        liveData->params.cumulativeEnergyDischargedKWhStart = liveData->params.cumulativeEnergyDischargedKWh;
      liveData->params.availableChargePower = float(strtol(liveData->responseRowMerged.substring(16, 20).c_str(), 0, 16)) / 100.0;
      liveData->params.availableDischargePower = float(strtol(liveData->responseRowMerged.substring(20, 24).c_str(), 0, 16)) / 100.0;
      //liveData->params.isolationResistanceKOhm = liveData->hexToDec(liveData->responseRowMerged.substring(118, 122).c_str(), 2, true);
      liveData->params.batFanStatus = liveData->hexToDec(liveData->responseRowMerged.substring(60, 62).c_str(), 2, true);
      liveData->params.batFanFeedbackHz = liveData->hexToDec(liveData->responseRowMerged.substring(62, 64).c_str(), 2, true);
      liveData->params.auxVoltage = liveData->hexToDec(liveData->responseRowMerged.substring(64, 66).c_str(), 2, true) / 10.0;
      liveData->params.batPowerAmp = - liveData->hexToDec(liveData->responseRowMerged.substring(26, 30).c_str(), 2, true) / 10.0;
      liveData->params.batVoltage = liveData->hexToDec(liveData->responseRowMerged.substring(30, 34).c_str(), 2, false) / 10.0;
      liveData->params.batPowerKw = (liveData->params.batPowerAmp * liveData->params.batVoltage) / 1000.0;
      if (liveData->params.batPowerKw < 0) // Reset charging start time
        liveData->params.chargingStartTime = liveData->params.currentTime;
      liveData->params.batPowerKwh100 = liveData->params.batPowerKw / liveData->params.speedKmh * 100;
      liveData->params.batCellMaxV = liveData->hexToDec(liveData->responseRowMerged.substring(52, 54).c_str(), 1, false) / 50.0;
      liveData->params.batCellMinV = liveData->hexToDec(liveData->responseRowMerged.substring(56, 58).c_str(), 1, false) / 50.0;
      liveData->params.batModuleTempC[0] = liveData->hexToDec(liveData->responseRowMerged.substring(38, 40).c_str(), 1, true);
      liveData->params.batModuleTempC[1] = liveData->hexToDec(liveData->responseRowMerged.substring(40, 42).c_str(), 1, true);
      liveData->params.batModuleTempC[2] = liveData->hexToDec(liveData->responseRowMerged.substring(42, 44).c_str(), 1, true);
      liveData->params.batModuleTempC[3] = liveData->hexToDec(liveData->responseRowMerged.substring(44, 46).c_str(), 1, true);
      liveData->params.motorRpm = liveData->hexToDec(liveData->responseRowMerged.substring(112, 116).c_str(), 2, false);
      //liveData->params.batTempC = liveData->hexToDec(liveData->responseRowMerged.substring(36, 38).c_str(), 1, true);
      //liveData->params.batMaxC = liveData->hexToDec(liveData->responseRowMerged.substring(34, 36).c_str(), 1, true);
      //liveData->params.batMinC = liveData->hexToDec(liveData->responseRowMerged.substring(36, 38).c_str(), 1, true);

      // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
      liveData->params.batMinC = liveData->params.batMaxC = liveData->params.batModuleTempC[0];
      for (uint16_t i = 1; i < liveData->params.batModuleTempCount; i++) {
        if (liveData->params.batModuleTempC[i] < liveData->params.batMinC)
          liveData->params.batMinC = liveData->params.batModuleTempC[i];
        if (liveData->params.batModuleTempC[i] > liveData->params.batMaxC)
          liveData->params.batMaxC = liveData->params.batModuleTempC[i];
      }
      liveData->params.batTempC = liveData->params.batMinC;

      liveData->params.batInletC = liveData->hexToDec(liveData->responseRowMerged.substring(50, 52).c_str(), 1, true);
      if (liveData->params.speedKmh < 10 && liveData->params.batPowerKw >= 1 && liveData->params.socPerc > 0 && liveData->params.socPerc <= 100) {
        if ( liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] < 0 || liveData->params.batPowerKw < liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)])
          liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
        if ( liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] < 0 || liveData->params.batPowerKw > liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)])
          liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
        liveData->params.chargingGraphBatMinTempC[int(liveData->params.socPerc)] = liveData->params.batMinC;
        liveData->params.chargingGraphBatMaxTempC[int(liveData->params.socPerc)] = liveData->params.batMaxC;
        liveData->params.chargingGraphHeaterTempC[int(liveData->params.socPerc)] = liveData->params.batHeaterC;
        liveData->params.chargingGraphWaterCoolantTempC[int(liveData->params.socPerc)] = liveData->params.coolingWaterTempC;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("220102") && liveData->responseRowMerged.substring(12, 14) == "FF") {
      for (int i = 0; i < 32; i++) {
        liveData->params.cellVoltage[i] = liveData->hexToDec(liveData->responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("220103")) {
      for (int i = 0; i < 32; i++) {
        liveData->params.cellVoltage[32 + i] = liveData->hexToDec(liveData->responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("220104")) {
      for (int i = 0; i < 32; i++) {
        liveData->params.cellVoltage[64 + i] = liveData->hexToDec(liveData->responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("220105")) {
      liveData->params.socPercPrevious = liveData->params.socPerc;
      liveData->params.sohPerc = liveData->hexToDec(liveData->responseRowMerged.substring(56, 60).c_str(), 2, false) / 10.0;
      liveData->params.socPerc = liveData->hexToDec(liveData->responseRowMerged.substring(68, 70).c_str(), 1, false) / 2.0;

      // Soc10ced table, record x0% CEC/CED table (ex. 90%->89%, 80%->79%)
      if (liveData->params.socPercPrevious - liveData->params.socPerc > 0) {
        byte index = (int(liveData->params.socPerc) == 4) ? 0 : (int)(liveData->params.socPerc / 10) + 1;
        if ((int(liveData->params.socPerc) % 10 == 9 || int(liveData->params.socPerc) == 4) && liveData->params.soc10ced[index] == -1) {
          liveData->params.soc10ced[index] = liveData->params.cumulativeEnergyDischargedKWh;
          liveData->params.soc10cec[index] = liveData->params.cumulativeEnergyChargedKWh;
          liveData->params.soc10odo[index] = liveData->params.odoKm;
          liveData->params.soc10time[index] = liveData->params.currentTime;
        }
      }
      liveData->params.bmsUnknownTempA = liveData->hexToDec(liveData->responseRowMerged.substring(30, 32).c_str(), 1, true);
      liveData->params.batHeaterC = liveData->hexToDec(liveData->responseRowMerged.substring(52, 54).c_str(), 1, true);
      liveData->params.bmsUnknownTempB = liveData->hexToDec(liveData->responseRowMerged.substring(82, 84).c_str(), 1, true);
      //
      for (int i = 30; i < 32; i++) { // ai/aj position
        liveData->params.cellVoltage[96 - 30 + i] = liveData->hexToDec(liveData->responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("220106")) {
      liveData->params.coolingWaterTempC = liveData->hexToDec(liveData->responseRowMerged.substring(14, 16).c_str(), 1, false);
      liveData->params.bmsUnknownTempC = liveData->hexToDec(liveData->responseRowMerged.substring(18, 20).c_str(), 1, true);
      liveData->params.bmsUnknownTempD = liveData->hexToDec(liveData->responseRowMerged.substring(46, 48).c_str(), 1, true);
    }
  }

  // TPMS 7a0
  if (liveData->currentAtshRequest.equals("ATSH7A0")) {
    if (liveData->commandRequest.equals("22c00b")) {
      liveData->params.tireFrontLeftPressureBar = liveData->hexToDec(liveData->responseRowMerged.substring(14, 16).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireFrontRightPressureBar = liveData->hexToDec(liveData->responseRowMerged.substring(22, 24).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireRearRightPressureBar = liveData->hexToDec(liveData->responseRowMerged.substring(30, 32).c_str(), 2, false) / 72.51886900361;    // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireRearLeftPressureBar = liveData->hexToDec(liveData->responseRowMerged.substring(38, 40).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireFrontLeftTempC = liveData->hexToDec(liveData->responseRowMerged.substring(16, 18).c_str(), 2, false)  - 50;      // === OK Valid
      liveData->params.tireFrontRightTempC = liveData->hexToDec(liveData->responseRowMerged.substring(24, 26).c_str(), 2, false) - 50;      // === OK Valid
      liveData->params.tireRearRightTempC = liveData->hexToDec(liveData->responseRowMerged.substring(32, 34).c_str(), 2, false) - 50;     // === OK Valid
      liveData->params.tireRearLeftTempC = liveData->hexToDec(liveData->responseRowMerged.substring(40, 42).c_str(), 2, false) - 50;     // === OK Valid
    }
  }
}

/**
 * loadTestData
 */
void CarKiaEniro::loadTestData() {

  // IGPM
  liveData->currentAtshRequest = "ATSH770";
  // 22BC03
  liveData->commandRequest = "22BC03";
  liveData->responseRowMerged = "62BC03FDEE7C730A600000AAAA";
  parseRowMerged();

  // ABS / ESP + AHB ATSH7D1
  liveData->currentAtshRequest = "ATSH7D1";
  // 2101
  liveData->commandRequest = "22C101";
  liveData->responseRowMerged = "62C1015FD7E7D0FFFF00FF04D0D400000000FF7EFF0030F5010000FFFF7F6307F207FE05FF00FF3FFFFFAAAAAAAAAAAA";
  parseRowMerged();

  // VMCU ATSH7E2
  liveData->currentAtshRequest = "ATSH7E2";
  // 2101
  liveData->commandRequest = "2101";
  liveData->responseRowMerged = "6101FFF8000009285A3B0648030000B4179D763404080805000000";
  parseRowMerged();
  // 2102
  liveData->commandRequest = "2102";
  liveData->responseRowMerged = "6102F8FFFC000101000000840FBF83BD33270680953033757F59291C76000001010100000007000000";
  liveData->responseRowMerged = "6102F8FFFC000101000000931CC77F4C39040BE09BA7385D8158832175000001010100000007000000";
  parseRowMerged();

  // "ATSH7DF",
  liveData->currentAtshRequest = "ATSH7DF";
  // 2106
  liveData->commandRequest = "2106";
  liveData->responseRowMerged = "6106FFFF800000000000000200001B001C001C000600060006000E000000010000000000000000013D013D013E013E00";
  parseRowMerged();

  // AIRCON / ACU ATSH7B3
  liveData->currentAtshRequest = "ATSH7B3";
  // 220100
  liveData->commandRequest = "220100";
  liveData->responseRowMerged = "6201007E5027C8FF7F765D05B95AFFFF5AFF11FFFFFFFFFFFF6AFFFF2DF0757630FFFF00FFFF000000";
  liveData->responseRowMerged = "6201007E5027C8FF867C58121010FFFF10FF8EFFFFFFFFFFFF10FFFF0DF0617900FFFF01FFFF000000";
  parseRowMerged();

  // BMS ATSH7E4
  liveData->currentAtshRequest = "ATSH7E4";
  // 220101
  liveData->commandRequest = "220101";
  liveData->responseRowMerged = "620101FFF7E7FF99000000000300B10EFE120F11100F12000018C438C30B00008400003864000035850000153A00001374000647010D017F0BDA0BDA03E8";
  liveData->responseRowMerged = "620101FFF7E7FFB3000000000300120F9B111011101011000014CC38CB3B00009100003A510000367C000015FB000013D3000690250D018E0000000003E8";
  parseRowMerged();
  // 220102
  liveData->commandRequest = "220102";
  liveData->responseRowMerged = "620102FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMerged();
  // 220103
  liveData->commandRequest = "220103";
  liveData->responseRowMerged = "620103FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCACBCACACFCCCBCBCBCBCBCBCBCBAAAA";
  parseRowMerged();
  // 220104
  liveData->commandRequest = "220104";
  liveData->responseRowMerged = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMerged();
  // 220105
  liveData->commandRequest = "220105";
  liveData->responseRowMerged = "620105003fff9000000000000000000F8A86012B4946500101500DAC03E800000000AC0000C7C701000F00000000AAAA";
  liveData->responseRowMerged = "620105003FFF90000000000000000014918E012927465000015013BB03E800000000BB0000CBCB01001300000000AAAA";
  parseRowMerged();
  // 220106
  liveData->commandRequest = "220106";
  liveData->responseRowMerged = "620106FFFFFFFF14001A00240000003A7C86B4B30000000928EA00";
  parseRowMerged();

  // BCM / TPMS ATSH7A0
  liveData->currentAtshRequest = "ATSH7A0";
  // 22c00b
  liveData->commandRequest = "22c00b";
  liveData->responseRowMerged = "62C00BFFFF0000B93D0100B43E0100B43D0100BB3C0100AAAAAAAA";
  parseRowMerged();

  // ATSH7C6
  liveData->currentAtshRequest = "ATSH7C6";
  // 22b002
  liveData->commandRequest = "22b002";
  liveData->responseRowMerged = "62B002E0000000FFB400330B0000000000000000";
  parseRowMerged();

  liveData->params.batModuleTempC[0] = 28;
  liveData->params.batModuleTempC[1] = 29;
  liveData->params.batModuleTempC[2] = 28;
  liveData->params.batModuleTempC[3] = 30;

  // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
  liveData->params.batMinC = liveData->params.batMaxC = liveData->params.batModuleTempC[0];
  for (uint16_t i = 1; i < liveData->params.batModuleTempCount; i++) {
    if (liveData->params.batModuleTempC[i] < liveData->params.batMinC)
      liveData->params.batMinC = liveData->params.batModuleTempC[i];
    if (liveData->params.batModuleTempC[i] > liveData->params.batMaxC)
      liveData->params.batMaxC = liveData->params.batModuleTempC[i];
  }
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

}

#endif // CARKIAENIRO_CPP
