/*
This C++ code is setting up a web interface for an EV dashboard application.
It includes header files for various functionality like WiFi, HTTP, and a web server. It defines ssid and password constants for connecting to a specific WiFi network.
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

ESP32WebServer server(80);
IPAddress ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

LiveData *liveDataWebInt;
BoardInterface *boardWebInt;

/**
 * Get off/on text
 */
String getOffOnText(uint8_t state)
{
  return String(state == 1 ? "<span class='on'>ON</span>" : "<span class='off'>off</span>");
}

/**
 *  Handle webserver root
 **/
void handleRoot()
{
  String text;
  char tmpStr1[20];

  // Render html5
  //<meta http-equiv='refresh' content='5'>
  text = "<html><head><link href='https://fonts.googleapis.com/css?family=Roboto+Condensed' rel='stylesheet'><style>body { margin:32px; background:black; color: white; font-family: 'Roboto Condensed', sans-serif; } td { padding: 0px 16px 0px 0px; text-align: left;} th { color: orange; text-align: left; padding: 16px 16px 4px 0px; } td:first-child { text-align: left; } .off { color: gray; } .on { color: lime; } .red { color: red; } .lime { color: lime; } .yellow { color: yellow; } .right {text-align:right; } h2 { margin-top: 12px; font-size: 1.1em; }</style></head>";
  text += "<body><h1>[ evDash " + String(APP_VERSION) + ", settings v" + String(liveDataWebInt->settings.settingsVersion) + " ] </h1>";

  // 1st column settings
  text += "<div style='float:left; width:50%;'>";

  text += "<table style='min-width: 25%;'>";
  text += "<tr><th colspan='2'>Communication</th></tr>";
  text += "<tr><td>Type</td><td>" + String(liveDataWebInt->settings.commType == COMM_TYPE_CAN_COMMU ? "CAN COMMU module" : liveDataWebInt->settings.commType == COMM_TYPE_OBD2_BLE4 ? "OBD2 BLE4"
                                                                                                                       : liveDataWebInt->settings.commType == COMM_TYPE_OBD2_WIFI   ? "OBD2 WIFI"
                                                                                                                                                                                    : "UNKNOWN") +
          "</td></tr>";
  text += "<tr><td>Obd2 Mac</td><td>" + String(liveDataWebInt->settings.obdMacAddress) + "</td></tr>";
  text += "<tr><td>Obd2 Ble4 Service uuid</td><td>" + String(liveDataWebInt->settings.serviceUUID) + "</td></tr>";
  text += "<tr><td>Obd2 Ble4 Tx uuid</td><td>" + String(liveDataWebInt->settings.charTxUUID) + "</td></tr>";
  text += "<tr><td>Obd2 Ble4 Rx uuid</td><td>" + String(liveDataWebInt->settings.charRxUUID) + "</td></tr>";
  text += "<tr><td>Disable command optimizer (log all cells)</td><td>" + getOffOnText(liveDataWebInt->settings.disableCommandOptimizer) + "</td></tr>";
  text += "<tr><td>CAN threading (very unstable)</td><td>" + getOffOnText(liveDataWebInt->settings.threading) + "</td></tr>";

  text += "<tr><th colspan='2'>Wifi client</th></tr>";
  text += "<tr><td>Enabled</td><td>" + getOffOnText(liveDataWebInt->settings.wifiEnabled) + "</td></tr>";
  text += "<tr><td>Ssid</td><td><input name='wifiSsid' value='" + String(liveDataWebInt->settings.wifiSsid) + "'></td></tr>";
  text += "<tr><td>Wifi password</td><td><input name='wifiPassword' value='" + String(liveDataWebInt->settings.wifiPassword) + "'></td></tr>";
  text += "<tr><td>Backup wifi enabled</td><td>" + String(liveDataWebInt->settings.backupWifiEnabled) + "</td></tr>";
  text += "<tr><td>Ssid backup</td><td><input name='wifiSsid2' value='" + String(liveDataWebInt->settings.wifiSsid2) + "'></td></tr>";
  text += "<tr><td>Backup wifi password</td><td><input name='wifiPassword2' value='" + String(liveDataWebInt->settings.wifiPassword2) + "'></td></tr>";
  text += "<tr><td>ntpEnabled</td><td>" + getOffOnText(liveDataWebInt->settings.ntpEnabled) + "</td></tr>";
  text += "<tr><td>ntpTimezone</td><td>" + String(liveDataWebInt->settings.ntpTimezone) + "</td></tr>";
  text += "<tr><td>ntpDaySaveTime</td><td>" + getOffOnText(liveDataWebInt->settings.ntpDaySaveTime) + "</td></tr>";

  text += "<tr><th colspan='2'>Sdcard</th></tr>";
  text += "<tr><td>Mode</td><td>" + String((liveDataWebInt->settings.sdcardEnabled == 0) ? "off" : (strlen(liveDataWebInt->params.sdcardFilename) != 0) ? "WRITE"
                                                                                               : (liveDataWebInt->params.sdcardInit)                    ? "READY"
                                                                                                                                                        : "MOUNTED") +
          "</td></tr>";

  text += "<tr><td>Enabled</td><td>" + getOffOnText(liveDataWebInt->settings.sdcardEnabled) + "</td></tr>";
  text += "<tr><td>Autostart log</td><td>" + getOffOnText(liveDataWebInt->settings.sdcardAutstartLog) + "</td></tr>";
  text += "<tr><td>Log interval [sec.]</td><td>" + String(liveDataWebInt->settings.sdcardAutstartLog) + "</td></tr>";
  text += "<tr><td>Used / Total MB</td><td>" + String(SD.usedBytes() / 1048576) + " / " + String(SD.totalBytes() / 1048576) + "</td></tr>";

  text += "<tr><th colspan='2'>Remote upload</th></tr>";
  text += "<tr><td>Module</td><td>" + String(liveDataWebInt->settings.remoteUploadModuleType == REMOTE_UPLOAD_OFF ? "OFF" : "WIFI") + "</td></tr>";
  text += "<tr><td>Remote API url</td><td>" + String(liveDataWebInt->settings.remoteApiUrl) + "</td></tr>";
  text += "<tr><td>Remote API key</td><td>" + String(liveDataWebInt->settings.remoteApiKey) + "</td></tr>";
  text += "<tr><td>Remote upload interval [sec.]</td><td>" + String(liveDataWebInt->settings.remoteUploadIntervalSec) + "</td></tr>";
  text += "<tr><td>Abrp API token</td><td>" + String(liveDataWebInt->settings.abrpApiToken) + "</td></tr>";
  text += "<tr><td>Abrp upload interval [sec.]</td><td>" + String(liveDataWebInt->settings.remoteUploadAbrpIntervalSec) + "</td></tr>";
  text += "<tr><td>Abrp sdcard logs</td><td>" + getOffOnText(liveDataWebInt->settings.abrpSdcardLog) + "</td></tr>";
  text += "<tr><td>MQTT enabled</td><td>" + getOffOnText(liveDataWebInt->settings.mqttEnabled) + "</td></tr>";
  text += "<tr><td>MQTT server</td><td>" + String(liveDataWebInt->settings.mqttServer) + "</td></tr>";
  text += "<tr><td>MQTT ID</td><td>" + String(liveDataWebInt->settings.mqttId) + "</td></tr>";
  text += "<tr><td>MQTT username</td><td>" + String(liveDataWebInt->settings.mqttUsername) + "</td></tr>";
  text += "<tr><td>MQTT password</td><td>" + String(liveDataWebInt->settings.mqttPassword) + "</td></tr>";
  text += "<tr><td>MQTT pub.topic</td><td>" + String(liveDataWebInt->settings.mqttPubTopic) + "</td></tr>";

  text += "<tr><th colspan='2'>GPS module</th></tr>";
  text += "<tr><td>GPS hw serial port</td><td>" + String(liveDataWebInt->settings.gpsHwSerialPort == 255 ? "off" : String(liveDataWebInt->settings.gpsHwSerialPort)) + "</td></tr>";
  text += "<tr><td>GPS serial speed</td><td>" + String(liveDataWebInt->settings.gpsSerialPortSpeed) + "</td></tr>";
  text += "<tr><td>GPS valid data</td><td>" + String(liveDataWebInt->params.gpsValid == 1 ? "YES" : "NO") + "</td></tr>";
  if (liveDataWebInt->params.gpsValid)
  {
    text += "<tr><td>Latitude</td><td>" + String(liveDataWebInt->params.gpsLat) + "</td></tr>";
    text += "<tr><td>Longitude</td><td>" + String(liveDataWebInt->params.gpsLon) + "</td></tr>";
    text += "<tr><td>Alt</td><td>" + String(liveDataWebInt->params.gpsAlt) + "</td></tr>";
    text += "<tr><td>Satellites</td><td>" + String(liveDataWebInt->params.gpsSat) + "</td></tr>";
  }

  text += "<tr><th colspan='2'>Voltmeter</th></tr>";
  text += "<tr><td>INA3221 enabled</td><td>" + getOffOnText(liveDataWebInt->settings.voltmeterEnabled) + "</td></tr>";
  text += "<tr><td>Voltmeter based sleep</td><td>" + getOffOnText(liveDataWebInt->settings.voltmeterBasedSleep) + "</td></tr>";
  text += "<tr><td>Cut off voltage [V]</td><td>" + String(liveDataWebInt->settings.voltmeterCutOff) + "</td></tr>";
  text += "<tr><td>Sleep voltage [V]</td><td>" + String(liveDataWebInt->settings.voltmeterSleep) + "</td></tr>";
  text += "<tr><td>Wake up voltage [V]</td><td>" + String(liveDataWebInt->settings.voltmeterCutOff) + "</td></tr>";

  text += "<tr><th colspan='2'>Console</th></tr>";
  text += "<tr><td>Enabled</td><td>" + String(liveDataWebInt->settings.serialConsolePort == 255 ? "off" : String(liveDataWebInt->settings.serialConsolePort)) + "</td></tr>";
  text += "<tr><td>Debug level</td><td>" + String(liveDataWebInt->settings.debugLevel) + "</td></tr>";

  text += "<tr><th colspan='2'>Sleep</th></tr>";
  text += "<tr><td>Mode</td><td>" + String(liveDataWebInt->settings.sleepModeLevel == SLEEP_MODE_OFF ? "off" : liveDataWebInt->settings.sleepModeLevel == SLEEP_MODE_SCREEN_ONLY ? "SCREEN ONLY"
                                                                                                                                                                                 // 202408: removed: : liveDataWebInt->settings.sleepModeLevel == SLEEP_MODE_DEEP_SLEEP    ? "DEEP SLEEP"
                                                                                                                                                                                 // 202408: removed: : liveDataWebInt->settings.sleepModeLevel == SLEEP_MODE_SHUTDOWN      ? "SHUTDOWN"
                                                                                                                                                                                 : "UNKNOWN") +
          "</td></tr>";
  text += "<tr><td>sleepModeIntervalSec</td><td>" + String(liveDataWebInt->settings.sleepModeIntervalSec) + "</td></tr>";
  text += "<tr><td>sleepModeShutdownHrs</td><td>" + String(liveDataWebInt->settings.sleepModeShutdownHrs) + "</td></tr>";

  text += "<tr><th colspan='2'>Time</th></tr>";
  text += "<tr><td>Mode</td><td>" + String(liveDataWebInt->params.ntpTimeSet == 1 ? " NTP " : "") + String(liveDataWebInt->params.currTimeSyncWithGps == 1 ? " GPS " : "") + "</td></tr>";
  struct tm now;
  getLocalTime(&now);
  sprintf(tmpStr1, "%02d-%02d-%02d %02d:%02d:%02d", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
  text += "<tr><td>Current time</td><td>" + String(tmpStr1) + "</td></tr>";
  text += "<tr><td>Timezone</td><td>" + String(liveDataWebInt->settings.timezone) + "</td></tr>";
  text += "<tr><td>Daylight saving</td><td>" + getOffOnText(liveDataWebInt->settings.daylightSaving) + "</td></tr>";

  text += "</table>";

  text += "</div>";

  // 2nd column - car info
  text += "<div style='float:left; width:50%;'>";

  text += "<table style='min-width: 25%;'>";
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

  text += "</body></html>";

  // Send page to client
  server.send(200, "text/html", text);
}

/**
   Handle 404 Page not found
*/
void handleNotFound()
{

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

/**
 * Init
 **/
void WebInterface::init(LiveData *pLiveData, BoardInterface *pBoard)
{
  liveDataWebInt = pLiveData;
  boardWebInt = pBoard;

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
  server.on("/", handleRoot);
  // server.on("/export", handleExport);
  /*server.on("/inline", []()
            { server.send(200, "text/plain", "this works as well"); });*/
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

/**
  Main loop
*/
void WebInterface::mainLoop()
{
  server.handleClient();
  // syslog->println("handleClient");
}
