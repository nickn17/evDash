
#define commandQueueCountRenaultZoe 10
#define commandQueueLoopFromRenaultZoe 8

String commandQueueRenaultZoe[commandQueueCountRenaultZoe] = {
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

  // Loop from (KIA ENIRO) // TODO
  // BMS
  "ATSH7E4",
  "220101",   // power kw, ...
};

/**
   Init command queue
*/
bool activateCommandQueueForRenaultZoe() {

  params.batModuleTempCount = 4;
  params.batteryTotalAvailableKWh = 22;
  
  //  Empty and fill command queue
  for (int i = 0; i < 300; i++) {
    commandQueue[i] = "";
  }
  for (int i = 0; i < commandQueueCountRenaultZoe; i++) {
    commandQueue[i] = commandQueueRenaultZoe[i];
  }

  commandQueueLoopFrom = commandQueueLoopFromRenaultZoe;
  commandQueueCount = commandQueueCountRenaultZoe;

  return true;
}

/**
  Parse merged row
*/
bool parseRowMergedRenaultZoe() {

  // TODO

  return true;
}

/**
   Test data
*/
bool testDataRenaultZoe() {

  // TODO
 
  return true;
}
