
#define commandQueueCountDebugObd2Kia 256
#define commandQueueLoopFromDebugObd2Kia 8

String commandQueueDebugObd2Kia[commandQueueCountDebugObd2Kia] = {
  "AT Z",      // Reset all
  "AT I",      // Print the version ID
  "AT E0",     // Echo off
  "AT L0",     // Linefeeds off
  "AT S0",     // Printing of spaces on
  "AT SP 6",   // Select protocol to ISO 15765-4 CAN (11 bit ID, 500 kbit/s)
  "AT DP",
  "AT ST16",

  // Loop from here

  // Request ID  Response ID ECU name  Can bus Protocol  Description
  // 725 72D WPS B   Wireless phone charger
  //"ATSH725",
  //"2201", // All with negative resp.   "2202",   "2203",  "2101",  "2102",  "220101",  "220102",  "22B001",  "22C001",  "22C101",

  // 736 73E VESS  P   Virtual Engine Sound system
  //"ATSH736",
  //"2201", // All with negative resp.   "2202",   "2203",  "2101",  "2102",
  //"220101", // All with 62 response  "220102",  "22B001",  "22C001",  "22C101",

  // 755 75D BSD Right     Blind spot detection Right
  // "ATSH755",
  // "2201", // ALL with negative 7F2213, etc    "2202",   "2203",  "2101",    "2102",   "220101",   "220102",   "22B001",   "22C001",  "22C101",

  // 770 778 IGPM  All UDS Integrated Gateway and power control module
  "ATSH770",
  "22BC01", // 009 62BC01400000000001AAAAAAAA
  "22BC02", // 62BC0200000000
  "22BC03", // 00B 62BC03FDEE7C730A600000AAAA
  "22BC04", // 00B 62BC04B33F74EA0D002042AAAA
  "22BC05", // 00B 62BC05BF13200001000000AAAA
  "22BC06", // 00B 62BC06B48000002C000000AAAA
  "22BC07", // 00B 62BC070849DBC000101900AAAA
  //"22BC08", // ALL with NEGATIVE RESPONSE   "22BC09",  "22BC0A",  "22BC0B",  "22BC0C",  "22BC0D",  "22BC0E",  "22BC0F",

  // 783 78B AMP M   Amplifier
  //"ATSH783",
  // "2201",// ALL with NEGATIVE RESPONSE   "2202",  "2203",  "2101",  "2102",  "220101",  "220102",  "22B001",  "22C001",  "22C101",

  // 796 79E PGS C   Parking Guide System
  //"ATSH796",
  //"2201", // ALL with NEGATIVE RESPONSE   "2202",  "2203",  "2101",  "2102",  "220101",  "220102",  "22B001",  "22C001",  "22C101",

  // 7A0 7A8 BCM / TPMS  B UDS Body control module 22 B0 01 to 22 B0 0E
  //       C   Tire Pressure Monitoring "At least 22 C0 01 to 22 C0 02  & 22 C0 0B to 22 C0 0F"
  "ATSH7A0",
  "22B001", // 016 62B00140C20000000000000000000001010000000001AAAAAAAAAA
  "22B002", // 009 62B002C00000000300AAAAAAAA
  "22B003", // 018 62B003BFCB8000A23D63B164F8F7F73DF80000A400A4A4A4AAAAAA
  "22B004", // 00B 62B0047402994E0E008800AAAA
  "22B005", // 00B 62B0052000100000000800AAAA
  "22B006", // 00B 62B0062000000000000000AAAA
  "22B007", // 00B 62B007002001E000040000AAAA
  "22B008", // 00B 62B00800510C2000880004AAAA
  "22B009", // 00B 62B009FEEEFEEE08000800AAAA
  "22B00A", // 00B 62B00AE3FEE3000040C500AAAA
  //"22B00B", // 7F2231
  "22B00C", // 00B 62B00C3F00000000000000AAAA
  "22B00D", // 00B 62B00DFCFCFC0000000000AAAA
  "22B00E", // 00B 62B00E0800000000000000AAAA
  //"22B00F", // 7F2231
  "22C001", // 01D 62C001000000002E2E02500706B5B50A098C3C0000000001FF01000101AAAAAAAAAA
  "22C002", // 017 62C002FFFF0000D2E149F3D2DBDACBD2E84EBBD2E84E93AAAAAAAA
  "22C003", // 021 62C00300000000444F303101002E2E02500706B5B50A098C3C0000000001FF0100AA
  "22C004", // 021 62C004000000004E41303101002E2E024B0005B5B508088C3C0100000001FF0100AA
  "22C005", // 021 62C005000000004E54504D0100302F02500000ABAB00008C3C0000030001FF0000AA
  "22C006", // 021 62C00600000000444F303201002E2E02500706B5AB0A098C3C0000000001010100AA
  "22C007", // 021 62C007000000004E41303201002E2E024B0005B5AB08088C3C0100000001010100AA
  "22C008", // 021 62C00800000000434E303101002E2E02500706B5B50A098C3C0000020001FF0100AA
  "22C009", // 021 62C00900000000303030360000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFAA
  "22C00A", // 021 62C00A00000000303030370000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFAA
  "22C00B", // 017 62C00BFFFF0000AF470100B4480100B5460100B3460100AAAAAAAA
  "22C00C", // 025 62C00CFFFF03000000000000FFFF0000000000000000FFFF0000000000000000FFFF000000AAAAAAAA
  "22C00D", // 025 62C00DFFFF03000000000000FFFF0000000000000000FFFF0000000000000000FFFF000000AAAAAAAA
  "22C00E", // 025 62C00EFFFF03000000000000FFFF0000000000000000FFFF0000000000000000FFFF000000AAAAAAAA
  "22C00F", // 025 62C00FFFFF03000000000000FFFF0000000000000000FFFF0000000000000000FFFF000000AAAAAAAA

  // 7A1 7A9 DDM B   Driver door module
  // "ATSH7A1",
  // "2201", // All with NO DATA  "2202",    "2203",   "2101",   "2102",   "220101",   "220102",   "22B001",   "22C001",   "22C101",

  // 7A2 7AA ADM B   Assist door module
  //"ATSH7A2",
  // "2201", // ALL with NO DATA   "2202",   "2203",   "2101",   "2102",   "220101",   "220102",   "22B001",   "22C001",   "22C101",

  // 7A3 7AB PSM B UDS Power Seat Module
  // "ATSH7A3",
  // "22B401", // All with NO DATA   "22B402",   "22B403",   "22B404",   "22B405",   "22B406",   "22B407",   "22B408",   "22B409",   "22B40A",

  // 7A5 7AD SMK B UDS Smart Key
  "ATSH7A5",
  "22B001", // 7F2278 7F2231
  "22B002", // positive 
  "22B003", // positive
  "22B004", // 7F2278 7F2231
  "22B005", // positive
  "22B006", // positive
  "22B007", // positive
  "22B008", // positive
  "22B009", // positive
  "22B00A", // positive

  // 7B3 7BB AIRCON / ACU    UDS Aircondition
  "ATSH7B3",
  "220100",   // 026 6201007E5027C8FF7C6D6B05EFBCFFFFEFFF10FFFFFFFFFFFFBFFFFF52B3919900FFFF01FFFF000000   in/out temp
  // "220101",   // 7F2231
  "220102",   // 014 620102FFF80000B36B0101000101003C00016E12   coolant temp1, 2
  // "220103",   // 7F2231

  // 7B7 7BF BSD Left      Blind spot detection Left
  "ATSH7B7",
  // "2201", // ALL NEGATIVE RESP   "2202",   "2203",   "2101",   "2102",   "220101",   "220102",   "22B001",   "22C001",   "22C101",

  // 7C4 7CC MFC     Multi Function Camera
  "ATSH7C4",
  "220101",   // 6201010E
  "220102",   // 62010200000000

  // 7C6 7CE CM  C & M UDS Cluster Module
  "ATSH7C6",
  "22B001",   // 008 62B00100000000000000000000
  "22B002",   // 00F 62B002E0000000FFA200AD8F0000000000000000 odo
  "22B003",   // 008 62B00398000000010000000000
  //"22B004",   // NO DATA

  // 7D0 7D8 AEB   UDS?  Autonomous Emergency Breaking
  // "ATSH7D0",
  // "2201",  // ALL CODES WITH NEGATIVE RESPONSE
  // "2202",  // "2203",  // "2101",     // "2102",     // "220101",     // "220102",     // "22B001",     // "22C001",     // "22C101",

  // 7D1 7D9 ABS / ESP + AHB   UDS
  "ATSH7D1",
  "22C101",   // 02A 62C1015FD7E7D0FFFF00FF04D0D400000000FF7EFF0030F5010000FFFF7F6307F207FE05FF00FF3FFFFFAAAAAAAAAAAA
  "22C102",   // 01A 62C10237000000FFFFFFFFFFFF00FF05FFFFFF00FF5501FFFFFFAA
  "22C103",   // 01A 62C103BE3000000DFFF0FCFE7FFF7FFFFFFFFFFF000005B50000AA

  // 7D2 7DA AIRBAG      SRS Sytem
  // "ATSH7D2",
  // "2101",   // 7F2211
  // "2102",   // 7F2211
  // "220101",   // 7F2211
  // "220102",   // 7F2211
  // "22B001",   // 7F2211
  // "22C001",   // 7F2211
  // "22C101",   // 7F2211

  // 7D4 7DC EPS     Electric power steering
  "ATSH7D4",
  //"2101",   // 7F2121
  //"2102",   // 7F2121
  "220101",   // 012 6201018387FD009DFFF90100000010101921AAAA
  "220102",   // 008 6201020000000500AAAAAAAAAA
  // "22B001",   // 7F2231
  // "22C001",   // 7F2231
  // "22C101",   // 7F2231

  // 7DF UNKNOWN
  //"ATSH7DF",
  //"2106",     // 013 7F2112 028 6106FFFF800000000000000300001C001C001C000600060006000F000000010000000000000000015801580158015700
  //"220106",   // 01B 620106FFFFFFFF12001200307C7C00317C830000B4B3000A28EA00

  // 7E2 7EA VMCU  H & P KWP2000 Vehicle Motor Control Unit 21 01 to 21 02 & 1A 80++
  "ATSH7E2",
  "2101",     // 018 6101FFF8000009285A3806480300000000C4693404080805000000  speed, ...
  "2102",     // 027 6102F8FFFC000101000000851BB5780234FA0BAB8D1933E07F5B211C74000001010100000007000000  aux, ...
  //"2103",   // 7F2112
  //"1A80",   // Working VIN 1A 8A 8C 8D ...

  // 7E3 7EB MCU H & P KWP2000 Motor Control Unit 21 01 to 21 06
  "ATSH7E3",
  "2101",   // 01E 610100007FFF0C3C00BD8D0A3300B00900002D0252025033D500C3FF68FF00000000
  "2102",   // 03A 610207FFFFFF00000D000D00260008080909000001004614CDABC2CD3F005581720085CAF5265D0DC1CD0000EC3400000000000000000000FF0000000000
  "2103",   // 06E 610300007FFF0000000000000000000000000000000000005C010E02FDFD040400000000000000000000000048058D0C0200160000000000AA3F000005000000AE0102000000000000000000000000000000000000000000BB0B00000000000000000000680000000000E803000000
  "2104",   // 060 6104000001FF000000000000D7425D03000000000000050000007A2B00000000000000003200000000000000000000000000000000000000000000000000000000000000010000000100010001000000030000000000000000006D0000008E1B00
  "2105",   // 067 6105000001FF630200010000005900000C00630200010000000100000C006B0200020000003300250D0136010096BA03000100000C000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
  "2106",   // 013 6106000000030000000000000000000000000300
  //"2107",   // 7F2112

  // 7E4 7EC BMS P UDS Battery Management System 22 01 01 to 22 01 06
  "ATSH7E4",
  "220101",   // 03E 620101FFF7E7FF6B0000000003001C0E2F161414141513000012B930B9380000830003E1E30003C95B0001722C00015B5B0040D1660D016B0000000003E8 power kw, ...
  "220102",   // 027 620102FFFFFFFFB9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9AAAA cell voltages, screen 3 only
  "220103",   // 027 620103FFFFFFFFB9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9AAAA cell voltages, screen 3 only
  "220104",   // 027 620104FFFFFFFFB9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9B9AAAA
  "220105",   // 02E 620105003FFF90000000000000000012836A0142684268000150126E03E8000000006E0000B9B900000F00000000AAAA soh, soc, ..
  "220106",   // 01B 620106FFFFFFFF12001200307C7C00007C030000B4B3000A28EA00 cooling water temp
  // "220107",   // 7F2231

  // 7E5 7ED OBC   KWP2000 OnBoard Charger 21 01 to 21 03
  "ATSH7E5",
  "2101",   // 01A 6101DFE0000001010001000000000E2F0533051D0078000000C800
  "2102",   // 011 6102FE000000000403E8000001BF000028000000
  "2103",   // 039 6103FFFFFDF80000000000000000000000000000000000000000000000000000000000000000007576767600000000000000000004000400030000000000
  "2104",   // 022 6104FFF000000A280A280A280000E324006900000003000000000000000000000000
  "2105",   // 046 61050000000081AA791E8013791E779C791E8BD37907874A79108D67791473777915727E7914753179156FAE7917768F79147650792876257930757E7914759379167545791D000000000000
  "2106",   // 028 6106FFFF8000001C001C001C000600060006000E000000010000000000000000015801580158015800
  //"2107",   // ret 7F2112

  // 7E6 7EE ??      ?? 21 08 05 to 21 08 0F -> All negative response
  //"ATSH7E6",
  //"210805",   // ret 7F2112
  //"210806",   // ret 7F2112
  //"210807",   // ret 7F2112
};

/**
   Init command queue
*/
bool activateCommandQueueForDebugObd2Kia() {

  // 39 or 64 kWh model?
  params.batteryTotalAvailableKWh = 64;

  //  Empty and fill command queue
  for (uint16_t i = 0; i < 300; i++) {
    commandQueue[i] = "";
  }
  for (uint16_t i = 0; i < commandQueueCountDebugObd2Kia; i++) {
    commandQueue[i] = commandQueueDebugObd2Kia[i];
  }

  commandQueueLoopFrom = commandQueueLoopFromDebugObd2Kia;
  commandQueueCount = commandQueueCountDebugObd2Kia;

  return true;
}

/**
  Parse merged row
*/
bool parseRowMergedDebugObd2Kia() {

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
      params.auxVoltage = hexToDec(responseRowMerged.substring(64, 66).c_str(), 2, true) / 10.0;
      params.batPowerAmp = - hexToDec(responseRowMerged.substring(26, 30).c_str(), 2, true) / 10.0;
      params.batVoltage = hexToDec(responseRowMerged.substring(30, 34).c_str(), 2, false) / 10.0;
      params.batPowerKw = (params.batPowerAmp * params.batVoltage) / 1000.0;
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
      params.batMinC = (params.batModuleTempC[1] < params.batMinC) ? params.batModuleTempC[1] : params.batMinC;
      params.batMinC = (params.batModuleTempC[2] < params.batMinC) ? params.batModuleTempC[2] : params.batMinC;
      params.batMinC = (params.batModuleTempC[3] < params.batMinC) ? params.batModuleTempC[3] : params.batMinC;
      params.batMaxC = (params.batModuleTempC[1] > params.batMaxC) ? params.batModuleTempC[1] : params.batMaxC;
      params.batMaxC = (params.batModuleTempC[2] > params.batMaxC) ? params.batModuleTempC[2] : params.batMaxC;
      params.batMaxC = (params.batModuleTempC[3] > params.batMaxC) ? params.batModuleTempC[3] : params.batMaxC;
      params.batTempC = params.batMinC;

      params.batInletC = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, true);
      if (params.speedKmh < 10 && params.batPowerKw >= 1 && params.socPerc > 0 && params.socPerc <= 100) {
        if ( params.chargingGraphMinKw[int(params.socPerc)] == -100 || params.batPowerKw < params.chargingGraphMinKw[int(params.socPerc)])
          params.chargingGraphMinKw[int(params.socPerc)] = params.batPowerKw;
        if ( params.chargingGraphMaxKw[int(params.socPerc)] == -100 || params.batPowerKw > params.chargingGraphMaxKw[int(params.socPerc)])
          params.chargingGraphMaxKw[int(params.socPerc)] = params.batPowerKw;
        params.chargingGraphBatMinTempC[int(params.socPerc)] = params.batMinC;
        params.chargingGraphBatMaxTempC[int(params.socPerc)] = params.batMaxC;
        params.chargingGraphHeaterTempC[int(params.socPerc)] = params.batHeaterC;
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
bool testDataDebugObd2Kia() {

  // VMCU ATSH7E2
  currentAtshRequest = "ATSH7E2";
  // 2101
  commandRequest = "2101";
  responseRowMerged = "6101FFF8000009285A3B0648030000B4179D763404080805000000";
  parseRowMergedDebugObd2Kia();
  // 2102
  commandRequest = "2102";
  responseRowMerged = "6102F8FFFC000101000000840FBF83BD33270680953033757F59291C76000001010100000007000000";
  responseRowMerged = "6102F8FFFC000101000000931CC77F4C39040BE09BA7385D8158832175000001010100000007000000";
  parseRowMergedDebugObd2Kia();

  // "ATSH7DF",
  currentAtshRequest = "ATSH7DF";
  // 2106
  commandRequest = "2106";
  responseRowMerged = "6106FFFF800000000000000200001B001C001C000600060006000E000000010000000000000000013D013D013E013E00";
  parseRowMergedDebugObd2Kia();

  // AIRCON / ACU ATSH7B3
  currentAtshRequest = "ATSH7B3";
  // 220100
  commandRequest = "220100";
  responseRowMerged = "6201007E5027C8FF7F765D05B95AFFFF5AFF11FFFFFFFFFFFF6AFFFF2DF0757630FFFF00FFFF000000";
  responseRowMerged = "6201007E5027C8FF867C58121010FFFF10FF8EFFFFFFFFFFFF10FFFF0DF0617900FFFF01FFFF000000";
  parseRowMergedDebugObd2Kia();

  // BMS ATSH7E4
  currentAtshRequest = "ATSH7E4";
  // 220101
  commandRequest = "220101";
  responseRowMerged = "620101FFF7E7FF99000000000300B10EFE120F11100F12000018C438C30B00008400003864000035850000153A00001374000647010D017F0BDA0BDA03E8";
  responseRowMerged = "620101FFF7E7FFB3000000000300120F9B111011101011000014CC38CB3B00009100003A510000367C000015FB000013D3000690250D018E0000000003E8";
  parseRowMergedDebugObd2Kia();
  // 220102
  commandRequest = "220102";
  responseRowMerged = "620102FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMergedDebugObd2Kia();
  // 220103
  commandRequest = "220103";
  responseRowMerged = "620103FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCACBCACACFCCCBCBCBCBCBCBCBCBAAAA";
  parseRowMergedDebugObd2Kia();
  // 220104
  commandRequest = "220104";
  responseRowMerged = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMergedDebugObd2Kia();
  // 220105
  commandRequest = "220105";
  responseRowMerged = "620105003fff9000000000000000000F8A86012B4946500101500DAC03E800000000AC0000C7C701000F00000000AAAA";
  responseRowMerged = "620105003FFF90000000000000000014918E012927465000015013BB03E800000000BB0000CBCB01001300000000AAAA";
  parseRowMergedDebugObd2Kia();
  // 220106
  commandRequest = "220106";
  responseRowMerged = "620106FFFFFFFF14001A00240000003A7C86B4B30000000928EA00";
  parseRowMergedDebugObd2Kia();

  // BCM / TPMS ATSH7A0
  currentAtshRequest = "ATSH7A0";
  // 22c00b
  commandRequest = "22c00b";
  responseRowMerged = "62C00BFFFF0000B93D0100B43E0100B43D0100BB3C0100AAAAAAAA";
  parseRowMergedDebugObd2Kia();

  // ATSH7C6
  currentAtshRequest = "ATSH7C6";
  // 22b002
  commandRequest = "22b002";
  responseRowMerged = "62B002E0000000FFB400330B0000000000000000";
  parseRowMergedDebugObd2Kia();

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
