#define ARDUINOJSON_USE_LONG_LONG 1

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include "BoardInterface.h"
#include "CommObd2Ble4.h"
#include "CommObd2Can.h"
#include "LiveData.h"

/**
   Set live data
*/
void BoardInterface::setLiveData(LiveData* pLiveData) {
  liveData = pLiveData;
}

/**
   Attach car interface
*/
void BoardInterface::attachCar(CarInterface* pCarInterface) {
  carInterface = pCarInterface;
}

/**
  Shutdown device
*/
void BoardInterface::shutdownDevice() {

  syslog->println("Shutdown.");

  char msg[20];
  for (int i = 3; i >= 1; i--) {
    sprintf(msg, "Shutdown in %d sec.", i);
    displayMessage(msg, "");
    delay(1000);
  }
/*
  if (liveData->params.sim800l_enabled) {
    if (sim800l->isConnectedGPRS()) {
      bool disconnected = sim800l->disconnectGPRS();
      for (uint8_t i = 0; i < 5 && !disconnected; i++) {
        delay(1000);
        disconnected = sim800l->disconnectGPRS();
      }
    }

    if (sim800l->getPowerMode() == NORMAL) {
      sim800l->setPowerMode(SLEEP);
      delay(1000);
    }
    sim800l->enterSleepMode();
  }
*/
  setCpuFrequencyMhz(80);
  setBrightness(0);
  //WiFi.disconnect(true);
  //WiFi.mode(WIFI_OFF);

  commInterface->disconnectDevice();
  //adc_power_off();
  //esp_wifi_stop();
  esp_bt_controller_disable();

  delay(2000);
  //esp_sleep_enable_timer_wakeup(525600L * 60L * 1000000L); // minutes
  esp_deep_sleep_start();
}

/**
  Load settings from flash memory, upgrade structure if version differs
*/
void BoardInterface::saveSettings() {

  // Flash to memory
  syslog->println("Settings saved to eeprom.");
  EEPROM.put(0, liveData->settings);
  EEPROM.commit();
}

/**
  Reset settings (factory reset)
*/
void BoardInterface::resetSettings() {

  // Flash to memory
  syslog->println("Factory reset.");
  liveData->settings.initFlag = 1;
  EEPROM.put(0, liveData->settings);
  EEPROM.commit();

  displayMessage("Settings erased", "Restarting in 5 seconds");

  delay(5000);
  ESP.restart();
}

/**
  Load setting from flash memory, upgrade structure if version differs
*/
void BoardInterface::loadSettings() {

  String tmpStr;

  // Default settings
  liveData->settings.initFlag = 183;
  liveData->settings.settingsVersion = 10;
  liveData->settings.carType = CAR_KIA_ENIRO_2020_64;
  tmpStr = "00:00:00:00:00:00"; // Pair via menu (middle button)
  tmpStr.toCharArray(liveData->settings.obdMacAddress, tmpStr.length() + 1);
  tmpStr = "000018f0-0000-1000-8000-00805f9b34fb"; // Default UUID's for VGate iCar Pro BLE4 adapter
  tmpStr.toCharArray(liveData->settings.serviceUUID, tmpStr.length() + 1);
  tmpStr = "00002af0-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(liveData->settings.charTxUUID, tmpStr.length() + 1);
  tmpStr = "00002af1-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(liveData->settings.charRxUUID, tmpStr.length() + 1);
  liveData->settings.displayRotation = 1; // 1,3
  liveData->settings.distanceUnit = 'k';
  liveData->settings.temperatureUnit = 'c';
  liveData->settings.pressureUnit = 'b';
  liveData->settings.defaultScreen = 1;
  liveData->settings.lcdBrightness = 0;
  liveData->settings.predrawnChargingGraphs = 1;
  liveData->settings.commType = COMM_TYPE_OBD2BLE4; // BLE4
  liveData->settings.wifiEnabled = 0;
  tmpStr = "empty";
  tmpStr.toCharArray(liveData->settings.wifiSsid, tmpStr.length() + 1);
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.wifiPassword, tmpStr.length() + 1);
  liveData->settings.ntpEnabled = 0;
  liveData->settings.ntpTimezone = 1;
  liveData->settings.ntpDaySaveTime = 0;
  liveData->settings.sdcardEnabled = 0;
  liveData->settings.sdcardAutstartLog = 1;
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.gprsApn, tmpStr.length() + 1);
  // Remote upload
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.remoteApiUrl, tmpStr.length() + 1);
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.remoteApiKey, tmpStr.length() + 1);
  liveData->settings.headlightsReminder = 0;
  liveData->settings.gpsHwSerialPort = 255; // off
  liveData->settings.gprsHwSerialPort = 255; // off
  liveData->settings.serialConsolePort = 0; // hwuart0
  liveData->settings.debugLevel = 1; // 0 - info only, 1 - debug communication (BLE/CAN), 2 - debug GSM, 3 - debug SDcard
  liveData->settings.sdcardLogIntervalSec = 2;
  liveData->settings.gprsLogIntervalSec = 60;
  liveData->settings.sleepModeLevel = 0;

  // Load settings and replace default values
  syslog->println("Reading settings from eeprom.");
  EEPROM.begin(sizeof(SETTINGS_STRUC));
  EEPROM.get(0, liveData->tmpSettings);

  // Init flash with default settings
  if (liveData->tmpSettings.initFlag != 183) {
    syslog->println("Settings not found. Initialization.");
    saveSettings();
  } else {
    syslog->print("Loaded settings ver.: ");
    syslog->println(liveData->tmpSettings.settingsVersion);

    // Upgrade structure
    if (liveData->settings.settingsVersion != liveData->tmpSettings.settingsVersion) {
      if (liveData->tmpSettings.settingsVersion == 1) {
        liveData->tmpSettings.settingsVersion = 2;
        liveData->tmpSettings.defaultScreen = liveData->settings.defaultScreen;
        liveData->tmpSettings.lcdBrightness = liveData->settings.lcdBrightness;
      }
      if (liveData->tmpSettings.settingsVersion == 2) {
        liveData->tmpSettings.settingsVersion = 3;
        liveData->tmpSettings.predrawnChargingGraphs = liveData->settings.predrawnChargingGraphs;
      }
      if (liveData->tmpSettings.settingsVersion == 3) {
        liveData->tmpSettings.settingsVersion = 4;
        liveData->tmpSettings.commType = COMM_TYPE_OBD2BLE4; // BLE4
        liveData->tmpSettings.wifiEnabled = 0;
        tmpStr = "empty";
        tmpStr.toCharArray(liveData->tmpSettings.wifiSsid, tmpStr.length() + 1);
        tmpStr = "not_set";
        tmpStr.toCharArray(liveData->tmpSettings.wifiPassword, tmpStr.length() + 1);
        liveData->tmpSettings.ntpEnabled = 0;
        liveData->tmpSettings.ntpTimezone = 1;
        liveData->tmpSettings.ntpDaySaveTime = 0;
        liveData->tmpSettings.sdcardEnabled = 0;
        liveData->tmpSettings.sdcardAutstartLog = 1;
        tmpStr = "internet.t-mobile.cz";
        tmpStr.toCharArray(liveData->tmpSettings.gprsApn, tmpStr.length() + 1);
        // Remote upload
        tmpStr = "http://api.example.com";
        tmpStr.toCharArray(liveData->tmpSettings.remoteApiUrl, tmpStr.length() + 1);
        tmpStr = "example";
        tmpStr.toCharArray(liveData->tmpSettings.remoteApiKey, tmpStr.length() + 1);
        liveData->tmpSettings.headlightsReminder = 0;
      }
      if (liveData->tmpSettings.settingsVersion == 4) {
        liveData->tmpSettings.settingsVersion = 5;
        liveData->tmpSettings.gpsHwSerialPort = 255; // off
      }
      if (liveData->tmpSettings.settingsVersion == 5) {
        liveData->tmpSettings.settingsVersion = 6;
        liveData->tmpSettings.serialConsolePort = 0; // hwuart0
        liveData->tmpSettings.debugLevel = 0; // show all
        liveData->tmpSettings.sdcardLogIntervalSec = 2;
        liveData->tmpSettings.gprsLogIntervalSec = 60;
      }
      if (liveData->tmpSettings.settingsVersion == 6) {
        liveData->tmpSettings.settingsVersion = 7;
        liveData->tmpSettings.sleepModeLevel = 0;
      }
      if (liveData->tmpSettings.settingsVersion == 7) {
        liveData->tmpSettings.settingsVersion = 8;
        liveData->tmpSettings.voltmeterEnabled = 0;
        liveData->tmpSettings.voltmeterBasedSleep = 0;
        liveData->tmpSettings.voltmeterCutOff = 12.0;
        liveData->tmpSettings.voltmeterSleep = 12.8;
        liveData->tmpSettings.voltmeterWakeUp = 13.0;
      }
      if (liveData->tmpSettings.settingsVersion == 8) {
        liveData->tmpSettings.settingsVersion = 9;
        liveData->tmpSettings.remoteUploadIntervalSec = 60;
        liveData->tmpSettings.sleepModeIntervalSec = 30;
        liveData->tmpSettings.sleepModeShutdownHrs = 72;
        liveData->tmpSettings.remoteUploadModuleType = 0;
      }
      if (liveData->tmpSettings.settingsVersion == 9) {
        liveData->tmpSettings.settingsVersion = 10;
        liveData->tmpSettings.remoteUploadAbrpIntervalSec = 0;
        tmpStr = "empty";
        tmpStr.toCharArray(liveData->tmpSettings.abrpApiToken, tmpStr.length() + 1);
      }

      // Save upgraded structure
      liveData->settings = liveData->tmpSettings;
      saveSettings();
    }

    // Apply settings from flash if needed
    liveData->settings = liveData->tmpSettings;
  }

  syslog->setDebugLevel(liveData->settings.debugLevel);
}

/**
   After setup
*/
void BoardInterface::afterSetup() {

  syslog->println("BoardInterface::afterSetup");

  // Init Comm iterface
  syslog->print("Init communication device: ");
  syslog->println(liveData->settings.commType);

  if (liveData->settings.commType == COMM_TYPE_OBD2BLE4) {;
    commInterface = new CommObd2Ble4();
  } else if (liveData->settings.commType == COMM_TYPE_OBD2CAN) {
    commInterface = new CommObd2Can();
  } else if (liveData->settings.commType == COMM_TYPE_OBD2BT3) {
    //commInterface = new CommObd2Bt3();
    syslog->println("BT3 not implemented");
  }

  commInterface->initComm(liveData, this);
  commInterface->connectDevice();
  carInterface->setCommInterface(commInterface);
}

/**
   Custom commands
*/
void BoardInterface::customConsoleCommand(String cmd) {

  if (cmd.equals("reboot")) ESP.restart();
  // CAN comparer
  if (cmd.equals("compare")) commInterface->compareCanRecords();
  
  int8_t idx = cmd.indexOf("=");
  if (idx == -1)
    return;

  String key = cmd.substring(0, idx);
  String value = cmd.substring(idx + 1);

  if (key == "serviceUUID") value.toCharArray(liveData->settings.serviceUUID, value.length() + 1);
  if (key == "charTxUUID") value.toCharArray(liveData->settings.charTxUUID, value.length() + 1);
  if (key == "charRxUUID") value.toCharArray(liveData->settings.charRxUUID, value.length() + 1);
  if (key == "wifiSsid") value.toCharArray(liveData->settings.wifiSsid, value.length() + 1);
  if (key == "wifiPassword") value.toCharArray(liveData->settings.wifiPassword, value.length() + 1);
  if (key == "gprsApn") value.toCharArray(liveData->settings.gprsApn, value.length() + 1);
  if (key == "remoteApiUrl") value.toCharArray(liveData->settings.remoteApiUrl, value.length() + 1);
  if (key == "remoteApiKey") value.toCharArray(liveData->settings.remoteApiKey, value.length() + 1);
  if (key == "abrpApiToken") value.toCharArray(liveData->settings.abrpApiToken, value.length() + 1);
  if (key == "debugLevel") { 
    liveData->settings.debugLevel = value.toInt(); 
    syslog->setDebugLevel(liveData->settings.debugLevel);     
  }
  // CAN comparer
  if (key == "record") commInterface->recordLoop(value.toInt());
  if (key == "test") carInterface->testHandler(value);
}

/**
   Parser response from obd2/can
*/
void BoardInterface::parseRowMerged() {

  carInterface->parseRowMerged();
}


/**
   Serialize parameters
*/
bool BoardInterface::serializeParamsToJson(File file, bool inclApiKey) {

  StaticJsonDocument<2048> jsonData;

  if (inclApiKey)
    jsonData["apiKey"] = liveData->settings.remoteApiKey;

  jsonData["carType"] = liveData->settings.carType;
  jsonData["batTotalKwh"] = liveData->params.batteryTotalAvailableKWh;
  jsonData["currTime"] = liveData->params.currentTime;
  jsonData["opTime"] = liveData->params.operationTimeSec;

  jsonData["gpsSat"] = liveData->params.gpsSat;
  jsonData["lat"] = liveData->params.gpsLat;
  jsonData["lon"] = liveData->params.gpsLon;
  jsonData["alt"] = liveData->params.gpsAlt;

  jsonData["ignitionOn"] = liveData->params.ignitionOn;
  jsonData["chargingOn"] = liveData->params.chargingOn;

  jsonData["socPerc"] = liveData->params.socPerc;
  jsonData["sohPerc"] = liveData->params.sohPerc;
  jsonData["powKwh100"] = liveData->params.batPowerKwh100;
  jsonData["speedKmh"] = liveData->params.speedKmh;
  jsonData["motorRpm"] = liveData->params.motorRpm;
  jsonData["odoKm"] = liveData->params.odoKm;

  jsonData["batPowKw"] = liveData->params.batPowerKw;
  jsonData["batPowA"] = liveData->params.batPowerAmp;
  jsonData["batV"] = liveData->params.batVoltage;
  jsonData["cecKwh"] = liveData->params.cumulativeEnergyChargedKWh;
  jsonData["cedKwh"] = liveData->params.cumulativeEnergyDischargedKWh;
  jsonData["maxChKw"] = liveData->params.availableChargePower;
  jsonData["maxDisKw"] = liveData->params.availableDischargePower;

  jsonData["cellMinV"] = liveData->params.batCellMinV;
  jsonData["cellMaxV"] = liveData->params.batCellMaxV;
  jsonData["bMinC"] = round(liveData->params.batMinC);
  jsonData["bMaxC"] = round(liveData->params.batMaxC);
  jsonData["bHeatC"] = round(liveData->params.batHeaterC);
  jsonData["bInletC"] = round(liveData->params.batInletC);
  jsonData["bFanSt"] = liveData->params.batFanStatus;
  jsonData["bWatC"] = round(liveData->params.coolingWaterTempC);
  jsonData["tmpA"] = round(liveData->params.bmsUnknownTempA);
  jsonData["tmpB"] = round(liveData->params.bmsUnknownTempB);
  jsonData["tmpC"] = round(liveData->params.bmsUnknownTempC);
  jsonData["tmpD"] = round(liveData->params.bmsUnknownTempD);

  jsonData["invC"] = round(liveData->params.inverterTempC);
  jsonData["motC"] = round(liveData->params.motorTempC);

  jsonData["auxPerc"] = liveData->params.auxPerc;
  jsonData["auxV"] = liveData->params.auxVoltage;
  jsonData["auxA"] = liveData->params.auxCurrentAmp;

  jsonData["inC"] = liveData->params.indoorTemperature;
  jsonData["outC"] = liveData->params.outdoorTemperature;
  jsonData["evapC"] = liveData->params.evaporatorTempC;
  jsonData["c1C"] = liveData->params.coolantTemp1C;
  jsonData["c2C"] = liveData->params.coolantTemp2C;

  jsonData["tFlC"] = liveData->params.tireFrontLeftTempC;
  jsonData["tFlBar"] = round(liveData->params.tireFrontLeftPressureBar * 10) / 10;
  jsonData["tFrC"] = liveData->params.tireFrontRightTempC;
  jsonData["tFrBar"] = round(liveData->params.tireFrontRightPressureBar * 10) / 10;
  jsonData["tRlC"] = liveData->params.tireRearLeftTempC;
  jsonData["tRlBar"] = round(liveData->params.tireRearLeftPressureBar * 10) / 10;
  jsonData["tRrC"] = liveData->params.tireRearRightTempC;
  jsonData["tRrBar"] = round(liveData->params.tireRearRightPressureBar * 10) / 10;

  jsonData["debug"] = liveData->params.debugData;
  jsonData["debug2"] = liveData->params.debugData2;

  serializeJson(jsonData, Serial);
  serializeJson(jsonData, file);

  return true;
}
