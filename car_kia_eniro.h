
#define commandQueueCountKiaENiro 30
#define commandQueueLoopFromKiaENiro 8

String commandQueueKiaENiro[commandQueueCountKiaENiro] = {
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

  // Loop from (KIA ENIRO)
  // BMS
  "ATSH7E4",
  "220101",   // power kw, ...
  "220102",   // cell voltages, screen 3 only
  "220103",   // cell voltages, screen 3 only
  "220104",   // cell voltages, screen 3 only
  "220105",   // soh, soc, ..
  "220106",   // cooling water temp

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

  //"ATSH7Df",
  //"2106",
  //"220106",

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

/**
   Init command queue
*/
bool activateCommandQueueForKiaENiro() {

  // 39 or 64 kWh model?
  params.batModuleTempCount = 4;
  params.batteryTotalAvailableKWh = 64;
  if (settings.carType == CAR_KIA_ENIRO_2020_39 || settings.carType == CAR_HYUNDAI_KONA_2020_39) {
    params.batteryTotalAvailableKWh = 39.2;
  }

  //  Empty and fill command queue
  for (int i = 0; i < 300; i++) {
    commandQueue[i] = "";
  }
  for (int i = 0; i < commandQueueCountKiaENiro; i++) {
    commandQueue[i] = commandQueueKiaENiro[i];
  }

  commandQueueLoopFrom = commandQueueLoopFromKiaENiro;
  commandQueueCount = commandQueueCountKiaENiro;

  return true;
}

/**
  Parse merged row
*/
bool parseRowMergedKiaENiro() {

  // ABS / ESP + AHB 7D1
  if (currentAtshRequest.equals("ATSH7D1")) {
    if (commandRequest.equals("22C101")) {
      params.driveMode = hexToDec(responseRowMerged.substring(22, 24).c_str(), 1, false);
      // 7 (val 128) 
      // 6 (val 64) 
      // 5 (val 32) 
      // 4 (val 16) 
      // 3 (val 8) 
      // 2 (val 4) DRIVE mode
      // 1 (val 2) REVERSE mode
      // 0 (val 1) PARK mode / NEUTRAL
  /*    params.espState = hexToDec(responseRowMerged.substring(42, 44).c_str(), 1, false);
      // b6 (val 64) 1 - ESP OFF, 0 - ESP ON
  */
/*      params.xxx = hexToDec(responseRowMerged.substring(44, 46).c_str(), 1, false);
      // 5 (val 32) default 1 
      // 4 (val 16) default 1 
      // 2 (val 4) BRAKE PRESSED
      // 0 (val 1) */
    }
  }

  // IGPM
  if (currentAtshRequest.equals("ATSH770")) {
    if (commandRequest.equals("22BC03")) {
      params.lightInfo = hexToDec(responseRowMerged.substring(18, 20).c_str(), 1, false);
      // 7 (val 128) 
      // 6 (val 64) 
      // 5 (val 32) 
      // 4 (val 16) 
      // 3 (val 8) 
      // 2 (val 4
      // 1 (val 2)
      // 0 (val 1)
    }
    if (commandRequest.equals("22BC06")) {
      params.brakeLightInfo = hexToDec(responseRowMerged.substring(14, 18).c_str(), 1, false);
      // 7 (val 128) 
      // 6 (val 64) 
      // 5 (val 32) 
      // 4 (val 16) 
      // 3 (val 8) 
      // 2 (val 4
      // 1 (val 2)
      // 0 (val 1)
    }
  }

  // VMCU 7E2
  if (currentAtshRequest.equals("ATSH7E2")) {
    if (commandRequest.equals("2101")) {
      params.speedKmh = hexToDec(responseRowMerged.substring(32, 36).c_str(), 2, false) * 0.0155; // / 100.0 *1.609 = real to gps is 1.750
      if (params.speedKmh < -99 || params.speedKmh > 200)
        params.speedKmh = 0;
    }
    if (commandRequest.equals("2102")) {
      params.auxPerc = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, false);
      params.auxCurrentAmp = - hexToDec(responseRowMerged.substring(46, 50).c_str(), 2, true) / 1000.0;
    }
  }

  // Cluster module 7c6
  if (currentAtshRequest.equals("ATSH7C6")) {
    if (commandRequest.equals("22B002")) {
      params.odoKm = float(strtol(responseRowMerged.substring(18, 24).c_str(), 0, 16));
    }
  }

  // Aircon 7b3
  if (currentAtshRequest.equals("ATSH7B3")) {
    if (commandRequest.equals("220100")) {
      params.indoorTemperature = (hexToDec(responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
      params.outdoorTemperature = (hexToDec(responseRowMerged.substring(18, 20).c_str(), 1, false) / 2) - 40;
    }
    if (commandRequest.equals("220102") && responseRowMerged.substring(12, 14) == "00") {
      params.coolantTemp1C = (hexToDec(responseRowMerged.substring(14, 16).c_str(), 1, false) / 2) - 40;
      params.coolantTemp2C = (hexToDec(responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
    }
  }

  // BMS 7e4
  if (currentAtshRequest.equals("ATSH7E4")) {
    if (commandRequest.equals("220101")) {
      params.cumulativeEnergyChargedKWh = float(strtol(responseRowMerged.substring(82, 90).c_str(), 0, 16)) / 10.0;
      if (params.cumulativeEnergyChargedKWhStart == -1)
        params.cumulativeEnergyChargedKWhStart = params.cumulativeEnergyChargedKWh;
      params.cumulativeEnergyDischargedKWh = float(strtol(responseRowMerged.substring(90, 98).c_str(), 0, 16)) / 10.0;
      if (params.cumulativeEnergyDischargedKWhStart == -1)
        params.cumulativeEnergyDischargedKWhStart = params.cumulativeEnergyDischargedKWh;
      params.availableChargePower = float(strtol(responseRowMerged.substring(16, 20).c_str(), 0, 16)) / 100.0;
      params.availableDischargePower = float(strtol(responseRowMerged.substring(20, 24).c_str(), 0, 16)) / 100.0;
      //params.isolationResistanceKOhm = hexToDec(responseRowMerged.substring(118, 122).c_str(), 2, true);
      params.batFanStatus = hexToDec(responseRowMerged.substring(58, 60).c_str(), 2, true);
      params.batFanFeedbackHz = hexToDec(responseRowMerged.substring(60, 62).c_str(), 2, true);
      params.auxVoltage = hexToDec(responseRowMerged.substring(64, 66).c_str(), 2, true) / 10.0;
      params.batPowerAmp = - hexToDec(responseRowMerged.substring(26, 30).c_str(), 2, true) / 10.0;
      params.batVoltage = hexToDec(responseRowMerged.substring(30, 34).c_str(), 2, false) / 10.0;
      params.batPowerKw = (params.batPowerAmp * params.batVoltage) / 1000.0;
      if (params.batPowerKw < 0) // Reset charging start time
        params.chargingStartTime = params.currentTime;
      params.batPowerKwh100 = params.batPowerKw / params.speedKmh * 100;
      params.batCellMaxV = hexToDec(responseRowMerged.substring(52, 54).c_str(), 1, false) / 50.0;
      params.batCellMinV = hexToDec(responseRowMerged.substring(56, 58).c_str(), 1, false) / 50.0;
      params.batModuleTempC[0] = hexToDec(responseRowMerged.substring(38, 40).c_str(), 1, true);
      params.batModuleTempC[1] = hexToDec(responseRowMerged.substring(40, 42).c_str(), 1, true);
      params.batModuleTempC[2] = hexToDec(responseRowMerged.substring(42, 44).c_str(), 1, true);
      params.batModuleTempC[3] = hexToDec(responseRowMerged.substring(44, 46).c_str(), 1, true);
      //params.batTempC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);
      //params.batMaxC = hexToDec(responseRowMerged.substring(34, 36).c_str(), 1, true);
      //params.batMinC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);

      // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
      params.batMinC = params.batMaxC = params.batModuleTempC[0];
      for (uint16_t i = 1; i < params.batModuleTempCount; i++) {
        if (params.batModuleTempC[i] < params.batMinC)
          params.batMinC = params.batModuleTempC[i];
        if (params.batModuleTempC[i] > params.batMaxC)
          params.batMaxC = params.batModuleTempC[i];
      }
      params.batTempC = params.batMinC;

      params.batInletC = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, true);
      if (params.speedKmh < 10 && params.batPowerKw >= 1 && params.socPerc > 0 && params.socPerc <= 100) {
        if ( params.chargingGraphMinKw[int(params.socPerc)] < 0 || params.batPowerKw < params.chargingGraphMinKw[int(params.socPerc)])
          params.chargingGraphMinKw[int(params.socPerc)] = params.batPowerKw;
        if ( params.chargingGraphMaxKw[int(params.socPerc)] < 0 || params.batPowerKw > params.chargingGraphMaxKw[int(params.socPerc)])
          params.chargingGraphMaxKw[int(params.socPerc)] = params.batPowerKw;
        params.chargingGraphBatMinTempC[int(params.socPerc)] = params.batMinC;
        params.chargingGraphBatMaxTempC[int(params.socPerc)] = params.batMaxC;
        params.chargingGraphHeaterTempC[int(params.socPerc)] = params.batHeaterC;
        params.chargingGraphWaterCoolantTempC[int(params.socPerc)] = params.coolingWaterTempC;
      }
    }
    // BMS 7e4
    if (commandRequest.equals("220102") && responseRowMerged.substring(12, 14) == "FF") {
      for (int i = 0; i < 32; i++) {
        params.cellVoltage[i] = hexToDec(responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (commandRequest.equals("220103")) {
      for (int i = 0; i < 32; i++) {
        params.cellVoltage[32 + i] = hexToDec(responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (commandRequest.equals("220104")) {
      for (int i = 0; i < 32; i++) {
        params.cellVoltage[64 + i] = hexToDec(responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (commandRequest.equals("220105")) {
      params.sohPerc = hexToDec(responseRowMerged.substring(56, 60).c_str(), 2, false) / 10.0;
      params.socPerc = hexToDec(responseRowMerged.substring(68, 70).c_str(), 1, false) / 2.0;

      // Soc10ced table, record x0% CEC/CED table (ex. 90%->89%, 80%->79%)
      if (oldParams.socPerc - params.socPerc > 0) {
        byte index = (int(params.socPerc) == 4) ? 0 : (int)(params.socPerc / 10) + 1;
        if ((int(params.socPerc) % 10 == 9 || int(params.socPerc) == 4) && params.soc10ced[index] == -1) {
          params.soc10ced[index] = params.cumulativeEnergyDischargedKWh;
          params.soc10cec[index] = params.cumulativeEnergyChargedKWh;
          params.soc10odo[index] = params.odoKm;
          params.soc10time[index] = params.currentTime;
        }
      }
      params.batHeaterC = hexToDec(responseRowMerged.substring(52, 54).c_str(), 1, true);
      //
      for (int i = 30; i < 32; i++) { // ai/aj position
        params.cellVoltage[96 - 30 + i] = hexToDec(responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
      }
    }
    // BMS 7e4
    if (commandRequest.equals("220106")) {
      params.coolingWaterTempC = hexToDec(responseRowMerged.substring(14, 16).c_str(), 1, false);
    }
  }

  // TPMS 7a0
  if (currentAtshRequest.equals("ATSH7A0")) {
    if (commandRequest.equals("22c00b")) {
      params.tireFrontLeftPressureBar = hexToDec(responseRowMerged.substring(14, 16).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
      params.tireFrontRightPressureBar = hexToDec(responseRowMerged.substring(22, 24).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
      params.tireRearRightPressureBar = hexToDec(responseRowMerged.substring(30, 32).c_str(), 2, false) / 72.51886900361;    // === OK Valid *0.2 / 14.503773800722
      params.tireRearLeftPressureBar = hexToDec(responseRowMerged.substring(38, 40).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
      params.tireFrontLeftTempC = hexToDec(responseRowMerged.substring(16, 18).c_str(), 2, false)  - 50;      // === OK Valid
      params.tireFrontRightTempC = hexToDec(responseRowMerged.substring(24, 26).c_str(), 2, false) - 50;      // === OK Valid
      params.tireRearRightTempC = hexToDec(responseRowMerged.substring(32, 34).c_str(), 2, false) - 50;     // === OK Valid
      params.tireRearLeftTempC = hexToDec(responseRowMerged.substring(40, 42).c_str(), 2, false) - 50;     // === OK Valid
    }
  }

  return true;
}

/**
   Test data
*/
bool testDataKiaENiro() {


  // IGPM
  currentAtshRequest = "ATSH770";
  // 22BC03
  commandRequest = "22BC03";
  responseRowMerged = "62BC03FDEE7C730A600000AAAA";
  parseRowMergedKiaENiro();
 
  // ABS / ESP + AHB ATSH7D1
  currentAtshRequest = "ATSH7D1";
  // 2101
  commandRequest = "22C101";
  responseRowMerged = "62C1015FD7E7D0FFFF00FF04D0D400000000FF7EFF0030F5010000FFFF7F6307F207FE05FF00FF3FFFFFAAAAAAAAAAAA";
  parseRowMergedKiaENiro();
  
  // VMCU ATSH7E2
  currentAtshRequest = "ATSH7E2";
  // 2101
  commandRequest = "2101";
  responseRowMerged = "6101FFF8000009285A3B0648030000B4179D763404080805000000";
  parseRowMergedKiaENiro();
  // 2102
  commandRequest = "2102";
  responseRowMerged = "6102F8FFFC000101000000840FBF83BD33270680953033757F59291C76000001010100000007000000";
  responseRowMerged = "6102F8FFFC000101000000931CC77F4C39040BE09BA7385D8158832175000001010100000007000000";
  parseRowMergedKiaENiro();

  // "ATSH7DF",
  currentAtshRequest = "ATSH7DF";
  // 2106
  commandRequest = "2106";
  responseRowMerged = "6106FFFF800000000000000200001B001C001C000600060006000E000000010000000000000000013D013D013E013E00";
  parseRowMergedKiaENiro();

  // AIRCON / ACU ATSH7B3
  currentAtshRequest = "ATSH7B3";
  // 220100
  commandRequest = "220100";
  responseRowMerged = "6201007E5027C8FF7F765D05B95AFFFF5AFF11FFFFFFFFFFFF6AFFFF2DF0757630FFFF00FFFF000000";
  responseRowMerged = "6201007E5027C8FF867C58121010FFFF10FF8EFFFFFFFFFFFF10FFFF0DF0617900FFFF01FFFF000000";
  parseRowMergedKiaENiro();

  // BMS ATSH7E4
  currentAtshRequest = "ATSH7E4";
  // 220101
  commandRequest = "220101";
  responseRowMerged = "620101FFF7E7FF99000000000300B10EFE120F11100F12000018C438C30B00008400003864000035850000153A00001374000647010D017F0BDA0BDA03E8";
  responseRowMerged = "620101FFF7E7FFB3000000000300120F9B111011101011000014CC38CB3B00009100003A510000367C000015FB000013D3000690250D018E0000000003E8";
  parseRowMergedKiaENiro();
  // 220102
  commandRequest = "220102";
  responseRowMerged = "620102FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMergedKiaENiro();
  // 220103
  commandRequest = "220103";
  responseRowMerged = "620103FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCACBCACACFCCCBCBCBCBCBCBCBCBAAAA";
  parseRowMergedKiaENiro();
  // 220104
  commandRequest = "220104";
  responseRowMerged = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMergedKiaENiro();
  // 220105
  commandRequest = "220105";
  responseRowMerged = "620105003fff9000000000000000000F8A86012B4946500101500DAC03E800000000AC0000C7C701000F00000000AAAA";
  responseRowMerged = "620105003FFF90000000000000000014918E012927465000015013BB03E800000000BB0000CBCB01001300000000AAAA";
  parseRowMergedKiaENiro();
  // 220106
  commandRequest = "220106";
  responseRowMerged = "620106FFFFFFFF14001A00240000003A7C86B4B30000000928EA00";
  parseRowMergedKiaENiro();

  // BCM / TPMS ATSH7A0
  currentAtshRequest = "ATSH7A0";
  // 22c00b
  commandRequest = "22c00b";
  responseRowMerged = "62C00BFFFF0000B93D0100B43E0100B43D0100BB3C0100AAAAAAAA";
  parseRowMergedKiaENiro();

  // ATSH7C6
  currentAtshRequest = "ATSH7C6";
  // 22b002
  commandRequest = "22b002";
  responseRowMerged = "62B002E0000000FFB400330B0000000000000000";
  parseRowMergedKiaENiro();

  params.batModuleTempC[0] = 28;
  params.batModuleTempC[1] = 29;
  params.batModuleTempC[2] = 28;
  params.batModuleTempC[3] = 30;

  // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
  params.batMinC = params.batMaxC = params.batModuleTempC[0];
  for (uint16_t i = 1; i < params.batModuleTempCount; i++) {
    if (params.batModuleTempC[i] < params.batMinC)
      params.batMinC = params.batModuleTempC[i];
    if (params.batModuleTempC[i] > params.batMaxC)
      params.batMaxC = params.batModuleTempC[i];
  }
  params.batTempC = params.batMinC;


  //
  params.soc10ced[10] = 2200;
  params.soc10cec[10] = 2500;
  params.soc10odo[10] = 13000;
  params.soc10time[10] = 13000;
  params.soc10ced[9] = params.soc10ced[10] + 6.4;
  params.soc10cec[9] = params.soc10cec[10] + 0;
  params.soc10odo[9] = params.soc10odo[10] + 30;
  params.soc10time[9] = params.soc10time[10] + 900;
  params.soc10ced[8] = params.soc10ced[9] + 6.8;
  params.soc10cec[8] = params.soc10cec[9] + 0;
  params.soc10odo[8] = params.soc10odo[9] + 30;
  params.soc10time[8] = params.soc10time[9] + 900;
  params.soc10ced[7] = params.soc10ced[8] + 7.2;
  params.soc10cec[7] = params.soc10cec[8] + 0.6;
  params.soc10odo[7] = params.soc10odo[8] + 30;
  params.soc10time[7] = params.soc10time[8] + 900;
  params.soc10ced[6] = params.soc10ced[7] + 6.7;
  params.soc10cec[6] = params.soc10cec[7] + 0;
  params.soc10odo[6] = params.soc10odo[7] + 30;
  params.soc10time[6] = params.soc10time[7] + 900;
  params.soc10ced[5] = params.soc10ced[6] + 6.7;
  params.soc10cec[5] = params.soc10cec[6] + 0;
  params.soc10odo[5] = params.soc10odo[6] + 30;
  params.soc10time[5] = params.soc10time[6] + 900;
  params.soc10ced[4] = params.soc10ced[5] + 6.4;
  params.soc10cec[4] = params.soc10cec[5] + 0.3;
  params.soc10odo[4] = params.soc10odo[5] + 30;
  params.soc10time[4] = params.soc10time[5] + 900;
  params.soc10ced[3] = params.soc10ced[4] + 6.4;
  params.soc10cec[3] = params.soc10cec[4] + 0;
  params.soc10odo[3] = params.soc10odo[4] + 30;
  params.soc10time[3] = params.soc10time[4] + 900;
  params.soc10ced[2] = params.soc10ced[3] + 5.4;
  params.soc10cec[2] = params.soc10cec[3] + 0.1;
  params.soc10odo[2] = params.soc10odo[3] + 30;
  params.soc10time[2] = params.soc10time[3] + 900;
  params.soc10ced[1] = params.soc10ced[2] + 6.2;
  params.soc10cec[1] = params.soc10cec[2] + 0.1;
  params.soc10odo[1] = params.soc10odo[2] + 30;
  params.soc10time[1] = params.soc10time[2] + 900;
  params.soc10ced[0] = params.soc10ced[1] + 2.9;
  params.soc10cec[0] = params.soc10cec[1] + 0.5;
  params.soc10odo[0] = params.soc10odo[1] + 15;
  params.soc10time[0] = params.soc10time[1] + 900;

  return true;
}
