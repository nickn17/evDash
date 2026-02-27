/*
This C++ code is setting up a web interface for an EV dashboard application.
It includes header files for various functionality like WiFi, HTTP, and a web webServer-> It defines ssid and password constants for connecting to a specific WiFi network.
An ESP32WebServer object is created to handle HTTP requests on port 80. Some IP address constants are defined for the local network configuration.
Pointers to LiveData and BoardInterface objects are declared, which seem to represent data and hardware access classes used elsewhere in the app.
The getOffOnText function takes a boolean state parameter and returns an HTML span tag with styling to display that state as "ON" or "off". This is likely used to neatly format boolean values on the web interface.
The handleRoot function renders the root HTML page. It builds an HTML string with styling and layout for the page. The page title includes the app name and version.
The page content is split into two columns. The left column renders settings from the LiveData object. It loops through and prints communication, WiFi, and other config values. Some values are printed as inputs to allow changing the setting.
So in summary, this sets up a web server to display a dashboard page with live data and settings, retrieving data from other parts of the application. It provides an interface to view and configure the EV telemetry system over HTTP.
*/

#include "WebInterface.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include "WiFi.h"
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ESP32WebServer.h>

char ssid[] = "evdash";
char password[] = "evaccess";

ESP32WebServer *webServer = nullptr;
IPAddress ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

LiveData *liveDataWebInt;
BoardInterface *boardWebInt;

String htmlEscape(const String &input)
{
  String out;
  out.reserve(input.length());
  for (size_t i = 0; i < input.length(); ++i)
  {
    char c = input.charAt(i);
    switch (c)
    {
    case '&':
      out += "&amp;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '"':
      out += "&quot;";
      break;
    case '\'':
      out += "&#39;";
      break;
    default:
      out += c;
      break;
    }
  }
  return out;
}

String jsonEscape(const String &input)
{
  String out;
  out.reserve(input.length());
  for (size_t i = 0; i < input.length(); ++i)
  {
    char c = input.charAt(i);
    switch (c)
    {
    case '\\':
      out += "\\\\";
      break;
    case '"':
      out += "\\\"";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      out += c;
      break;
    }
  }
  return out;
}

String checkedAttr(uint8_t value)
{
  return value ? " checked" : "";
}

String selectedAttr(bool selected)
{
  return selected ? " selected" : "";
}

/**
 * Get off/on text
 */
String getOffOnText(uint8_t state)
{
  return String(state == 1 ? "<span class='on'>ON</span>" : "<span class='off'>off</span>");
}

/**
 * Handle webserver root
 **/
void handleRoot()
{
  if (webServer == nullptr)
  {
    return;
  }
  String text;
  text.reserve(12288);
  char tmpStr1[20];

  // Render html5
  text = "<html><head><link href='https://fonts.googleapis.com/css?family=Roboto+Condensed' rel='stylesheet'><style>body { margin:32px; background:black; color: white; font-family: 'Roboto Condensed', sans-serif; } td { padding: 0px 16px 4px 0px; text-align: left; vertical-align: top;} th { color: orange; text-align: left; padding: 16px 16px 4px 0px; } td:first-child { text-align: left; } input, select, button { font-family: 'Roboto Condensed', sans-serif; font-size: 0.95em; } input[type='text'], input[type='password'], input[type='number'], select { background: #111; color: #fff; border: 1px solid #444; padding: 4px 6px; } button { background: #222; color: #fff; border: 1px solid #555; padding: 6px 10px; cursor: pointer; } button:hover { border-color: #777; } .off { color: gray; } .on { color: lime; } .red { color: red; } .lime { color: lime; } .yellow { color: yellow; } .right {text-align:right; } h2 { margin-top: 12px; font-size: 1.1em; } .status { margin: 8px 0 12px 0; color: #9f9; } .status.err { color: #f66; }</style></head>";
  text += "<body><h1>[ evDash " + String(APP_VERSION) + ", settings v" + String(liveDataWebInt->settings.settingsVersion) + " ] </h1>";
  text += "<div id='status' class='status'></div>";
  text += "<div style='margin-bottom:12px;'><button data-action='saveSettings'>Save settings</button> <button data-action='reboot'>Reboot</button> <button data-action='shutdown'>Shutdown</button></div>";

  // 1st column settings
  text += "<div style='float:left; width:50%;'>";
  text += "<table style='min-width: 25%;'>";
  text += "<tr><th colspan='2'>Communication</th></tr>";
  text += "<tr><td>Type</td><td><select data-key='commType'>";
  text += "<option value='" + String(COMM_TYPE_CAN_COMMU) + "'" + selectedAttr(liveDataWebInt->settings.commType == COMM_TYPE_CAN_COMMU) + ">CAN COMMU module</option>";
  text += "<option value='" + String(COMM_TYPE_OBD2_BLE4) + "'" + selectedAttr(liveDataWebInt->settings.commType == COMM_TYPE_OBD2_BLE4) + ">OBD2 BLE4</option>";
  text += "<option value='" + String(COMM_TYPE_OBD2_BT3) + "'" + selectedAttr(liveDataWebInt->settings.commType == COMM_TYPE_OBD2_BT3) + ">OBD2 Bluetooth3 classic</option>";
  text += "<option value='" + String(COMM_TYPE_OBD2_WIFI) + "'" + selectedAttr(liveDataWebInt->settings.commType == COMM_TYPE_OBD2_WIFI) + ">OBD2 WIFI</option>";
  text += "</select></td></tr>";
  text += "<tr><td>OBD2 MAC</td><td><input id='obdMacAddress' data-key='obdMacAddress' value='" + htmlEscape(String(liveDataWebInt->settings.obdMacAddress)) + "'></td></tr>";
  text += "<tr><td>BLE4 scan</td><td><button id='bleScan'>Scan BLE4 adapters</button></td></tr>";
  text += "<tr><td>BLE4 devices</td><td><select id='bleDevices'><option value=''>-- select device --</option>";
  bool hasBleCurrent = false;
  for (uint16_t i = 0; i < liveDataWebInt->menuItemsCount; ++i)
  {
    if (liveDataWebInt->menuItems[i].id >= LIST_OF_BLE_1 && liveDataWebInt->menuItems[i].id <= LIST_OF_BLE_10)
    {
      if (strlen(liveDataWebInt->menuItems[i].title) == 0 || strcmp(liveDataWebInt->menuItems[i].title, "-") == 0)
        continue;
      if (strlen(liveDataWebInt->menuItems[i].obdMacAddress) == 0)
        continue;
      String mac = String(liveDataWebInt->menuItems[i].obdMacAddress);
      String title = String(liveDataWebInt->menuItems[i].title);
      text += "<option value='" + htmlEscape(mac) + "'" + selectedAttr(mac == String(liveDataWebInt->settings.obdMacAddress)) + ">" + htmlEscape(title) + "</option>";
      if (mac == String(liveDataWebInt->settings.obdMacAddress))
        hasBleCurrent = true;
    }
  }
  if (!hasBleCurrent && strlen(liveDataWebInt->settings.obdMacAddress) != 0)
  {
    text += "<option value='" + htmlEscape(String(liveDataWebInt->settings.obdMacAddress)) + "' selected>Current: " + htmlEscape(String(liveDataWebInt->settings.obdMacAddress)) + "</option>";
  }
  text += "</select></td></tr>";
  text += "<tr><td>OBD2 BLE4 Service UUID</td><td><input data-key='serviceUUID' value='" + htmlEscape(String(liveDataWebInt->settings.serviceUUID)) + "'></td></tr>";
  text += "<tr><td>OBD2 BLE4 Tx UUID</td><td><input data-key='charTxUUID' value='" + htmlEscape(String(liveDataWebInt->settings.charTxUUID)) + "'></td></tr>";
  text += "<tr><td>OBD2 BLE4 Rx UUID</td><td><input data-key='charRxUUID' value='" + htmlEscape(String(liveDataWebInt->settings.charRxUUID)) + "'></td></tr>";
  text += "<tr><td>Disable command optimizer (log all cells)</td><td><input type='checkbox' data-key='disableCommandOptimizer'" + checkedAttr(liveDataWebInt->settings.disableCommandOptimizer) + "></td></tr>";
  text += "<tr><td>CAN queue autostop</td><td><input type='checkbox' data-key='commandQueueAutoStop'" + checkedAttr(liveDataWebInt->settings.commandQueueAutoStop) + "></td></tr>";
  text += "<tr><td>OBD2 name (BT3)</td><td><input data-key='obd2Name' value='" + htmlEscape(String(liveDataWebInt->settings.obd2Name)) + "'></td></tr>";
  text += "<tr><td>OBD2 WIFI IP</td><td><input data-key='obd2WifiIp' value='" + htmlEscape(String(liveDataWebInt->settings.obd2WifiIp)) + "'></td></tr>";
  text += "<tr><td>OBD2 WIFI port</td><td><input type='number' data-key='obd2WifiPort' value='" + String(liveDataWebInt->settings.obd2WifiPort) + "'></td></tr>";

  text += "<tr><th colspan='2'>WiFi client</th></tr>";
  text += "<tr><td>Enabled</td><td><input type='checkbox' data-key='wifiEnabled'" + checkedAttr(liveDataWebInt->settings.wifiEnabled) + "></td></tr>";
  text += "<tr><td>SSID</td><td><input data-key='wifiSsid' value='" + htmlEscape(String(liveDataWebInt->settings.wifiSsid)) + "'></td></tr>";
  text += "<tr><td>WiFi password</td><td><input type='password' data-key='wifiPassword' value='" + htmlEscape(String(liveDataWebInt->settings.wifiPassword)) + "'></td></tr>";
  text += "<tr><td>Backup WiFi enabled</td><td><input type='checkbox' data-key='backupWifiEnabled'" + checkedAttr(liveDataWebInt->settings.backupWifiEnabled) + "></td></tr>";
  text += "<tr><td>SSID backup</td><td><input data-key='wifiSsid2' value='" + htmlEscape(String(liveDataWebInt->settings.wifiSsid2)) + "'></td></tr>";
  text += "<tr><td>Backup WiFi password</td><td><input type='password' data-key='wifiPassword2' value='" + htmlEscape(String(liveDataWebInt->settings.wifiPassword2)) + "'></td></tr>";
  text += "<tr><td>NTP enabled</td><td><input type='checkbox' data-key='ntpEnabled'" + checkedAttr(liveDataWebInt->settings.ntpEnabled) + "></td></tr>";
  text += "<tr><td>NTP timezone</td><td><input type='number' data-key='ntpTimezone' value='" + String(liveDataWebInt->settings.ntpTimezone) + "'></td></tr>";
  text += "<tr><td>NTP daylight saving</td><td><input type='checkbox' data-key='ntpDaySaveTime'" + checkedAttr(liveDataWebInt->settings.ntpDaySaveTime) + "></td></tr>";

  text += "<tr><th colspan='2'>Sdcard</th></tr>";
  text += "<tr><td>Mode</td><td>" + String((liveDataWebInt->settings.sdcardEnabled == 0) ? "off" : (strlen(liveDataWebInt->params.sdcardFilename) != 0) ? "WRITE"
                                                                                               : (liveDataWebInt->params.sdcardInit)                    ? "READY"
                                                                                                                                                        : "MOUNTED") +
          "</td></tr>";

  text += "<tr><td>Enabled</td><td><input type='checkbox' data-key='sdcardEnabled'" + checkedAttr(liveDataWebInt->settings.sdcardEnabled) + "></td></tr>";
  text += "<tr><td>Autostart log</td><td><input type='checkbox' data-key='sdcardAutstartLog'" + checkedAttr(liveDataWebInt->settings.sdcardAutstartLog) + "></td></tr>";
  text += "<tr><td>Log interval [sec.]</td><td><input type='number' data-key='sdcardLogIntervalSec' value='" + String(liveDataWebInt->settings.sdcardLogIntervalSec) + "'></td></tr>";
  text += "<tr><td>Used / Total MB</td><td>" + String(SD.usedBytes() / 1048576) + " / " + String(SD.totalBytes() / 1048576) + "</td></tr>";

  text += "<tr><th colspan='2'>Remote upload</th></tr>";
  text += "<tr><td>Module</td><td><select data-key='remoteUploadModuleType'>";
  text += "<option value='" + String(REMOTE_UPLOAD_OFF) + "'" + selectedAttr(liveDataWebInt->settings.remoteUploadModuleType == REMOTE_UPLOAD_OFF) + ">OFF</option>";
  text += "<option value='" + String(REMOTE_UPLOAD_WIFI) + "'" + selectedAttr(liveDataWebInt->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI) + ">WIFI</option>";
  text += "</select></td></tr>";
  text += "<tr><td>Remote API url</td><td><input data-key='remoteApiUrl' value='" + htmlEscape(String(liveDataWebInt->settings.remoteApiUrl)) + "'></td></tr>";
  text += "<tr><td>Remote API key</td><td><input data-key='remoteApiKey' value='" + htmlEscape(String(liveDataWebInt->settings.remoteApiKey)) + "'></td></tr>";
  text += "<tr><td>Remote upload interval [sec.]</td><td><input type='number' data-key='remoteUploadIntervalSec' value='" + String(liveDataWebInt->settings.remoteUploadIntervalSec) + "'></td></tr>";
  text += "<tr><td>Abrp API token</td><td><input data-key='abrpApiToken' value='" + htmlEscape(String(liveDataWebInt->settings.abrpApiToken)) + "'></td></tr>";
  const float abrpIntervalSec = liveDataWebInt->settings.remoteUploadAbrpIntervalSec * 0.5f;
  text += "<tr><td>Abrp upload interval [sec.]</td><td><input type='number' min='0' max='5' step='0.5' data-key='remoteUploadAbrpIntervalSec' value='" + String(abrpIntervalSec, 1) + "'></td></tr>";
  text += "<tr><td>Abrp sdcard logs</td><td><input type='checkbox' data-key='abrpSdcardLog'" + checkedAttr(liveDataWebInt->settings.abrpSdcardLog) + "></td></tr>";
  text += "<tr><td>MQTT enabled</td><td><input type='checkbox' data-key='mqttEnabled'" + checkedAttr(liveDataWebInt->settings.mqttEnabled) + "></td></tr>";
  text += "<tr><td>MQTT server</td><td><input data-key='mqttServer' value='" + htmlEscape(String(liveDataWebInt->settings.mqttServer)) + "'></td></tr>";
  text += "<tr><td>MQTT ID</td><td><input data-key='mqttId' value='" + htmlEscape(String(liveDataWebInt->settings.mqttId)) + "'></td></tr>";
  text += "<tr><td>MQTT username</td><td><input data-key='mqttUsername' value='" + htmlEscape(String(liveDataWebInt->settings.mqttUsername)) + "'></td></tr>";
  text += "<tr><td>MQTT password</td><td><input type='password' data-key='mqttPassword' value='" + htmlEscape(String(liveDataWebInt->settings.mqttPassword)) + "'></td></tr>";
  text += "<tr><td>MQTT pub.topic</td><td><input data-key='mqttPubTopic' value='" + htmlEscape(String(liveDataWebInt->settings.mqttPubTopic)) + "'></td></tr>";
  text += "<tr><td>Contribute data</td><td><input type='checkbox' data-key='contributeData'" + checkedAttr(liveDataWebInt->settings.contributeData) + "></td></tr>";
  text += "<tr><td>Contribute token</td><td><input data-key='contributeToken' value='" + htmlEscape(String(liveDataWebInt->settings.contributeToken)) + "'></td></tr>";

  text += "<tr><th colspan='2'>GPS module</th></tr>";
  text += "<tr><td>GPS module type</td><td><select data-key='gpsModuleType'>";
  text += "<option value='" + String(GPS_MODULE_TYPE_NONE) + "'" + selectedAttr(liveDataWebInt->settings.gpsModuleType == GPS_MODULE_TYPE_NONE) + ">None</option>";
  text += "<option value='" + String(GPS_MODULE_TYPE_NEO_M8N) + "'" + selectedAttr(liveDataWebInt->settings.gpsModuleType == GPS_MODULE_TYPE_NEO_M8N) + ">M5 NEO-M8N</option>";
  text += "<option value='" + String(GPS_MODULE_TYPE_M5_GNSS) + "'" + selectedAttr(liveDataWebInt->settings.gpsModuleType == GPS_MODULE_TYPE_M5_GNSS) + ">M5 GNSS</option>";
  text += "<option value='" + String(GPS_MODULE_TYPE_GPS_V21_GNSS) + "'" + selectedAttr(liveDataWebInt->settings.gpsModuleType == GPS_MODULE_TYPE_GPS_V21_GNSS) + ">GPS v2.1 GNSS</option>";
  text += "</select></td></tr>";
  text += "<tr><td>GPS hw serial port</td><td><input type='number' data-key='gpsHwSerialPort' value='" + String(liveDataWebInt->settings.gpsHwSerialPort) + "'></td></tr>";
  text += "<tr><td>GPS serial speed</td><td><input type='number' data-key='gpsSerialPortSpeed' value='" + String(liveDataWebInt->settings.gpsSerialPortSpeed) + "'></td></tr>";
  text += "<tr><td>Car speed source</td><td><select data-key='carSpeedType'>";
  text += "<option value='" + String(CAR_SPEED_TYPE_AUTO) + "'" + selectedAttr(liveDataWebInt->settings.carSpeedType == CAR_SPEED_TYPE_AUTO) + ">Automatic</option>";
  text += "<option value='" + String(CAR_SPEED_TYPE_CAR) + "'" + selectedAttr(liveDataWebInt->settings.carSpeedType == CAR_SPEED_TYPE_CAR) + ">Car only</option>";
  text += "<option value='" + String(CAR_SPEED_TYPE_GPS) + "'" + selectedAttr(liveDataWebInt->settings.carSpeedType == CAR_SPEED_TYPE_GPS) + ">GPS only</option>";
  text += "</select></td></tr>";
  text += "<tr><td>GPS valid data</td><td>" + String(liveDataWebInt->params.gpsValid == 1 ? "YES" : "NO") + "</td></tr>";
  if (liveDataWebInt->params.gpsValid)
  {
    text += "<tr><td>Latitude</td><td>" + String(liveDataWebInt->params.gpsLat) + "</td></tr>";
    text += "<tr><td>Longitude</td><td>" + String(liveDataWebInt->params.gpsLon) + "</td></tr>";
    text += "<tr><td>Alt</td><td>" + String(liveDataWebInt->params.gpsAlt) + "</td></tr>";
    text += "<tr><td>Satellites</td><td>" + String(liveDataWebInt->params.gpsSat) + "</td></tr>";
  }

  text += "<tr><th colspan='2'>Voltmeter</th></tr>";
  text += "<tr><td>INA3221 enabled</td><td><input type='checkbox' data-key='voltmeterEnabled'" + checkedAttr(liveDataWebInt->settings.voltmeterEnabled) + "></td></tr>";
  text += "<tr><td>Voltmeter based sleep</td><td><input type='checkbox' data-key='voltmeterBasedSleep'" + checkedAttr(liveDataWebInt->settings.voltmeterBasedSleep) + "></td></tr>";
  text += "<tr><td>Cut off voltage [V]</td><td><input type='number' step='0.1' data-key='voltmeterCutOff' value='" + String(liveDataWebInt->settings.voltmeterCutOff, 2) + "'></td></tr>";
  text += "<tr><td>Sleep voltage [V]</td><td><input type='number' step='0.1' data-key='voltmeterSleep' value='" + String(liveDataWebInt->settings.voltmeterSleep, 2) + "'></td></tr>";
  text += "<tr><td>Wake up voltage [V]</td><td><input type='number' step='0.1' data-key='voltmeterWakeUp' value='" + String(liveDataWebInt->settings.voltmeterWakeUp, 2) + "'></td></tr>";

  text += "<tr><th colspan='2'>Console</th></tr>";
  text += "<tr><td>Serial console</td><td><input type='number' data-key='serialConsolePort' value='" + String(liveDataWebInt->settings.serialConsolePort) + "'></td></tr>";
  text += "<tr><td>Debug level</td><td><select data-key='debugLevel'>";
  for (int i = 0; i <= 4; ++i)
  {
    text += "<option value='" + String(i) + "'" + selectedAttr(liveDataWebInt->settings.debugLevel == i) + ">" + String(i) + "</option>";
  }
  text += "</select></td></tr>";

  text += "<tr><th colspan='2'>Sleep</th></tr>";
  text += "<tr><td>Mode</td><td><select data-key='sleepModeLevel'>";
  text += "<option value='" + String(SLEEP_MODE_OFF) + "'" + selectedAttr(liveDataWebInt->settings.sleepModeLevel == SLEEP_MODE_OFF) + ">Off</option>";
  text += "<option value='" + String(SLEEP_MODE_SCREEN_ONLY) + "'" + selectedAttr(liveDataWebInt->settings.sleepModeLevel == SLEEP_MODE_SCREEN_ONLY) + ">Screen only</option>";
  text += "</select></td></tr>";
  text += "<tr><td>Sleep mode interval [sec.]</td><td><input type='number' data-key='sleepModeIntervalSec' value='" + String(liveDataWebInt->settings.sleepModeIntervalSec) + "'></td></tr>";
  text += "<tr><td>Shutdown after [hrs]</td><td><input type='number' data-key='sleepModeShutdownHrs' value='" + String(liveDataWebInt->settings.sleepModeShutdownHrs) + "'></td></tr>";

  text += "<tr><th colspan='2'>Time</th></tr>";
  text += "<tr><td>Mode</td><td>" + String(liveDataWebInt->params.ntpTimeSet == 1 ? " NTP " : "") + String(liveDataWebInt->params.currTimeSyncWithGps == 1 ? " GPS " : "") + "</td></tr>";
  struct tm now;
  getLocalTime(&now);
  sprintf(tmpStr1, "%02d-%02d-%02d %02d:%02d:%02d", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
  text += "<tr><td>Current time</td><td>" + String(tmpStr1) + "</td></tr>";
  text += "<tr><td>Timezone</td><td><input type='number' data-key='timezone' value='" + String(liveDataWebInt->settings.timezone) + "'></td></tr>";
  text += "<tr><td>Daylight saving</td><td><input type='checkbox' data-key='daylightSaving'" + checkedAttr(liveDataWebInt->settings.daylightSaving) + "></td></tr>";

  text += "<tr><th colspan='2'>Board</th></tr>";
  text += "<tr><td>Power mode</td><td><select data-key='boardPowerMode'>";
  text += "<option value='1'" + selectedAttr(liveDataWebInt->settings.boardPowerMode == 1) + ">External</option>";
  text += "<option value='0'" + selectedAttr(liveDataWebInt->settings.boardPowerMode == 0) + ">USB</option>";
  text += "</select></td></tr>";
  text += "<tr><td>Screen rotation</td><td><select data-key='displayRotation'>";
  text += "<option value='1'" + selectedAttr(liveDataWebInt->settings.displayRotation == 1) + ">Vertical</option>";
  text += "<option value='3'" + selectedAttr(liveDataWebInt->settings.displayRotation == 3) + ">Normal</option>";
  text += "</select></td></tr>";
  text += "<tr><td>Default screen</td><td><select data-key='defaultScreen'>";
  text += "<option value='1'" + selectedAttr(liveDataWebInt->settings.defaultScreen == 1) + ">Auto mode</option>";
  text += "<option value='2'" + selectedAttr(liveDataWebInt->settings.defaultScreen == 2) + ">Basic info</option>";
  text += "<option value='3'" + selectedAttr(liveDataWebInt->settings.defaultScreen == 3) + ">Speed</option>";
  text += "<option value='4'" + selectedAttr(liveDataWebInt->settings.defaultScreen == 4) + ">Battery cells</option>";
  text += "<option value='5'" + selectedAttr(liveDataWebInt->settings.defaultScreen == 5) + ">Charging graph</option>";
  text += "<option value='8'" + selectedAttr(liveDataWebInt->settings.defaultScreen == 8) + ">HUD</option>";
  text += "</select></td></tr>";
  text += "<tr><td>LCD brightness</td><td><input type='number' data-key='lcdBrightness' value='" + String(liveDataWebInt->settings.lcdBrightness) + "'></td></tr>";

  text += "<tr><th colspan='2'>Units</th></tr>";
  text += "<tr><td>Distance</td><td><select data-key='distanceUnit'>";
  text += "<option value='k'" + selectedAttr(liveDataWebInt->settings.distanceUnit == 'k') + ">Kilometers</option>";
  text += "<option value='m'" + selectedAttr(liveDataWebInt->settings.distanceUnit == 'm') + ">Miles</option>";
  text += "</select></td></tr>";
  text += "<tr><td>Temperature</td><td><select data-key='temperatureUnit'>";
  text += "<option value='c'" + selectedAttr(liveDataWebInt->settings.temperatureUnit == 'c') + ">Celsius</option>";
  text += "<option value='f'" + selectedAttr(liveDataWebInt->settings.temperatureUnit == 'f') + ">Fahrenheit</option>";
  text += "</select></td></tr>";
  text += "<tr><td>Pressure</td><td><select data-key='pressureUnit'>";
  text += "<option value='b'" + selectedAttr(liveDataWebInt->settings.pressureUnit == 'b') + ">Bar</option>";
  text += "<option value='p'" + selectedAttr(liveDataWebInt->settings.pressureUnit == 'p') + ">Psi</option>";
  text += "</select></td></tr>";

  text += "<tr><th colspan='2'>Other</th></tr>";
  text += "<tr><td>Speed correction</td><td><input type='number' data-key='speedCorrection' value='" + String(liveDataWebInt->settings.speedCorrection) + "'></td></tr>";
  text += "<tr><td>Right hand drive</td><td><input type='checkbox' data-key='rightHandDrive'" + checkedAttr(liveDataWebInt->settings.rightHandDrive) + "></td></tr>";

  text += "</table>";
  text += "</div>";

  // 2nd column - car info
  text += "<div style='float:left; width:50%;'>";
  text += "<table style='min-width: 25%;'>";
  text += "<tr><th colspan='2'>Car settings</th></tr>";
  text += "<tr><td>Car model</td><td><select data-key='carType'>";
  text += "<option value='" + String(CAR_KIA_ENIRO_2020_64) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_KIA_ENIRO_2020_64) + ">Kia eNiro 2020 64kWh</option>";
  text += "<option value='" + String(CAR_KIA_ENIRO_2020_39) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_KIA_ENIRO_2020_39) + ">Kia eNiro 2020 39kWh</option>";
  text += "<option value='" + String(CAR_KIA_ESOUL_2020_64) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_KIA_ESOUL_2020_64) + ">Kia eSoul 2020 64kWh</option>";
  text += "<option value='" + String(CAR_KIA_EV6_58_63) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_KIA_EV6_58_63) + ">Kia EV6 2022 58/63kWh</option>";
  text += "<option value='" + String(CAR_KIA_EV6_77_84) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_KIA_EV6_77_84) + ">Kia EV6 2022 77/84kWh</option>";
  text += "<option value='" + String(CAR_KIA_EV9_100) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_KIA_EV9_100) + ">Kia EV9 2025 100kWh</option>";
  text += "<option value='" + String(CAR_HYUNDAI_IONIQ_2018) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_HYUNDAI_IONIQ_2018) + ">Hyundai Ioniq 2018 28kWh</option>";
  text += "<option value='" + String(CAR_HYUNDAI_IONIQ_PHEV) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_HYUNDAI_IONIQ_PHEV) + ">Hyundai Ioniq 18 PHEV 8.8kWh</option>";
  text += "<option value='" + String(CAR_HYUNDAI_KONA_2020_64) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_HYUNDAI_KONA_2020_64) + ">Hyundai Kona 2020 64kWh</option>";
  text += "<option value='" + String(CAR_HYUNDAI_KONA_2020_39) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_HYUNDAI_KONA_2020_39) + ">Hyundai Kona 2020 39kWh</option>";
  text += "<option value='" + String(CAR_HYUNDAI_IONIQ5_58_63) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_HYUNDAI_IONIQ5_58_63) + ">Hyundai Ioniq5 2022 58/63kWh</option>";
  text += "<option value='" + String(CAR_HYUNDAI_IONIQ5_72) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_HYUNDAI_IONIQ5_72) + ">Hyundai Ioniq5 2022 72.6kWh</option>";
  text += "<option value='" + String(CAR_HYUNDAI_IONIQ5_77_84) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_HYUNDAI_IONIQ5_77_84) + ">Hyundai Ioniq5 2022 77/84kWh</option>";
  text += "<option value='" + String(CAR_HYUNDAI_IONIQ6_58_63) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_HYUNDAI_IONIQ6_58_63) + ">Hyundai Ioniq6 2023 58/63kWh</option>";
  text += "<option value='" + String(CAR_HYUNDAI_IONIQ6_77_84) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_HYUNDAI_IONIQ6_77_84) + ">Hyundai Ioniq6 2023 77/84kWh</option>";
  text += "<option value='" + String(CAR_AUDI_Q4_35) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_AUDI_Q4_35) + ">Audi Q4 35 (CAN)</option>";
  text += "<option value='" + String(CAR_AUDI_Q4_40) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_AUDI_Q4_40) + ">Audi Q4 40 (CAN)</option>";
  text += "<option value='" + String(CAR_AUDI_Q4_45) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_AUDI_Q4_45) + ">Audi Q4 45 (CAN)</option>";
  text += "<option value='" + String(CAR_AUDI_Q4_50) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_AUDI_Q4_50) + ">Audi Q4 50 (CAN)</option>";
  text += "<option value='" + String(CAR_SKODA_ENYAQ_55) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_SKODA_ENYAQ_55) + ">Skoda Enyaq iV 55 (CAN)</option>";
  text += "<option value='" + String(CAR_SKODA_ENYAQ_62) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_SKODA_ENYAQ_62) + ">Skoda Enyaq iV 62 (CAN)</option>";
  text += "<option value='" + String(CAR_SKODA_ENYAQ_82) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_SKODA_ENYAQ_82) + ">Skoda Enyaq iV 82 (CAN)</option>";
  text += "<option value='" + String(CAR_VW_ID3_2021_45) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_VW_ID3_2021_45) + ">VW ID.3 2021 45 kWh (CAN)</option>";
  text += "<option value='" + String(CAR_VW_ID3_2021_58) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_VW_ID3_2021_58) + ">VW ID.3 2021 58 kWh (CAN)</option>";
  text += "<option value='" + String(CAR_VW_ID3_2021_77) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_VW_ID3_2021_77) + ">VW ID.3 2021 77 kWh (CAN)</option>";
  text += "<option value='" + String(CAR_VW_ID4_2021_45) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_VW_ID4_2021_45) + ">VW ID.4 2021 45 kWh (CAN)</option>";
  text += "<option value='" + String(CAR_VW_ID4_2021_58) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_VW_ID4_2021_58) + ">VW ID.4 2021 58 kWh (CAN)</option>";
  text += "<option value='" + String(CAR_VW_ID4_2021_77) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_VW_ID4_2021_77) + ">VW ID.4 2021 77 kWh (CAN)</option>";
  text += "<option value='" + String(CAR_RENAULT_ZOE) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_RENAULT_ZOE) + ">Renault Zoe 22kWh (DEV)</option>";
  text += "<option value='" + String(CAR_BMW_I3_2014) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_BMW_I3_2014) + ">BMW i3 2014 22kWh (DEV)</option>";
  text += "<option value='" + String(CAR_PEUGEOT_E208) + "'" + selectedAttr(liveDataWebInt->settings.carType == CAR_PEUGEOT_E208) + ">Peugeot e-208 (DEV)</option>";
  text += "</select></td></tr>";

  text += "<tr><th colspan='2'>Car status</th></tr>";
  text += "<tr><td>Car mode</td><td>" + String(liveDataWebInt->params.carMode) + "</td></tr>";
  text += "<tr><td>Power (kW)</td><td>" + String(liveDataWebInt->params.batPowerKw * -1) + "</td></tr>";
  text += "<tr><td>Ignition</td><td>" + getOffOnText(liveDataWebInt->params.ignitionOn) + "</td></tr>";
  text += "<tr><td>Charging</td><td>" + getOffOnText(liveDataWebInt->params.chargingOn) + "</td></tr>";
  text += "<tr><td>AC charger connected</td><td>" + getOffOnText(liveDataWebInt->params.chargerACconnected) + "</td></tr>";
  text += "<tr><td>DC charger connected</td><td>" + getOffOnText(liveDataWebInt->params.chargerDCconnected) + "</td></tr>";
  text += "<tr><td>Forward drive mode</td><td>" + getOffOnText(liveDataWebInt->params.forwardDriveMode) + "</td></tr>";
  text += "<tr><td>Reverse drive mode</td><td>" + getOffOnText(liveDataWebInt->params.reverseDriveMode) + "</td></tr>";

  text += "</table>";
  text += "</div>";
  text += "<div style='clear:both;'></div>";
  text += "<script>";
  text += "const statusEl=document.getElementById('status');";
  text += "let statusTimer=null;";
  text += "function setStatus(msg,isErr){statusEl.textContent=msg;statusEl.className='status'+(isErr?' err':'');if(statusTimer){clearTimeout(statusTimer);}statusTimer=setTimeout(()=>{statusEl.textContent='';},4000);}";
  text += "function sendSetting(key,value){fetch('/set?key='+encodeURIComponent(key)+'&value='+encodeURIComponent(value)).then(r=>r.text()).then(t=>{if(t.startsWith('OK')){const msg=t.indexOf(':')>-1?t.substring(3):'Saved';setStatus(msg,false);}else{setStatus('Save failed',true);}}).catch(()=>setStatus('Save failed',true));}";
  text += "document.querySelectorAll('[data-key]').forEach(el=>{const handler=()=>{const key=el.getAttribute('data-key');let val='';if(el.type==='checkbox'){val=el.checked?'1':'0';}else{val=el.value;}sendSetting(key,val);};if(el.tagName==='SELECT'||el.type==='checkbox'){el.addEventListener('change',handler);}else{el.addEventListener('blur',handler);}});";
  text += "document.querySelectorAll('[data-action]').forEach(btn=>{btn.addEventListener('click',()=>{const cmd=btn.getAttribute('data-action');if(cmd==='shutdown'||cmd==='reboot'){if(!confirm('Confirm '+cmd+'?')){return;}}fetch('/action?cmd='+encodeURIComponent(cmd)).then(r=>r.text()).then(()=>setStatus(cmd+' requested',false)).catch(()=>setStatus('Action failed',true));});});";
  text += "const bleBtn=document.getElementById('bleScan');const bleSelect=document.getElementById('bleDevices');";
  text += "function refreshBleList(){fetch('/ble-devices').then(r=>r.json()).then(data=>{if(!data.devices){return;}const current=document.getElementById('obdMacAddress').value;bleSelect.innerHTML='<option value=\"\">-- select device --</option>';data.devices.forEach(d=>{const opt=document.createElement('option');opt.value=d.mac;opt.textContent=d.title; if(d.mac===current){opt.selected=true;}bleSelect.appendChild(opt);});}).catch(()=>{});}";
  text += "if(bleBtn){bleBtn.addEventListener('click',()=>{setStatus('Scanning BLE4 devices...',false);fetch('/ble-scan').then(()=>{setTimeout(()=>{refreshBleList();setStatus('Scan complete',false);},11000);}).catch(()=>setStatus('Scan failed',true));});}";
  text += "if(bleSelect){bleSelect.addEventListener('change',()=>{const val=bleSelect.value;if(!val){return;}const input=document.getElementById('obdMacAddress');if(input){input.value=val;}sendSetting('obdMacAddress',val);});}";
  text += "</script>";
  text += "</body></html>";

  // Send page to client
  webServer->send(200, "text/html", text);
}

/**
   Handle 404 Page not found
*/
void handleNotFound()
{
  if (webServer == nullptr)
  {
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer->uri();
  message += "\nMethod: ";
  message += (webServer->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer->args();
  message += "\n";

  for (uint8_t i = 0; i < webServer->args(); i++)
  {
    message += " " + webServer->argName(i) + ": " + webServer->arg(i) + "\n";
  }

  webServer->send(404, "text/plain", message);
}

template <size_t N>
void setCharField(const String &value, char (&dest)[N])
{
  value.toCharArray(dest, N);
}

void handleSet()
{
  if (webServer == nullptr)
  {
    return;
  }
  if (!webServer->hasArg("key"))
  {
    webServer->send(400, "text/plain", "ERR missing key");
    return;
  }

  String key = webServer->arg("key");
  String value = webServer->hasArg("value") ? webServer->arg("value") : "";
  String note = "";
  bool ok = true;

  if (key == "wifiEnabled")
    liveDataWebInt->settings.wifiEnabled = value.toInt() ? 1 : 0;
  else if (key == "wifiSsid")
    setCharField(value, liveDataWebInt->settings.wifiSsid);
  else if (key == "wifiPassword")
    setCharField(value, liveDataWebInt->settings.wifiPassword);
  else if (key == "backupWifiEnabled")
    liveDataWebInt->settings.backupWifiEnabled = value.toInt() ? 1 : 0;
  else if (key == "wifiSsid2")
    setCharField(value, liveDataWebInt->settings.wifiSsid2);
  else if (key == "wifiPassword2")
    setCharField(value, liveDataWebInt->settings.wifiPassword2);
  else if (key == "ntpEnabled")
    liveDataWebInt->settings.ntpEnabled = value.toInt() ? 1 : 0;
  else if (key == "ntpTimezone")
    liveDataWebInt->settings.ntpTimezone = value.toInt();
  else if (key == "ntpDaySaveTime")
    liveDataWebInt->settings.ntpDaySaveTime = value.toInt() ? 1 : 0;
  else if (key == "commType")
  {
    liveDataWebInt->settings.commType = value.toInt();
    note = "Saved (reboot required)";
  }
  else if (key == "obdMacAddress")
    setCharField(value, liveDataWebInt->settings.obdMacAddress);
  else if (key == "serviceUUID")
    setCharField(value, liveDataWebInt->settings.serviceUUID);
  else if (key == "charTxUUID")
    setCharField(value, liveDataWebInt->settings.charTxUUID);
  else if (key == "charRxUUID")
    setCharField(value, liveDataWebInt->settings.charRxUUID);
  else if (key == "disableCommandOptimizer")
    liveDataWebInt->settings.disableCommandOptimizer = value.toInt() ? 1 : 0;
  else if (key == "commandQueueAutoStop")
    liveDataWebInt->settings.commandQueueAutoStop = value.toInt() ? 1 : 0;
  else if (key == "obd2Name")
    setCharField(value, liveDataWebInt->settings.obd2Name);
  else if (key == "obd2WifiIp")
    setCharField(value, liveDataWebInt->settings.obd2WifiIp);
  else if (key == "obd2WifiPort")
    liveDataWebInt->settings.obd2WifiPort = value.toInt();
  else if (key == "carType")
    liveDataWebInt->settings.carType = value.toInt();
  else if (key == "boardPowerMode")
    liveDataWebInt->settings.boardPowerMode = value.toInt() ? 1 : 0;
  else if (key == "displayRotation")
  {
    liveDataWebInt->settings.displayRotation = value.toInt();
    note = "Saved (reboot required)";
  }
  else if (key == "defaultScreen")
    liveDataWebInt->settings.defaultScreen = value.toInt();
  else if (key == "lcdBrightness")
  {
    liveDataWebInt->settings.lcdBrightness = value.toInt();
    boardWebInt->setBrightness();
  }
  else if (key == "distanceUnit")
    liveDataWebInt->settings.distanceUnit = value.length() ? value.charAt(0) : 'k';
  else if (key == "temperatureUnit")
    liveDataWebInt->settings.temperatureUnit = value.length() ? value.charAt(0) : 'c';
  else if (key == "pressureUnit")
    liveDataWebInt->settings.pressureUnit = value.length() ? value.charAt(0) : 'b';
  else if (key == "speedCorrection")
    liveDataWebInt->settings.speedCorrection = value.toInt();
  else if (key == "rightHandDrive")
    liveDataWebInt->settings.rightHandDrive = value.toInt() ? 1 : 0;
  else if (key == "gpsModuleType")
  {
    liveDataWebInt->settings.gpsModuleType = value.toInt();
    if (liveDataWebInt->settings.gpsModuleType == GPS_MODULE_TYPE_NEO_M8N)
    {
      liveDataWebInt->settings.gpsSerialPortSpeed = 9600;
      note = "Saved (GPS speed auto-set to 9600, reboot required)";
    }
    else if (liveDataWebInt->settings.gpsModuleType == GPS_MODULE_TYPE_M5_GNSS)
    {
      liveDataWebInt->settings.gpsSerialPortSpeed = 38400;
      note = "Saved (GPS speed auto-set to 38400, reboot required)";
    }
    else if (liveDataWebInt->settings.gpsModuleType == GPS_MODULE_TYPE_GPS_V21_GNSS)
    {
      liveDataWebInt->settings.gpsSerialPortSpeed = 115200;
      note = "Saved (GPS speed auto-set to 115200, reboot required)";
    }
    else
    {
      note = "Saved (reboot required)";
    }
  }
  else if (key == "gpsHwSerialPort")
    liveDataWebInt->settings.gpsHwSerialPort = value.toInt();
  else if (key == "gpsSerialPortSpeed")
    liveDataWebInt->settings.gpsSerialPortSpeed = value.toInt();
  else if (key == "carSpeedType")
    liveDataWebInt->settings.carSpeedType = value.toInt();
  else if (key == "sdcardEnabled")
    liveDataWebInt->settings.sdcardEnabled = value.toInt() ? 1 : 0;
  else if (key == "sdcardAutstartLog")
    liveDataWebInt->settings.sdcardAutstartLog = value.toInt() ? 1 : 0;
  else if (key == "sdcardLogIntervalSec")
    liveDataWebInt->settings.sdcardLogIntervalSec = value.toInt();
  else if (key == "remoteUploadModuleType")
    liveDataWebInt->settings.remoteUploadModuleType = value.toInt();
  else if (key == "remoteApiUrl")
    setCharField(value, liveDataWebInt->settings.remoteApiUrl);
  else if (key == "remoteApiKey")
    setCharField(value, liveDataWebInt->settings.remoteApiKey);
  else if (key == "remoteUploadIntervalSec")
    liveDataWebInt->settings.remoteUploadIntervalSec = value.toInt();
  else if (key == "abrpApiToken")
    setCharField(value, liveDataWebInt->settings.abrpApiToken);
  else if (key == "remoteUploadAbrpIntervalSec")
  {
    const float intervalSec = value.toFloat();
    if (intervalSec <= 0.0f)
    {
      liveDataWebInt->settings.remoteUploadAbrpIntervalSec = 0;
    }
    else
    {
      int steps = static_cast<int>(intervalSec * 2.0f + 0.5f);
      if (steps < 1)
        steps = 1;
      if (steps > 10)
        steps = 10;
      liveDataWebInt->settings.remoteUploadAbrpIntervalSec = steps;
    }
  }
  else if (key == "abrpSdcardLog")
    liveDataWebInt->settings.abrpSdcardLog = value.toInt() ? 1 : 0;
  else if (key == "mqttEnabled")
    liveDataWebInt->settings.mqttEnabled = value.toInt() ? 1 : 0;
  else if (key == "mqttServer")
    setCharField(value, liveDataWebInt->settings.mqttServer);
  else if (key == "mqttId")
    setCharField(value, liveDataWebInt->settings.mqttId);
  else if (key == "mqttUsername")
    setCharField(value, liveDataWebInt->settings.mqttUsername);
  else if (key == "mqttPassword")
    setCharField(value, liveDataWebInt->settings.mqttPassword);
  else if (key == "mqttPubTopic")
    setCharField(value, liveDataWebInt->settings.mqttPubTopic);
  else if (key == "contributeData")
    liveDataWebInt->settings.contributeData = value.toInt() ? 1 : 0;
  else if (key == "contributeToken")
    setCharField(value, liveDataWebInt->settings.contributeToken);
  else if (key == "voltmeterEnabled")
    liveDataWebInt->settings.voltmeterEnabled = value.toInt() ? 1 : 0;
  else if (key == "voltmeterBasedSleep")
    liveDataWebInt->settings.voltmeterBasedSleep = value.toInt() ? 1 : 0;
  else if (key == "voltmeterCutOff")
    liveDataWebInt->settings.voltmeterCutOff = value.toFloat();
  else if (key == "voltmeterSleep")
    liveDataWebInt->settings.voltmeterSleep = value.toFloat();
  else if (key == "voltmeterWakeUp")
    liveDataWebInt->settings.voltmeterWakeUp = value.toFloat();
  else if (key == "serialConsolePort")
    liveDataWebInt->settings.serialConsolePort = value.toInt();
  else if (key == "debugLevel")
  {
    liveDataWebInt->settings.debugLevel = value.toInt();
    syslog->setDebugLevel(liveDataWebInt->settings.debugLevel);
  }
  else if (key == "sleepModeLevel")
    liveDataWebInt->settings.sleepModeLevel = value.toInt();
  else if (key == "sleepModeIntervalSec")
    liveDataWebInt->settings.sleepModeIntervalSec = value.toInt();
  else if (key == "sleepModeShutdownHrs")
    liveDataWebInt->settings.sleepModeShutdownHrs = value.toInt();
  else if (key == "timezone")
    liveDataWebInt->settings.timezone = value.toInt();
  else if (key == "daylightSaving")
    liveDataWebInt->settings.daylightSaving = value.toInt() ? 1 : 0;
  else
    ok = false;

  if (!ok)
  {
    webServer->send(400, "text/plain", "ERR unknown key");
    return;
  }

  boardWebInt->saveSettings();
  if (note.length() > 0)
    webServer->send(200, "text/plain", "OK:" + note);
  else
    webServer->send(200, "text/plain", "OK");
}

void handleAction()
{
  if (webServer == nullptr)
  {
    return;
  }
  String cmd = webServer->hasArg("cmd") ? webServer->arg("cmd") : "";
  if (cmd == "reboot")
  {
    webServer->send(200, "text/plain", "OK");
    ESP.restart();
    return;
  }
  if (cmd == "shutdown")
  {
    webServer->send(200, "text/plain", "OK");
    boardWebInt->enterSleepMode(0);
    return;
  }
  if (cmd == "saveSettings")
  {
    boardWebInt->saveSettings();
    webServer->send(200, "text/plain", "OK");
    return;
  }

  webServer->send(400, "text/plain", "ERR unknown action");
}

void handleBleScan()
{
  if (webServer == nullptr)
  {
    return;
  }
  boardWebInt->scanDevices = true;
  webServer->send(200, "text/plain", "OK");
}

void handleBleDevices()
{
  if (webServer == nullptr)
  {
    return;
  }
  String json = "{\"devices\":[";
  bool first = true;
  for (uint16_t i = 0; i < liveDataWebInt->menuItemsCount; ++i)
  {
    if (liveDataWebInt->menuItems[i].id < LIST_OF_BLE_1 || liveDataWebInt->menuItems[i].id > LIST_OF_BLE_10)
      continue;
    if (strlen(liveDataWebInt->menuItems[i].title) == 0 || strcmp(liveDataWebInt->menuItems[i].title, "-") == 0)
      continue;
    if (strlen(liveDataWebInt->menuItems[i].obdMacAddress) == 0)
      continue;
    if (!first)
      json += ",";
    first = false;
    json += "{\"mac\":\"" + jsonEscape(String(liveDataWebInt->menuItems[i].obdMacAddress)) + "\",\"title\":\"" +
            jsonEscape(String(liveDataWebInt->menuItems[i].title)) + "\"}";
  }
  json += "]}";
  webServer->send(200, "application/json", json);
}

/**
 * Init
 **/
void WebInterface::init(LiveData *pLiveData, BoardInterface *pBoard)
{
  liveDataWebInt = pLiveData;
  boardWebInt = pBoard;

  if (webServer == nullptr)
  {
    webServer = new ESP32WebServer(80);
    if (webServer == nullptr)
    {
      syslog->println("Failed to allocate web server");
      return;
    }
  }

  liveDataWebInt->params.wifiApMode = true;

  // Switch wifi to AP mode
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.begin();
  syslog->println("Ready");
  syslog->print("IP address: ");
  syslog->println(WiFi.softAPIP());

  // Enable webserver
  webServer->on("/", handleRoot);
  webServer->on("/set", handleSet);
  webServer->on("/action", handleAction);
  webServer->on("/ble-scan", handleBleScan);
  webServer->on("/ble-devices", handleBleDevices);
  webServer->onNotFound(handleNotFound);
  webServer->begin();
  Serial.println("HTTP server started");
}

/**
  Main loop
*/
void WebInterface::mainLoop()
{
  if (webServer != nullptr)
  {
    webServer->handleClient();
  }
}
