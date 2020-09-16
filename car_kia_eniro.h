
#define commandQueueCountKiaENiro 25
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
  "atsh7e4",
  "220101",   // power kw, ...
  "220102",   // cell voltages, screen 3 only
  "220103",   // cell voltages, screen 3 only
  "220104",   // cell voltages, screen 3 only
  "220105",   // soh, soc, ..
  "220106",   // cooling water temp

  // VMCU
  "atsh7e2",
  "2101",     // speed, ...
  "2102",     // aux, ...

  //"atsh7df",
  //"2106",
  //"220106",

  // ECU - Aircondition
  "atsh7b3",
  "220100",   // in/out temp
  "220102",   // coolant temp1, 2

  // BCM / TPMS
  "atsh7a0",
  "22c00b",   // tire pressure/temp

  // CLUSTER MODULE
  "atsh7c6",
  "22B002",   // odo
};

/**
 * Init command queue
 */
bool activateCommandQueueForKiaENiro() {

  // 39 or 64 kWh model?
  params.batteryTotalAvailableKWh = 64;
  if (settings.carType == CAR_KIA_ENIRO_2020_39 || settings.carType == CAR_HYUNDAI_KONA_2020_39) {
    params.batteryTotalAvailableKWh = 39.2;
  }

  //  Empty and fill command queue
  for (int i = 0; i < 100; i++) {
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

  if (commandRequest.equals("2101")) {
    params.speedKmh = hexToDec(responseRowMerged.substring(32, 36).c_str(), 2, false) * 0.0155; // / 100.0 *1.609 = real to gps is 1.750
  }
  if (commandRequest.equals("2102")) {
    params.auxPerc = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, false);
    params.auxCurrentAmp = - hexToDec(responseRowMerged.substring(46, 50).c_str(), 2, true) / 1000.0;
  }
  // Cluster module 7c6
  if (commandRequest.equals("22B002")) {
    params.odoKm = float(strtol(responseRowMerged.substring(18, 24).c_str(), 0, 16));
  }

  // Aircon 7b3
  if (commandRequest.equals("220100")) {
    params.indoorTemperature = (hexToDec(responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
    params.outdoorTemperature = (hexToDec(responseRowMerged.substring(18, 20).c_str(), 1, false) / 2) - 40;
  }
  // Aircon 7b3
  if (commandRequest.equals("220102") && responseRowMerged.substring(12, 14) == "00") {
    params.coolantTemp1C = (hexToDec(responseRowMerged.substring(14, 16).c_str(), 1, false) / 2) - 40;
    params.coolantTemp2C = (hexToDec(responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40;
  }
  // BMS 7e4
  if (commandRequest.equals("220101")) {
    params.cumulativeEnergyChargedKWh = float(strtol(responseRowMerged.substring(82, 90).c_str(), 0, 16)) / 10.0;
    if (params.cumulativeEnergyChargedKWhStart == -1)
      params.cumulativeEnergyChargedKWhStart = params.cumulativeEnergyChargedKWh;
    params.cumulativeEnergyDischargedKWh = float(strtol(responseRowMerged.substring(90, 98).c_str(), 0, 16)) / 10.0;
    if (params.cumulativeEnergyDischargedKWhStart == -1)
      params.cumulativeEnergyDischargedKWhStart = params.cumulativeEnergyDischargedKWh;
    params.auxVoltage = hexToDec(responseRowMerged.substring(64, 66).c_str(), 2, true) / 10.0;
    params.batPowerAmp = - hexToDec(responseRowMerged.substring(26, 30).c_str(), 2, true) / 10.0;
    params.batVoltage = hexToDec(responseRowMerged.substring(30, 34).c_str(), 2, false) / 10.0;
    params.batPowerKw = (params.batPowerAmp * params.batVoltage) / 1000.0;
    params.batPowerKwh100 = params.batPowerKw / params.speedKmh * 100;
    params.batCellMaxV = hexToDec(responseRowMerged.substring(52, 54).c_str(), 1, false) / 50.0;
    params.batCellMinV = hexToDec(responseRowMerged.substring(56, 58).c_str(), 1, false) / 50.0;
    params.batModule01TempC = hexToDec(responseRowMerged.substring(38, 40).c_str(), 1, true);
    params.batModule02TempC = hexToDec(responseRowMerged.substring(40, 42).c_str(), 1, true);
    params.batModule03TempC = hexToDec(responseRowMerged.substring(42, 44).c_str(), 1, true);
    params.batModule04TempC = hexToDec(responseRowMerged.substring(44, 46).c_str(), 1, true);
    //params.batTempC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);
    //params.batMaxC = hexToDec(responseRowMerged.substring(34, 36).c_str(), 1, true);
    //params.batMinC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);

    // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
    params.batMinC = params.batMaxC = params.batModule01TempC;
    params.batMinC = (params.batModule02TempC < params.batMinC) ? params.batModule02TempC : params.batMinC;
    params.batMinC = (params.batModule03TempC < params.batMinC) ? params.batModule03TempC : params.batMinC;
    params.batMinC = (params.batModule04TempC < params.batMinC) ? params.batModule04TempC : params.batMinC;
    params.batMaxC = (params.batModule02TempC > params.batMaxC) ? params.batModule02TempC : params.batMaxC;
    params.batMaxC = (params.batModule03TempC > params.batMaxC) ? params.batModule03TempC : params.batMaxC;
    params.batMaxC = (params.batModule04TempC > params.batMaxC) ? params.batModule04TempC : params.batMaxC;
    params.batTempC = params.batMinC;

    params.batInletC = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, true);
    if (params.speedKmh < 15 && params.batPowerKw >= 1 && params.socPerc > 0 && params.socPerc <= 100) {
      params.chargingGraphKw[int(params.socPerc)] = params.batPowerKw;
      params.chargingGraphMinTempC[int(params.socPerc)] = params.batMinC;
      params.chargingGraphMaxTempC[int(params.socPerc)] = params.batMaxC;
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
        struct tm now;
        getLocalTime(&now, 0);
        time_t time_now_epoch = mktime(&now);
        params.soc10ced[index] = params.cumulativeEnergyDischargedKWh;
        params.soc10cec[index] = params.cumulativeEnergyChargedKWh;
        params.soc10odo[index] = params.odoKm;
        params.soc10time[index] = time_now_epoch;
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
  // TPMS 7a0
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
