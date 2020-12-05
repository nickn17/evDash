#ifndef BOARDINTERFACE_CPP
#define BOARDINTERFACE_CPP

#include <EEPROM.h>
#include <BLEDevice.h>
#include "BoardInterface.h"
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

  displayMessage("Shutdown in 3 sec.", "");
  delay(3000);

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
  liveData->settings.settingsVersion = 4;
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
  tmpStr = "internet.t-mobile.cz";
  tmpStr.toCharArray(liveData->settings.gprsApn, tmpStr.length() + 1);
  // Remote upload
  liveData->settings.remoteUploadEnabled = 0;
  tmpStr = "http://api.example.com";
  tmpStr.toCharArray(liveData->settings.remoteApiUrl, tmpStr.length() + 1);
  tmpStr = "example";
  tmpStr.toCharArray(liveData->settings.remoteApiKey, tmpStr.length() + 1);
  liveData->settings.headlightsReminder = 0;

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
        liveData->settings.headlightsReminder = 0;
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
 * Custom commands
 */
void BoardInterface::customConsoleCommand(String cmd) {
  
}

#endif // BOARDINTERFACE_CPP
