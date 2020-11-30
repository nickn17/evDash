#ifndef CARHYUNDAIIONIQ_CPP
#define CARHYUNDAIIONIQ_CPP

#include "CarHyundaiIoniq.h"

#define commandQueueCountHyundaiIoniq 25
#define commandQueueLoopFromHyundaiIoniq 8

/**
   activatethis->liveData->commandQueue
*/
void CarHyundaiIoniq::activateCommandQueue() {

  String commandQueueHyundaiIoniq[commandQueueCountHyundaiIoniq] = {
    "AT Z",      // Reset all
    "AT I",      // Print the version ID
    "AT E0",     // Echo off
    "AT L0",     // Linefeeds off
    "AT S0",     // Printing of spaces on
    "AT SP 6",   // Select protocol to ISO 15765-4 CAN (11 bit ID, 500 kbit/s)
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
    "2101",   // power kw, ...
    "2102",   // cell voltages, screen 3 only
    "2103",   // cell voltages, screen 3 only
    "2104",   // cell voltages, screen 3 only
    "2105",   // soh, soc, ..
    "2106",   // cooling water temp

    // VMCU
    "ATSH7E2",
    "2101",     // speed, ...
    "2102",     // aux, ...

    //"ATSH7Df",
    //"2106",
    //"220106",

    // Aircondition
    // IONIQ OK
    "ATSH7B3",
    "220100",   // in/out temp
    "220102",   // coolant temp1, 2

    // BCM / TPMS
    // IONIQ OK
    "ATSH7A0",
    "22c00b",   // tire pressure/temp

    // CLUSTER MODULE
    // IONIQ OK
    "ATSH7C6",
    "22B002",   // odo
  };

  // 28kWh version
  this->liveData->params.batteryTotalAvailableKWh = 28;
  this->liveData->params.batModuleTempCount = 12;

  //  Empty and fill command queue
  for (int i = 0; i < 300; i++) {
    this->liveData->commandQueue[i] = "";
  }
  for (int i = 0; i < commandQueueCountHyundaiIoniq; i++) {
    this->liveData->commandQueue[i] = commandQueueHyundaiIoniq[i];
  }

  this->liveData->commandQueueLoopFrom = commandQueueLoopFromHyundaiIoniq;
  this->liveData->commandQueueCount = commandQueueCountHyundaiIoniq;
}

/**
   parseRowMerged
*/
void CarHyundaiIoniq::parseRowMerged() {

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
    if (this->liveData->commandRequest.equals("2101")) {
      this->liveData->params.cumulativeEnergyChargedKWh = float(strtol(this->liveData->responseRowMerged.substring(80, 88).c_str(), 0, 16)) / 10.0;
      if (this->liveData->params.cumulativeEnergyChargedKWhStart == -1)
        this->liveData->params.cumulativeEnergyChargedKWhStart = this->liveData->params.cumulativeEnergyChargedKWh;
      this->liveData->params.cumulativeEnergyDischargedKWh = float(strtol(this->liveData->responseRowMerged.substring(88, 96).c_str(), 0, 16)) / 10.0;
      if (this->liveData->params.cumulativeEnergyDischargedKWhStart == -1)
        this->liveData->params.cumulativeEnergyDischargedKWhStart = this->liveData->params.cumulativeEnergyDischargedKWh;
      this->liveData->params.availableChargePower = float(strtol(this->liveData->responseRowMerged.substring(16, 20).c_str(), 0, 16)) / 100.0;
      this->liveData->params.availableDischargePower = float(strtol(this->liveData->responseRowMerged.substring(20, 24).c_str(), 0, 16)) / 100.0;
      this->liveData->params.isolationResistanceKOhm = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(118, 122).c_str(), 2, true);
      this->liveData->params.batFanStatus = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(58, 60).c_str(), 2, true);
      this->liveData->params.batFanFeedbackHz = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(60, 62).c_str(), 2, true);
      this->liveData->params.auxVoltage = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(62, 64).c_str(), 2, true) / 10.0;
      this->liveData->params.batPowerAmp = - this->liveData->hexToDec(this->liveData->responseRowMerged.substring(24, 28).c_str(), 2, true) / 10.0;
      this->liveData->params.batVoltage = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(28, 32).c_str(), 2, false) / 10.0;
      this->liveData->params.batPowerKw = (this->liveData->params.batPowerAmp * this->liveData->params.batVoltage) / 1000.0;
      if (this->liveData->params.batPowerKw < 1) // Reset charging start time
        this->liveData->params.chargingStartTime = this->liveData->params.currentTime;
      this->liveData->params.batPowerKwh100 = this->liveData->params.batPowerKw / this->liveData->params.speedKmh * 100;
      this->liveData->params.batCellMaxV = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(50, 52).c_str(), 1, false) / 50.0;
      this->liveData->params.batCellMinV = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(54, 56).c_str(), 1, false) / 50.0;
      this->liveData->params.batModuleTempC[0] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(36, 38).c_str(), 1, true);
      this->liveData->params.batModuleTempC[1] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(38, 40).c_str(), 1, true);
      this->liveData->params.batModuleTempC[2] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(40, 42).c_str(), 1, true);
      this->liveData->params.batModuleTempC[3] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(42, 44).c_str(), 1, true);
      this->liveData->params.batModuleTempC[4] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(44, 46).c_str(), 1, true);
      //this->liveData->params.batTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(34, 36).c_str(), 1, true);
      //this->liveData->params.batMaxC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(32, 34).c_str(), 1, true);
      //this->liveData->params.batMinC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(34, 36).c_str(), 1, true);

      // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
      this->liveData->params.batInletC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(48, 50).c_str(), 1, true);
      if (this->liveData->params.speedKmh < 10 && this->liveData->params.batPowerKw >= 1 && this->liveData->params.socPerc > 0 && this->liveData->params.socPerc <= 100) {
        if ( this->liveData->params.chargingGraphMinKw[int(this->liveData->params.socPerc)] == -100 || this->liveData->params.batPowerKw < this->liveData->params.chargingGraphMinKw[int(this->liveData->params.socPerc)])
          this->liveData->params.chargingGraphMinKw[int(this->liveData->params.socPerc)] = this->liveData->params.batPowerKw;
        if ( this->liveData->params.chargingGraphMaxKw[int(this->liveData->params.socPerc)] == -100 || this->liveData->params.batPowerKw > this->liveData->params.chargingGraphMaxKw[int(this->liveData->params.socPerc)])
          this->liveData->params.chargingGraphMaxKw[int(this->liveData->params.socPerc)] = this->liveData->params.batPowerKw;
        this->liveData->params.chargingGraphBatMinTempC[int(this->liveData->params.socPerc)] = this->liveData->params.batMinC;
        this->liveData->params.chargingGraphBatMaxTempC[int(this->liveData->params.socPerc)] = this->liveData->params.batMaxC;
        this->liveData->params.chargingGraphHeaterTempC[int(this->liveData->params.socPerc)] = this->liveData->params.batHeaterC;
      }
    }
    // BMS 7e4
    if (this->liveData->commandRequest.equals("2102") && this->liveData->responseRowMerged.substring(10, 12) == "FF") {
      for (int i = 0; i < 32; i++) {
        this->liveData->params.cellVoltage[i] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(12 + (i * 2), 12 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (this->liveData->commandRequest.equals("2103")) {
      for (int i = 0; i < 32; i++) {
        this->liveData->params.cellVoltage[32 + i] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(12 + (i * 2), 12 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (this->liveData->commandRequest.equals("2104")) {
      for (int i = 0; i < 32; i++) {
        this->liveData->params.cellVoltage[64 + i] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(12 + (i * 2), 12 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (this->liveData->commandRequest.equals("2105")) {
      this->liveData->params.socPercPrevious = this->liveData->params.socPerc;
      this->liveData->params.sohPerc = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(54, 58).c_str(), 2, false) / 10.0;
      this->liveData->params.socPerc = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(66, 68).c_str(), 1, false) / 2.0;

      // Remaining battery modules (tempC)
      this->liveData->params.batModuleTempC[5] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(22, 24).c_str(), 1, true);
      this->liveData->params.batModuleTempC[6] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(24, 26).c_str(), 1, true);
      this->liveData->params.batModuleTempC[7] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(26, 28).c_str(), 1, true);
      this->liveData->params.batModuleTempC[8] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(28, 30).c_str(), 1, true);
      this->liveData->params.batModuleTempC[9] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(30, 32).c_str(), 1, true);
      this->liveData->params.batModuleTempC[10] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(32, 34).c_str(), 1, true);
      this->liveData->params.batModuleTempC[11] = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(34, 36).c_str(), 1, true);

      this->liveData->params.batMinC = this->liveData->params.batMaxC = this->liveData->params.batModuleTempC[0];
      for (uint16_t i = 1; i < this->liveData->params.batModuleTempCount; i++) {
        if (this->liveData->params.batModuleTempC[i] < this->liveData->params.batMinC)
          this->liveData->params.batMinC = this->liveData->params.batModuleTempC[i];
        if (this->liveData->params.batModuleTempC[i] > this->liveData->params.batMaxC)
          this->liveData->params.batMaxC = this->liveData->params.batModuleTempC[i];
      }
      this->liveData->params.batTempC = this->liveData->params.batMinC;

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
      this->liveData->params.batHeaterC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(50, 52).c_str(), 1, true);
      //
      for (int i = 30; i < 32; i++) { // ai/aj position
        this->liveData->params.cellVoltage[96 - 30 + i] = -1;
      }
    }
    // BMS 7e4
    // IONIQ FAILED
    if (this->liveData->commandRequest.equals("2106")) {
      this->liveData->params.coolingWaterTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(14, 16).c_str(), 1, false);
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
   loadTestData
*/
void CarHyundaiIoniq::loadTestData() {

  // VMCU ATSH7E2
  this->liveData->currentAtshRequest = "ATSH7E2";
  // 2101
  this->liveData->commandRequest = "2101";
  this->liveData->responseRowMerged = "6101FFE0000009211222062F03000000001D7734";
  this->parseRowMerged();
  // 2102
  this->liveData->commandRequest = "2102";
  this->liveData->responseRowMerged = "6102FF80000001010000009315B2888D390B08618B683900000000";
  this->parseRowMerged();

  // "ATSH7DF",
  this->liveData->currentAtshRequest = "ATSH7DF";

  // AIRCON / ACU ATSH7B3
  this->liveData->currentAtshRequest = "ATSH7B3";
  // 220100
  this->liveData->commandRequest = "220100";
  this->liveData->responseRowMerged = "6201007E5007C8FF8A876A011010FFFF10FF10FFFFFFFFFFFFFFFFFF2EEF767D00FFFF00FFFF000000";
  this->parseRowMerged();
  // 220102
  this->liveData->commandRequest = "220102";
  this->liveData->responseRowMerged = "620102FF800000A3950000000000002600000000";
  this->parseRowMerged();

  // BMS ATSH7E4
  this->liveData->currentAtshRequest = "ATSH7E4";
  // 220101
  this->liveData->commandRequest = "2101";
  this->liveData->responseRowMerged = "6101FFFFFFFF5026482648A3FFC30D9E181717171718170019B50FB501000090000142230001425F0000771B00007486007815D809015C0000000003E800";
  this->parseRowMerged();
  // 220102
  this->liveData->commandRequest = "2102";
  this->liveData->responseRowMerged = "6102FFFFFFFFB5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5000000";
  this->parseRowMerged();
  // 220103
  this->liveData->commandRequest = "2103";
  this->liveData->responseRowMerged = "6103FFFFFFFFB5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5000000";
  this->parseRowMerged();
  // 220104
  this->liveData->commandRequest = "2104";
  this->liveData->responseRowMerged = "6104FFFFFFFFB5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5000000";
  this->parseRowMerged();
  // 220105
  this->liveData->commandRequest = "2105";
  this->liveData->responseRowMerged = "6105FFFFFFFF00000000001717171817171726482648000150181703E81A03E801520029000000000000000000000000";
  this->parseRowMerged();
  // 220106
  this->liveData->commandRequest = "2106";
  this->liveData->responseRowMerged = "7F2112"; // n/a on ioniq
  this->parseRowMerged();

  // BCM / TPMS ATSH7A0
  this->liveData->currentAtshRequest = "ATSH7A0";
  // 22c00b
  this->liveData->commandRequest = "22c00b";
  this->liveData->responseRowMerged = "62C00BFFFF0000B9510100B9510100B84F0100B54F0100AAAAAAAA";
  this->parseRowMerged();

  // ATSH7C6
  this->liveData->currentAtshRequest = "ATSH7C6";
  // 22b002
  this->liveData->commandRequest = "22b002";
  this->liveData->responseRowMerged = "62B002E000000000AD003D2D0000000000000000";
  this->parseRowMerged();

  /*  this->liveData->params.batModule01TempC = 28;
    this->liveData->params.batModule02TempC = 29;
    this->liveData->params.batModule03TempC = 28;
    this->liveData->params.batModule04TempC = 30;
    //this->liveData->params.batTempC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(36, 38).c_str(), 1, true);
    //this->liveData->params.batMaxC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(34, 36).c_str(), 1, true);
    //this->liveData->params.batMinC = this->liveData->hexToDec(this->liveData->responseRowMerged.substring(36, 38).c_str(), 1, true);

    // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
    this->liveData->params.batMinC = this->liveData->params.batMaxC = this->liveData->params.batModule01TempC;
    this->liveData->params.batMinC = (this->liveData->params.batModule02TempC < this->liveData->params.batMinC) ? this->liveData->params.batModule02TempC : this->liveData->params.batMinC ;
    this->liveData->params.batMinC = (this->liveData->params.batModule03TempC < this->liveData->params.batMinC) ? this->liveData->params.batModule03TempC : this->liveData->params.batMinC ;
    this->liveData->params.batMinC = (this->liveData->params.batModule04TempC < this->liveData->params.batMinC) ? this->liveData->params.batModule04TempC : this->liveData->params.batMinC ;
    this->liveData->params.batMaxC = (this->liveData->params.batModule02TempC > this->liveData->params.batMaxC) ? this->liveData->params.batModule02TempC : this->liveData->params.batMaxC ;
    this->liveData->params.batMaxC = (this->liveData->params.batModule03TempC > this->liveData->params.batMaxC) ? this->liveData->params.batModule03TempC : this->liveData->params.batMaxC ;
    this->liveData->params.batMaxC = (this->liveData->params.batModule04TempC > this->liveData->params.batMaxC) ? this->liveData->params.batModule04TempC : this->liveData->params.batMaxC ;
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
  */

}

#endif //CARHYUNDAIIONIQ_CPP
