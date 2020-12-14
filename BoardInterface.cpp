#ifndef BOARDINTERFACE_CPP
#define BOARDINTERFACE_CPP

#define ARDUINOJSON_USE_LONG_LONG 1

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include "BoardInterface.h"
#include "CommObd2Ble4.h";
#include "CommObd2Can.h";
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

  Serial.println("Shutdown.");

  char msg[20];
  for (int i = 3; i >= 1; i--) {
    sprintf(msg, "Shutdown in %d sec.", i);
    displayMessage(msg, "");
    delay(1000);
  }

#ifdef SIM800L_ENABLED
  if(sim800l->isConnectedGPRS()) {
    sim800l->disconnectGPRS();
  }
  sim800l->setPowerMode(MINIMUM);
#endif //SIM800L_ENABLED

  setCpuFrequencyMhz(80);
  setBrightness(0);
  //WiFi.disconnect(true);
  //WiFi.mode(WIFI_OFF);
  btStop();
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
  Serial.println("Settings saved to eeprom.");
  EEPROM.put(0, liveData->settings);
  EEPROM.commit();
}

/**
  Reset settings (factory reset)
*/
void BoardInterface::resetSettings() {

  // Flash to memory
  Serial.println("Factory reset.");
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
  liveData->settings.settingsVersion = 5;
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
  liveData->settings.debugScreen = 0;
  liveData->settings.predrawnChargingGraphs = 1;
  liveData->settings.commType = 0; // BLE4
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
  liveData->settings.gprsEnabled = 0;
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.gprsApn, tmpStr.length() + 1);
  // Remote upload
  liveData->settings.remoteUploadEnabled = 0;
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.remoteApiUrl, tmpStr.length() + 1);
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.remoteApiKey, tmpStr.length() + 1);
  liveData->settings.headlightsReminder = 0;
  liveData->settings.gpsHwSerialPort = 255; // off

  // Load settings and replace default values
  Serial.println("Reading settings from eeprom.");
  EEPROM.begin(sizeof(SETTINGS_STRUC));
  EEPROM.get(0, liveData->tmpSettings);

  // Init flash with default settings
  if (liveData->tmpSettings.initFlag != 183) {
    Serial.println("Settings not found. Initialization.");
    saveSettings();
  } else {
    Serial.print("Loaded settings ver.: ");
    Serial.println(liveData->tmpSettings.settingsVersion);

    // Upgrade structure
    if (liveData->settings.settingsVersion != liveData->tmpSettings.settingsVersion) {
      if (liveData->tmpSettings.settingsVersion == 1) {
        liveData->tmpSettings.settingsVersion = 2;
        liveData->tmpSettings.defaultScreen = liveData->settings.defaultScreen;
        liveData->tmpSettings.lcdBrightness = liveData->settings.lcdBrightness;
        liveData->tmpSettings.debugScreen = liveData->settings.debugScreen;
      }
      if (liveData->tmpSettings.settingsVersion == 2) {
        liveData->tmpSettings.settingsVersion = 3;
        liveData->tmpSettings.predrawnChargingGraphs = liveData->settings.predrawnChargingGraphs;
      }
      if (liveData->tmpSettings.settingsVersion == 3) {
        liveData->tmpSettings.settingsVersion = 4;
        liveData->tmpSettings.commType = 0; // BLE4
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
        liveData->tmpSettings.gprsEnabled = 0;
        tmpStr = "internet.t-mobile.cz";
        tmpStr.toCharArray(liveData->tmpSettings.gprsApn, tmpStr.length() + 1);
        // Remote upload
        liveData->tmpSettings.remoteUploadEnabled = 0;
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

      // Save upgraded structure
      liveData->settings = liveData->tmpSettings;
      saveSettings();
    }

    // Apply settings from flash if needed
    liveData->settings = liveData->tmpSettings;
  }
}

/**
   After setup
*/
void BoardInterface::afterSetup() {

  // Init Comm iterface
  if (liveData->settings.commType == COMM_TYPE_OBD2BLE4) {
    commInterface = new CommObd2Ble4();
  } else if (liveData->settings.commType == COMM_TYPE_OBD2CAN) {
    commInterface = new CommObd2Ble4();
    //commInterface = new CommObd2Can();
  }
  //commInterface->initComm(liveData, NULL);
  commInterface->connectDevice();
}

/**
   Custom commands
*/
void BoardInterface::customConsoleCommand(String cmd) {

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

  jsonData["auxPerc"] = liveData->params.auxPerc;
  jsonData["auxV"] = liveData->params.auxVoltage;
  jsonData["auxA"] = liveData->params.auxCurrentAmp;

  jsonData["inC"] = liveData->params.indoorTemperature;
  jsonData["outC"] = liveData->params.outdoorTemperature;
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
}

#endif // BOARDINTERFACE_CPP
