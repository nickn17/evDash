#define commandQueueCountHyundaiIoniq 25
#define commandQueueLoopFromHyundaiIoniq 8
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
  "atsh7e4",
  "2101",   // power kw, ...
  "2102",   // cell voltages, screen 3 only
  "2103",   // cell voltages, screen 3 only
  "2104",   // cell voltages, screen 3 only
  "2105",   // soh, soc, ..
  "2106",   // cooling water temp

  // VMCU
  "atsh7e2",
  "2101",     // speed, ...
  "2102",     // aux, ...

  //"atsh7df",
  //"2106",
  //"220106",

  // ECU - Aircondition
  // IONIQ OK
  "atsh7b3",
  "220100",   // in/out temp
  "220102",   // coolant temp1, 2

  // BCM / TPMS
  // IONIQ OK
  "atsh7a0",
  "22c00b",   // tire pressure/temp

  // CLUSTER MODULE
  // IONIQ OK
  "atsh7c6",
  "22B002",   // odo
};

/**
   Init command queue
*/
bool activateCommandQueueForHyundaiIoniq() {

  // 28kWh version
  params.batteryTotalAvailableKWh = 28;

  //  Empty and fill command queue
  for (int i = 0; i < 100; i++) {
    commandQueue[i] = "";
  }
  for (int i = 0; i < commandQueueCountHyundaiIoniq; i++) {
    commandQueue[i] = commandQueueHyundaiIoniq[i];
  }

  commandQueueLoopFrom = commandQueueLoopFromHyundaiIoniq;
  commandQueueCount = commandQueueCountHyundaiIoniq;

  return true;
}

/**
  Parse merged row
*/
bool parseRowMergedHyundaiIoniq() {

  // VMCU 7e2
  if (commandRequest.equals("2101")) {
    params.speedKmh = hexToDec(responseRowMerged.substring(32, 36).c_str(), 2, false) * 0.0155; // / 100.0 *1.609 = real to gps is 1.750
  }
  // VMCU 7e2
  if (commandRequest.equals("2102")) {
    params.auxPerc = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, false);
    params.auxCurrentAmp = - hexToDec(responseRowMerged.substring(46, 50).c_str(), 2, true) / 1000.0;
  }
  // Cluster module 7c6
  if (commandRequest.equals("22B002")) {
    params.odoKm = float(strtol(responseRowMerged.substring(18, 24).c_str(), 0, 16));
  }

  // Aircon 7b3
  // IONIQ OK
  if (commandRequest.equals("220100")) {
    params.indoorTemperature = (hexToDec(responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
    params.outdoorTemperature = (hexToDec(responseRowMerged.substring(18, 20).c_str(), 1, false) / 2) - 40;
  }
  // Aircon 7b3
  // IONIQ OK
  if (commandRequest.equals("220102") && responseRowMerged.substring(12, 14) == "00") {
    params.coolantTemp1C = (hexToDec(responseRowMerged.substring(14, 16).c_str(), 1, false) / 2) - 40;
    params.coolantTemp2C = (hexToDec(responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
  }
  // BMS 7e4
  if (commandRequest.equals("2101")) {
    params.cumulativeEnergyChargedKWh = float(strtol(responseRowMerged.substring(80, 88).c_str(), 0, 16)) / 10.0;
    if (params.cumulativeEnergyChargedKWhStart == -1)
      params.cumulativeEnergyChargedKWhStart = params.cumulativeEnergyChargedKWh;
    params.cumulativeEnergyDischargedKWh = float(strtol(responseRowMerged.substring(88, 96).c_str(), 0, 16)) / 10.0;
    if (params.cumulativeEnergyDischargedKWhStart == -1)
      params.cumulativeEnergyDischargedKWhStart = params.cumulativeEnergyDischargedKWh;
    params.auxVoltage = hexToDec(responseRowMerged.substring(62, 64).c_str(), 2, true) / 10.0;
    params.batPowerAmp = - hexToDec(responseRowMerged.substring(24, 28).c_str(), 2, true) / 10.0;
    params.batVoltage = hexToDec(responseRowMerged.substring(28, 32).c_str(), 2, false) / 10.0;
    params.batPowerKw = (params.batPowerAmp * params.batVoltage) / 1000.0;
    params.batPowerKwh100 = params.batPowerKw / params.speedKmh * 100;
    params.batCellMaxV = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, false) / 50.0;
    params.batCellMinV = hexToDec(responseRowMerged.substring(54, 56).c_str(), 1, false) / 50.0;
    params.batModule01TempC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);
    params.batModule02TempC = hexToDec(responseRowMerged.substring(38, 40).c_str(), 1, true);
    params.batModule03TempC = hexToDec(responseRowMerged.substring(40, 42).c_str(), 1, true);
    params.batModule04TempC = hexToDec(responseRowMerged.substring(42, 44).c_str(), 1, true);
    //params.batTempC = hexToDec(responseRowMerged.substring(34, 36).c_str(), 1, true);
    //params.batMaxC = hexToDec(responseRowMerged.substring(32, 34).c_str(), 1, true);
    //params.batMinC = hexToDec(responseRowMerged.substring(34, 36).c_str(), 1, true);

    // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
    params.batMinC = params.batMaxC = params.batModule01TempC;
    params.batMinC = (params.batModule02TempC < params.batMinC) ? params.batModule02TempC : params.batMinC;
    params.batMinC = (params.batModule03TempC < params.batMinC) ? params.batModule03TempC : params.batMinC;
    params.batMinC = (params.batModule04TempC < params.batMinC) ? params.batModule04TempC : params.batMinC;
    params.batMaxC = (params.batModule02TempC > params.batMaxC) ? params.batModule02TempC : params.batMaxC;
    params.batMaxC = (params.batModule03TempC > params.batMaxC) ? params.batModule03TempC : params.batMaxC;
    params.batMaxC = (params.batModule04TempC > params.batMaxC) ? params.batModule04TempC : params.batMaxC;
    params.batTempC = params.batMinC;

    params.batInletC = hexToDec(responseRowMerged.substring(48, 50).c_str(), 1, true);
    if (params.speedKmh < 15 && params.batPowerKw >= 1 && params.socPerc > 0 && params.socPerc <= 100) {
      params.chargingGraphKw[int(params.socPerc)] = params.batPowerKw;
      params.chargingGraphMinTempC[int(params.socPerc)] = params.batMinC;
      params.chargingGraphMaxTempC[int(params.socPerc)] = params.batMaxC;
    }
  }
  // BMS 7e4
  if (commandRequest.equals("2102") && responseRowMerged.substring(10, 12) == "FF") {
    for (int i = 0; i < 32; i++) {
      params.cellVoltage[i] = hexToDec(responseRowMerged.substring(12 + (i * 2), 12 + (i * 2) + 2).c_str(), 1, false) / 50;
    }
  }
  // BMS 7e4
  if (commandRequest.equals("2103")) {
    for (int i = 0; i < 32; i++) {
      params.cellVoltage[32 + i] = hexToDec(responseRowMerged.substring(12 + (i * 2), 12 + (i * 2) + 2).c_str(), 1, false) / 50;
    }
  }
  // BMS 7e4
  if (commandRequest.equals("2104")) {
    for (int i = 0; i < 32; i++) {
      params.cellVoltage[64 + i] = hexToDec(responseRowMerged.substring(12 + (i * 2), 12 + (i * 2) + 2).c_str(), 1, false) / 50;
    }
  }
  // BMS 7e4
  if (commandRequest.equals("2105")) {
    params.sohPerc = hexToDec(responseRowMerged.substring(54, 58).c_str(), 2, false) / 10.0;
    params.socPerc = hexToDec(responseRowMerged.substring(66, 68).c_str(), 1, false) / 2.0;

    // Soc10ced table, record x0% CEC/CED table (ex. 90%->89%, 80%->79%)
    if (oldParams.socPerc - params.socPerc > 0) {
      byte index = (int(params.socPerc) == 4) ? 0 : (int)(params.socPerc / 10) + 1;
      if ((int(params.socPerc) % 10 == 9 || int(params.socPerc) == 4) && params.soc10ced[index] == -1) {
        struct tm now;
        getLocalTime(&now, 0);
        time_t time_now_epoch = mktime(&now);
        params.soc10ced[index] = params.cumulativeEnergyDischargedKWh;
        params.soc10cec[index] = params.cumulativeEnergyChargedKWh;
        params.soc10odo[index] = params.odoKm;
        params.soc10time[index] = time_now_epoch;
      }
    }
    params.batHeaterC = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, true);
    //
    for (int i = 30; i < 32; i++) { // ai/aj position
      params.cellVoltage[96 - 30 + i] = -1;
    }
  }
  // BMS 7e4
  // IONIQ FAILED
  if (commandRequest.equals("2106")) {
    params.coolingWaterTempC = hexToDec(responseRowMerged.substring(14, 16).c_str(), 1, false);
  }
  // TPMS 7a0
  // IONIQ OK
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

  return true;
}

/**
   Test data
*/
bool testDataHyundaiIoniq() {

  // VMCU atsh7e2
  // 2101
  commandRequest = "2101";
  //eniro           = "6101FFF8000009285A3B0648030000B4179D763404080805000000";
  responseRowMerged = "6101FFE0000009211222062F03000000001D7734";
  parseRowMergedHyundaiIoniq();

  // VMCU atsh7e2
  // 2102
  commandRequest = "2102";
  //eniro           = "6102F8FFFC000101000000840FBF83BD33270680953033757F59291C76000001010100000007000000";
  responseRowMerged = "6102FF80000001010000009311AE8C9839640611887C3900000000";
  parseRowMergedHyundaiIoniq();

  // "atsh7df",
  // 2106
  //commandRequest = "2106";
  //eniro           = "6106FFFF800000000000000200001B001C001C000600060006000E000000010000000000000000013D013D013E013E00";
  //parseRowMergedHyundaiIoniq();

  // AIRCON / ACU atsh7b3
  // 220100
  commandRequest = "220100";
  //eniro           = "6201007E5027C8FF7F765D05B95AFFFF5AFF11FFFFFFFFFFFF6AFFFF2DF0757630FFFF00FFFF000000";
  responseRowMerged = "6201007E5007C8FF8A876A011010FFFF10FF10FFFFFFFFFFFFFFFFFF2EEF767D00FFFF00FFFF000000";
  parseRowMergedHyundaiIoniq();

  // AIRCON / ACU atsh7b3
  // 220102
  commandRequest = "220102";
  responseRowMerged = "620102FF800000A3950000000000002600000000";
  parseRowMergedHyundaiIoniq();


  // BMS atsh7e4
  // 220101
  commandRequest = "2101";
  //eniro           = "620101FFF7E7FFB3000000000300120F9B111011101011000014CC38CB3B00009100003A510000367C000015FB000013D3000690250D018E0000000003E8";
  responseRowMerged = "6101FFFFFFFF5026482648A3FFC30D9E181717171718170019B50FB501000090000142230001425F0000771B00007486007815D809015C0000000003E800";
  parseRowMergedHyundaiIoniq();

  // BMS atsh7e4
  // 220102
  commandRequest = "2102";
  // eniro          = "620102FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  responseRowMerged = "6102FFFFFFFFB5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5000000";
  parseRowMergedHyundaiIoniq();

  // BMS atsh7e4
  // 220103
  commandRequest = "2103";
  //eniro           = "620103FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCACBCACACFCCCBCBCBCBCBCBCBCBAAAA";
  responseRowMerged = "6103FFFFFFFFB5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5000000";
  parseRowMergedHyundaiIoniq();

  // BMS atsh7e4
  // 220104
  commandRequest = "2104";
  //eniro           = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  responseRowMerged = "6104FFFFFFFFB5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5B5000000";
  parseRowMergedHyundaiIoniq();

  // BMS atsh7e4
  // 220105
  commandRequest = "2105";
  //eniro           = "620105003fff9000000000000000000F8A86012B4946500101500DAC03E800000000AC0000C7C701000F00000000AAAA";
  responseRowMerged = "6105FFFFFFFF00000000001717171817171726482648000150181703E81A03E801520029000000000000000000000000";
  parseRowMergedHyundaiIoniq();

  // BMS atsh7e4
  // 220106
  commandRequest = "2106";
  //eniro           = "620106FFFFFFFF14001A00240000003A7C86B4B30000000928EA00";
  responseRowMerged = "7F2112"; // n/a on ioniq
  parseRowMergedHyundaiIoniq();

  // 22c002
  //commandRequest = "22c002";
  // eniro          = "62C002FFFF0000D2E84E93D2E84EBBD2DBDACBD2E149F3AAAAAAAA";
  //responseRowMerged = "";
  //parseRowMergedHyundaiIoniq();

  // BCM / TPMS atsh7a0
  // 22c00b
  commandRequest = "22c00b";
  // eniro           = "62C00BFFFF0000B93D0100B43E0100B43D0100BB3C0100AAAAAAAA";
  responseRowMerged = "62C00BFFFF0000B9510100B9510100B84F0100B54F0100AAAAAAAA";
  parseRowMergedHyundaiIoniq();

  // atsh7c6
  // 22b002
  commandRequest = "22b002";
  // eniro          = "62B002E0000000FFB400330B0000000000000000";
  responseRowMerged = "62B002E000000000AD003D2D0000000000000000";
  parseRowMergedHyundaiIoniq();

  /*  params.batModule01TempC = 28;
    params.batModule02TempC = 29;
    params.batModule03TempC = 28;
    params.batModule04TempC = 30;
    //params.batTempC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);
    //params.batMaxC = hexToDec(responseRowMerged.substring(34, 36).c_str(), 1, true);
    //params.batMinC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);

    // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
    params.batMinC = params.batMaxC = params.batModule01TempC;
    params.batMinC = (params.batModule02TempC < params.batMinC) ? params.batModule02TempC : params.batMinC ;
    params.batMinC = (params.batModule03TempC < params.batMinC) ? params.batModule03TempC : params.batMinC ;
    params.batMinC = (params.batModule04TempC < params.batMinC) ? params.batModule04TempC : params.batMinC ;
    params.batMaxC = (params.batModule02TempC > params.batMaxC) ? params.batModule02TempC : params.batMaxC ;
    params.batMaxC = (params.batModule03TempC > params.batMaxC) ? params.batModule03TempC : params.batMaxC ;
    params.batMaxC = (params.batModule04TempC > params.batMaxC) ? params.batModule04TempC : params.batMaxC ;
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
  */
  return true;
}
