#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "config.h"
#include "CarModelUtils.h"
#include "Board320_240.h"

#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
#include "WebInterface.h"
extern WebInterface *webInterface;
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3

String Board320_240::menuItemText(int16_t menuItemId, String title)
{
  String prefix = "", suffix = "";
  uint8_t perc = 0;

  switch (menuItemId)
  {
  // Set vehicle type
  case MENU_VEHICLE_TYPE:
    suffix = getCarModelAbrpStr(liveData->settings.carType);
    suffix = String("[" + suffix.substring(0, suffix.indexOf(":", 12)) + "]");
    break;
  case MENU_SAVE_SETTINGS:
    sprintf(tmpStr1, "[v%d]", liveData->settings.settingsVersion);
    suffix = tmpStr1;
    break;
  case MENU_APP_VERSION:
    sprintf(tmpStr1, "[%s]", APP_VERSION);
    suffix = tmpStr1;
    break;
  // TODO: Why is do these cases not match the vehicle type id?
  case VEHICLE_TYPE_IONIQ_2018_28:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ_2018) ? ">" : "";
    break;
  case VEHICLE_TYPE_IONIQ_2018_PHEV:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ_PHEV) ? ">" : "";
    break;
  case VEHICLE_TYPE_KONA_2020_64:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64) ? ">" : "";
    break;
  case VEHICLE_TYPE_KONA_2020_39:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_KONA_2020_39) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ5_58_63:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_58_63) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ5_72:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_72) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ5_77_84:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_77_84) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ6_58_63:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ6_58_63) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ6_77_84:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ6_77_84) ? ">" : "";
    break;
  case VEHICLE_TYPE_ENIRO_2020_64:
    prefix = (liveData->settings.carType == CAR_KIA_ENIRO_2020_64) ? ">" : "";
    break;
  case VEHICLE_TYPE_ENIRO_2020_39:
    prefix = (liveData->settings.carType == CAR_KIA_ENIRO_2020_39) ? ">" : "";
    break;
  case VEHICLE_TYPE_ESOUL_2020_64:
    prefix = (liveData->settings.carType == CAR_KIA_ESOUL_2020_64) ? ">" : "";
    break;
  case VEHICLE_TYPE_KIA_EV6_58_63:
    prefix = (liveData->settings.carType == CAR_KIA_EV6_58_63) ? ">" : "";
    break;
  case VEHICLE_TYPE_KIA_EV6_77_84:
    prefix = (liveData->settings.carType == CAR_KIA_EV6_77_84) ? ">" : "";
    break;
  case VEHICLE_TYPE_KIA_EV9_100:
    prefix = (liveData->settings.carType == CAR_KIA_EV9_100) ? ">" : "";
    break;
  case VEHICLE_TYPE_AUDI_Q4_35:
    prefix = (liveData->settings.carType == CAR_AUDI_Q4_35) ? ">" : "";
    break;
  case VEHICLE_TYPE_AUDI_Q4_40:
    prefix = (liveData->settings.carType == CAR_AUDI_Q4_40) ? ">" : "";
    break;
  case VEHICLE_TYPE_AUDI_Q4_45:
    prefix = (liveData->settings.carType == CAR_AUDI_Q4_45) ? ">" : "";
    break;
  case VEHICLE_TYPE_AUDI_Q4_50:
    prefix = (liveData->settings.carType == CAR_AUDI_Q4_50) ? ">" : "";
    break;
  case VEHICLE_TYPE_SKODA_ENYAQ_55:
    prefix = (liveData->settings.carType == CAR_SKODA_ENYAQ_55) ? ">" : "";
    break;
  case VEHICLE_TYPE_SKODA_ENYAQ_62:
    prefix = (liveData->settings.carType == CAR_SKODA_ENYAQ_62) ? ">" : "";
    break;
  case VEHICLE_TYPE_SKODA_ENYAQ_82:
    prefix = (liveData->settings.carType == CAR_SKODA_ENYAQ_82) ? ">" : "";
    break;
  case VEHICLE_TYPE_VW_ID3_2021_45:
    prefix = (liveData->settings.carType == CAR_VW_ID3_2021_45) ? ">" : "";
    break;
  case VEHICLE_TYPE_VW_ID3_2021_58:
    prefix = (liveData->settings.carType == CAR_VW_ID3_2021_58) ? ">" : "";
    break;
  case VEHICLE_TYPE_VW_ID3_2021_77:
    prefix = (liveData->settings.carType == CAR_VW_ID3_2021_77) ? ">" : "";
    break;
  case VEHICLE_TYPE_VW_ID4_2021_45:
    prefix = (liveData->settings.carType == CAR_VW_ID4_2021_45) ? ">" : "";
    break;
  case VEHICLE_TYPE_VW_ID4_2021_58:
    prefix = (liveData->settings.carType == CAR_VW_ID4_2021_58) ? ">" : "";
    break;
  case VEHICLE_TYPE_VW_ID4_2021_77:
    prefix = (liveData->settings.carType == CAR_VW_ID4_2021_77) ? ">" : "";
    break;
  case VEHICLE_TYPE_ZOE_22_DEV:
    prefix = (liveData->settings.carType == CAR_RENAULT_ZOE) ? ">" : "";
    break;
  case VEHICLE_TYPE_BMWI3_2014_22:
    prefix = (liveData->settings.carType == CAR_BMW_I3_2014) ? ">" : "";
    break;
  case VEHICLE_TYPE_PEUGEOT_E208:
    prefix = (liveData->settings.carType == CAR_PEUGEOT_E208) ? ">" : "";
    break;
  //
  case MENU_ADAPTER_CAN_COMMU:
    prefix = (liveData->settings.commType == COMM_TYPE_CAN_COMMU) ? ">" : "";
    break;
  case MENU_ADAPTER_OBD2_BLE4:
    prefix = (liveData->settings.commType == COMM_TYPE_OBD2_BLE4) ? ">" : "";
    break;
  case MENU_ADAPTER_OBD2_WIFI:
    prefix = (liveData->settings.commType == COMM_TYPE_OBD2_WIFI) ? ">" : "";
    break;
  case MENU_ADAPTER_OBD2_NAME:
    sprintf(tmpStr1, "%s", liveData->settings.obd2Name);
    suffix = tmpStr1;
    break;
  case MENU_ADAPTER_OBD2_IP:
    sprintf(tmpStr1, "%s", liveData->settings.obd2WifiIp);
    suffix = tmpStr1;
    break;
  case MENU_ADAPTER_OBD2_PORT:
    sprintf(tmpStr1, "%d", liveData->settings.obd2WifiPort);
    suffix = tmpStr1;
    break;
  case MENU_ADAPTER_COMMAND_QUEUE_AUTOSTOP:
    suffix = (liveData->settings.commandQueueAutoStop == 0) ? "[off]" : "[on]";
    break;
  case MENU_ADAPTER_DISABLE_COMMAND_OPTIMIZER:
    suffix = (liveData->settings.disableCommandOptimizer == 0) ? "[off]" : "[on]";
    break;
  case MENU_ADAPTER_THREADING:
    suffix = (liveData->settings.threading == 0) ? "[off]" : "[on]";
    break;
  case MENU_BOARD_POWER_MODE:
    suffix = (liveData->settings.boardPowerMode == 1) ? "[ext.]" : "[USB]";
    break;
  case MENU_REMOTE_UPLOAD:
    sprintf(tmpStr1, "[HW UART=%d]", liveData->settings.gprsHwSerialPort);
    suffix = (liveData->settings.gprsHwSerialPort == 255) ? "[off]" : tmpStr1;
    break;
  case MENU_REMOTE_UPLOAD_UART:
    sprintf(tmpStr1, "[HW UART=%d]", liveData->settings.gprsHwSerialPort);
    suffix = (liveData->settings.gprsHwSerialPort == 255) ? "[off]" : tmpStr1;
    break;
  case MENU_REMOTE_UPLOAD_TYPE:
    switch (liveData->settings.remoteUploadModuleType)
    {
    case REMOTE_UPLOAD_OFF:
      suffix = "[OFF]";
      break;
    case REMOTE_UPLOAD_WIFI:
      suffix = "[WIFI]";
      break;
    default:
      suffix = "[unknown]";
    }
    break;
  case MENU_REMOTE_UPLOAD_API_INTERVAL:
    sprintf(tmpStr1, "[%d sec]", liveData->settings.remoteUploadIntervalSec);
    suffix = (liveData->settings.remoteUploadIntervalSec == 0) ? "[off]" : tmpStr1;
    break;
  case MENU_REMOTE_UPLOAD_ABRP_INTERVAL:
    sprintf(tmpStr1, "[%d sec]", liveData->settings.remoteUploadAbrpIntervalSec);
    suffix = (liveData->settings.remoteUploadAbrpIntervalSec == 0) ? "[off]" : tmpStr1;
    break;
  case MENU_REMOTE_UPLOAD_ABRP_LOG_SDCARD:
    suffix = (liveData->settings.abrpSdcardLog == 0) ? "[off]" : "[on]";
    break;
  case MENU_REMOTE_UPLOAD_MQTT_ENABLED:
    suffix = (liveData->settings.mqttEnabled == 0) ? "[off]" : "[on]";
    break;
  case MENU_REMOTE_UPLOAD_MQTT_SERVER:
    sprintf(tmpStr1, "%s", liveData->settings.mqttServer);
    suffix = tmpStr1;
    break;
  case MENU_REMOTE_UPLOAD_MQTT_ID:
    sprintf(tmpStr1, "%s", liveData->settings.mqttId);
    suffix = tmpStr1;
    break;
  case MENU_REMOTE_UPLOAD_MQTT_USERNAME:
    sprintf(tmpStr1, "%s", liveData->settings.mqttUsername);
    suffix = tmpStr1;
    break;
  case MENU_REMOTE_UPLOAD_MQTT_PASSWORD:
    sprintf(tmpStr1, "%s", liveData->settings.mqttPassword);
    suffix = tmpStr1;
    break;
  case MENU_REMOTE_UPLOAD_MQTT_TOPIC:
    sprintf(tmpStr1, "%s", liveData->settings.mqttPubTopic);
    suffix = tmpStr1;
    break;
  case MENU_REMOTE_UPLOAD_CONTRIBUTE_ANONYMOUS_DATA_TO_EVDASH_DEV_TEAM:
    suffix = (liveData->settings.contributeData == 0) ? "[off]" : "[on]";
    break;
  case MENU_SDCARD:
    perc = 0;
    if (SD.totalBytes() != 0)
      perc = (uint8_t)(((float)SD.usedBytes() / (float)SD.totalBytes()) * 100.0);
    sprintf(tmpStr1, "[%d] %lluMB %d%%", SD.cardType(), SD.cardSize() / (1024 * 1024), perc);
    suffix = tmpStr1;
    break;
  case MENU_SERIAL_CONSOLE:
    suffix = (liveData->settings.serialConsolePort == 255) ? "[off]" : "[on]";
    break;
  case MENU_VOLTMETER:
    suffix = (liveData->settings.voltmeterEnabled == 0) ? "[off]" : "[on]";
    break;
  case MENU_DEBUG_LEVEL:
    switch (liveData->settings.debugLevel)
    {
    case DEBUG_NONE:
      suffix = "[all]";
      break;
    case DEBUG_COMM:
      suffix = "[comm]";
      break;
    case DEBUG_GSM:
      suffix = "[net]";
      break;
    case DEBUG_SDCARD:
      suffix = "[sdcard]";
      break;
    case DEBUG_GPS:
      suffix = "[gps]";
      break;
    default:
      suffix = "[unknown]";
    }
    break;
  case MENU_SCREEN_ROTATION:
    suffix = (liveData->settings.displayRotation == 1) ? "[vertical]" : "[normal]";
    break;
  case MENU_DEFAULT_SCREEN:
    sprintf(tmpStr1, "[%d]", liveData->settings.defaultScreen);
    suffix = tmpStr1;
    break;
  case MENU_SCREEN_BRIGHTNESS:
    sprintf(tmpStr1, "[%d%%]", liveData->settings.lcdBrightness);
    sprintf(tmpStr2, "[auto/%d%%]", ((liveData->params.lcdBrightnessCalc == -1) ? 100 : liveData->params.lcdBrightnessCalc));
    suffix = ((liveData->settings.lcdBrightness == 0) ? tmpStr2 : tmpStr1);
    break;
  case MENU_SLEEP_MODE:
    switch (liveData->settings.sleepModeLevel)
    {
    case SLEEP_MODE_OFF:
      suffix = "[off]";
      break;
    case SLEEP_MODE_SCREEN_ONLY:
      suffix = "[screen only]";
      break;
    default:
      suffix = "[unknown]";
    }
    break;
  case MENU_SLEEP_MODE_MODE:
    switch (liveData->settings.sleepModeLevel)
    {
    case SLEEP_MODE_OFF:
      suffix = "[off]";
      break;
    case SLEEP_MODE_SCREEN_ONLY:
      suffix = "[screen only]";
      break;
    default:
      suffix = "[unknown]";
    }
    break;
  case MENU_GPS_MODULE_TYPE:
    switch (liveData->settings.gpsModuleType)
    {
    case GPS_MODULE_TYPE_NEO_M8N:
      suffix = "[M5 NEO-M8N]";
      break;
    case GPS_MODULE_TYPE_M5_GNSS:
      suffix = "[M5 GNSS]";
      break;
    default:
      suffix = "[none]";
    }
    break;
  case MENU_GPS:
  case MENU_GPS_PORT:
    sprintf(tmpStr1, "[UART=%d]", liveData->settings.gpsHwSerialPort);
    suffix = (liveData->settings.gpsHwSerialPort == 255) ? "[off]" : tmpStr1;
    break;
  case MENU_GPS_SPEED:
    sprintf(tmpStr1, "[%d bps]", liveData->settings.gpsSerialPortSpeed);
    suffix = tmpStr1;
    break;
  case MENU_CAR_SPEED_TYPE:
    switch (liveData->settings.carSpeedType)
    {
    case CAR_SPEED_TYPE_CAR:
      suffix = "[CAR]";
      break;
    case CAR_SPEED_TYPE_GPS:
      suffix = "[GPS]";
      break;
    default:
      suffix = "[automatic]";
    }
    break;
  case MENU_CURRENT_TIME:
    struct tm now;
    if (getLocalTime(&now, 0))
    {
      sprintf(tmpStr1, "%02d-%02d-%02d %02d:%02d:%02d", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
      suffix = tmpStr1;
    }
    else
    {
      suffix = "[-- -- -- --:--:--]";
    }
    break;
  case MENU_TIMEZONE:
    sprintf(tmpStr1, "[%d hrs]", liveData->settings.timezone);
    suffix = tmpStr1;
    break;
  case MENU_DAYLIGHT_SAVING:
    suffix = (liveData->settings.daylightSaving == 1) ? "[on]" : "[off]";
    break;
  case MENU_SPEED_CORRECTION:
    sprintf(tmpStr1, "[%d]", liveData->settings.speedCorrection);
    suffix = tmpStr1;
    break;
  case MENU_RHD:
    suffix = (liveData->settings.rightHandDrive == 1) ? "[on]" : "[off]";
    break;
  //
  case MENU_SDCARD_ENABLED:
    sprintf(tmpStr1, "[%s]", (liveData->settings.sdcardEnabled == 1) ? "on" : "off");
    suffix = tmpStr1;
    break;
  case MENU_SDCARD_AUTOSTARTLOG:
    sprintf(tmpStr1, "[%s]", (liveData->settings.sdcardEnabled == 0) ? "n/a" : (liveData->settings.sdcardAutstartLog == 1) ? "on"
                                                                                                                           : "off");
    suffix = tmpStr1;
    break;
  case MENU_SDCARD_MOUNT_STATUS:
    sprintf(tmpStr1, "[%s]", (liveData->settings.sdcardEnabled == 0) ? "n/a" : (strlen(liveData->params.sdcardFilename) != 0) ? liveData->params.sdcardFilename
                                                                           : (liveData->params.sdcardInit)                    ? "READY"
                                                                                                                              : "MOUNT");
    suffix = tmpStr1;
    break;
  case MENU_SDCARD_REC:
    sprintf(tmpStr1, "[%s]", (liveData->settings.sdcardEnabled == 0) ? "n/a" : (liveData->params.sdcardRecording) ? "STOP"
                                                                                                                  : "START");
    suffix = tmpStr1;
    break;
  case MENU_SDCARD_INTERVAL:
    sprintf(tmpStr1, "[%d]", liveData->settings.sdcardLogIntervalSec);
    suffix = tmpStr1;
    break;
  //
  case MENU_VOLTMETER_ENABLED:
    sprintf(tmpStr1, "[%s]", (liveData->settings.voltmeterEnabled == 1) ? "on" : "off");
    suffix = tmpStr1;
    break;
  case MENU_VOLTMETER_SLEEP:
    sprintf(tmpStr1, "[%s]", (liveData->settings.voltmeterBasedSleep == 1) ? "on" : "off");
    suffix = tmpStr1;
    break;
  case MENU_VOLTMETER_SLEEPVOL:
    sprintf(tmpStr1, "[%.1f V]", liveData->settings.voltmeterSleep);
    suffix = tmpStr1;
    break;
  case MENU_VOLTMETER_WAKEUPVOL:
    sprintf(tmpStr1, "[%.1f V]", liveData->settings.voltmeterWakeUp);
    suffix = tmpStr1;
    break;
  case MENU_VOLTMETER_CUTOFFVOL:
    sprintf(tmpStr1, "[%.1f V]", liveData->settings.voltmeterCutOff);
    suffix = tmpStr1;
    break;

  //
  case MENU_WIFI_ENABLED:
    suffix = (liveData->settings.wifiEnabled == 1) ? "[on]" : "[off]";
    break;
  case MENU_WIFI_SSID:
    sprintf(tmpStr1, "%s", liveData->settings.wifiSsid);
    suffix = tmpStr1;
    break;
  case MENU_WIFI_PASSWORD:
    sprintf(tmpStr1, "%s", liveData->settings.wifiPassword);
    suffix = tmpStr1;
    break;
  case MENU_WIFI_ENABLED2:
    suffix = (liveData->settings.backupWifiEnabled == 1) ? "[on]" : "[off]";
    break;
  case MENU_WIFI_SSID2:
    sprintf(tmpStr1, "%s", liveData->settings.wifiSsid2);
    suffix = tmpStr1;
    break;
  case MENU_WIFI_PASSWORD2:
    sprintf(tmpStr1, "%s", liveData->settings.wifiPassword2);
    suffix = tmpStr1;
    break;
  case MENU_WIFI_NTP:
    suffix = (liveData->settings.ntpEnabled == 1) ? "[on]" : "[off]";
    break;
  case MENU_WIFI_ACTIVE:
    if (liveData->params.isWifiBackupLive == true)
    {
      suffix = "backup";
    }
    else
    {
      suffix = "main";
    }
    break;
  case MENU_WIFI_IPADDR:
    suffix = WiFi.localIP().toString();
    break;
  //
  case MENU_DISTANCE_UNIT:
    suffix = (liveData->settings.distanceUnit == 'k') ? "[km]" : "[mi]";
    break;
  case MENU_TEMPERATURE_UNIT:
    suffix = (liveData->settings.temperatureUnit == 'c') ? "[C]" : "[F]";
    break;
  case MENU_PRESSURE_UNIT:
    suffix = (liveData->settings.pressureUnit == 'b') ? "[bar]" : "[psi]";
    break;
  }

  title = ((prefix == "") ? "" : prefix + " ") + title + ((suffix == "") ? "" : " " + suffix);

  return title;
}

/**
 * Renders the menu UI on the screen.
 * Handles pagination/scrolling of menu items.
 * Highlights selected menu item.
 * Renders menu items from static menuItems array
 * and dynamic customMenu vector.
 */
void Board320_240::showMenu()
{
  uint16_t posY = 0, tmpCurrMenuItem = 0;
  int8_t idx, off = 2;

  liveData->menuVisible = true;
  spr.fillSprite(TFT_BLACK);
  spr.setTextDatum(TL_DATUM);
  sprSetFont(fontRobotoThin24);

  // dynamic car menu
  std::vector<String> customMenu;
  customMenu = carInterface->customMenu(liveData->menuCurrent);

  // Page scroll
  menuItemHeight = spr.fontHeight();
#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
  off = menuItemHeight / 4;
  menuItemHeight = menuItemHeight * 1.5;
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3
  menuVisibleCount = (int)(tft.height() / menuItemHeight);

  if (liveData->menuItemSelected >= liveData->menuItemOffset + menuVisibleCount)
    liveData->menuItemOffset = liveData->menuItemSelected - menuVisibleCount + 1;
  if (liveData->menuItemSelected < liveData->menuItemOffset)
    liveData->menuItemOffset = liveData->menuItemSelected;

  // Print items
  for (uint16_t i = 0; i < liveData->menuItemsCount; ++i)
  {
    if (liveData->menuCurrent == liveData->menuItems[i].parentId)
    {
      if (tmpCurrMenuItem >= liveData->menuItemOffset)
      {
        bool isMenuItemSelected = liveData->menuItemSelected == tmpCurrMenuItem;
        // spr.fillRect(0, posY, 320, menuItemHeight, TFT_BLACK);
        if (isMenuItemSelected)
        {
          spr.fillRect(0, posY, 320, 2, TFT_WHITE);
          spr.fillRect(0, posY + menuItemHeight - 2, 320, 2, TFT_WHITE);
        }
        spr.setTextColor(TFT_WHITE);
        sprDrawString(menuItemText(liveData->menuItems[i].id, liveData->menuItems[i].title).c_str(), 0, posY + off);
        posY += menuItemHeight;
      }
      tmpCurrMenuItem++;
    }
  }

  for (uint16_t i = 0; i < customMenu.size(); ++i)
  {
    if (tmpCurrMenuItem >= liveData->menuItemOffset)
    {
      bool isMenuItemSelected = liveData->menuItemSelected == tmpCurrMenuItem;
      if (isMenuItemSelected)
      {
        spr.fillRect(0, posY, 320, 2, TFT_WHITE);
        spr.fillRect(0, posY + menuItemHeight - 2, 320, 2, TFT_WHITE);
      }
      // spr.fillRect(0, posY, 320, menuItemHeight + 2, isMenuItemSelected ? TFT_WHITE : TFT_BLACK);
      spr.setTextColor(TFT_WHITE);
      idx = customMenu.at(i).indexOf("=");
      sprDrawString(customMenu.at(i).substring(idx + 1).c_str(), 0, posY + off);
      posY += menuItemHeight;
    }
    tmpCurrMenuItem++;
  }

  spr.pushSprite(0, 0);
}

/**
 * Hide menu
 */
void Board320_240::hideMenu()
{
  liveData->menuVisible = false;
  liveData->menuCurrent = 0;
  liveData->menuItemSelected = 0;
  redrawScreen();
}

/**
 * Move in menu with left/right button
 */
void Board320_240::menuMove(bool forward, bool rotate)
{
  uint16_t tmpCount = 0 + carInterface->customMenu(liveData->menuCurrent).size();

  for (uint16_t i = 0; i < liveData->menuItemsCount; ++i)
  {
    if (liveData->menuCurrent == liveData->menuItems[i].parentId && strlen(liveData->menuItems[i].title) != 0)
      tmpCount++;
  }
  if (forward)
  {
    liveData->menuItemSelected = (liveData->menuItemSelected >= tmpCount - 1) ? (rotate ? 0 : tmpCount - 1)
                                                                              : liveData->menuItemSelected + 1;
  }
  else
  {
    liveData->menuItemSelected = (liveData->menuItemSelected <= 0) ? (rotate ? tmpCount - 1 : 0)
                                                                   : liveData->menuItemSelected - 1;
  }
  showMenu();
}

/**
 * Enter menu item
 */
void Board320_240::menuItemClick()
{
  // Locate menu item for meta data
  MENU_ITEM *tmpMenuItem = NULL;
  uint16_t tmpCurrMenuItem = 0;
  int16_t parentMenu = -1;
  uint16_t i;
  String m1, m2;

  for (i = 0; i < liveData->menuItemsCount; ++i)
  {
    if (liveData->menuCurrent == liveData->menuItems[i].parentId)
    {
      if (parentMenu == -1)
        parentMenu = liveData->menuItems[i].targetParentId;
      if (liveData->menuItemSelected == tmpCurrMenuItem)
      {
        tmpMenuItem = &liveData->menuItems[i];
        break;
      }
      tmpCurrMenuItem++;
    }
  }

  // Look for item in car custom menu
  if (tmpMenuItem == NULL)
  {
    std::vector<String> customMenu;
    customMenu = carInterface->customMenu(liveData->menuCurrent);
    for (uint16_t i = 0; i < customMenu.size(); ++i)
    {
      if (liveData->menuItemSelected == tmpCurrMenuItem)
      {
        String tmp = customMenu.at(i).substring(0, customMenu.at(i).indexOf("="));
        syslog->println(tmp);
        carInterface->carCommand(tmp);
        return;
      }
      tmpCurrMenuItem++;
    }
  }

  // Exit menu, parent level menu, open item
  bool showParentMenu = false;
  if (liveData->menuItemSelected > 0 && tmpMenuItem != NULL)
  {
    syslog->print("Menu item: ");
    syslog->println(tmpMenuItem->id);
    printHeapMemory();
    // Device list
    if (tmpMenuItem->id > LIST_OF_BLE_DEV_TOP && tmpMenuItem->id < MENU_LAST)
    {
      strlcpy((char *)liveData->settings.obdMacAddress, (char *)tmpMenuItem->obdMacAddress, 20);
      strlcpy((char *)liveData->settings.obd2Name, (char *)tmpMenuItem->title, 18);
      syslog->print("Selected adapter MAC address ");
      syslog->println(liveData->settings.obdMacAddress);
      saveSettings();
      ESP.restart();
    }
    // Other menus
    switch (tmpMenuItem->id)
    {
    case MENU_CLEAR_STATS:
      liveData->clearDrivingAndChargingStats(CAR_MODE_DRIVE);
      hideMenu();
      return;
      break;
    // Set vehicle type
    case VEHICLE_TYPE_IONIQ_2018_28:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ_2018;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_IONIQ_2018_PHEV:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ_PHEV;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_KONA_2020_64:
      liveData->settings.carType = CAR_HYUNDAI_KONA_2020_64;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_KONA_2020_39:
      liveData->settings.carType = CAR_HYUNDAI_KONA_2020_39;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_HYUNDAI_IONIQ5_58_63:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ5_58_63;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_HYUNDAI_IONIQ5_72:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ5_72;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_HYUNDAI_IONIQ5_77_84:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ5_77_84;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_HYUNDAI_IONIQ6_58_63:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ6_58_63;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_HYUNDAI_IONIQ6_77_84:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ6_77_84;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_ENIRO_2020_64:
      liveData->settings.carType = CAR_KIA_ENIRO_2020_64;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_ENIRO_2020_39:
      liveData->settings.carType = CAR_KIA_ENIRO_2020_39;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_ESOUL_2020_64:
      liveData->settings.carType = CAR_KIA_ESOUL_2020_64;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_KIA_EV6_58_63:
      liveData->settings.carType = CAR_KIA_EV6_58_63;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_KIA_EV6_77_84:
      liveData->settings.carType = CAR_KIA_EV6_77_84;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_KIA_EV9_100:
      liveData->settings.carType = CAR_KIA_EV9_100;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_AUDI_Q4_35:
      liveData->settings.carType = CAR_AUDI_Q4_35;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_AUDI_Q4_40:
      liveData->settings.carType = CAR_AUDI_Q4_40;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_AUDI_Q4_45:
      liveData->settings.carType = CAR_AUDI_Q4_45;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_AUDI_Q4_50:
      liveData->settings.carType = CAR_AUDI_Q4_50;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_SKODA_ENYAQ_55:
      liveData->settings.carType = CAR_SKODA_ENYAQ_55;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_SKODA_ENYAQ_62:
      liveData->settings.carType = CAR_SKODA_ENYAQ_62;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_SKODA_ENYAQ_82:
      liveData->settings.carType = CAR_SKODA_ENYAQ_82;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_VW_ID3_2021_45:
      liveData->settings.carType = CAR_VW_ID3_2021_45;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_VW_ID3_2021_58:
      liveData->settings.carType = CAR_VW_ID3_2021_58;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_VW_ID3_2021_77:
      liveData->settings.carType = CAR_VW_ID3_2021_77;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_VW_ID4_2021_45:
      liveData->settings.carType = CAR_VW_ID4_2021_45;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_VW_ID4_2021_58:
      liveData->settings.carType = CAR_VW_ID4_2021_58;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_VW_ID4_2021_77:
      liveData->settings.carType = CAR_VW_ID4_2021_77;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_ZOE_22_DEV:
      liveData->settings.carType = CAR_RENAULT_ZOE;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_BMWI3_2014_22:
      liveData->settings.carType = CAR_BMW_I3_2014;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_PEUGEOT_E208:
      liveData->settings.carType = CAR_PEUGEOT_E208;
      showMenu();
      return;
      break;
    // Comm type
    case MENU_ADAPTER_OBD2_BLE4:
      liveData->settings.commType = COMM_TYPE_OBD2_BLE4;
      displayMessage("COMM_TYPE_OBD2_BLE4", "Rebooting");
      saveSettings();
      ESP.restart();
      break;
    case MENU_ADAPTER_CAN_COMMU:
      liveData->settings.commType = COMM_TYPE_CAN_COMMU;
      displayMessage("COMM_TYPE_CAN_COMMU", "Rebooting");
      saveSettings();
      ESP.restart();
      break;
    case MENU_ADAPTER_OBD2_WIFI:
      liveData->settings.commType = COMM_TYPE_OBD2_WIFI;
      displayMessage("COMM_TYPE_OBD2_WIFI", "Rebooting");
      saveSettings();
      ESP.restart();
      break;
    case MENU_ADAPTER_OBD2_NAME:
    case MENU_ADAPTER_OBD2_IP:
    case MENU_ADAPTER_OBD2_PORT:
      return;
      break;
    case MENU_ADAPTER_COMMAND_QUEUE_AUTOSTOP:
      liveData->settings.commandQueueAutoStop = (liveData->settings.commandQueueAutoStop == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_ADAPTER_DISABLE_COMMAND_OPTIMIZER:
      liveData->settings.disableCommandOptimizer = (liveData->settings.disableCommandOptimizer == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_ADAPTER_THREADING:
      liveData->settings.threading = (liveData->settings.threading == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_ADAPTER_LOAD_TEST_DATA:
      loadTestData();
      break;
    case MENU_BOARD_POWER_MODE:
      liveData->settings.boardPowerMode = (liveData->settings.boardPowerMode == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    // Screen orientation
    case MENU_SCREEN_ROTATION:
      liveData->settings.displayRotation = (liveData->settings.displayRotation == 1) ? 3 : 1;
      tft.setRotation(liveData->settings.displayRotation);
      showMenu();
      return;
      break;
    // Default screen
    case DEFAULT_SCREEN_AUTOMODE:
      liveData->settings.defaultScreen = 1;
      showParentMenu = true;
      break;
    case DEFAULT_SCREEN_BASIC_INFO:
      liveData->settings.defaultScreen = 2;
      showParentMenu = true;
      break;
    case DEFAULT_SCREEN_SPEED:
      liveData->settings.defaultScreen = 3;
      showParentMenu = true;
      break;
    case DEFAULT_SCREEN_BATT_CELL:
      liveData->settings.defaultScreen = 4;
      showParentMenu = true;
      break;
    case DEFAULT_SCREEN_CHG_GRAPH:
      liveData->settings.defaultScreen = 5;
      showParentMenu = true;
      break;
    case DEFAULT_SCREEN_HUD:
      liveData->settings.defaultScreen = 8;
      showParentMenu = true;
      break;
    // SleepMode off/on
    case MENU_SLEEP_MODE_MODE:
      liveData->settings.sleepModeLevel = (liveData->settings.sleepModeLevel == SLEEP_MODE_SCREEN_ONLY) ? SLEEP_MODE_OFF : liveData->settings.sleepModeLevel + 1;
      showMenu();
      return;
      break;
    case MENU_SCREEN_BRIGHTNESS:
      liveData->settings.lcdBrightness += 20;
      if (liveData->settings.lcdBrightness > 100)
        liveData->settings.lcdBrightness = 0;
      setBrightness();
      showMenu();
      return;
      break;
    case MENU_REMOTE_UPLOAD_UART:
      liveData->settings.gprsHwSerialPort = (liveData->settings.gprsHwSerialPort == 2) ? 255 : liveData->settings.gprsHwSerialPort + 1;
      showMenu();
      return;
      break;
    case MENU_REMOTE_UPLOAD_TYPE:
      // liveData->settings.remoteUploadModuleType = 0; //Currently only one module is supported (0 = SIM800L)
      liveData->settings.remoteUploadModuleType = (liveData->settings.remoteUploadModuleType == 1) ? REMOTE_UPLOAD_OFF : liveData->settings.remoteUploadModuleType + 1;
      showMenu();
      return;
      break;
    case MENU_REMOTE_UPLOAD_API_INTERVAL:
      liveData->settings.remoteUploadIntervalSec = (liveData->settings.remoteUploadIntervalSec == 120) ? 0 : liveData->settings.remoteUploadIntervalSec + 10;
      liveData->settings.remoteUploadAbrpIntervalSec = 0;
      showMenu();
      return;
      break;
    case MENU_REMOTE_UPLOAD_ABRP_INTERVAL:
      liveData->settings.remoteUploadAbrpIntervalSec = (liveData->settings.remoteUploadAbrpIntervalSec == 30) ? 0 : liveData->settings.remoteUploadAbrpIntervalSec + 2; // @spot2000 Better with smaller steps and maximum 30 seconds
      liveData->settings.remoteUploadIntervalSec = 0;
      showMenu();
      return;
      break;
    case MENU_REMOTE_UPLOAD_ABRP_LOG_SDCARD:
      liveData->settings.abrpSdcardLog = (liveData->settings.abrpSdcardLog == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_REMOTE_UPLOAD_MQTT_ENABLED:
      liveData->settings.mqttEnabled = (liveData->settings.mqttEnabled == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_REMOTE_UPLOAD_CONTRIBUTE_ONCE:
      syslog->println("CONTRIBUTE_WAITING");
      liveData->params.contributeStatus = CONTRIBUTE_WAITING;
      break;
    case MENU_REMOTE_UPLOAD_LOGS_TO_EVDASH_SERVER:
      uploadSdCardLogToEvDashServer();
      delay(2000);
      showMenu();
      return;
      break;
    case MENU_GPS_MODULE_TYPE:
      liveData->settings.gpsModuleType = (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_M5_GNSS) ? GPS_MODULE_TYPE_NONE : liveData->settings.gpsModuleType + 1;
      if (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_NEO_M8N)
      {
        liveData->settings.gpsSerialPortSpeed = 9600;
      }
      else if (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_M5_GNSS)
      {
        liveData->settings.gpsSerialPortSpeed = 38400;
      }
      showMenu();
      return;
      break;
    case MENU_GPS_PORT:
      liveData->settings.gpsHwSerialPort = (liveData->settings.gpsHwSerialPort == 2) ? 0 : 2;
      showMenu();
      return;
      break;
    case MENU_GPS_SPEED:
      liveData->settings.gpsSerialPortSpeed = (liveData->settings.gpsSerialPortSpeed == 9600) ? 38400 : (liveData->settings.gpsSerialPortSpeed == 38400) ? 115200
                                                                                                                                                         : 9600;
      showMenu();
      return;
      break;
    case MENU_CAR_SPEED_TYPE:
      liveData->settings.carSpeedType = (liveData->settings.carSpeedType == CAR_SPEED_TYPE_GPS) ? 0 : liveData->settings.carSpeedType + 1;
      showMenu();
      return;
      break;
    case MENU_SERIAL_CONSOLE:
      liveData->settings.serialConsolePort = (liveData->settings.serialConsolePort == 0) ? 255 : liveData->settings.serialConsolePort + 1;
      showMenu();
      return;
      break;
    case MENU_DEBUG_LEVEL:
      liveData->settings.debugLevel = (liveData->settings.debugLevel == DEBUG_GPS) ? 0 : liveData->settings.debugLevel + 1;
      syslog->setDebugLevel(liveData->settings.debugLevel);
      showMenu();
      return;
      break;
    case MENU_CURRENT_TIME:
      showMenu();
      return;
      break;
    case MENU_TIMEZONE:
      liveData->settings.timezone = (liveData->settings.timezone == 14) ? -11 : liveData->settings.timezone + 1;
      showMenu();
      return;
      break;
    case MENU_DAYLIGHT_SAVING:
      liveData->settings.daylightSaving = (liveData->settings.daylightSaving == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_SPEED_CORRECTION:
      liveData->settings.speedCorrection = (liveData->settings.speedCorrection == 5) ? -5 : liveData->settings.speedCorrection + 1;
      showMenu();
      return;
      break;
    case MENU_RHD:
      liveData->settings.rightHandDrive = (liveData->settings.rightHandDrive == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    // Wifi menu
    case MENU_WIFI_ENABLED:
      liveData->settings.wifiEnabled = (liveData->settings.wifiEnabled == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_WIFI_ENABLED2:
      liveData->settings.backupWifiEnabled = (liveData->settings.backupWifiEnabled == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_WIFI_HOTSPOT_WEBADMIN:
#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
      webInterface = new WebInterface();
      webInterface->init(liveData, this);
      displayMessage("ssid evdash [evaccess]", "http://192.168.0.1:80");
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3
      return;
      break;
    case MENU_WIFI_NTP:
      liveData->settings.ntpEnabled = (liveData->settings.ntpEnabled == 1) ? 0 : 1;
      if (liveData->settings.ntpEnabled)
        ntpSync();
      showMenu();
      return;
      break;
    case MENU_WIFI_SSID:
    case MENU_WIFI_PASSWORD:
    case MENU_WIFI_SSID2:
    case MENU_WIFI_PASSWORD2:
    case MENU_WIFI_ACTIVE:
    case MENU_WIFI_IPADDR:
      return;
      break;
    // Sdcard
    case MENU_SDCARD_ENABLED:
      liveData->settings.sdcardEnabled = (liveData->settings.sdcardEnabled == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_SDCARD_AUTOSTARTLOG:
      liveData->settings.sdcardAutstartLog = (liveData->settings.sdcardAutstartLog == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_SDCARD_MOUNT_STATUS:
      sdcardMount();
      break;
    case MENU_SDCARD_REC:
      sdcardToggleRecording();
      showMenu();
      return;
      break;
    case MENU_SDCARD_SETTINGS_SAVE:
      if (!confirmMessage("Confirm action", "Do you want to save?"))
      {
        showMenu();
        return;
      }
      showMenu();
      if (liveData->settings.sdcardEnabled && sdcardMount())
      {
        File file = SD.open("/settings_backup.bin", FILE_WRITE);
        if (!file)
        {
          syslog->println("Failed to create file");
          displayMessage("SDCARD", "Failed to create file");
        }
        if (file)
        {
          syslog->info(DEBUG_NONE, "Save settings to SD card");
          uint8_t *buff = (uint8_t *)&liveData->settings;
          file.write(buff, sizeof(liveData->settings));
          file.close();
          displayMessage("SDCARD", "Saved");
          delay(2000);
        }
      }
      else
      {
        displayMessage("SDCARD", "Not mounted");
      }
      showMenu();
      return;
      break;
    case MENU_SDCARD_SETTINGS_RESTORE:
      if (!confirmMessage("Confirm action", "Do you want to restore?"))
      {
        showMenu();
        return;
      }
      showMenu();
      if (liveData->settings.sdcardEnabled && sdcardMount())
      {
        File file = SD.open("/settings_backup.bin", FILE_READ);
        if (!file)
        {
          syslog->println("Failed to open file for reading");
          displayMessage("SDCARD", "Failed to open file for reading");
        }
        else
        {
          syslog->info(DEBUG_NONE, "Restore settings from SD card");
          syslog->println(file.size());
          size_t size = file.read((uint8_t *)&liveData->settings, file.size());
          syslog->println(size);
          file.close();
          //
          saveSettings();
          ESP.restart();
        }
      }
      else
      {
        displayMessage("SDCARD", "Not mounted or not exists");
      }
      showMenu();
      return;
      break;
    case MENU_SDCARD_SAVE_CONSOLE_TO_SDCARD:

      if (liveData->settings.sdcardEnabled == 1 && !liveData->params.sdcardInit)
      {

        displayMessage("SDCARD", "Mounting SD...");
        sdcardMount();
      }
      if (liveData->settings.sdcardEnabled == 1 && liveData->params.sdcardInit)
      {
        syslog->info(DEBUG_NONE, "Save console output to sdcard started.");
        displayMessage("SDCARD", "Console to SD enable");
        syslog->setLogToSdcard(true);
        hideMenu();
        return;
        break;
      }
      showMenu();
      return;
      break;
    // Voltmeter INA 3221
    case MENU_VOLTMETER_ENABLED:
      liveData->settings.voltmeterEnabled = (liveData->settings.voltmeterEnabled == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_VOLTMETER_SLEEP:
      liveData->settings.voltmeterBasedSleep = (liveData->settings.voltmeterBasedSleep == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_VOLTMETER_INFO:
      if (liveData->settings.voltmeterEnabled == 1)
      {
        m1 = "";
        m1.concat(ina3221.getBusVoltage_V(1));
        m1.concat("/");
        m1.concat(ina3221.getBusVoltage_V(2));
        m1.concat("/");
        m1.concat(ina3221.getBusVoltage_V(3));
        m1.concat("V");
        m2 = "";
        m2.concat(ina3221.getCurrent_mA(1));
        m2.concat("/");
        m2.concat(ina3221.getCurrent_mA(2));
        m2.concat("/");
        m2.concat(ina3221.getCurrent_mA(3));
        m2.concat("mA");
        displayMessage(m1.c_str(), m2.c_str());
      }
      return;
      break;
    case MENU_VOLTMETER_SLEEPVOL:
      liveData->settings.voltmeterSleep = (liveData->settings.voltmeterSleep >= 14.0) ? 11.0 : liveData->settings.voltmeterSleep + 0.2;
      showMenu();
      return;
      break;
    case MENU_VOLTMETER_WAKEUPVOL:
      liveData->settings.voltmeterWakeUp = (liveData->settings.voltmeterWakeUp >= 14.0) ? 11.0 : liveData->settings.voltmeterWakeUp + 0.2;
      showMenu();
      return;
      break;
    case MENU_VOLTMETER_CUTOFFVOL:
      liveData->settings.voltmeterCutOff = (liveData->settings.voltmeterCutOff >= 13.0) ? 10.0 : liveData->settings.voltmeterCutOff + 0.2;
      showMenu();
      return;
      break;
    // Distance
    case DISTANCE_UNIT_KM:
      liveData->settings.distanceUnit = 'k';
      showParentMenu = true;
      break;
    case DISTANCE_UNIT_MI:
      liveData->settings.distanceUnit = 'm';
      showParentMenu = true;
      break;
    // Temperature
    case TEMPERATURE_UNIT_CEL:
      liveData->settings.temperatureUnit = 'c';
      showParentMenu = true;
      break;
    case TEMPERATURE_UNIT_FAR:
      liveData->settings.temperatureUnit = 'f';
      showParentMenu = true;
      break;
    // Pressure
    case PRESURE_UNIT_BAR:
      liveData->settings.pressureUnit = 'b';
      showParentMenu = true;
      break;
    case PRESURE_UNIT_PSI:
      liveData->settings.pressureUnit = 'p';
      showParentMenu = true;
      break;
    // Pair ble device
    case MENU_ADAPTER_BLE_SELECT:
      if (liveData->settings.commType == COMM_TYPE_CAN_COMMU)
      {
        displayMessage("Not supported", "in CAN mode");
        delay(3000);
        hideMenu();
        return;
      }
      scanDevices = true;
      liveData->menuCurrent = 9999;
      commInterface->scanDevices();
      return;
    // Reset settings
    case MENU_FACTORY_RESET:
      if (confirmMessage("Confirm action", "Do you want to reset?"))
      {
        showMenu();
        resetSettings();
        hideMenu();
      }
      else
      {
        showMenu();
      }
      return;
    // Save settings
    case MENU_SAVE_SETTINGS:
      saveSettings();
      break;
    // Version
    case MENU_APP_VERSION:
      if (confirmMessage("Confirm action", "Do you want to run OTA?"))
      {
        showMenu();
        otaUpdate();
      }
      showMenu();
      return;
      break;
    // Memory usage
    case MENU_MEMORY_USAGE:
      m1 = "Heap ";
      m1.concat(ESP.getHeapSize());
      m1.concat("/");
      m1.concat(heap_caps_get_free_size(MALLOC_CAP_8BIT));
      m2 = "Psram ";
      m2.concat(ESP.getPsramSize());
      m2.concat("/");
      m2.concat(ESP.getFreePsram());
      displayMessage(m1.c_str(), m2.c_str());
      return;
      break;
    // Shutdown
    case MENU_SHUTDOWN:
      enterSleepMode(0);
      return;
    // Reboot
    case MENU_REBOOT:
      ESP.restart();
      return;
    default:
      // Submenu
      liveData->menuCurrent = tmpMenuItem->id;
      liveData->menuItemSelected = 0;
      showMenu();
      return;
    }
  }

  // Exit menu
  if (liveData->menuItemSelected == 0 || (showParentMenu && parentMenu != -1))
  {
    if (tmpMenuItem->parentId == 0 && tmpMenuItem->id == 0)
    {
      liveData->menuVisible = false;
      redrawScreen();
    }
    else
    {
      // Parent menu
      liveData->menuItemSelected = 0;
      for (i = 0; i < liveData->menuItemsCount; ++i)
      {
        if (parentMenu == liveData->menuItems[i].parentId)
        {
          if (liveData->menuItems[i].id == liveData->menuCurrent)
            break;
          liveData->menuItemSelected++;
        }
      }

      liveData->menuCurrent = parentMenu;
      syslog->println(liveData->menuCurrent);
      showMenu();
    }
    return;
  }

  // Close menu
  hideMenu();
}
