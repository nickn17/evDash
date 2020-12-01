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
  this->liveData->params.batModuleTempCount = 4;
  this->liveData->params.batteryTotalAvailableKWh = 64;
  // =(I18*0,615)*(1+(I18*0,0008)) soc to kwh niro ev 2020
  if (this->liveData->settings.carType == CAR_KIA_ENIRO_2020_39 || this->liveData->settings.carType == CAR_HYUNDAI_KONA_2020_39) {
    this->liveData->params.batteryTotalAvailableKWh = 39.2;
  }

  //  Empty and fill command queue
  for (int i = 0; i < 300; i++) {
    this->liveData->commandQueue[i] = "";
  }
  for (int i = 0; i < commandQueueCountKiaENiro; i++) {
    this->liveData->commandQueue[i] = commandQueueKiaENiro[i];
  }

  this->liveData->commandQueueLoopFrom = commandQueueLoopFromKiaENiro;
  this->liveData->commandQueueCount = commandQueueCountKiaENiro;
}

/**
 * parseRowMerged
 */
void CarKiaEniro::parseRowMerged() {

  bool tempByte;

  // ABS / ESP + AHB 7D1
  if (this->liveData->currentAtshRequest.equals("ATSH7D1")) {
    if (this->liveData->commandRequest.equals("22C101")) {
      uint8_t driveMode = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(22, 24).c_str(), 1, false);
      this->liveData->params.forwardDriveMode = (driveMode == 4);
      this->liveData->params.reverseDriveMode = (driveMode == 2);
      this->liveData->params.parkModeOrNeutral  = (driveMode == 1);
    }
  }

  // IGPM
  if (this->liveData->currentAtshRequest.equals("ATSH770")) {
    if (this->liveData->commandRequest.equals("22BC03")) {
      tempByte = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(16, 18).c_str(), 1, false);
      this->liveData->params.ignitionOnPrevious = this->liveData->params.ignitionOn;
      this->liveData->params.ignitionOn = (bitRead(tempByte, 5) == 1);
      if (this->liveData->params.ignitionOnPrevious && !this->liveData->params.ignitionOn)
        this->liveData->params.automatickShutdownTimer = this->liveData->params.currentTime;

      this->liveData->params.lightInfo = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(18, 20).c_str(), 1, false);
      this->liveData->params.headLights = (bitRead(this->liveData->params.lightInfo, 5) == 1);
      this->liveData->params.dayLights = (bitRead(this->liveData->params.lightInfo, 3) == 1);
    }
    if (this->liveData->commandRequest.equals("22BC06")) {
      this->liveData->params.brakeLightInfo = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(14, 16).c_str(), 1, false);
      this->liveData->params.brakeLights = (bitRead(this->liveData->params.brakeLightInfo, 5) == 1);
    }
  }

  // VMCU 7E2
  if (this->liveData->currentAtshRequest.equals("ATSH7E2")) {
    if (this->liveData->commandRequest.equals("2101")) {
      this->liveData->params.speedKmh = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(32, 36).c_str(), 2, false) * 0.0155; // / 100.0 *1.609 = real to gps is 1.750
      if (this->liveData->params.speedKmh < -99 || this->liveData->params.speedKmh > 200)
        this->liveData->params.speedKmh = 0;
    }
    if (this->liveData->commandRequest.equals("2102")) {
      this->liveData->params.auxPerc = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(50, 52).c_str(), 1, false);
      this->liveData->params.auxCurrentAmp = - this->liveData->hexToDec(this->liveData->responseRowMerged.substring(46, 50).c_str(), 2, true) / 1000.0;
    }
  }

  // Cluster module 7c6
  if (this->liveData->currentAtshRequest.equals("ATSH7C6")) {
    if (this->liveData->commandRequest.equals("22B002")) {
      this->liveData->params.odoKm = float(strtol(this->liveData->responseRowMerged.substring(18, 24).c_str(), 0, 16));
    }
  }

  // Aircon 7b3
  if (this->liveData->currentAtshRequest.equals("ATSH7B3")) {
    if (this->liveData->commandRequest.equals("220100")) {
      this->liveData->params.indoorTemperature = (this->liveData->hexToDec(this->liveData->responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
      this->liveData->params.outdoorTemperature = (this->liveData->hexToDec(this->liveData->responseRowMerged.substring(18, 20).c_str(), 1, false) / 2) - 40;
    }
    if (this->liveData->commandRequest.equals("220102") && this->liveData->responseRowMerged.substring(12, 14) == "00") {
      this->liveData->params.coolantTemp1C = (this->liveData->hexToDec(this->liveData->responseRowMerged.substring(14, 16).c_str(), 1, false) / 2) - 40;
      this->liveData->params.coolantTemp2C = (this->liveData->hexToDec(this->liveData->responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
    }
  }

  // BMS 7e4
  if (this->liveData->currentAtshRequest.equals("ATSH7E4")) {
    if (this->liveData->commandRequest.equals("220101")) {
      this->liveData->params.cumulativeEnergyChargedKWh = float(strtol(this->liveData->responseRowMerged.substring(82, 90).c_str(), 0, 16)) / 10.0;
      if (this->liveData->params.cumulativeEnergyChargedKWhStart == -1)
        this->liveData->params.cumulativeEnergyChargedKWhStart = this->liveData->params.cumulativeEnergyChargedKWh;
      this->liveData->params.cumulativeEnergyDischargedKWh = float(strtol(this->liveData->responseRowMerged.substring(90, 98).c_str(), 0, 16)) / 10.0;
      if (this->liveData->params.cumulativeEnergyDischargedKWhStart == -1)
        this->liveData->params.cumulativeEnergyDischargedKWhStart = this->liveData->params.cumulativeEnergyDischargedKWh;
      this->liveData->params.availableChargePower = float(strtol(this->liveData->responseRowMerged.substring(16, 20).c_str(), 0, 16)) / 100.0;
      this->liveData->params.availableDischargePower = float(strtol(this->liveData->responseRowMerged.substring(20, 24).c_str(), 0, 16)) / 100.0;
      //this->liveData->params.isolationResistanceKOhm = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(118, 122).c_str(), 2, true);
      this->liveData->params.batFanStatus = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(60, 62).c_str(), 2, true);
      this->liveData->params.batFanFeedbackHz = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(62, 64).c_str(), 2, true);
      this->liveData->params.auxVoltage = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(64, 66).c_str(), 2, true) / 10.0;
      this->liveData->params.batPowerAmp = - this->liveData->hexToDec(this->liveData->responseRowMerged.substring(26, 30).c_str(), 2, true) / 10.0;
      this->liveData->params.batVoltage = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(30, 34).c_str(), 2, false) / 10.0;
      this->liveData->params.batPowerKw = (this->liveData->params.batPowerAmp * this->liveData->params.batVoltage) / 1000.0;
      if (this->liveData->params.batPowerKw < 0) // Reset charging start time
        this->liveData->params.chargingStartTime = this->liveData->params.currentTime;
      this->liveData->params.batPowerKwh100 = this->liveData->params.batPowerKw / this->liveData->params.speedKmh * 100;
      this->liveData->params.batCellMaxV = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(52, 54).c_str(), 1, false) / 50.0;
      this->liveData->params.batCellMinV = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(56, 58).c_str(), 1, false) / 50.0;
      this->liveData->params.batModuleTempC[0] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(38, 40).c_str(), 1, true);
      this->liveData->params.batModuleTempC[1] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(40, 42).c_str(), 1, true);
      this->liveData->params.batModuleTempC[2] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(42, 44).c_str(), 1, true);
      this->liveData->params.batModuleTempC[3] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(44, 46).c_str(), 1, true);
      this->liveData->params.motorRpm = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(112, 116).c_str(), 2, false);
      //this->liveData->params.batTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(36, 38).c_str(), 1, true);
      //this->liveData->params.batMaxC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(34, 36).c_str(), 1, true);
      //this->liveData->params.batMinC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(36, 38).c_str(), 1, true);

      // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
      this->liveData->params.batMinC = this->liveData->params.batMaxC = this->liveData->params.batModuleTempC[0];
      for (uint16_t i = 1; i < this->liveData->params.batModuleTempCount; i++) {
        if (this->liveData->params.batModuleTempC[i] < this->liveData->params.batMinC)
          this->liveData->params.batMinC = this->liveData->params.batModuleTempC[i];
        if (this->liveData->params.batModuleTempC[i] > this->liveData->params.batMaxC)
          this->liveData->params.batMaxC = this->liveData->params.batModuleTempC[i];
      }
      this->liveData->params.batTempC = this->liveData->params.batMinC;

      this->liveData->params.batInletC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(50, 52).c_str(), 1, true);
      if (this->liveData->params.speedKmh < 10 && this->liveData->params.batPowerKw >= 1 && this->liveData->params.socPerc > 0 && this->liveData->params.socPerc <= 100) {
        if ( this->liveData->params.chargingGraphMinKw[int(this->liveData->params.socPerc)] < 0 || this->liveData->params.batPowerKw < this->liveData->params.chargingGraphMinKw[int(this->liveData->params.socPerc)])
          this->liveData->params.chargingGraphMinKw[int(this->liveData->params.socPerc)] = this->liveData->params.batPowerKw;
        if ( this->liveData->params.chargingGraphMaxKw[int(this->liveData->params.socPerc)] < 0 || this->liveData->params.batPowerKw > this->liveData->params.chargingGraphMaxKw[int(this->liveData->params.socPerc)])
          this->liveData->params.chargingGraphMaxKw[int(this->liveData->params.socPerc)] = this->liveData->params.batPowerKw;
        this->liveData->params.chargingGraphBatMinTempC[int(this->liveData->params.socPerc)] = this->liveData->params.batMinC;
        this->liveData->params.chargingGraphBatMaxTempC[int(this->liveData->params.socPerc)] = this->liveData->params.batMaxC;
        this->liveData->params.chargingGraphHeaterTempC[int(this->liveData->params.socPerc)] = this->liveData->params.batHeaterC;
        this->liveData->params.chargingGraphWaterCoolantTempC[int(this->liveData->params.socPerc)] = this->liveData->params.coolingWaterTempC;
      }
    }
    // BMS 7e4
    if (this->liveData->commandRequest.equals("220102") && this->liveData->responseRowMerged.substring(12, 14) == "FF") {
      for (int i = 0; i < 32; i++) {
        this->liveData->params.cellVoltage[i] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (this->liveData->commandRequest.equals("220103")) {
      for (int i = 0; i < 32; i++) {
        this->liveData->params.cellVoltage[32 + i] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (this->liveData->commandRequest.equals("220104")) {
      for (int i = 0; i < 32; i++) {
        this->liveData->params.cellVoltage[64 + i] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (this->liveData->commandRequest.equals("220105")) {
      this->liveData->params.socPercPrevious = this->liveData->params.socPerc;
      this->liveData->params.sohPerc = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(56, 60).c_str(), 2, false) / 10.0;
      this->liveData->params.socPerc = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(68, 70).c_str(), 1, false) / 2.0;

      // Soc10ced table, record x0% CEC/CED table (ex. 90%->89%, 80%->79%)
      if (this->liveData->params.socPercPrevious - this->liveData->params.socPerc > 0) {
        byte index = (int(this->liveData->params.socPerc) == 4) ? 0 : (int)(this->liveData->params.socPerc / 10) + 1;
        if ((int(this->liveData->params.socPerc) % 10 == 9 || int(this->liveData->params.socPerc) == 4) && this->liveData->params.soc10ced[index] == -1) {
          this->liveData->params.soc10ced[index] = this->liveData->params.cumulativeEnergyDischargedKWh;
          this->liveData->params.soc10cec[index] = this->liveData->params.cumulativeEnergyChargedKWh;
          this->liveData->params.soc10odo[index] = this->liveData->params.odoKm;
          this->liveData->params.soc10time[index] = this->liveData->params.currentTime;
        }
      }
      this->liveData->params.bmsUnknownTempA = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(30, 32).c_str(), 1, true);
      this->liveData->params.batHeaterC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(52, 54).c_str(), 1, true);
      this->liveData->params.bmsUnknownTempB = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(82, 84).c_str(), 1, true);
      //
      for (int i = 30; i < 32; i++) { // ai/aj position
        this->liveData->params.cellVoltage[96 - 30 + i] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (this->liveData->commandRequest.equals("220106")) {
      this->liveData->params.coolingWaterTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(14, 16).c_str(), 1, false);
      this->liveData->params.bmsUnknownTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(18, 20).c_str(), 1, true);
      this->liveData->params.bmsUnknownTempD = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(46, 48).c_str(), 1, true);
    }
  }

  // TPMS 7a0
  if (this->liveData->currentAtshRequest.equals("ATSH7A0")) {
    if (this->liveData->commandRequest.equals("22c00b")) {
      this->liveData->params.tireFrontLeftPressureBar = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(14, 16).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
      this->liveData->params.tireFrontRightPressureBar = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(22, 24).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
      this->liveData->params.tireRearRightPressureBar = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(30, 32).c_str(), 2, false) / 72.51886900361;    // === OK Valid *0.2 / 14.503773800722
      this->liveData->params.tireRearLeftPressureBar = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(38, 40).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
      this->liveData->params.tireFrontLeftTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(16, 18).c_str(), 2, false)  - 50;      // === OK Valid
      this->liveData->params.tireFrontRightTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(24, 26).c_str(), 2, false) - 50;      // === OK Valid
      this->liveData->params.tireRearRightTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(32, 34).c_str(), 2, false) - 50;     // === OK Valid
      this->liveData->params.tireRearLeftTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(40, 42).c_str(), 2, false) - 50;     // === OK Valid
    }
  }
}

/**
 * loadTestData
 */
void CarKiaEniro::loadTestData() {

  // IGPM
  this->liveData->currentAtshRequest = "ATSH770";
  // 22BC03
  this->liveData->commandRequest = "22BC03";
  this->liveData->responseRowMerged = "62BC03FDEE7C730A600000AAAA";
  this->parseRowMerged();

  // ABS / ESP + AHB ATSH7D1
  this->liveData->currentAtshRequest = "ATSH7D1";
  // 2101
  this->liveData->commandRequest = "22C101";
  this->liveData->responseRowMerged = "62C1015FD7E7D0FFFF00FF04D0D400000000FF7EFF0030F5010000FFFF7F6307F207FE05FF00FF3FFFFFAAAAAAAAAAAA";
  this->parseRowMerged();

  // VMCU ATSH7E2
  this->liveData->currentAtshRequest = "ATSH7E2";
  // 2101
  this->liveData->commandRequest = "2101";
  this->liveData->responseRowMerged = "6101FFF8000009285A3B0648030000B4179D763404080805000000";
  this->parseRowMerged();
  // 2102
  this->liveData->commandRequest = "2102";
  this->liveData->responseRowMerged = "6102F8FFFC000101000000840FBF83BD33270680953033757F59291C76000001010100000007000000";
  this->liveData->responseRowMerged = "6102F8FFFC000101000000931CC77F4C39040BE09BA7385D8158832175000001010100000007000000";
  this->parseRowMerged();

  // "ATSH7DF",
  this->liveData->currentAtshRequest = "ATSH7DF";
  // 2106
  this->liveData->commandRequest = "2106";
  this->liveData->responseRowMerged = "6106FFFF800000000000000200001B001C001C000600060006000E000000010000000000000000013D013D013E013E00";
  this->parseRowMerged();

  // AIRCON / ACU ATSH7B3
  this->liveData->currentAtshRequest = "ATSH7B3";
  // 220100
  this->liveData->commandRequest = "220100";
  this->liveData->responseRowMerged = "6201007E5027C8FF7F765D05B95AFFFF5AFF11FFFFFFFFFFFF6AFFFF2DF0757630FFFF00FFFF000000";
  this->liveData->responseRowMerged = "6201007E5027C8FF867C58121010FFFF10FF8EFFFFFFFFFFFF10FFFF0DF0617900FFFF01FFFF000000";
  this->parseRowMerged();

  // BMS ATSH7E4
  this->liveData->currentAtshRequest = "ATSH7E4";
  // 220101
  this->liveData->commandRequest = "220101";
  this->liveData->responseRowMerged = "620101FFF7E7FF99000000000300B10EFE120F11100F12000018C438C30B00008400003864000035850000153A00001374000647010D017F0BDA0BDA03E8";
  this->liveData->responseRowMerged = "620101FFF7E7FFB3000000000300120F9B111011101011000014CC38CB3B00009100003A510000367C000015FB000013D3000690250D018E0000000003E8";
  this->parseRowMerged();
  // 220102
  this->liveData->commandRequest = "220102";
  this->liveData->responseRowMerged = "620102FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  this->parseRowMerged();
  // 220103
  this->liveData->commandRequest = "220103";
  this->liveData->responseRowMerged = "620103FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCACBCACACFCCCBCBCBCBCBCBCBCBAAAA";
  this->parseRowMerged();
  // 220104
  this->liveData->commandRequest = "220104";
  this->liveData->responseRowMerged = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  this->parseRowMerged();
  // 220105
  this->liveData->commandRequest = "220105";
  this->liveData->responseRowMerged = "620105003fff9000000000000000000F8A86012B4946500101500DAC03E800000000AC0000C7C701000F00000000AAAA";
  this->liveData->responseRowMerged = "620105003FFF90000000000000000014918E012927465000015013BB03E800000000BB0000CBCB01001300000000AAAA";
  this->parseRowMerged();
  // 220106
  this->liveData->commandRequest = "220106";
  this->liveData->responseRowMerged = "620106FFFFFFFF14001A00240000003A7C86B4B30000000928EA00";
  this->parseRowMerged();

  // BCM / TPMS ATSH7A0
  this->liveData->currentAtshRequest = "ATSH7A0";
  // 22c00b
  this->liveData->commandRequest = "22c00b";
  this->liveData->responseRowMerged = "62C00BFFFF0000B93D0100B43E0100B43D0100BB3C0100AAAAAAAA";
  this->parseRowMerged();

  // ATSH7C6
  this->liveData->currentAtshRequest = "ATSH7C6";
  // 22b002
  this->liveData->commandRequest = "22b002";
  this->liveData->responseRowMerged = "62B002E0000000FFB400330B0000000000000000";
  this->parseRowMerged();

  this->liveData->params.batModuleTempC[0] = 28;
  this->liveData->params.batModuleTempC[1] = 29;
  this->liveData->params.batModuleTempC[2] = 28;
  this->liveData->params.batModuleTempC[3] = 30;

  // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
  this->liveData->params.batMinC = this->liveData->params.batMaxC = this->liveData->params.batModuleTempC[0];
  for (uint16_t i = 1; i < this->liveData->params.batModuleTempCount; i++) {
    if (this->liveData->params.batModuleTempC[i] < this->liveData->params.batMinC)
      this->liveData->params.batMinC = this->liveData->params.batModuleTempC[i];
    if (this->liveData->params.batModuleTempC[i] > this->liveData->params.batMaxC)
      this->liveData->params.batMaxC = this->liveData->params.batModuleTempC[i];
  }
  this->liveData->params.batTempC = this->liveData->params.batMinC;


  //
  this->liveData->params.soc10ced[10] = 2200;
  this->liveData->params.soc10cec[10] = 2500;
  this->liveData->params.soc10odo[10] = 13000;
  this->liveData->params.soc10time[10] = 13000;
  this->liveData->params.soc10ced[9] = this->liveData->params.soc10ced[10] + 6.4;
  this->liveData->params.soc10cec[9] = this->liveData->params.soc10cec[10] + 0;
  this->liveData->params.soc10odo[9] = this->liveData->params.soc10odo[10] + 30;
  this->liveData->params.soc10time[9] = this->liveData->params.soc10time[10] + 900;
  this->liveData->params.soc10ced[8] = this->liveData->params.soc10ced[9] + 6.8;
  this->liveData->params.soc10cec[8] = this->liveData->params.soc10cec[9] + 0;
  this->liveData->params.soc10odo[8] = this->liveData->params.soc10odo[9] + 30;
  this->liveData->params.soc10time[8] = this->liveData->params.soc10time[9] + 900;
  this->liveData->params.soc10ced[7] = this->liveData->params.soc10ced[8] + 7.2;
  this->liveData->params.soc10cec[7] = this->liveData->params.soc10cec[8] + 0.6;
  this->liveData->params.soc10odo[7] = this->liveData->params.soc10odo[8] + 30;
  this->liveData->params.soc10time[7] = this->liveData->params.soc10time[8] + 900;
  this->liveData->params.soc10ced[6] = this->liveData->params.soc10ced[7] + 6.7;
  this->liveData->params.soc10cec[6] = this->liveData->params.soc10cec[7] + 0;
  this->liveData->params.soc10odo[6] = this->liveData->params.soc10odo[7] + 30;
  this->liveData->params.soc10time[6] = this->liveData->params.soc10time[7] + 900;
  this->liveData->params.soc10ced[5] = this->liveData->params.soc10ced[6] + 6.7;
  this->liveData->params.soc10cec[5] = this->liveData->params.soc10cec[6] + 0;
  this->liveData->params.soc10odo[5] = this->liveData->params.soc10odo[6] + 30;
  this->liveData->params.soc10time[5] = this->liveData->params.soc10time[6] + 900;
  this->liveData->params.soc10ced[4] = this->liveData->params.soc10ced[5] + 6.4;
  this->liveData->params.soc10cec[4] = this->liveData->params.soc10cec[5] + 0.3;
  this->liveData->params.soc10odo[4] = this->liveData->params.soc10odo[5] + 30;
  this->liveData->params.soc10time[4] = this->liveData->params.soc10time[5] + 900;
  this->liveData->params.soc10ced[3] = this->liveData->params.soc10ced[4] + 6.4;
  this->liveData->params.soc10cec[3] = this->liveData->params.soc10cec[4] + 0;
  this->liveData->params.soc10odo[3] = this->liveData->params.soc10odo[4] + 30;
  this->liveData->params.soc10time[3] = this->liveData->params.soc10time[4] + 900;
  this->liveData->params.soc10ced[2] = this->liveData->params.soc10ced[3] + 5.4;
  this->liveData->params.soc10cec[2] = this->liveData->params.soc10cec[3] + 0.1;
  this->liveData->params.soc10odo[2] = this->liveData->params.soc10odo[3] + 30;
  this->liveData->params.soc10time[2] = this->liveData->params.soc10time[3] + 900;
  this->liveData->params.soc10ced[1] = this->liveData->params.soc10ced[2] + 6.2;
  this->liveData->params.soc10cec[1] = this->liveData->params.soc10cec[2] + 0.1;
  this->liveData->params.soc10odo[1] = this->liveData->params.soc10odo[2] + 30;
  this->liveData->params.soc10time[1] = this->liveData->params.soc10time[2] + 900;
  this->liveData->params.soc10ced[0] = this->liveData->params.soc10ced[1] + 2.9;
  this->liveData->params.soc10cec[0] = this->liveData->params.soc10cec[1] + 0.5;
  this->liveData->params.soc10odo[0] = this->liveData->params.soc10odo[1] + 15;
  this->liveData->params.soc10time[0] = this->liveData->params.soc10time[1] + 900;

}

#endif // CARKIAENIRO_CPP
