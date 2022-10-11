//#include <SD.h>
#include <FS.h>
#include <analogWrite.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Update.h>
#include "config.h"
#include "BoardInterface.h"
#include "Board320_240.h"
#include <time.h>
#include <ArduinoJson.h>

RTC_DATA_ATTR unsigned int bootCount = 0;
RTC_DATA_ATTR unsigned int sleepCount = 0;

/**
   Init board
*/
void Board320_240::initBoard()
{
  liveData->params.booting = true;

// Set button pins for input
#ifdef BOARD_TTGO_T4
  pinMode(pinButtonMiddle, INPUT);
  pinMode(pinButtonLeft, INPUT);
  pinMode(pinButtonRight, INPUT);
#endif // BOARD_TTGO_T4

  // Init time library
  struct timeval tv;

#ifdef BOARD_M5STACK_CORE2
  RTC_TimeTypeDef RTCtime;
  RTC_DateTypeDef RTCdate;

  M5.Rtc.GetTime(&RTCtime);
  M5.Rtc.GetDate(&RTCdate);

  if (RTCdate.Year > 2020)
  {
    struct tm tm_tmp;
    tm_tmp.tm_year = RTCdate.Year - 1900;
    tm_tmp.tm_mon = RTCdate.Month - 1;
    tm_tmp.tm_mday = RTCdate.Date;
    tm_tmp.tm_hour = RTCtime.Hours;
    tm_tmp.tm_min = RTCtime.Minutes;
    tm_tmp.tm_sec = RTCtime.Seconds;

    time_t t = mktime(&tm_tmp);
    tv.tv_sec = t;
  }
  else
  {
    tv.tv_sec = 1589011873;
  }
#else
  tv.tv_sec = 1589011873;
#endif

  /*struct timezone tz;
  tz.tz_minuteswest = (liveData->settings.timezone + liveData->settings.daylightSaving) * 60;
  tz.tz_dsttime = 0;*/
  settimeofday(&tv, NULL);
  struct tm tm;
  getLocalTime(&tm);
  liveData->params.currentTime = mktime(&tm);
  liveData->params.chargingStartTime = liveData->params.currentTime;

  // Boot counter
  ++bootCount;
}

/**
   After setup device
*/
void Board320_240::afterSetup()
{
  // Check if board was sleeping
  bool afterSetup = false;

  syslog->print("Boot count: ");
  syslog->println(bootCount);
  syslog->print("SleepMode: ");
  syslog->println(liveData->settings.sleepModeLevel);
  syslog->print("Continuous sleep count: ");
  syslog->println(sleepCount);

  // Init Voltmeter
  if (liveData->settings.voltmeterEnabled == 1)
  {
    syslog->println("Initializing INA3221 voltmeter:");
    ina3221.begin();
    syslog->print("ch1:");
    syslog->print(ina3221.getBusVoltage_V(1));
    syslog->print("V\t ch2:");
    syslog->print(ina3221.getBusVoltage_V(2));
    syslog->print("V\t ch3:");
    syslog->print(ina3221.getBusVoltage_V(3));
    syslog->println("V");
    syslog->print("ch1:");
    syslog->print(ina3221.getCurrent_mA(1));
    syslog->print("mA\t ch2:");
    syslog->print(ina3221.getCurrent_mA(2));
    syslog->print("mA\t ch3:");
    syslog->print(ina3221.getCurrent_mA(3));
    syslog->println("mA");
  }

  if (liveData->settings.sleepModeLevel >= SLEEP_MODE_DEEP_SLEEP && !skipAdapterScan() && bootCount > 1)
  {
    // Init comm device if COMM device based wakeup
    if (liveData->settings.voltmeterBasedSleep == 0)
    {
      afterSetup = true;
      BoardInterface::afterSetup();
    }

    // Wake or go to sleep again
    afterSleep();
    sleepCount = 0;
  }

  wakeupBoard();

  // Init display
  syslog->println("Init TFT display");
  tft.begin();
  tft.invertDisplay(invertDisplay);
  tft.setRotation(liveData->settings.displayRotation);
  setBrightness();
  tft.fillScreen(TFT_BLACK);

  bool psramUsed = false; // 320x240 16bpp sprites requires psram
#if defined(ESP32) && defined(CONFIG_SPIRAM_SUPPORT)
  if (psramFound())
    psramUsed = true;
#endif
  syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
  syslog->println((psramUsed) ? "Sprite 16" : "Sprite 8");
  spr.setColorDepth((psramUsed) ? 16 : 8);
  spr.createSprite(320, 240);
  liveData->params.spriteInit = true;
  syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());

  // Show test data on right button during boot device
  liveData->params.displayScreen = liveData->settings.defaultScreen;
  if (isButtonPressed(pinButtonRight))
  {
    syslog->printf("Right button pressed");
    loadTestData();
    syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
  }
  // Wifi
  // Starting Wifi after BLE prevents reboot loop

  if (liveData->settings.wifiEnabled == 1)
  {
    wifiSetup();
    syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
  }

  // Init GPS
  if (liveData->settings.gpsHwSerialPort <= 2)
  {
    initGPS();
    syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
  }

  // SD card
  if (liveData->settings.sdcardEnabled == 1)
  {
    if (sdcardMount() && liveData->settings.sdcardAutstartLog == 1)
    {
      syslog->println("Toggle recording on SD card");
      sdcardToggleRecording();
      syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
    }
  }

  // Init SIM800L
  if (liveData->settings.gprsHwSerialPort <= 2)
  {
    sim800lSetup();
    syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
  }

  // Init comm device
  if (!afterSetup)
  {
    BoardInterface::afterSetup();
    syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
  }

  // Threading
  // Comm via thread (ble/can)
  if (liveData->settings.threading)
  {
    xTaskCreate(xTaskCommLoop, "xTaskCommLoop", 4096, (void *)this, 0, NULL);
  }

  showTime();

  liveData->params.wakeUpTime = liveData->params.currentTime;
  liveData->params.lastCanbusResponseTime = liveData->params.currentTime;
  liveData->params.booting = false;
}

/**
 * OTA Update
 */
void Board320_240::otaUpdate()
{
#include "raw_githubusercontent_com.h" // the root certificate is now in const char * root_cert

  syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());

  String url = "https://raw.githubusercontent.com/nickn17/evDash/master/dist/m5stack-core2/evDash.ino.bin";
#ifdef BOARD_TTGO_T4
  url = "https://raw.githubusercontent.com/nickn17/evDash/master/dist/ttgo-t4-v13/evDash.ino.bin";
#endif // BOARD_TTGO_T4
#ifdef BOARD_M5STACK_CORE
  url = "https://raw.githubusercontent.com/nickn17/evDash/master/dist/m5stack-core/evDash.ino.bin";
#endif // BOARD_M5STACK_CORE

  if (!WiFi.isConnected())
  {
    displayMessage("No WiFi connection.", "");
    delay(2000);
    return;
  }

  if (!url.startsWith("https://"))
  {
    displayMessage("URL must start with 'https://'", "");
    delay(2000);
    return;
  }

  displayMessage("Downloading OTA...", "");
  url = url.substring(8);

  String host, file;
  uint16_t port;
  int16_t first_slash_pos = url.indexOf("/");
  if (first_slash_pos == -1)
  {
    host = url;
    file = "/";
  }
  else
  {
    host = url.substring(0, first_slash_pos);
    file = url.substring(first_slash_pos);
  }
  int16_t colon = host.indexOf(":");

  if (colon == -1)
  {
    port = 443;
  }
  else
  {
    host = host.substring(0, colon);
    port = host.substring(colon + 1).toInt();
  }

  WiFiClientSecure client;
  client.setTimeout(20000);
  client.setCACert(root_cert);

  int contentLength = 0;

  if (!client.connect(host.c_str(), port))
  {
    syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
    displayMessage("Connection failed.", "");
    delay(2000);
    return;
  }

  client.print(String("GET ") + file + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Cache-Control: no-cache\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (!client.available())
  {
    if (millis() - timeout > 10000)
    {
      displayMessage("Client Timeout", "");
      client.stop();
      delay(2000);
      return;
    }
  }

  // Process header
  while (client.available())
  {
    String line = client.readStringUntil('\n');
    line.trim();
    if (!line.length())
      break; // empty line, assume headers done

    if (line.startsWith("HTTP/1.1"))
    {
      String http_response = line.substring(line.indexOf(" ") + 1);
      http_response.trim();
      if (http_response.indexOf("200") == -1)
      {
        syslog->println(http_response);
        displayMessage("Got response: must be 200", "");
        // displayMessage("Got response: \"" + http_response + "\", must be 200", http_response);
        delay(2000);
        return;
      }
    }

    if (line.startsWith("Content-Length: "))
    {
      contentLength = atoi(line.substring(line.indexOf(":") + 1).c_str());
      if (contentLength <= 0)
      {
        displayMessage("Content-Length zero", "");
        delay(2000);
        return;
      }
    }

    if (line.startsWith("Content-Type: "))
    {
      String contentType = line.substring(line.indexOf(":") + 1);
      contentType.trim();
      if (contentType != "application/octet-stream")
      {
        displayMessage("Content-Type must be", "application/octet-stream");
        delay(2000);
        return;
      }
    }
  }

  // Process payload
  displayMessage("Installing OTA...", "");
  if (!Update.begin(contentLength))
  {
    syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
    syslog->printf("Error: %i, malloc %i bytes\n", Update.getError(), SPI_FLASH_SEC_SIZE);
    syslog->print("contentLength: ");
    syslog->println(contentLength);
    displayMessage("Not enough space", "to begin OTA");
    client.flush();
    delay(2000);
    return;
  }

  displayMessage("Writing stream...", "Please wait!");
  /*size_t written =*/Update.writeStream(client);

  displayMessage("End...", "");
  if (!Update.end())
  {
    displayMessage("Downloading error", "");
    delay(2000);
    return;
  }

  displayMessage("Is finished?", "");
  if (!Update.isFinished())
  {
    displayMessage("Update not finished.", "Something went wrong.");
    delay(2000);
    return;
  }

  displayMessage("OTA installed.", "Reboot device.");
  delay(2000);
  ESP.restart();
}

/**
   Go to Sleep for TIME_TO_SLEEP seconds
*/
void Board320_240::goToSleep()
{
  syslog->println("Going to sleep.");

  // Sleep SIM800L
  if (liveData->params.sim800l_enabled)
  {
    if (sim800l->isConnectedGPRS())
    {
      bool disconnected = sim800l->disconnectGPRS();
      for (uint8_t i = 0; i < 5 && !disconnected; i++)
      {
        delay(1000);
        disconnected = sim800l->disconnectGPRS();
      }
    }

    if (sim800l->getPowerMode() == NORMAL)
    {
      sim800l->setPowerMode(SLEEP);
      delay(1000);
    }
    sim800l->enterSleepMode();
  }

  // Sleep GPS
  if (gpsHwUart != NULL)
  {
    uint8_t GPSoff[] = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4D, 0x3B};
    gpsHwUart->write(GPSoff, sizeof(GPSoff));
  }

  int sleepSeconds = 0;

  if (liveData->settings.sleepModeLevel == SLEEP_MODE_DEEP_SLEEP &&
      (sleepCount * liveData->settings.sleepModeIntervalSec <= liveData->settings.sleepModeShutdownHrs * 3600 ||
       liveData->settings.sleepModeShutdownHrs == 0))
  {
    sleepSeconds = liveData->settings.sleepModeIntervalSec;
  }

  ++sleepCount;

  enterSleepMode(sleepSeconds);
}

/*
  Wake up board from sleep
  Iterate thru commands and determine if car is charging or ignition is on
*/
void Board320_240::afterSleep()
{
  // Wakeup reason
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    syslog->println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    syslog->println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    syslog->println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    syslog->println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    syslog->println("Wakeup caused by ULP program");
    break;
  default:
    syslog->printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
  {
    syslog->println("Button pressed = Waking up");
    return;
  }

  if (liveData->settings.voltmeterBasedSleep == 1)
  {
    liveData->params.auxVoltage = ina3221.getBusVoltage_V(1);

#ifdef BOARD_M5STACK_CORE2
    if (liveData->settings.sdcardEnabled == 1 && bootCount % (unsigned int)(300 / liveData->settings.sleepModeIntervalSec) == 0)
    {
      tft.begin();
      sdcardMount();

      if (liveData->params.sdcardInit)
      {
        struct tm now;
        getLocalTime(&now);
        char filename[32];
        strftime(filename, sizeof(filename), "/sleep_%y-%m-%d.json", &now);

        File file = SD.open(filename, FILE_APPEND);
        if (!file)
        {
          syslog->println("Failed to open file for appending");
          File file = SD.open(filename, FILE_WRITE);
        }
        if (file)
        {
          StaticJsonDocument<128> jsonData;
          jsonData["carType"] = liveData->settings.carType;
          jsonData["currTime"] = liveData->params.currentTime;
          jsonData["auxV"] = liveData->params.auxVoltage;
          jsonData["bootCount"] = bootCount;
          jsonData["sleepCount"] = sleepCount;
          serializeJson(jsonData, file);

          file.print(",\n");
          file.close();
        }
      }
    }
#endif

    if (liveData->params.auxVoltage > 5 && liveData->params.auxVoltage < liveData->settings.voltmeterCutOff)
    {
      syslog->print("AUX voltage under cut-off voltage: ");
      syslog->println(liveData->settings.voltmeterCutOff);
      liveData->settings.sleepModeLevel = SLEEP_MODE_SHUTDOWN;
      goToSleep();
    }
    else if (liveData->params.auxVoltage > 0 && liveData->params.auxVoltage < liveData->settings.voltmeterWakeUp)
    {
      syslog->print("AUX voltage under: ");
      syslog->println(liveData->settings.voltmeterWakeUp);
      goToSleep();
    }
    else
    {
      liveData->params.lastVoltageOkTime = liveData->params.currentTime;
      syslog->println("Wake up conditions satisfied... Good morning!");
      return;
    }
  }

  liveData->params.sleepModeQueue = true;

  bool firstRun = true;
  while (liveData->commandQueueIndex - 1 > liveData->commandQueueLoopFrom || firstRun)
  {
    if (liveData->commandQueueIndex - 1 == liveData->commandQueueLoopFrom)
    {
      firstRun = false;
    }

    if (millis() > 30000)
    {
      syslog->println("Time's up (30s timeout)...");
      goToSleep();
    }

    commInterface->mainLoop();
  }

  if (liveData->params.getValidResponse)
  {
    syslog->println("Wake up conditions satisfied... Good morning!");
    liveData->params.sleepModeQueue = false;
    return;
  }
  else
  {
    syslog->println("No response from module...");
    goToSleep();
  }
}

/**
 * Turn off screen (brightness 0)
 */
void Board320_240::turnOffScreen()
{
  bool debugTurnOffScreen = false;
  // debugTurnOffScreen = true;
  if (debugTurnOffScreen)
    displayMessage("Turn off screen", (liveData->params.stopCommandQueue ? "Command queue stopped" : "Queue is running"));
  if (currentBrightness == 0)
    return;

  syslog->println("Turn off screen");
  currentBrightness = 0;

  if (debugTurnOffScreen)
    return;

#ifdef BOARD_TTGO_T4
  analogWrite(4 /*TFT_BL*/, 0);
#endif // BOARD_TTGO_T4
#ifdef BOARD_M5STACK_CORE
  tft.setBrightness(0);
#endif // BOARD_M5STACK_CORE
#ifdef BOARD_M5STACK_CORE2
  M5.Axp.SetDCDC3(false);
  M5.Axp.SetLcdVoltage(2500);
#endif // BOARD_M5STACK_CORE2
}

/**
   Set brightness level
*/
void Board320_240::setBrightness()
{
  uint8_t lcdBrightnessPerc;

  liveData->params.stopCommandQueue = false;
  lcdBrightnessPerc = liveData->settings.lcdBrightness;
  if (lcdBrightnessPerc == 0)
  { // automatic brightness (based on gps&and sun angle)
    lcdBrightnessPerc = ((liveData->params.lcdBrightnessCalc == -1) ? 100 : liveData->params.lcdBrightnessCalc);
  }
  if (liveData->params.displayScreen == SCREEN_BLANK)
  {
    turnOffScreen();
    return;
  }
  if (liveData->params.displayScreen == SCREEN_HUD)
  {
    lcdBrightnessPerc = 100;
  }

  if (currentBrightness == lcdBrightnessPerc)
    return;

  syslog->print("Set brightness: ");
  syslog->println(lcdBrightnessPerc);
  currentBrightness = lcdBrightnessPerc;

#ifdef BOARD_TTGO_T4
  analogWrite(4 /*TFT_BL*/, lcdBrightnessPerc);
#endif // BOARD_TTGO_T4
#ifdef BOARD_M5STACK_CORE
  uint8_t brightnessVal = map(lcdBrightnessPerc, 0, 100, 0, 255);
  tft.setBrightness(brightnessVal);
#endif // BOARD_M5STACK_CORE
#ifdef BOARD_M5STACK_CORE2
  M5.Axp.SetDCDC3(true);
  uint16_t lcdVolt = map(lcdBrightnessPerc, 0, 100, 2500, 3300);
  M5.Axp.SetLcdVoltage(lcdVolt);
#endif // BOARD_M5STACK_CORE2
}

/**
  Message dialog
*/
void Board320_240::displayMessage(const char *row1, const char *row2)
{
  uint16_t height = tft.height();
  syslog->print("Message: ");
  syslog->print(row1);
  syslog->print(" ");
  syslog->println(row2);

  // Must draw directly without sprite (psramUsed check)
  if (liveData->params.spriteInit)
  {
    spr.fillRect(0, (height / 2) - 45, tft.width(), 90, TFT_NAVY);
    spr.setTextDatum(ML_DATUM);
    spr.setTextColor(TFT_YELLOW, TFT_NAVY);
    spr.setFreeFont(&Roboto_Thin_24);
    spr.setTextDatum(BL_DATUM);
    spr.drawString(row1, 0, height / 2, GFXFF);
    spr.drawString(row2, 0, (height / 2) + 30, GFXFF);
    spr.pushSprite(0, 0);
  }
  else
  {
    tft.fillRect(0, (height / 2) - 45, tft.width(), 90, TFT_NAVY);
    // tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    tft.setFreeFont(&Roboto_Thin_24);
    tft.setTextDatum(BL_DATUM);
    tft.drawString(row1, 0, height / 2, GFXFF);
    tft.drawString(row2, 0, (height / 2) + 30, GFXFF);
  }
}

/**
  Confirm message
*/
bool Board320_240::confirmMessage(const char *row1, const char *row2)
{
  uint16_t height = tft.height();
  syslog->print("Confirm: ");
  syslog->print(row1);
  syslog->print(" ");
  syslog->println(row2);

  spr.fillRect(0, (height / 2) - 45, tft.width(), 90, TFT_NAVY);
  spr.setTextDatum(ML_DATUM);
  spr.setTextColor(TFT_YELLOW, TFT_NAVY);
  spr.setFreeFont(&Roboto_Thin_24);
  spr.setTextDatum(BL_DATUM);
  spr.drawString(row1, 0, height / 2, GFXFF);
  spr.drawString(row2, 0, (height / 2) + 30, GFXFF);
  spr.fillRect(0, height - 50, 100, 50, TFT_NAVY);
  spr.fillRect(tft.width() - 100, height - 50, 100, 50, TFT_NAVY);
  spr.setTextDatum(BL_DATUM);
  spr.drawString("YES", 10, height - 10, GFXFF);
  spr.setTextDatum(BR_DATUM);
  spr.drawString("NO", tft.width() - 10, height - 10, GFXFF);
  spr.pushSprite(0, 0);

  bool res = false;
  for (uint16_t i = 0; i < 20 * 10; i++)
  {
    boardLoop();
    if (isButtonPressed(pinButtonLeft))
      return true;
    if (isButtonPressed(pinButtonRight))
      return false;
    delay(100);
  }

  return res;
}

/**
  Draw cell on dashboard
*/
void Board320_240::drawBigCell(int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const char *desc, uint16_t bgColor, uint16_t fgColor)
{

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 60) + 1;

  spr.fillRect(x * 80, y * 60, ((w)*80) - 1, ((h)*60) - 1, bgColor);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y)*60) - 1, h * 60, TFT_BLACK);
  spr.drawFastHLine(((x)*80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);
  spr.setTextDatum(TL_DATUM);   // Topleft
  spr.setTextColor(TFT_SILVER); // Bk, fg color
  spr.setTextSize(1);           // Size for small 5x7 font
  spr.drawString(desc, posx, posy, 2);

  // Big 2x2 cell in the middle of screen
  if (w == 2 && h == 2)
  {

    // Bottom 2 numbers with charged/discharged kWh from start
    posx = (x * 80) + 5;
    posy = ((y + h) * 60) - 32;
    sprintf(tmpStr3, "-%01.01f", liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
    spr.setFreeFont(&Roboto_Thin_24);
    spr.setTextDatum(TL_DATUM);
    spr.drawString(tmpStr3, posx, posy, GFXFF);

    posx = ((x + w) * 80) - 8;
    sprintf(tmpStr3, "+%01.01f", liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
    spr.setTextDatum(TR_DATUM);
    spr.drawString(tmpStr3, posx, posy, GFXFF);

    // Main number - kwh on roads, amps on charges
    posy = (y * 60) + 24;
    spr.setTextColor(fgColor);
    spr.setFreeFont(&Orbitron_Light_32);
    spr.drawString(text, posx, posy, 7);
  }
  else
  {
    // All others 1x1 cells
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(fgColor);
    spr.setFreeFont(&Orbitron_Light_24);
    posx = (x * 80) + (w * 80 / 2) - 3;
    posy = (y * 60) + (h * 60 / 2) + 4;
    spr.drawString(text, posx, posy, (w == 2 ? 7 : GFXFF));
  }
}

/**
  Draw small rect 80x30
*/
void Board320_240::drawSmallCell(int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const char *desc, int16_t bgColor, int16_t fgColor)
{

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 32) + 1;

  spr.fillRect(x * 80, y * 32, ((w)*80), ((h)*32), bgColor);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y)*32) - 1, h * 32, TFT_BLACK);
  spr.drawFastHLine(((x)*80) - 1, ((y + h) * 32) - 1, w * 80, TFT_BLACK);
  spr.setTextDatum(TL_DATUM);   // Topleft
  spr.setTextColor(TFT_SILVER); // Bk, fg bgColor
  spr.setTextSize(1);           // Size for small 5x7 font
  spr.drawString(desc, posx, posy, 2);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(fgColor);
  posx = (x * 80) + (w * 80 / 2) - 3;
  spr.drawString(text, posx, posy + 14, 2);
}

/**
  Show tire pressures / temperatures
  Custom field
*/
void Board320_240::showTires(int32_t x, int32_t y, int32_t w, int32_t h, const char *topleft, const char *topright, const char *bottomleft, const char *bottomright, uint16_t color)
{

  int32_t posx, posy;

  spr.fillRect(x * 80, y * 60, ((w)*80) - 1, ((h)*60) - 1, color);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y)*60) - 1, h * 60, TFT_BLACK);
  spr.drawFastHLine(((x)*80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_SILVER);
  spr.setTextSize(1);
  posx = (x * 80) + 4;
  posy = (y * 60) + 0;
  spr.drawString(topleft, posx, posy, 2);
  posy = (y * 60) + 14;
  spr.drawString(bottomleft, posx, posy, 2);

  spr.setTextDatum(TR_DATUM);
  posx = ((x + w) * 80) - 4;
  posy = (y * 60) + 0;
  spr.drawString(topright, posx, posy, 2);
  posy = (y * 60) + 14;
  spr.drawString(bottomright, posx, posy, 2);
}

/**
   Main screen (Screen 1)
*/
void Board320_240::drawSceneMain()
{

  // Tire pressure
  char pressureStr[4] = "bar";
  char temperatureStr[2] = "C";
  if (liveData->settings.pressureUnit != 'b')
    strcpy(pressureStr, "psi");
  if (liveData->settings.temperatureUnit != 'c')
    strcpy(temperatureStr, "F");
  if (liveData->params.tireFrontLeftPressureBar == -1)
  {
    sprintf(tmpStr1, "n/a %s", pressureStr);
    sprintf(tmpStr2, "n/a %s", pressureStr);
    sprintf(tmpStr3, "n/a %s", pressureStr);
    sprintf(tmpStr4, "n/a %s", pressureStr);
  }
  else
  {
    sprintf(tmpStr1, "%01.01f%s %02.00f%s", liveData->bar2pressure(liveData->params.tireFrontLeftPressureBar), pressureStr, liveData->celsius2temperature(liveData->params.tireFrontLeftTempC), temperatureStr);
    sprintf(tmpStr2, "%02.00f%s %01.01f%s", liveData->celsius2temperature(liveData->params.tireFrontRightTempC), temperatureStr, liveData->bar2pressure(liveData->params.tireFrontRightPressureBar), pressureStr);
    sprintf(tmpStr3, "%01.01f%s %02.00f%s", liveData->bar2pressure(liveData->params.tireRearLeftPressureBar), pressureStr, liveData->celsius2temperature(liveData->params.tireRearLeftTempC), temperatureStr);
    sprintf(tmpStr4, "%02.00f%s %01.01f%s", liveData->celsius2temperature(liveData->params.tireRearRightTempC), temperatureStr, liveData->bar2pressure(liveData->params.tireRearRightPressureBar), pressureStr);
  }
  showTires(1, 0, 2, 1, tmpStr1, tmpStr2, tmpStr3, tmpStr4, TFT_BLACK);

  // Added later - kwh total in tires box
  // TODO: refactoring
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_GREEN);
  sprintf(tmpStr1, ((liveData->params.cumulativeEnergyChargedKWh == -1) ? "CEC: n/a" : "C: %01.01f +%01.01fkWh"), liveData->params.cumulativeEnergyChargedKWh, liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
  spr.drawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 30, 2);
  spr.setTextColor(TFT_YELLOW);
  sprintf(tmpStr1, ((liveData->params.cumulativeEnergyDischargedKWh == -1) ? "CED: n/a" : "D: %01.01f -%01.01fkWh"), liveData->params.cumulativeEnergyDischargedKWh, liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
  spr.drawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 44, 2);

  // batPowerKwh100 on roads, else batPowerAmp
  if (liveData->params.speedKmh > 20)
  {
    sprintf(tmpStr1, (liveData->params.batPowerKwh100 == -1 ? "n/a" : "%01.01f"), liveData->km2distance(liveData->params.batPowerKwh100));
    drawBigCell(1, 1, 2, 2, tmpStr1, ((liveData->settings.distanceUnit == 'k') ? "POWER KWH/100KM" : "POWER KWH/100MI"), (liveData->params.batPowerKwh100 >= 0 ? TFT_DARKGREEN2 : (liveData->params.batPowerKwh100 < -30.0 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  }
  else
  {
    // batPowerAmp on chargers (under 10kmh)
    sprintf(tmpStr1, (liveData->params.batPowerKw == -1000) ? "---" : "%01.01f", liveData->params.batPowerKw);
    drawBigCell(1, 1, 2, 2, tmpStr1, "POWER KW", (liveData->params.batPowerKw >= 0 ? TFT_DARKGREEN2 : (liveData->params.batPowerKw <= -30 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  }

  // socPerc
  sprintf(tmpStr1, (liveData->params.socPerc == -1 ? "n/a" : "%01.00f%%"), liveData->params.socPerc);
  sprintf(tmpStr2, (liveData->params.sohPerc == -1) ? "SOC/SOH?" : (liveData->params.sohPerc == 100.0 ? "SOC/H%01.00f%%" : "SOC/H%01.01f%%"), liveData->params.sohPerc);
  drawBigCell(0, 0, 1, 1, ((liveData->params.socPerc == 255) ? "---" : tmpStr1), tmpStr2,
              (liveData->params.socPerc < 10 || (liveData->params.sohPerc != -1 && liveData->params.sohPerc < 100) ? TFT_RED : (liveData->params.socPerc > 80 ? TFT_DARKGREEN2 : TFT_DEFAULT_BK)), TFT_WHITE);

  // batPowerAmp
  sprintf(tmpStr1, (liveData->params.batPowerAmp == -1000) ? "n/a" : (abs(liveData->params.batPowerAmp) > 9.9 ? "%01.00f" : "%01.01f"), liveData->params.batPowerAmp);
  drawBigCell(0, 1, 1, 1, tmpStr1, "CURRENT A", (liveData->params.batPowerAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // batVoltage
  sprintf(tmpStr1, (liveData->params.batVoltage == -1) ? "n/a" : "%03.00f", liveData->params.batVoltage);
  drawBigCell(0, 2, 1, 1, tmpStr1, "VOLTAGE", TFT_DEFAULT_BK, TFT_WHITE);

  // batCellMinV
  sprintf(tmpStr1, "%01.02f", liveData->params.batCellMaxV - liveData->params.batCellMinV);
  sprintf(tmpStr2, (liveData->params.batCellMinV == -1) ? "CELLS" : "CELLS %01.02f", liveData->params.batCellMinV);
  drawBigCell(0, 3, 1, 1, (liveData->params.batCellMaxV - liveData->params.batCellMinV == 0.00 ? "OK" : tmpStr1), tmpStr2, TFT_DEFAULT_BK, TFT_WHITE);

  // batTempC
  sprintf(tmpStr1, (liveData->params.batMinC == -100) ? "n/a" : ((liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), liveData->celsius2temperature(liveData->params.batMinC));
  sprintf(tmpStr2, (liveData->params.batMaxC == -100) ? "BAT.TEMP" : ((liveData->settings.temperatureUnit == 'c') ? "BATT. %01.00fC" : "BATT. %01.01fF"), liveData->celsius2temperature(liveData->params.batMaxC));
  drawBigCell(1, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);

  // batHeaterC
  sprintf(tmpStr1, (liveData->params.batHeaterC == -100) ? "n/a" : ((liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), liveData->celsius2temperature(liveData->params.batHeaterC));
  drawBigCell(2, 3, 1, 1, tmpStr1, "BAT.HEAT", TFT_TEMP, TFT_WHITE);

  // Aux perc / temp
  if (liveData->settings.carType == CAR_BMW_I3_2014)
  { // TODO: use invalid auxPerc value as decision point here?
    sprintf(tmpStr1, "%01.00f", liveData->params.auxTemperature);
    drawBigCell(3, 0, 1, 1, tmpStr1, "AUX TEMP.", (liveData->params.auxTemperature < 5 ? TFT_RED : TFT_DEFAULT_BK), TFT_WHITE);
  }
  else
  {
    sprintf(tmpStr1, (liveData->params.auxPerc == -1) ? "n/a" : "%01.00f%%", liveData->params.auxPerc);
    drawBigCell(3, 0, 1, 1, tmpStr1, "AUX BAT.", (liveData->params.auxPerc < 60 ? TFT_RED : TFT_DEFAULT_BK), TFT_WHITE);
  }

  // Aux amp
  sprintf(tmpStr1, (liveData->params.auxCurrentAmp == -1000) ? "n/a" : (abs(liveData->params.auxCurrentAmp) > 9.9 ? "%01.00f" : "%01.01f"), liveData->params.auxCurrentAmp);
  drawBigCell(3, 1, 1, 1, tmpStr1, "AUX AMPS", (liveData->params.auxCurrentAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // auxVoltage
  sprintf(tmpStr1, (liveData->params.auxVoltage == -1) ? "n/a" : "%01.01f", liveData->params.auxVoltage);
  drawBigCell(3, 2, 1, 1, tmpStr1, "AUX VOLTS", (liveData->params.auxVoltage < 12.1 ? TFT_RED : (liveData->params.auxVoltage < 12.6 ? TFT_ORANGE : TFT_DEFAULT_BK)), TFT_WHITE);

  // indoorTemperature
  sprintf(tmpStr1, (liveData->params.indoorTemperature == -100) ? "n/a" : "%01.01f", liveData->celsius2temperature(liveData->params.indoorTemperature));
  sprintf(tmpStr2, (liveData->params.outdoorTemperature == -100) ? "IN/OUT" : "IN/OUT%01.01f", liveData->celsius2temperature(liveData->params.outdoorTemperature));
  drawBigCell(3, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, TFT_WHITE);
}

/**
   Speed + kwh/100km (Screen 2)
*/
void Board320_240::drawSceneSpeed()
{

  int32_t posx, posy;

  // spr.fillRect(0, 34, 206, 170, TFT_BLACK); // TFT_DARKRED

  posx = 320 / 2;
  posy = 40;
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor(TFT_WHITE);
  spr.setTextSize(2); // Size for small 5cix7 font
  sprintf(tmpStr3, "0");
  if (liveData->params.speedKmhGPS > 10)
  {
    sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.speedKmhGPS));
  }
  else if (liveData->params.speedKmh > 10)
  {
    sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.speedKmh));
  }
  spr.drawString(tmpStr3, 200, posy, 7);

  posy = 140;
  spr.setTextDatum(TR_DATUM); // Top center
  spr.setTextSize(1);
  if (liveData->params.speedKmh > 25 && liveData->params.batPowerKw < 0)
  {
    sprintf(tmpStr3, (liveData->params.batPowerKwh100 == -1000) ? "n/a" : "%01.01f", liveData->km2distance(liveData->params.batPowerKwh100));
    spr.drawString("kWh/100km", 200, posy + 46, 2);
  }
  else
  {
    sprintf(tmpStr3, (liveData->params.batPowerKw == -1000) ? "n/a" : "%01.01f", liveData->params.batPowerKw);
    spr.drawString("kW", 200, posy + 48, 2);
  }
  spr.drawString(tmpStr3, 200, posy, 7);

  // Bottom 2 numbers with charged/discharged kWh from start
  spr.setFreeFont(&Roboto_Thin_24);
  spr.setTextColor(TFT_WHITE);
  posx = 0;
  posy = 0;
  spr.setTextDatum(TL_DATUM);
  sprintf(tmpStr3, (liveData->params.odoKm == -1) ? "n/a km" : ((liveData->settings.distanceUnit == 'k') ? "%01.00fkm" : "%01.00fmi"), liveData->km2distance(liveData->params.odoKm));
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  if (liveData->params.odoKm > 0 && liveData->params.odoKmStart > 0 && liveData->params.speedKmhGPS < 100)
  {
    sprintf(tmpStr3, ((liveData->settings.distanceUnit == 'k') ? "%01.00f" : "%01.00f"), liveData->km2distance(liveData->params.odoKm - liveData->params.odoKmStart));
    spr.drawString(tmpStr3, posx, posy + 20, GFXFF);
  }

  spr.setTextDatum(TR_DATUM);
  if (liveData->params.batteryManagementMode != BAT_MAN_MODE_NOT_IMPLEMENTED)
  {
    sprintf(tmpStr1, "%s %01.00f", liveData->getBatteryManagementModeStr(liveData->params.batteryManagementMode).c_str(),
            liveData->celsius2temperature(liveData->params.coolingWaterTempC));
    spr.drawString(tmpStr1, 319 - posx, posy, GFXFF);
  }
  else if (liveData->params.motorRpm > -1)
  {
    sprintf(tmpStr3, "%01.00frpm", liveData->params.motorRpm);
    spr.drawString(tmpStr3, 319 - posx, posy, GFXFF);
  }

  // AUX voltage
  if (liveData->params.auxVoltage > 5 && liveData->params.speedKmhGPS < 100)
  {
    posy = 80;
    sprintf(tmpStr3, "%01.01f", liveData->params.auxVoltage);
    spr.setTextDatum(TL_DATUM);
    spr.drawString(tmpStr3, posx, posy, GFXFF);
    spr.drawString("aux V", 0, posy + 20, 2);
  }

  // Avg speed
  posy = 120;
  sprintf(tmpStr3, "%01.00f", liveData->params.avgSpeedKmh);
  spr.setTextDatum(TL_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  spr.drawString("avg.km/h", 0, posy + 20, 2);

  // Bottom info
  // Cummulative regen/power
  posy = 240;
  sprintf(tmpStr3, "-%01.01f +%01.01f", liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart,
          liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
  spr.setTextDatum(BL_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  spr.drawString("cons./regen.kWh", 0, 240 - 28, 2);
  posx = 319;
  float kwh100a = 0;
  float kwh100b = 0;
  if (liveData->params.odoKm != -1 && liveData->params.odoKm != -1 && liveData->params.odoKm != liveData->params.odoKmStart)
    kwh100a = ((100 * ((liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart) -
                       (liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart))) /
               (liveData->params.odoKm - liveData->params.odoKmStart));
  if (liveData->params.odoKm != -1 && liveData->params.odoKm != -1 && liveData->params.odoKm != liveData->params.odoKmStart)
    kwh100b = ((100 * ((liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart))) /
               (liveData->params.odoKm - liveData->params.odoKmStart));
  sprintf(tmpStr3, "%01.01f/%01.01f", kwh100a, kwh100b);
  spr.setTextDatum(BR_DATUM);
  spr.drawString("avg.kWh/100km", posx, 240 - 28, 2);
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  // Bat.power
  /*posx = 320 / 2;
  sprintf(tmpStr3, (liveData->params.batPowerKw == -1000) ? "n/a kw" : "%01.01fkw", liveData->params.batPowerKw);
  spr.setTextDatum(BC_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);*/

  // RIGHT INFO
  // Battery "cold gate" detection - red < 15C (43KW limit), <25 (blue - 55kW limit), green all ok
  spr.fillRect(210, 55, 110, 5, (liveData->params.batMaxC >= 15) ? ((liveData->params.batMaxC >= 25) ? ((liveData->params.batMaxC >= 35) ? TFT_YELLOW : TFT_DARKGREEN2) : TFT_BLUE) : TFT_RED);
  spr.fillRect(210, 120, 110, 5, (liveData->params.batMinC >= 15) ? ((liveData->params.batMinC >= 25) ? ((liveData->params.batMinC >= 35) ? TFT_YELLOW : TFT_DARKGREEN2) : TFT_BLUE) : TFT_RED);
  spr.setTextColor(TFT_WHITE);
  spr.setFreeFont(&Roboto_Thin_24);
  spr.setTextDatum(TR_DATUM);
  sprintf(tmpStr3, (liveData->params.batMaxC == -100) ? "-" : "%01.00f", liveData->celsius2temperature(liveData->params.batMaxC));
  spr.drawString(tmpStr3, 319, 66, GFXFF);
  sprintf(tmpStr3, (liveData->params.batMinC == -100) ? "-" : "%01.00f", liveData->celsius2temperature(liveData->params.batMinC));
  spr.drawString(tmpStr3, 319, 92, GFXFF);
  if (liveData->params.motorTempC != -100)
  {
    sprintf(tmpStr3, "i%01.00f / m%01.00f", liveData->celsius2temperature(liveData->params.inverterTempC), liveData->celsius2temperature(liveData->params.motorTempC));
    spr.drawString(tmpStr3, 319, 26, GFXFF);
  }
  // Min.Cell V
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor((liveData->params.batCellMinV > 1.5 && liveData->params.batCellMinV < 3.0) ? TFT_RED : TFT_WHITE);
  sprintf(tmpStr3, (liveData->params.batCellMaxV == -1) ? "n/a V" : "%01.02fV", liveData->params.batCellMaxV);
  spr.drawString(tmpStr3, 280, 66, GFXFF);
  spr.setTextColor((liveData->params.batCellMinV > 1.5 && liveData->params.batCellMinV < 3.0) ? TFT_RED : TFT_WHITE);
  sprintf(tmpStr3, (liveData->params.batCellMinV == -1) ? "n/a V" : "%01.02fV", liveData->params.batCellMinV);
  spr.drawString(tmpStr3, 280, 92, GFXFF);

  // Brake lights
  spr.fillRect(140, 240 - 16, 18, 12, (liveData->params.brakeLights) ? TFT_RED : TFT_BLACK);
  spr.fillRect(180 - 18, 240 - 16, 18, 12, (liveData->params.brakeLights) ? TFT_RED : TFT_BLACK);
  // Lights
  uint16_t tmpWord;
  tmpWord = (liveData->params.headLights) ? TFT_GREEN : (liveData->params.autoLights) ? TFT_YELLOW
                                                    : (liveData->params.dayLights)    ? TFT_BLUE
                                                                                      : TFT_BLACK;
  spr.fillRect(145, 26, 20, 4, tmpWord);
  spr.fillRect(170, 26, 20, 4, tmpWord);

  // Soc%, bat.kWh
  spr.setTextColor((liveData->params.socPerc <= 15) ? TFT_RED : (liveData->params.socPerc > 80 || (liveData->params.socPerc == -1 && liveData->params.socPerc > 80)) ? TFT_YELLOW
                                                                                                                                                                     : TFT_GREEN);
  spr.setTextDatum(BR_DATUM);
  sprintf(tmpStr3, (liveData->params.socPerc == -1) ? "n/a" : "%01.00f", liveData->params.socPerc);
  spr.setFreeFont(&Orbitron_Light_32);
  spr.drawString(tmpStr3, 285, 165, GFXFF);
  spr.setFreeFont(&Orbitron_Light_24);
  spr.drawString("%", 319, 155, GFXFF);
  if (liveData->params.socPerc > 0)
  {
    float capacity = liveData->params.batteryTotalAvailableKWh * (liveData->params.socPerc / 100);
    // calibration for Niro/Kona, real available capacity is ~66.5kWh, 0-10% ~6.2kWh, 90-100% ~7.2kWh
    if (liveData->settings.carType == CAR_KIA_ENIRO_2020_64 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64 || liveData->settings.carType == CAR_KIA_ESOUL_2020_64)
    {
      capacity = (liveData->params.socPerc * 0.615) * (1 + (liveData->params.socPerc * 0.0008));
    }
    spr.setTextColor(TFT_WHITE);
    spr.setFreeFont(&Orbitron_Light_32);
    sprintf(tmpStr3, "%01.00f", capacity);
    spr.drawString(tmpStr3, 285, 200, GFXFF);
    spr.setFreeFont(&Orbitron_Light_24);
    sprintf(tmpStr3, ".%d", int(10 * (capacity - (int)capacity)));
    spr.drawString(tmpStr3, 319, 200, GFXFF);
    spr.setTextColor(TFT_SILVER);
    spr.drawString("kWh", 319, 174, 2);
  }
}

void Board320_240::drawSceneHud()
{

  float batColor;

  // Change rotation to vertical & mirror
  if (tft.getRotation() != 7)
  {
    tft.setRotation(7);
    tft.fillScreen(TFT_BLACK);
  }

  if (liveData->commConnected && firstReload < 3)
  {
    tft.fillScreen(TFT_BLACK);
    firstReload++;
  }

  tft.setTextDatum(TR_DATUM);             // top-right alignment
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // foreground, background text color

  // Draw speed
  tft.setTextSize(3);
  sprintf(tmpStr3, "0");
  if (liveData->params.speedKmhGPS > 10)
  {
    tft.fillRect(0, 210, 320, 30, TFT_BLACK);
    sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.speedKmhGPS));
    lastSpeedKmh = liveData->params.speedKmhGPS;
  }
  else if (liveData->params.speedKmh > 10)
  {
    if (liveData->params.speedKmh != lastSpeedKmh)
    {
      tft.fillRect(0, 210, 320, 30, TFT_BLACK);
      sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.speedKmh));
      lastSpeedKmh = liveData->params.speedKmh;
    }
  }
  else
  {
    sprintf(tmpStr3, "0");
  }
  tft.drawString(tmpStr3, 319, 0, 7);

  // Draw power kWh/100km (>25kmh) else kW
  tft.setTextSize(1);
  if (liveData->params.speedKmh > 25 && liveData->params.batPowerKw < 0)
  {
    sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.batPowerKwh100));
  }
  else
  {
    sprintf(tmpStr3, "%01.01f", liveData->params.batPowerKw);
  }
  tft.fillRect(181, 149, 150, 50, TFT_BLACK);
  tft.drawString(tmpStr3, 320, 150, 7);

  // Draw soc%
  sprintf(tmpStr3, "%01.00f%c", liveData->params.socPerc, '%');
  tft.drawString(tmpStr3, 160, 150, 7);

  // Cold gate battery
  batColor = (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED;
  tft.fillRect(0, 70, 50, 140, batColor);
  tft.fillRect(15, 60, 20, 10, batColor);
  tft.setTextColor(TFT_WHITE, batColor);
  tft.setFreeFont(&Roboto_Thin_24);
  tft.setTextDatum(MC_DATUM);
  sprintf(tmpStr3, "%01.00f", liveData->celsius2temperature(liveData->params.batTempC));
  tft.drawString(tmpStr3, 25, 180, GFXFF);

  // Brake lights
  tft.fillRect(0, 215, 320, 25, (liveData->params.brakeLights) ? TFT_DARKRED : TFT_BLACK);
}

/**
   Battery cells (Screen 3)
*/
void Board320_240::drawSceneBatteryCells()
{

  int32_t posx, posy, lastPosY;

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batHeaterC), char(127));
  drawSmallCell(0, 0, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batInletC), char(127));
  drawSmallCell(1, 0, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);

  lastPosY = 64 - 16;

  if (liveData->params.batModuleTempCount <= 4)
  {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[0]), char(127));
    drawSmallCell(0, 1, 1, 1, tmpStr1, "MO1", TFT_TEMP, (liveData->params.batModuleTempC[0] >= 15) ? ((liveData->params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[1]), char(127));
    drawSmallCell(1, 1, 1, 1, tmpStr1, "MO2", TFT_TEMP, (liveData->params.batModuleTempC[1] >= 15) ? ((liveData->params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[2]), char(127));
    drawSmallCell(2, 1, 1, 1, tmpStr1, "MO3", TFT_TEMP, (liveData->params.batModuleTempC[2] >= 15) ? ((liveData->params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[3]), char(127));
    drawSmallCell(3, 1, 1, 1, tmpStr1, "MO4", TFT_TEMP, (liveData->params.batModuleTempC[3] >= 15) ? ((liveData->params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  }
  else
  {
    // Ioniq (up to 12 cells)
    for (uint16_t i = 0; i < liveData->params.batModuleTempCount; i++)
    {
      if (/*liveData->params.batModuleTempC[i] == 0 && */ liveData->params.batModuleTempC[i] == -100)
        continue;
      posx = (((i - 0) % 8) * 40);
      posy = lastPosY = ((floor((i - 0) / 8)) * 13) + 32;
      // spr.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
      spr.setTextSize(1); // Size for small 5x7 font
      spr.setTextDatum(TL_DATUM);
      spr.setTextColor(((liveData->params.batModuleTempC[i] >= 15) ? ((liveData->params.batModuleTempC[i] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED));
      sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[i]), char(127));
      spr.drawString(tmpStr1, posx + 4, posy, 2);
    }
  }

  spr.setTextDatum(TL_DATUM); // Topleft
  spr.setTextSize(1);         // Size for small 5x7 font

  // Find min and max val
  float minVal = -1, maxVal = -1;
  for (int i = 0; i < liveData->params.cellCount; i++)
  {
    if ((liveData->params.cellVoltage[i] < minVal || minVal == -1) && liveData->params.cellVoltage[i] != -1)
      minVal = liveData->params.cellVoltage[i];
    if ((liveData->params.cellVoltage[i] > maxVal || maxVal == -1) && liveData->params.cellVoltage[i] != -1)
      maxVal = liveData->params.cellVoltage[i];
    /*if (liveData->params.cellVoltage[i] > 0 && i > liveData->params.cellCount + 1)
      liveData->params.cellCount = i + 1;*/
  }

  // Draw cell matrix
  for (int i = 0; i < liveData->params.cellCount; i++)
  {
    if (liveData->params.cellVoltage[i] == -1)
      continue;
    posx = ((i % 8) * 40) + 4;
    posy = ((floor(i / 8)) * 13) + lastPosY + 16; // 68;
    sprintf(tmpStr3, "%01.02f", liveData->params.cellVoltage[i]);
    spr.setTextColor(TFT_SILVER);
    if (liveData->params.cellVoltage[i] == minVal && minVal != maxVal)
      spr.setTextColor(TFT_RED);
    if (liveData->params.cellVoltage[i] == maxVal && minVal != maxVal)
      spr.setTextColor(TFT_GREEN);
    // Battery cell imbalance detetection
    if (liveData->params.cellVoltage[i] > 1.5 && liveData->params.cellVoltage[i] < 3.0)
      spr.setTextColor(TFT_WHITE, TFT_RED);
    spr.drawString(tmpStr3, posx, posy, 2);
  }
}

/**
   drawPreDrawnChargingGraphs
   P = U.I
*/
void Board320_240::drawPreDrawnChargingGraphs(int zeroX, int zeroY, int mulX, float mulY)
{

  // Rapid gate
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 180 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 180 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_RAPIDGATE35);
  // Coldgate <5C
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE0_5);
  // Coldgate 5-14C
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 64 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (64 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 64 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (64 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 82 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 82 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 83 * mulX), zeroY - (/*I*/ 40 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  // Coldgate 15-24C
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  spr.drawLine(zeroX + (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  spr.drawLine(zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 78 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  // Optimal
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 51 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (51 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 51 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (51 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 53 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (53 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 53 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (53 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 55 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (55 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 55 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (55 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 77 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (77 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 71 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (71 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 71 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (71 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 73 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (73 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 73 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (73 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 75 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (75 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 75 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (75 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 77 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (77 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 78 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 78 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 82 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 82 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 83 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 83 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 92 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (92 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 92 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (92 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 95 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (95 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 95 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (95 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 98 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (98 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 98 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (98 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 100 * mulX), zeroY - (/*I*/ 15 * /*U SOC*/ (100 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  // Triangles
  int x = zeroX;
  int y;
  if (liveData->params.batMaxC >= 35)
  {
    y = zeroY - (/*I*/ 180 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  }
  else if (liveData->params.batMinC >= 25)
  {
    y = zeroY - (/*I*/ 200 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  }
  else if (liveData->params.batMinC >= 15)
  {
    y = zeroY - (/*I*/ 150 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  }
  else if (liveData->params.batMinC >= 5)
  {
    y = zeroY - (/*I*/ 110 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  }
  else
  {
    y = zeroY - (/*I*/ 60 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  }
  spr.fillTriangle(x + 5, y, x, y - 5, x, y + 5, TFT_ORANGE);
}

/**
   Charging graph (Screen 4)
*/
void Board320_240::drawSceneChargingGraph()
{

  int zeroX = 0;
  int zeroY = 238;
  int mulX = 3;   // 100% = 300px;
  float mulY = 2; // 100kW = 200px
  int maxKw = 75;
  int posy = 0;
  uint16_t color;

  // Autoscale Y-axis
  for (int i = 0; i <= 100; i++)
    if (liveData->params.chargingGraphMaxKw[i] > maxKw)
      maxKw = liveData->params.chargingGraphMaxKw[i];
  maxKw = (ceil(maxKw / 10) + 1) * 10;
  mulY = 160.0 / maxKw;

  spr.fillSprite(TFT_BLACK);

  sprintf(tmpStr1, "%01.00f", liveData->params.socPerc);
  drawSmallCell(0, 0, 1, 1, tmpStr1, "SOC", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", liveData->params.batPowerKw);
  drawSmallCell(1, 0, 1, 1, tmpStr1, "POWER kW", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", liveData->params.batPowerAmp);
  drawSmallCell(2, 0, 1, 1, tmpStr1, "CURRENT A", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%03.00f", liveData->params.batVoltage);
  drawSmallCell(3, 0, 1, 1, tmpStr1, "VOLTAGE", TFT_TEMP, TFT_CYAN);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batHeaterC), char(127));
  drawSmallCell(0, 1, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_RED);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batInletC), char(127));
  drawSmallCell(1, 1, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batMinC), char(127));
  drawSmallCell(2, 1, 1, 1, tmpStr1, "BAT.MIN", (liveData->params.batMinC >= 15) ? ((liveData->params.batMinC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.outdoorTemperature), char(127));
  drawSmallCell(3, 1, 1, 1, tmpStr1, "OUT.TEMP.", TFT_TEMP, TFT_CYAN);

  spr.setTextColor(TFT_SILVER);

  for (int i = 0; i <= 10; i++)
  {
    color = TFT_DARKRED2;
    if (i == 0 || i == 5 || i == 10)
      color = TFT_DARKRED;
    spr.drawFastVLine(zeroX + (i * 10 * mulX), zeroY - (maxKw * mulY), maxKw * mulY, color);
    /*if (i != 0 && i != 10) {
      sprintf(tmpStr1, "%d%%", i * 10);
      spr.setTextDatum(BC_DATUM);
      spr.drawString(tmpStr1, zeroX + (i * 10 * mulX),  zeroY - (maxKw * mulY), 2);
      }*/
    if (i <= (maxKw / 10))
    {
      spr.drawFastHLine(zeroX, zeroY - (i * 10 * mulY), 100 * mulX, color);
      if (i > 0)
      {
        sprintf(tmpStr1, "%d", i * 10);
        spr.setTextDatum(ML_DATUM);
        spr.drawString(tmpStr1, zeroX + (100 * mulX) + 3, zeroY - (i * 10 * mulY), 2);
      }
    }
  }

  // Draw suggested curves
  if (liveData->settings.predrawnChargingGraphs == 1)
  {
    drawPreDrawnChargingGraphs(zeroX, zeroY, mulX, mulY);
  }

  // Draw realtime values
  for (int i = 0; i <= 100; i++)
  {
    if (liveData->params.chargingGraphBatMinTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphBatMinTempC[i] * mulY), mulX, TFT_BLUE);
    if (liveData->params.chargingGraphBatMaxTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphBatMaxTempC[i] * mulY), mulX, TFT_BLUE);
    if (liveData->params.chargingGraphWaterCoolantTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphWaterCoolantTempC[i] * mulY), mulX, TFT_PURPLE);
    if (liveData->params.chargingGraphHeaterTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphHeaterTempC[i] * mulY), mulX, TFT_RED);

    if (liveData->params.chargingGraphMinKw[i] > 0)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphMinKw[i] * mulY), mulX, TFT_GREENYELLOW);
    if (liveData->params.chargingGraphMaxKw[i] > 0)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphMaxKw[i] * mulY), mulX, TFT_YELLOW);
  }

  // Bat.module temperatures
  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextDatum(BL_DATUM);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "1=%01.00f%cC" : "1=%01.00f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[0]), char(127));
  spr.setTextColor((liveData->params.batModuleTempC[0] >= 15) ? ((liveData->params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  spr.drawString(tmpStr1, 0, zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "2=%01.00f%cC" : "2=%01.00f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[1]), char(127));
  spr.setTextColor((liveData->params.batModuleTempC[1] >= 15) ? ((liveData->params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  spr.drawString(tmpStr1, 48, zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "3=%01.00f%cC" : "3=%01.00f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[2]), char(127));
  spr.setTextColor((liveData->params.batModuleTempC[2] >= 15) ? ((liveData->params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  spr.drawString(tmpStr1, 96, zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "4=%01.00f%cC" : "4=%01.00f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[3]), char(127));
  spr.setTextColor((liveData->params.batModuleTempC[3] >= 15) ? ((liveData->params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  spr.drawString(tmpStr1, 144, zeroY - (maxKw * mulY), 2);
  sprintf(tmpStr1, "ir %01.00fkOhm", liveData->params.isolationResistanceKOhm);

  // Bms max.regen/power available
  spr.setTextColor(TFT_WHITE);
  sprintf(tmpStr1, "xC=%01.00fkW ", liveData->params.availableChargePower);
  spr.drawString(tmpStr1, 192, zeroY - (maxKw * mulY), 2);
  spr.setTextColor(TFT_WHITE);
  sprintf(tmpStr1, "xD=%01.00fkW", liveData->params.availableDischargePower);
  spr.drawString(tmpStr1, 256, zeroY - (maxKw * mulY), 2);

  //
  spr.setTextDatum(TR_DATUM);
  if (liveData->params.coolingWaterTempC != -100)
  {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%s / W=%01.00f%cC" : "%s /W=%01.00f%cF"),
            liveData->getBatteryManagementModeStr(liveData->params.batteryManagementMode).c_str(),
            liveData->celsius2temperature(liveData->params.coolingWaterTempC),
            char(127));
    spr.setTextColor(TFT_PINK);
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX), zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  spr.setTextColor(TFT_WHITE);
  if (liveData->params.batFanFeedbackHz > 0)
  {
    sprintf(tmpStr1, "FF=%03.00fHz", liveData->params.batFanFeedbackHz);
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX), zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.batFanStatus > 0)
  {
    sprintf(tmpStr1, "FS=%03.00f", liveData->params.batFanStatus);
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX), zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.coolantTemp1C != -100 && liveData->params.coolantTemp2C != -100)
  {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "C1/2:%01.00f/%01.00f%cC" : "C1/2:%01.00f/%01.00f%cF"), liveData->celsius2temperature(liveData->params.coolantTemp1C), liveData->celsius2temperature(liveData->params.coolantTemp2C), char(127));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX), zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempA != -100)
  {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "A=%01.00f%cC" : "W=%01.00f%cF"), liveData->celsius2temperature(liveData->params.bmsUnknownTempA), char(127));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX), zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempB != -100)
  {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "B=%01.00f%cC" : "W=%01.00f%cF"), liveData->celsius2temperature(liveData->params.bmsUnknownTempB), char(127));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX), zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempC != -100)
  {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "C=%01.00f%cC" : "W=%01.00f%cF"), liveData->celsius2temperature(liveData->params.bmsUnknownTempC), char(127));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX), zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempD != -100)
  {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "D=%01.00f%cC" : "W=%01.00f%cF"), liveData->celsius2temperature(liveData->params.bmsUnknownTempD), char(127));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX), zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.chargerACconnected || liveData->params.chargerDCconnected)
  {
    sprintf(tmpStr1, ((liveData->params.chargerACconnected) ? "AC=%d" : "DC=%d"), ((liveData->params.chargingOn) ? 1 : 0));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX), zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }

  // Print charging time
  time_t diffTime = liveData->params.currentTime - liveData->params.chargingStartTime;
  if ((diffTime / 60) > 99)
    sprintf(tmpStr1, "%02ld:%02ld:%02ld", (diffTime / 3600) % 24, (diffTime / 60) % 60, diffTime % 60);
  else
    sprintf(tmpStr1, "%02ld:%02ld", (diffTime / 60), diffTime % 60);
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_SILVER);
  spr.drawString(tmpStr1, 0, zeroY - (maxKw * mulY), 2);
}

/**
  SOC 10% table (screen 5)
*/
void Board320_240::drawSceneSoc10Table()
{

  int zeroY = 2;
  float diffCec, diffCed, diffOdo = -1;
  float firstCed = -1, lastCed = -1, diffCed0to5 = 0;
  float firstCec = -1, lastCec = -1, diffCec0to5 = 0;
  float firstOdo = -1, lastOdo = -1, diffOdo0to5 = 0;
  float diffTime;

  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextColor(TFT_SILVER);
  spr.setTextDatum(TL_DATUM);
  spr.drawString("CONSUMPTION | DISCH.100%->4% SOC", 2, zeroY, 2);

  spr.setTextDatum(TR_DATUM);

  spr.drawString("dis./char.kWh", 128, zeroY + (1 * 15), 2);
  spr.drawString(((liveData->settings.distanceUnit == 'k') ? "km" : "mi"), 160, zeroY + (1 * 15), 2);
  spr.drawString("kWh100", 224, zeroY + (1 * 15), 2);
  spr.drawString("avg.speed", 310, zeroY + (1 * 15), 2);

  for (int i = 0; i <= 10; i++)
  {
    sprintf(tmpStr1, "%d%%", (i == 0) ? 5 : i * 10);
    spr.drawString(tmpStr1, 32, zeroY + ((12 - i) * 15), 2);

    firstCed = (liveData->params.soc10ced[i] != -1) ? liveData->params.soc10ced[i] : firstCed;
    lastCed = (lastCed == -1 && liveData->params.soc10ced[i] != -1) ? liveData->params.soc10ced[i] : lastCed;
    firstCec = (liveData->params.soc10cec[i] != -1) ? liveData->params.soc10cec[i] : firstCec;
    lastCec = (lastCec == -1 && liveData->params.soc10cec[i] != -1) ? liveData->params.soc10cec[i] : lastCec;
    firstOdo = (liveData->params.soc10odo[i] != -1) ? liveData->params.soc10odo[i] : firstOdo;
    lastOdo = (lastOdo == -1 && liveData->params.soc10odo[i] != -1) ? liveData->params.soc10odo[i] : lastOdo;

    if (i != 10)
    {
      diffCec = (liveData->params.soc10cec[i + 1] != -1 && liveData->params.soc10cec[i] != -1) ? (liveData->params.soc10cec[i] - liveData->params.soc10cec[i + 1]) : 0;
      diffCed = (liveData->params.soc10ced[i + 1] != -1 && liveData->params.soc10ced[i] != -1) ? (liveData->params.soc10ced[i + 1] - liveData->params.soc10ced[i]) : 0;
      diffOdo = (liveData->params.soc10odo[i + 1] != -1 && liveData->params.soc10odo[i] != -1) ? (liveData->params.soc10odo[i] - liveData->params.soc10odo[i + 1]) : -1;
      diffTime = (liveData->params.soc10time[i + 1] != -1 && liveData->params.soc10time[i] != -1) ? (liveData->params.soc10time[i] - liveData->params.soc10time[i + 1]) : -1;
      if (diffCec != 0)
      {
        sprintf(tmpStr1, "+%01.01f", diffCec);
        spr.drawString(tmpStr1, 128, zeroY + ((12 - i) * 15), 2);
        diffCec0to5 = (i == 0) ? diffCec : diffCec0to5;
      }
      if (diffCed != 0)
      {
        sprintf(tmpStr1, "%01.01f", diffCed);
        spr.drawString(tmpStr1, 80, zeroY + ((12 - i) * 15), 2);
        diffCed0to5 = (i == 0) ? diffCed : diffCed0to5;
      }
      if (diffOdo != -1)
      {
        sprintf(tmpStr1, "%01.00f", liveData->km2distance(diffOdo));
        spr.drawString(tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
        diffOdo0to5 = (i == 0) ? diffOdo : diffOdo0to5;
        if (diffTime > 0)
        {
          sprintf(tmpStr1, "%01.01f", liveData->km2distance(diffOdo) / (diffTime / 3600));
          spr.drawString(tmpStr1, 310, zeroY + ((12 - i) * 15), 2);
        }
      }
      if (diffOdo > 0 && diffCed != 0)
      {
        sprintf(tmpStr1, "%01.1f", (-diffCed * 100.0 / liveData->km2distance(diffOdo)));
        spr.drawString(tmpStr1, 224, zeroY + ((12 - i) * 15), 2);
      }
    }

    if (diffOdo == -1 && liveData->params.soc10odo[i] != -1)
    {
      sprintf(tmpStr1, "%01.00f", liveData->km2distance(liveData->params.soc10odo[i]));
      spr.drawString(tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
    }
  }

  spr.drawString("0%", 32, zeroY + (13 * 15), 2);
  spr.drawString("0-5% is calculated (same) as 5-10%", 310, zeroY + (13 * 15), 2);

  spr.drawString("TOT.", 32, zeroY + (14 * 15), 2);
  diffCed = (lastCed != -1 && firstCed != -1) ? firstCed - lastCed + diffCed0to5 : 0;
  sprintf(tmpStr1, "%01.01f", diffCed);
  spr.drawString(tmpStr1, 80, zeroY + (14 * 15), 2);
  diffCec = (lastCec != -1 && firstCec != -1) ? lastCec - firstCec + diffCec0to5 : 0;
  sprintf(tmpStr1, "+%01.01f", diffCec);
  spr.drawString(tmpStr1, 128, zeroY + (14 * 15), 2);
  diffOdo = (lastOdo != -1 && firstOdo != -1) ? lastOdo - firstOdo + diffOdo0to5 : 0;
  sprintf(tmpStr1, "%01.00f", liveData->km2distance(diffOdo));
  spr.drawString(tmpStr1, 160, zeroY + (14 * 15), 2);
  sprintf(tmpStr1, "AVAIL.CAP: %01.01f kWh", -diffCed - diffCec);
  spr.drawString(tmpStr1, 310, zeroY + (14 * 15), 2);
}

/**
   Modify caption
*/
String Board320_240::menuItemCaption(int16_t menuItemId, String title)
{

  String prefix = "", suffix = "";
  uint8_t perc = 0;

  switch (menuItemId)
  {
  // Set vehicle type
  case MENU_VEHICLE_TYPE:
    sprintf(tmpStr1, "[%d]", liveData->settings.carType);
    suffix = tmpStr1;
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
  case VEHICLE_TYPE_KONA_2020_64:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64) ? ">" : "";
    break;
  case VEHICLE_TYPE_KONA_2020_39:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_KONA_2020_39) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ5_58:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_58) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ5_72:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_72) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ5_77:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_77) ? ">" : "";
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
  case VEHICLE_TYPE_KIA_EV6_58:
    prefix = (liveData->settings.carType == CAR_KIA_EV6_58) ? ">" : "";
    break;
  case VEHICLE_TYPE_KIA_EV6_77:
    prefix = (liveData->settings.carType == CAR_KIA_EV6_77) ? ">" : "";
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
  case VEHICLE_TYPE_DEBUG_OBD_KIA:
    prefix = (liveData->settings.carType == CAR_DEBUG_OBD2_KIA) ? ">" : "";
    break;
  //
  case MENU_ADAPTER_BLE4:
    prefix = (liveData->settings.commType == COMM_TYPE_OBD2BLE4) ? ">" : "";
    break;
  case MENU_ADAPTER_CAN:
    prefix = (liveData->settings.commType == COMM_TYPE_OBD2CAN) ? ">" : "";
    break;
  case MENU_ADAPTER_BT3:
    prefix = (liveData->settings.commType == COMM_TYPE_OBD2BT3) ? ">" : "";
    break;
  case MENU_ADAPTER_THREADING:
    suffix = (liveData->settings.threading == 0) ? "[off]" : "[on]";
    break;
  /*case MENU_WIFI:
    suffix = "n/a";
    switch (WiFi.status())
    {
    WL_CONNECTED:
      suffix = "CONNECTED";
      break;
    WL_NO_SHIELD:
      suffix = "NO_SHIELD";
      break;
    WL_IDLE_STATUS:
      suffix = "IDLE_STATUS";
      break;
    WL_SCAN_COMPLETED:
      suffix = "SCAN_COMPLETED";
      break;
    WL_CONNECT_FAILED:
      suffix = "CONNECT_FAILED";
      break;
    WL_CONNECTION_LOST:
      suffix = "CONNECTION_LOST";
      break;
    WL_DISCONNECTED:
      suffix = "DISCONNECTED";
      break;
    }
    break;*/
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
    case REMOTE_UPLOAD_SIM800L:
      suffix = "[SIM800L]";
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
  case MENU_PREDRAWN_GRAPHS:
    suffix = (liveData->settings.predrawnChargingGraphs == 1) ? "[on]" : "[off]";
    break;
  case MENU_HEADLIGHTS_REMINDER:
    suffix = (liveData->settings.headlightsReminder == 1) ? "[on]" : "[off]";
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
    case SLEEP_MODE_DEEP_SLEEP:
      suffix = "[deep sleep]";
      break;
    case SLEEP_MODE_SHUTDOWN:
      suffix = "[shutdown]";
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
    case SLEEP_MODE_DEEP_SLEEP:
      suffix = "[deep sleep]";
      break;
    case SLEEP_MODE_SHUTDOWN:
      suffix = "[shutdown]";
      break;
    default:
      suffix = "[unknown]";
    }
    break;
  case MENU_SLEEP_MODE_WAKEINTERVAL:
    sprintf(tmpStr1, "[%d sec]", liveData->settings.sleepModeIntervalSec);
    suffix = tmpStr1;
    break;
  case MENU_SLEEP_MODE_SHUTDOWNHRS:
    sprintf(tmpStr1, "[%d hrs]", liveData->settings.sleepModeShutdownHrs);
    suffix = tmpStr1;
    break;
  case MENU_GPS:
    sprintf(tmpStr1, "[HW UART=%d]", liveData->settings.gpsHwSerialPort);
    suffix = (liveData->settings.gpsHwSerialPort == 255) ? "[off]" : tmpStr1;
    break;
  case MENU_CURRENT_TIME:
    struct tm now;
    getLocalTime(&now);
    sprintf(tmpStr1, "%02d-%02d-%02d %02d:%02d:%02d", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
    suffix = tmpStr1;
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
  case MENU_WIFI_SSID2:
    sprintf(tmpStr1, "%s", liveData->settings.wifiSsidb);
    suffix = tmpStr1;
    break;
  case MENU_WIFI_PASSWORD2:
    sprintf(tmpStr1, "%s", liveData->settings.wifiPasswordb);
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
  Display menu
*/
void Board320_240::showMenu()
{

  uint16_t posY = 0, tmpCurrMenuItem = 0;
  int8_t idx, off = 2;

  liveData->menuVisible = true;
  spr.fillSprite(TFT_BLACK);
  spr.setTextDatum(TL_DATUM);
  spr.setFreeFont(&Roboto_Thin_24);

  // dynamic car menu
  std::vector<String> customMenu;
  customMenu = carInterface->customMenu(liveData->menuCurrent);

  // Page scroll
  menuItemHeight = spr.fontHeight();
#ifdef BOARD_M5STACK_CORE2
  off = menuItemHeight / 4;
  menuItemHeight = menuItemHeight * 1.5;
#endif
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
        spr.drawString(menuItemCaption(liveData->menuItems[i].id, liveData->menuItems[i].title), 0, posY + off, GFXFF);
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
      spr.drawString(customMenu.at(i).substring(idx + 1), 0, posY + off, GFXFF);
      posY += menuItemHeight;
    }
    tmpCurrMenuItem++;
  }

  spr.pushSprite(0, 0);
}

/**
   Hide menu
*/
void Board320_240::hideMenu()
{
  liveData->menuVisible = false;
  liveData->menuCurrent = 0;
  liveData->menuItemSelected = 0;
  redrawScreen();
}

/**
  Move in menu with left/right button
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
   Enter menu item
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
    syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
    // Device list
    if (tmpMenuItem->id > LIST_OF_BLE_DEV_TOP && tmpMenuItem->id < MENU_LAST)
    {
      strlcpy((char *)liveData->settings.obdMacAddress, (char *)tmpMenuItem->obdMacAddress, 20);
      syslog->print("Selected adapter MAC address ");
      syslog->println(liveData->settings.obdMacAddress);
      saveSettings();
      ESP.restart();
    }
    // Other menus
    switch (tmpMenuItem->id)
    {
    // Set vehicle type
    case VEHICLE_TYPE_IONIQ_2018_28:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ_2018;
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
    case VEHICLE_TYPE_HYUNDAI_IONIQ5_58:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ5_58;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_HYUNDAI_IONIQ5_72:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ5_72;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_HYUNDAI_IONIQ5_77:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ5_77;
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
    case VEHICLE_TYPE_KIA_EV6_58:
      liveData->settings.carType = CAR_KIA_EV6_58;
      showMenu();
      return;
      break;
    case VEHICLE_TYPE_KIA_EV6_77:
      liveData->settings.carType = CAR_KIA_EV6_77;
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
    case VEHICLE_TYPE_DEBUG_OBD_KIA:
      liveData->settings.carType = CAR_DEBUG_OBD2_KIA;
      showMenu();
      return;
      break;
    // Comm type
    case MENU_ADAPTER_BLE4:
      liveData->settings.commType = COMM_TYPE_OBD2BLE4;
      saveSettings();
      ESP.restart();
      break;
    case MENU_ADAPTER_CAN:
      liveData->settings.commType = COMM_TYPE_OBD2CAN;
      saveSettings();
      ESP.restart();
      break;
    case MENU_ADAPTER_BT3:
      liveData->settings.commType = COMM_TYPE_OBD2BT3;
      saveSettings();
      ESP.restart();
      break;
    case MENU_ADAPTER_THREADING:
      liveData->settings.threading = (liveData->settings.threading == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_ADAPTER_LOAD_TEST_DATA:
      loadTestData();
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
      liveData->settings.defaultScreen = 7;
      showParentMenu = true;
      break;
    // SleepMode off/on
    case MENU_SLEEP_MODE_MODE:
      liveData->settings.sleepModeLevel = (liveData->settings.sleepModeLevel == SLEEP_MODE_SHUTDOWN) ? SLEEP_MODE_OFF : liveData->settings.sleepModeLevel + 1;
      showMenu();
      return;
      break;
    case MENU_SLEEP_MODE_WAKEINTERVAL:
      liveData->settings.sleepModeIntervalSec = (liveData->settings.sleepModeIntervalSec == 600) ? 30 : liveData->settings.sleepModeIntervalSec + 30;
      showMenu();
      return;
      break;
    case MENU_SLEEP_MODE_SHUTDOWNHRS:
      liveData->settings.sleepModeShutdownHrs = (liveData->settings.sleepModeShutdownHrs == 168) ? 0 : liveData->settings.sleepModeShutdownHrs + 12;
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
    // Pre-drawn charg.graphs off/on
    case MENU_PREDRAWN_GRAPHS:
      liveData->settings.predrawnChargingGraphs = (liveData->settings.predrawnChargingGraphs == 1) ? 0 : 1;
      showMenu();
      return;
      break;
    case MENU_HEADLIGHTS_REMINDER:
      liveData->settings.headlightsReminder = (liveData->settings.headlightsReminder == 1) ? 0 : 1;
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
      liveData->settings.remoteUploadModuleType = (liveData->settings.remoteUploadModuleType == 1) ? REMOTE_UPLOAD_SIM800L : liveData->settings.remoteUploadModuleType + 1;
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
    case MENU_GPS:
      liveData->settings.gpsHwSerialPort = (liveData->settings.gpsHwSerialPort == 2) ? 255 : liveData->settings.gpsHwSerialPort + 1;
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
      if (liveData->settings.commType == COMM_TYPE_OBD2CAN)
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
      /*  commInterface->executeCommand("ATSH770");
          delay(50);
          commInterface->executeCommand("3E");
          delay(50);
          commInterface->executeCommand("1003");
          delay(50);
          commInterface->executeCommand("2FBC1003");
          delay(5000);
          commInterface->executeCommand("ATSH770");
          delay(50);
          commInterface->executeCommand("3E");
          delay(50);
          commInterface->executeCommand("1003");
          delay(50);
          commInterface->executeCommand("2FBC1103");
          delay(5000);*/
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

/**
  Redraw screen
*/
void Board320_240::redrawScreen()
{
  lastRedrawTime = liveData->params.currentTime;
  liveData->redrawScreenRequested = false;

  if (liveData->menuVisible || currentBrightness == 0 || !liveData->params.spriteInit)
  {
    return;
  }
  if (redrawScreenIsRunning)
  {
    return;
  }
  redrawScreenIsRunning = true;

  // Headlights reminders
  if (!testDataMode && liveData->settings.headlightsReminder == 1 && liveData->params.forwardDriveMode &&
      !liveData->params.headLights && !liveData->params.autoLights)
  {
    spr.fillSprite(TFT_RED);
    spr.setFreeFont(&Orbitron_Light_32);
    spr.setTextColor(TFT_WHITE);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("! LIGHTS OFF !", 160, 120, GFXFF);
    spr.pushSprite(0, 0);
    redrawScreenIsRunning = false;
    return;
  }

  spr.fillSprite(TFT_BLACK);

  liveData->params.displayScreenAutoMode = SCREEN_AUTO;

  // Display selected screen
  switch (liveData->params.displayScreen)
  {
  // 1. Auto mode = >5kpm Screen 3 - speed, other wise basic Screen2 - Main screen, if charging then Screen 5 Graph
  case SCREEN_AUTO:
    if (liveData->params.batCellMinV > 1.5 && liveData->params.batCellMinV < 3.0)
    {
      if (liveData->params.displayScreenAutoMode != SCREEN_CELLS)
        liveData->params.displayScreenAutoMode = SCREEN_CELLS;
      drawSceneBatteryCells();
    }
    else if (liveData->params.speedKmh > 15)
    {
      if (liveData->params.displayScreenAutoMode != SCREEN_SPEED)
        liveData->params.displayScreenAutoMode = SCREEN_SPEED;
      drawSceneSpeed();
    }
    else if (liveData->params.chargingOn)
    {
      if (liveData->params.displayScreenAutoMode != SCREEN_CHARGING)
        liveData->params.displayScreenAutoMode = SCREEN_CHARGING;
      drawSceneChargingGraph();
    }
    else
    {
      if (liveData->params.displayScreenAutoMode != SCREEN_DASH)
        liveData->params.displayScreenAutoMode = SCREEN_DASH;
      drawSceneMain();
    }
    break;
  // 2. Main screen
  case SCREEN_DASH:
    drawSceneMain();
    break;
  // 3. Big speed + kwh/100km
  case SCREEN_SPEED:
    drawSceneSpeed();
    break;
  // 4. Battery cells
  case SCREEN_CELLS:
    drawSceneBatteryCells();
    break;
  // 5. Charging graph
  case SCREEN_CHARGING:
    drawSceneChargingGraph();
    break;
  // 6. SOC10% table (CEC-CED)
  case SCREEN_SOC10:
    drawSceneSoc10Table();
    break;
  // 7. HUD
  case SCREEN_HUD:
    drawSceneHud();
    break;
  }

  // Skip following lines for HUD display mode
  if (liveData->params.displayScreen == SCREEN_HUD)
  {
    redrawScreenIsRunning = false;
    return;
  }

  // GPS state
  if ((gpsHwUart != NULL || liveData->params.gpsSat > 0) && (liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED))
  {
    spr.fillCircle(160, 10, 8, (liveData->params.gpsValid) ? TFT_GREEN : TFT_RED);
    spr.fillCircle(160, 10, 6, TFT_BLACK);
    spr.setTextSize(1);
    spr.setTextColor((liveData->params.gpsValid) ? TFT_GREEN : TFT_WHITE);
    spr.setTextDatum(TL_DATUM);
    sprintf(tmpStr1, "%d", liveData->params.gpsSat);
    spr.drawString(tmpStr1, 174, 2, 2);
  }

  // SDCARD recording
  /*liveData->params.sdcardRecording*/
  if (liveData->settings.sdcardEnabled == 1 && (liveData->params.queueLoopCounter & 1) == 1)
  {
    spr.fillCircle((liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED) ? 160 : 310, 10, 4, TFT_BLACK);
    spr.fillCircle((liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED) ? 160 : 310, 10, 3,
                   (liveData->params.sdcardInit == 1) ? (liveData->params.sdcardRecording) ? (strlen(liveData->params.sdcardFilename) != 0) ? TFT_GREEN /* assigned filename (opsec from bms or gsm/gps timestamp */ : TFT_BLUE /* recording started but waiting for data */ : TFT_ORANGE /* sdcard init ready but recording not started*/ : TFT_YELLOW /* failed to initialize sdcard */
    );
  }

  int tmp_send_interval = 0;

  if (liveData->settings.remoteUploadIntervalSec > 0)
  {
    tmp_send_interval = liveData->settings.remoteUploadIntervalSec;
  }
  else if (liveData->settings.remoteUploadAbrpIntervalSec > 0)
  {
    tmp_send_interval = liveData->settings.remoteUploadAbrpIntervalSec;
  }

  // SIM800L status
  if (liveData->params.sim800l_enabled && liveData->settings.remoteUploadModuleType == 0)
  {
    if (liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED)
    {
      spr.fillRect(140, 7, 7, 7,
                   (sim800l->isConnectedGPRS()) ? (liveData->params.sim800l_lastOkSendTime + tmp_send_interval >= liveData->params.currentTime) ? TFT_GREEN /* last request was 200 OK */ : TFT_YELLOW /* data sent but response timed out */ : TFT_RED /* failed to send data */
      );
    }
    else if (liveData->params.displayScreen != SCREEN_BLANK)
    {
      spr.fillRect(308, 0, 5, 5,
                   (sim800l->isConnectedGPRS()) ? (liveData->params.sim800l_lastOkSendTime + tmp_send_interval >= liveData->params.currentTime) ? TFT_GREEN /* last request was 200 OK */ : TFT_YELLOW /* data sent but response timed out */ : TFT_RED /* failed to send data */
      );
    }
  }

  // WiFi Status
  if (liveData->settings.wifiEnabled == 1 && liveData->settings.remoteUploadModuleType == 1)
  {
    if (liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED)
    {
      spr.fillRect(140, 7, 7, 7,
                   (WiFi.status() == WL_CONNECTED) ? (liveData->params.sim800l_lastOkSendTime + tmp_send_interval >= liveData->params.currentTime) ? TFT_GREEN /* last request was 200 OK */ : TFT_YELLOW /* wifi connected but not send */ : TFT_RED /* wifi not connected */
      );
    }
    else if (liveData->params.displayScreen != SCREEN_BLANK)
    {
      spr.fillRect(308, 0, 5, 5,
                   (WiFi.status() == WL_CONNECTED) ? (liveData->params.sim800l_lastOkSendTime + tmp_send_interval >= liveData->params.currentTime) ? TFT_GREEN /* last request was 200 OK */ : TFT_YELLOW /* wifi connected but not send */ : TFT_RED /* wifi not connected */
      );
    }
  }

  // Door status
  if (liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED)
  {
    if (liveData->params.trunkDoorOpen || liveData->params.leftFrontDoorOpen || liveData->params.rightFrontDoorOpen || liveData->params.leftRearDoorOpen || liveData->params.rightRearDoorOpen || liveData->params.hoodDoorOpen)
    {
      spr.fillRect(40, 40, 50, 90, 0x4208);
      if (liveData->params.trunkDoorOpen)
        spr.fillRect(45, 36, 40, 20, TFT_GOLD);
      if (liveData->params.leftFrontDoorOpen)
        spr.fillRect(20, 60, 20, 4, TFT_GOLD);
      if (liveData->params.leftRearDoorOpen)
        spr.fillRect(90, 60, 20, 4, TFT_GOLD);
      if (liveData->params.rightFrontDoorOpen)
        spr.fillRect(20, 90, 20, 4, TFT_GOLD);
      if (liveData->params.rightRearDoorOpen)
        spr.fillRect(90, 90, 20, 4, TFT_GOLD);
      if (liveData->params.hoodDoorOpen)
        spr.fillRect(45, 120, 40, 15, TFT_GOLD);
    }
  }
  else
  {
    if (liveData->params.trunkDoorOpen)
      spr.fillRect(20, 0, 320 - 40, 4, TFT_GOLD);
    if (liveData->params.leftFrontDoorOpen)
      spr.fillRect(0, 20, 4, 98, TFT_GOLD);
    if (liveData->params.rightFrontDoorOpen)
      spr.fillRect(0, 122, 4, 98, TFT_GOLD);
    if (liveData->params.leftRearDoorOpen)
      spr.fillRect(320 - 4, 20, 4, 98, TFT_GOLD);
    if (liveData->params.rightRearDoorOpen)
      spr.fillRect(320 - 4, 122, 4, 98, TFT_GOLD);
    if (liveData->params.hoodDoorOpen)
      spr.fillRect(20, 240 - 4, 320 - 40, 4, TFT_GOLD);
  }

  // BLE not connected
  if (!liveData->commConnected && liveData->bleConnect && liveData->tmpSettings.commType == COMM_TYPE_OBD2BLE4)
  {
    // Print message
    spr.setTextSize(1);
    spr.setTextColor(TFT_WHITE);
    spr.setTextDatum(TL_DATUM);
    spr.drawString("BLE4 OBDII not connected...", 0, 180, 2);
    spr.drawString("Press middle button to menu.", 0, 200, 2);
    spr.drawString(APP_VERSION, 0, 220, 2);
  }

  spr.pushSprite(0, 0);
  redrawScreenIsRunning = false;
}

/**
  Parse test data
*/
void Board320_240::loadTestData()
{

  syslog->println("Loading test data");

  testDataMode = true; // skip lights off message
  carInterface->loadTestData();
  redrawScreen();
}

/**
 * void Board320_240::xTaskCommLoop(void * pvParameters) {
 */
void Board320_240::xTaskCommLoop(void *pvParameters)
{
  // LiveData * liveData = (LiveData *) pvParameters;
  BoardInterface *boardObj = (BoardInterface *)pvParameters;
  while (1)
  {
    boardObj->commLoop();
    delay(10);
  }
}

/**
 * Board loop - secondary thread
 */
void Board320_240::commLoop()
{
  // Read data from BLE/CAN
  commInterface->mainLoop();
}

/**
 * Board loop
 *
 */
void Board320_240::boardLoop()
{
  // touch events, m5.update
}

/**
  Main loop - primary thread
  */
void Board320_240::mainLoop()
{
  boardLoop();

  ///////////////////////////////////////////////////////////////////////
  // Handle buttons
  // MIDDLE - menu select
  if (!isButtonPressed(pinButtonMiddle))
  {
    btnMiddlePressed = false;
  }
  else
  {
    if (!btnMiddlePressed)
    {
      btnMiddlePressed = true;
      liveData->params.lastButtonPushedTime = liveData->params.currentTime;
      tft.setRotation(liveData->settings.displayRotation);
      if (liveData->menuVisible)
      {
        menuItemClick();
      }
      else
      {
        showMenu();
      }
    }
  }
  // LEFT - screen rotate, menu
  if (!isButtonPressed(pinButtonLeft))
  {
    btnLeftPressed = false;
  }
  else
  {
    if (!btnLeftPressed)
    {
      btnLeftPressed = true;
      liveData->params.lastButtonPushedTime = liveData->params.currentTime;
      tft.setRotation(liveData->settings.displayRotation);
      // Menu handling
      if (liveData->menuVisible)
      {
        menuMove(false);
      }
      else
      {
        liveData->params.displayScreen++;
        if (liveData->params.displayScreen > displayScreenCount - 1)
          liveData->params.displayScreen = 0; // rotate screens
        // Turn off display on screen 0
        setBrightness();
        redrawScreen();
      }
    }
  }
  // RIGHT - menu, debug screen rotation
  if (!isButtonPressed(pinButtonRight))
  {
    btnRightPressed = false;
  }
  else
  {
    if (!btnRightPressed)
    {
      btnRightPressed = true;
      liveData->params.lastButtonPushedTime = liveData->params.currentTime;
      tft.setRotation(liveData->settings.displayRotation);
      // Menu handling
      if (liveData->menuVisible)
      {
        menuMove(true);
      }
      else
      {
        // doAction
        if (liveData->params.displayScreen == SCREEN_SPEED)
        {
          liveData->params.displayScreen = SCREEN_HUD;
          tft.fillScreen(TFT_BLACK);
          redrawScreen();
        }
        else if (liveData->params.displayScreen == SCREEN_HUD)
        {
          liveData->params.displayScreen = SCREEN_SPEED;
          redrawScreen();
        }

        setBrightness();
      }
    }
  }
  // Both left&right button (hide menu)
  if (isButtonPressed(pinButtonLeft) && isButtonPressed(pinButtonRight))
  {
    hideMenu();
  }

  // GPS process
  if (gpsHwUart != NULL)
  {
    unsigned long start = millis();
    do
    {
      while (gpsHwUart->available())
      {
        int ch = gpsHwUart->read();
        if (ch != -1)
          syslog->infoNolf(DEBUG_GPS, char(ch));
        gps.encode(ch);
      }
    } while (millis() - start < 20);
    //
    syncGPS();
  }
  else
  {
    // MEB CAR GPS
    if (liveData->params.gpsValid && liveData->params.gpsLat != -1 && liveData->params.gpsLon != -1)
      calcAutomaticBrightnessLatLon();
  }
  if (liveData->params.setGpsTimeFromCar != 0)
  {
    struct tm *tmm = gmtime(&liveData->params.setGpsTimeFromCar);
    tmm->tm_isdst = 0;
    setGpsTime(tmm->tm_year + 1900, tmm->tm_mon + 1, tmm->tm_mday, tmm->tm_hour, tmm->tm_min, tmm->tm_sec);
    liveData->params.setGpsTimeFromCar = 0;
  }

  // currentTime
  struct tm now;
  getLocalTime(&now);
  liveData->params.currentTime = mktime(&now);


  // Check and eventually reconnect WIFI aconnection

  if (WiFi.status() != WL_CONNECTED && liveData->params.currentTime - liveData->params.wifiLastConnectedTime > 60 && liveData->settings.remoteUploadModuleType == 1)
    {
        wifiFallback();
    }
  
  // SIM800L + WiFI remote upload
  netLoop();

  // SD card recording
  if (liveData->params.sdcardInit && liveData->params.sdcardRecording && liveData->params.sdcardCanNotify &&
      (liveData->params.odoKm != -1 && liveData->params.socPerc != -1))
  {

    // syslog->println(&now, "%y%m%d%H%M");

    // create filename
    if (liveData->params.operationTimeSec > 0 && strlen(liveData->params.sdcardFilename) == 0)
    {
      sprintf(liveData->params.sdcardFilename, "/%llu.json", uint64_t(liveData->params.operationTimeSec / 60));
      syslog->print("Log filename by opTimeSec: ");
      syslog->println(liveData->params.sdcardFilename);
    }
    if (liveData->params.currTimeSyncWithGps && strlen(liveData->params.sdcardFilename) < 15)
    {
      strftime(liveData->params.sdcardFilename, sizeof(liveData->params.sdcardFilename), "/%y%m%d%H%M.json", &now);
      syslog->print("Log filename by GPS: ");
      syslog->println(liveData->params.sdcardFilename);
    }

    // append buffer, clear buffer & notify state
    if (strlen(liveData->params.sdcardFilename) != 0)
    {
      liveData->params.sdcardCanNotify = false;
      File file = SD.open(liveData->params.sdcardFilename, FILE_APPEND);
      if (!file)
      {
        syslog->println("Failed to open file for appending");
        File file = SD.open(liveData->params.sdcardFilename, FILE_WRITE);
      }
      if (!file)
      {
        syslog->println("Failed to create file");
      }
      if (file)
      {
        syslog->info(DEBUG_SDCARD, "Save buffer to SD card");
        serializeParamsToJson(file);
        file.print(",\n");
        file.close();
      }
    }
  }

  // Read voltmeter INA3221 (if enabled)
  if (liveData->settings.voltmeterEnabled == 1 && liveData->params.currentTime - liveData->params.lastVoltageReadTime > 2)
  {
    liveData->params.auxVoltage = ina3221.getBusVoltage_V(1);

    liveData->params.lastVoltageReadTime = liveData->params.currentTime;

    if (liveData->settings.carType == CAR_HYUNDAI_IONIQ_2018)
    {
      float tmpAuxPerc = (float)(liveData->params.auxVoltage - 11.6) * 100 / (float)(12.8 - 11.6); // min 11.6V; max: 12.8V

      if (tmpAuxPerc > 100)
      {
        liveData->params.auxPerc = 100;
      }
      else if (tmpAuxPerc < 0)
      {
        liveData->params.auxPerc = 0;
      }
      else
      {
        liveData->params.auxPerc = tmpAuxPerc;
      }
    }

    if (liveData->params.auxVoltage > liveData->settings.voltmeterSleep)
      liveData->params.lastVoltageOkTime = liveData->params.currentTime;
  }

  // Turn off display if Ignition is off for more than 10s
  if (liveData->settings.sleepModeLevel == SLEEP_MODE_SCREEN_ONLY)
  {
    if (liveData->settings.voltmeterEnabled == 1)
    {
      syslog->print("Voltmeter");
      syslog->print(ina3221.getBusVoltage_V(1));
      syslog->print("V\t ");
      syslog->print(ina3221.getCurrent_mA(1));
      syslog->print("mA\t QUEUE:");
      syslog->println((liveData->params.stopCommandQueue ? "stopped" : "running"));
    }
  }
  // Wake up when engine on and SLEEP_MODE_SCREEN_ONLY and external voltmeter detects DC2DC charging
  if (liveData->settings.voltmeterEnabled == 1 &&liveData->settings.voltmeterBasedSleep == 1 &&
      liveData->settings.sleepModeLevel == SLEEP_MODE_SCREEN_ONLY && liveData->params.auxVoltage > 14.0)
  {
      liveData->params.stopCommandQueue = false;
      liveData->params.lastButtonPushedTime = liveData->params.currentTime;
  }
  // Automatic sleep after inactivity
  if (liveData->params.currentTime - liveData->params.lastIgnitionOnTime > 10 &&
      liveData->settings.sleepModeLevel >= SLEEP_MODE_SCREEN_ONLY &&
      liveData->params.currentTime - liveData->params.lastButtonPushedTime > 15 &&
      (liveData->params.currentTime - liveData->params.wakeUpTime > 30 || bootCount > 1))
  {
    turnOffScreen();
  }
  else
  {
    setBrightness();
  }

  // Go to sleep when car is off for more than 30s and not charging (AC charger is disabled for few seconds when ignition is turned off)
  if (liveData->params.currentTime - liveData->params.lastIgnitionOnTime > 30 && !liveData->params.chargingOn && liveData->settings.sleepModeLevel >= SLEEP_MODE_DEEP_SLEEP && liveData->params.currentTime - liveData->params.wakeUpTime > 30 && liveData->params.currentTime - liveData->params.lastButtonPushedTime > 10 && liveData->settings.voltmeterBasedSleep == 0)
  {
    goToSleep();
  }

  // Go to sleep when liveData->params.auxVoltage <= liveData->settings.voltmeterSleep for 30 seconds
  if (liveData->settings.voltmeterEnabled == 1 && liveData->settings.voltmeterBasedSleep == 1 && liveData->params.auxVoltage > 0 && liveData->params.currentTime - liveData->params.lastVoltageOkTime > 30 && liveData->params.currentTime - liveData->params.wakeUpTime > 30 && liveData->params.currentTime - liveData->params.lastButtonPushedTime > 10)
  {
    if (liveData->settings.sleepModeLevel == SLEEP_MODE_SCREEN_ONLY)
      liveData->params.stopCommandQueue = true;
    if (liveData->settings.sleepModeLevel >= SLEEP_MODE_DEEP_SLEEP)
      goToSleep();
  }

  // Read data from BLE/CAN
  if (!liveData->settings.threading)
  {
    commInterface->mainLoop();
  }

  // Reconnect CAN bus if no response for 5s
  if (liveData->settings.commType == 1 && liveData->params.currentTime - liveData->params.lastCanbusResponseTime > 5 && commInterface->checkConnectAttempts())
  {
    syslog->println("No response from CANbus for 5 seconds, reconnecting");
    commInterface->connectDevice();
    liveData->params.lastCanbusResponseTime = liveData->params.currentTime;
  }

  // force redraw (min 1 sec update)
  if (liveData->params.currentTime - lastRedrawTime >= 1 || liveData->redrawScreenRequested)
  {
    redrawScreen();
  }

  // Calculating avg.speed and time in forward mode
  if (liveData->params.odoKm != -1 && forwardDriveOdoKmLast == -1)
  {
    forwardDriveOdoKmLast = liveData->params.odoKm;
  }
  if (liveData->params.forwardDriveMode != lastForwardDriveMode)
  {
    if (liveData->params.forwardDriveMode)
    {
      if (forwardDriveOdoKmStart != -1 ||
          (liveData->params.odoKm != -1 && forwardDriveOdoKmLast != -1 && liveData->params.odoKm != forwardDriveOdoKmLast))
      {
        if (forwardDriveOdoKmStart == -1)
          forwardDriveOdoKmStart = liveData->params.odoKm;
        lastForwardDriveModeStart = liveData->params.currentTime;
        lastForwardDriveMode = liveData->params.forwardDriveMode;
      }
    }
    else
    {
      if (lastForwardDriveModeStart != 0)
      {
        previousForwardDriveModeTotal = previousForwardDriveModeTotal + (liveData->params.currentTime - lastForwardDriveModeStart);
        lastForwardDriveModeStart = 0;
      }
      lastForwardDriveMode = liveData->params.forwardDriveMode;
    }
  }
  liveData->params.timeInForwardDriveMode = previousForwardDriveModeTotal +
                                            (lastForwardDriveModeStart == 0 ? 0 : liveData->params.currentTime - lastForwardDriveModeStart);
  if (liveData->params.odoKm != -1 && forwardDriveOdoKmLast != -1 && liveData->params.odoKm != forwardDriveOdoKmLast && liveData->params.timeInForwardDriveMode > 0)
  {
    forwardDriveOdoKmLast = liveData->params.odoKm;
    liveData->params.avgSpeedKmh = (double)(liveData->params.odoKm - forwardDriveOdoKmStart) /
                                   ((double)liveData->params.timeInForwardDriveMode / 3600.0);
  }

  // Automatic reset charging or drive data after switch between drive / charging or longer standing (1800 seconds)
  if (liveData->params.chargingOn && liveData->params.carMode != CAR_MODE_CHARGING)
  {
    liveData->clearDrivingAndChargingStats(CAR_MODE_CHARGING);
  }
  else if (liveData->params.forwardDriveMode && liveData->params.speedKmh > 15 && liveData->params.carMode != CAR_MODE_DRIVE)
  {
    liveData->clearDrivingAndChargingStats(CAR_MODE_DRIVE);
  }
  else if (!liveData->params.chargingOn && !liveData->params.forwardDriveMode && liveData->params.carMode != CAR_MODE_NONE &&
           liveData->params.currentTime - liveData->params.carModeChanged > 1800)
  {
    liveData->clearDrivingAndChargingStats(CAR_MODE_NONE);
  }
}

/**
 * Set GPS time
 */
void Board320_240::setGpsTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t seconds)
{
  liveData->params.currTimeSyncWithGps = true;

  struct tm tm;
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = seconds;
  time_t t = mktime(&tm);
  printf("%02d%02d%02d%02d%02d%02d\n", year - 2000, month, day, hour, minute, seconds);
  struct timeval now = {.tv_sec = t};
  /*struct timezone tz;
  tz.tz_minuteswest = (liveData->settings.timezone + liveData->settings.daylightSaving) * 60;
  tz.tz_dsttime = 0;*/
  settimeofday(&now, NULL);
  syncTimes(t);

#ifdef BOARD_M5STACK_CORE2
  RTC_TimeTypeDef RTCtime;
  RTC_DateTypeDef RTCdate;

  RTCdate.Year = year;
  RTCdate.Month = month;
  RTCdate.Date = day;
  RTCtime.Hours = hour;
  RTCtime.Minutes = minute;
  RTCtime.Seconds = seconds;

  M5.Rtc.SetTime(&RTCtime);
  M5.Rtc.SetDate(&RTCdate);
#endif
}

/**
  sync all times
*/
void Board320_240::syncTimes(time_t newTime)
{
  if (liveData->params.chargingStartTime != 0)
    liveData->params.chargingStartTime = newTime - (liveData->params.currentTime - liveData->params.chargingStartTime);

  if (liveData->params.lastDataSent != 0)
    liveData->params.lastDataSent = newTime - (liveData->params.currentTime - liveData->params.lastDataSent);

  if (liveData->params.sim800l_lastOkReceiveTime != 0)
    liveData->params.sim800l_lastOkReceiveTime = newTime - (liveData->params.currentTime - liveData->params.sim800l_lastOkReceiveTime);

  if (liveData->params.sim800l_lastOkSendTime != 0)
    liveData->params.sim800l_lastOkSendTime = newTime - (liveData->params.currentTime - liveData->params.sim800l_lastOkSendTime);

  if (liveData->params.lastButtonPushedTime != 0)
    liveData->params.lastButtonPushedTime = newTime - (liveData->params.currentTime - liveData->params.lastButtonPushedTime);

  if (liveData->params.wakeUpTime != 0)
    liveData->params.wakeUpTime = newTime - (liveData->params.currentTime - liveData->params.wakeUpTime);

  if (liveData->params.lastIgnitionOnTime != 0)
    liveData->params.lastIgnitionOnTime = newTime - (liveData->params.currentTime - liveData->params.lastIgnitionOnTime);

  if (liveData->params.lastChargingOnTime != 0)
    liveData->params.lastChargingOnTime = newTime - (liveData->params.currentTime - liveData->params.lastChargingOnTime);

  if (liveData->params.lastVoltageReadTime != 0)
    liveData->params.lastVoltageReadTime = newTime - (liveData->params.currentTime - liveData->params.lastVoltageReadTime);

  if (liveData->params.lastVoltageOkTime != 0)
    liveData->params.lastVoltageOkTime = newTime - (liveData->params.currentTime - liveData->params.lastVoltageOkTime);

  if (liveData->params.lastCanbusResponseTime != 0)
    liveData->params.lastCanbusResponseTime = newTime - (liveData->params.currentTime - liveData->params.lastCanbusResponseTime);

  // reset avg speed counter
  lastForwardDriveModeStart = 0;
  lastForwardDriveMode = false;
}

/**
   skipAdapterScan
*/
bool Board320_240::skipAdapterScan()
{
  return isButtonPressed(pinButtonMiddle) || isButtonPressed(pinButtonLeft) || isButtonPressed(pinButtonRight);
}

/**
   Mount sdcard
*/
bool Board320_240::sdcardMount()
{

  if (liveData->params.sdcardInit)
  {
    syslog->println("SD card already mounted...");
    return true;
  }

  int8_t countdown = 3;
  bool SdState = false;

  while (1)
  {
    syslog->print("Initializing SD card...");

    /*    syslog->print(" TTGO-T4 ");
        SPIClass * hspi = new SPIClass(HSPI);
        spiSD.begin(pinSdcardSclk, pinSdcardMiso, pinSdcardMosi, pinSdcardCs); //SCK,MISO,MOSI,ss
        SdState = SD.begin(pinSdcardCs, *hspi, SPI_FREQUENCY);*/

    syslog->print(" M5STACK ");
    SdState = SD.begin(TFCARD_CS_PIN, SPI, 40000000);

    if (SdState)
    {

      uint8_t cardType = SD.cardType();
      if (cardType == CARD_NONE)
      {
        syslog->println("No SD card attached");
        return false;
      }

      syslog->println("SD card found.");
      liveData->params.sdcardInit = true;

      syslog->print("SD Card Type: ");
      if (cardType == CARD_MMC)
      {
        syslog->println("MMC");
      }
      else if (cardType == CARD_SD)
      {
        syslog->println("SDSC");
      }
      else if (cardType == CARD_SDHC)
      {
        syslog->println("SDHC");
      }
      else
      {
        syslog->println("UNKNOWN");
      }

      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
      syslog->printf("SD Card Size: %lluMB\n", cardSize);

      return true;
    }

    syslog->println("Initialization failed!");
    countdown--;
    if (countdown <= 0)
    {
      break;
    }
    delay(500);
  }

  return false;
}

/**
   Toggle sdcard recording
*/
void Board320_240::sdcardToggleRecording()
{

  if (!liveData->params.sdcardInit)
    return;

  syslog->println("Toggle SD card recording...");
  liveData->params.sdcardRecording = !liveData->params.sdcardRecording;
  if (liveData->params.sdcardRecording)
  {
    liveData->params.sdcardCanNotify = true;
  }
  else
  {
    String tmpStr = "";
    tmpStr.toCharArray(liveData->params.sdcardFilename, tmpStr.length() + 1);
  }
}

/**
   Sync gps data
*/
void Board320_240::syncGPS()
{
  liveData->params.gpsValid = gps.location.isValid();
  if (liveData->params.gpsValid)
  {
    liveData->params.gpsLat = gps.location.lat();
    liveData->params.gpsLon = gps.location.lng();
    if (gps.altitude.isValid())
      liveData->params.gpsAlt = gps.altitude.meters();
    calcAutomaticBrightnessLatLon();
  }
  if (gps.speed.isValid())
  {
    liveData->params.speedKmhGPS = gps.speed.kmph();
  }
  else
  {
    liveData->params.speedKmhGPS = -1;
  }
  if (gps.satellites.isValid())
  {
    liveData->params.gpsSat = gps.satellites.value();
  }
  if (!liveData->params.currTimeSyncWithGps && gps.date.isValid() && gps.time.isValid())
  {
    setGpsTime(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
  }
}

/**
   Setup WiFi
 **/
bool Board320_240::wifiSetup()
{
  syslog->print("WiFi init: ");
  syslog->println(liveData->settings.wifiSsid);
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(liveData->settings.wifiSsid, liveData->settings.wifiPassword);
  liveData->params.wifiLastConnectedTime = liveData->params.currentTime;
  return true;
}

/**
  Switch Wifi network
 **/
void Board320_240::wifiFallback()
{
  WiFi.disconnect(true);

  if (liveData->settings.backupWifiEnabled == 1)
  {
    if (liveData->params.isWifiBackupLive == false)
    {
      wifiSwitchToBackup();
    }
    else
    {
      wifiSwitchToMain();
    }
  }
  else
  {
      // if there is no backup wifi config we try to connect to the main wifi anyway. Maybe there was an interruption.
      wifiSwitchToMain();
  }
}

void Board320_240::wifiSwitchToBackup()
{
  syslog->print("WiFi switchover to backup: ");
  syslog->println(liveData->settings.wifiSsidb);
  WiFi.begin(liveData->settings.wifiSsidb, liveData->settings.wifiPasswordb);
  liveData->params.isWifiBackupLive = true;
  liveData->params.wifiBackupUptime = liveData->params.currentTime;
  liveData->params.wifiLastConnectedTime = liveData->params.currentTime;
}

void Board320_240::wifiSwitchToMain()
{
  syslog->print("WiFi switchover to main: ");
  syslog->println(liveData->settings.wifiSsid);
  WiFi.begin(liveData->settings.wifiSsid, liveData->settings.wifiPassword);
  liveData->params.isWifiBackupLive = false;
  liveData->params.wifiLastConnectedTime = liveData->params.currentTime;
}

/**
  SIM800L
*/
bool Board320_240::sim800lSetup()
{
  syslog->print("Setting SIM800L module. HW port: ");
  syslog->println(liveData->settings.gprsHwSerialPort);

  gprsHwUart = new HardwareSerial(liveData->settings.gprsHwSerialPort);

  if (liveData->settings.gprsHwSerialPort == 2)
  {
    gprsHwUart->begin(9600, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);
  }
  else
  {
    gprsHwUart->begin(9600);
  }

  sim800l = new SIM800L((Stream *)gprsHwUart, SIM800L_RST, SIM800L_INT_BUFFER, SIM800L_RCV_BUFFER);
  // SIM800L DebugMode:
  // sim800l = new SIM800L((Stream *)gprsHwUart, SIM800L_RST, SIM800L_INT_BUFFER , SIM800L_RCV_BUFFER, syslog);

  bool sim800l_ready = sim800l->isReady();
  for (uint8_t i = 0; i < 3 && !sim800l_ready; i++)
  {
    syslog->println("Problem to initialize SIM800L module, retry in 0.5 sec");
    delay(500);
    sim800l_ready = sim800l->isReady();
  }

  if (!sim800l_ready)
  {
    syslog->println("Problem to initialize SIM800L module");
  }
  else
  {
    syslog->println("SIM800L module initialized");

    sim800l->exitSleepMode();

    if (sim800l->getPowerMode() != NORMAL)
    {
      syslog->println("SIM800L module in sleep mode - Waking up");
      if (sim800l->setPowerMode(NORMAL))
      {
        syslog->println("SIM800L in normal power mode");
      }
      else
      {
        syslog->println("Failed to switch SIM800L to normal power mode");
      }
    }

    syslog->print("Setting GPRS APN to: ");
    syslog->println(liveData->settings.gprsApn);

    bool sim800l_gprs = sim800l->setupGPRS(liveData->settings.gprsApn);
    for (uint8_t i = 0; i < 5 && !sim800l_gprs; i++)
    {
      syslog->println("Problem to set GPRS APN, retry in 1 sec");
      delay(1000);
      sim800l_gprs = sim800l->setupGPRS(liveData->settings.gprsApn);
    }

    if (sim800l_gprs)
    {
      liveData->params.sim800l_enabled = true;
      syslog->println("GPRS APN set OK");
    }
    else
    {
      syslog->println("Problem to set GPRS APN");
    }
  }

  return true;
}

void Board320_240::netLoop()
{
  // Sync NTP firsttime
  if (liveData->settings.wifiEnabled == 1 && !liveData->params.ntpTimeSet && WiFi.status() == WL_CONNECTED && liveData->settings.ntpEnabled)
  {
    ntpSync();
  }

  // Upload to API - SIM800L or WIFI is supported
  if (liveData->params.currentTime - liveData->params.lastDataSent > liveData->settings.remoteUploadIntervalSec && liveData->settings.remoteUploadIntervalSec != 0)
  {
    liveData->params.lastDataSent = liveData->params.currentTime;
    netSendData();
  }

  // Upload to ABRP
  if (liveData->params.currentTime - liveData->params.lastDataSent > liveData->settings.remoteUploadAbrpIntervalSec && liveData->settings.remoteUploadAbrpIntervalSec != 0)
  {
    liveData->params.lastDataSent = liveData->params.currentTime;
    netSendData();
  }

  // SIM800
  if (liveData->params.sim800l_enabled && liveData->settings.remoteUploadModuleType == 0)
  {
    uint16_t rc = sim800l->readPostResponse(SIM800L_RCV_TIMEOUT * 1000);
    if (rc == 200)
    {
      syslog->print("HTTP POST OK: ");
      syslog->println(sim800l->getDataReceived());
      liveData->params.sim800l_lastOkReceiveTime = liveData->params.currentTime;
    }
    else if (rc != 1 && rc != 0)
    {
      syslog->print("HTTP POST error: ");
      syslog->println(rc);
    }
  }
}

bool Board320_240::netSendData()
{
  uint16_t rc = 0;
  /*
  syslog->println();
  syslog->print("liveData->params.currentTime: ");
  syslog->println(liveData->params.currentTime);
  syslog->print("liveData->params.wifiLastConnectedTime: ");
  syslog->println(liveData->params.wifiLastConnectedTime);
  syslog->print("liveData->params.wifiBackupUptime: ");
  syslog->println(liveData->params.wifiBackupUptime);
  syslog->print("liveData->settings.backupWifiEnabled: ");
  syslog->println(liveData->settings.backupWifiEnabled);
  syslog->print("liveData->params.isWifiBackupLive: ");
  syslog->println(liveData->params.isWifiBackupLive);
  syslog->print("WiFi.status: ");
  syslog->println(WiFi.status());
*/
  if (liveData->params.socPerc < 0)
  {
    syslog->println("No valid data, skipping data send");
    return false;
  }

  // WIFI
  if (liveData->settings.remoteUploadModuleType == 1)
  {
    syslog->println("Sending data to API - via WIFI");
  }

  // SIM800L
  else if (liveData->settings.remoteUploadModuleType == 0)
  {
    if (liveData->params.sim800l_enabled)
    {
      syslog->println("Sending data to API - via SIM800L");

      NetworkRegistration network = sim800l->getRegistrationStatus();
      if (network != REGISTERED_HOME && network != REGISTERED_ROAMING)
      {
        syslog->println("SIM800L module not connected to network, skipping data send");
        return false;
      }

      if (!sim800l->isConnectedGPRS())
      {
        syslog->println("GPRS not connected... Connecting");
        bool connected = sim800l->connectGPRS();
        for (uint8_t i = 0; i < 5 && !connected; i++)
        {
          syslog->println("Problem to connect GPRS, retry in 1 sec");
          delay(1000);
          connected = sim800l->connectGPRS();
        }
        if (connected)
        {
          syslog->println("GPRS connected!");
        }
        else
        {
          syslog->println("GPRS not connected! Reseting module...");
          sim800l->reset();
          sim800lSetup();
          return false;
        }
      }
    }
    else
    {
      syslog->println("SIM800L module not present, skipping data send");
      return false;
    }
  }
  else
  {
    syslog->println("Unsupported module");
    return false;
  }

  syslog->println("Start HTTP POST...");

  if (liveData->settings.remoteUploadIntervalSec != 0)
  {

    StaticJsonDocument<768> jsonData;

    jsonData["apikey"] = liveData->settings.remoteApiKey;
    jsonData["carType"] = liveData->settings.carType;
    jsonData["ignitionOn"] = liveData->params.ignitionOn;
    jsonData["chargingOn"] = liveData->params.chargingOn;
    jsonData["chargingDc"] = liveData->params.chargerDCconnected;
    jsonData["socPerc"] = liveData->params.socPerc;
    if (liveData->params.socPercBms != -1)
      jsonData["socPercBms"] = liveData->params.socPercBms;
    jsonData["sohPerc"] = liveData->params.sohPerc;
    jsonData["batPowerKw"] = liveData->params.batPowerKw;
    jsonData["batPowerAmp"] = liveData->params.batPowerAmp;
    jsonData["batVoltage"] = liveData->params.batVoltage;
    jsonData["auxVoltage"] = liveData->params.auxVoltage;
    jsonData["auxAmp"] = liveData->params.auxCurrentAmp;
    jsonData["batMinC"] = liveData->params.batMinC;
    jsonData["batMaxC"] = liveData->params.batMaxC;
    jsonData["batInletC"] = liveData->params.batInletC;
    jsonData["extTemp"] = liveData->params.outdoorTemperature;
    jsonData["batFanStatus"] = liveData->params.batFanStatus;
    jsonData["speedKmh"] = liveData->params.speedKmh;

    jsonData["odoKm"] = liveData->params.odoKm;
    jsonData["cumulativeEnergyChargedKWh"] = liveData->params.cumulativeEnergyChargedKWh;
    jsonData["cumulativeEnergyDischargedKWh"] = liveData->params.cumulativeEnergyDischargedKWh;

    // Send GPS data via GPRS (if enabled && valid)
    if ((liveData->settings.gpsHwSerialPort <= 2 && gps.location.isValid()) || // HW GPS or MEB GPS
        (liveData->settings.gpsHwSerialPort == 255 && liveData->params.gpsLat != -1))
    {
      jsonData["gpsLat"] = liveData->params.gpsLat;
      jsonData["gpsLon"] = liveData->params.gpsLon;
      jsonData["gpsAlt"] = liveData->params.gpsAlt;
      jsonData["gpsSpeed"] = liveData->params.speedKmhGPS;
    }

    char payload[768];
    serializeJson(jsonData, payload);

    syslog->print("Sending payload: ");
    syslog->println(payload);

    syslog->print("Remote API server: ");
    syslog->println(liveData->settings.remoteApiUrl);

    // WIFI
    rc = 0;
    if (liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI && liveData->settings.wifiEnabled == 1)
    {
      WiFiClient client;
      HTTPClient http;

      http.begin(client, liveData->settings.remoteApiUrl);
      http.setConnectTimeout(500);
      http.addHeader("Content-Type", "application/json");
      rc = http.POST(payload);
      http.end();
    }
    else if (liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_SIM800L)
    {
      // SIM800L
      rc = sim800l->doPost(liveData->settings.remoteApiUrl, "application/json", payload, SIM800L_SND_TIMEOUT * 1000);
    }

    if (rc == 200)
    {
      syslog->println("HTTP POST send successful");
      liveData->params.sim800l_lastOkSendTime = liveData->params.currentTime;
    }
    else
    {
      // Failed...
      syslog->print("HTTP POST error: ");
      syslog->println(rc);
    }
  }
  else if (liveData->settings.remoteUploadAbrpIntervalSec != 0)
  {

    StaticJsonDocument<768> jsonData;

    switch (liveData->settings.carType)
    {
    case CAR_HYUNDAI_KONA_2020_39:
      jsonData["car_model"] = "hyundai:kona:19:39:other";
      break;
    case CAR_HYUNDAI_IONIQ_2018:
      jsonData["car_model"] = "hyundai:ioniq:17:28:other";
      break;
    case CAR_HYUNDAI_IONIQ5_58:
      jsonData["car_model"] = "hyundai:ioniq5:21:58:mr";
      break;
    case CAR_HYUNDAI_IONIQ5_72:
      jsonData["car_model"] = "hyundai:ioniq5:21:72:lr";
      break;
    case CAR_HYUNDAI_IONIQ5_77:
      jsonData["car_model"] = "hyundai:ioniq5:21:77:lr";
      break;
    case CAR_KIA_ENIRO_2020_64:
      jsonData["car_model"] = "kia:niro:19:64:other";
      break;
    case CAR_KIA_ESOUL_2020_64:
      jsonData["car_model"] = "kia:soul:19:64:other";
      break;
    case CAR_HYUNDAI_KONA_2020_64:
      jsonData["car_model"] = "hyundai:kona:19:64:other";
      break;
    case CAR_KIA_ENIRO_2020_39:
      jsonData["car_model"] = "kia:niro:19:39:other";
      break;
    case CAR_KIA_EV6_58:
      jsonData["car_model"] = "kia:ev6:22:58:mr";
      break;
    case CAR_KIA_EV6_77:
      jsonData["car_model"] = "kia:ev6:22:77:lr";
      break;
    case CAR_AUDI_Q4_35:
      jsonData["car_model"] = "audi:q4:21:52:meb";
      break;
    case CAR_AUDI_Q4_40:
      jsonData["car_model"] = "audi:q4:21:77:meb";
      break;
    case CAR_AUDI_Q4_45:
      jsonData["car_model"] = "audi:q4:21:77:meb";
      break;
    case CAR_AUDI_Q4_50:
      jsonData["car_model"] = "audi:q4:21:77:meb";
      break;
    case CAR_SKODA_ENYAQ_55:
      jsonData["car_model"] = "skoda:enyaq:21:52:meb";
      break;
    case CAR_SKODA_ENYAQ_62:
      jsonData["car_model"] = "skoda:enyaq:21:55:meb";
      break;
    case CAR_SKODA_ENYAQ_82:
      jsonData["car_model"] = "skoda:enyaq:21:77:meb";
      break;
    case CAR_VW_ID3_2021_45:
      jsonData["car_model"] = "volkswagen:id3:20:45:sr";
      break;
    case CAR_VW_ID3_2021_58:
      jsonData["car_model"] = "volkswagen:id3:20:58:mr";
      break;
    case CAR_VW_ID3_2021_77:
      jsonData["car_model"] = "volkswagen:id3:20:77:lr";
      break;
    case CAR_VW_ID4_2021_45:
      jsonData["car_model"] = "volkswagen:id4:20:45:sr"; // not valid in the iterno list of cars
      break;
    case CAR_VW_ID4_2021_58:
      jsonData["car_model"] = "volkswagen:id4:21:52";
      break;
    case CAR_VW_ID4_2021_77:
      jsonData["car_model"] = "volkswagen:id4:21:77";
      break;
    case CAR_RENAULT_ZOE:
      jsonData["car_model"] = "renault:zoe:r240:22:other";
      break;
    case CAR_BMW_I3_2014:
      jsonData["car_model"] = "bmw:i3:14:22:other";
      break;
    default:
      syslog->println("Car not supported by ABRP Uploader");
      return false;
      break;
    }

    jsonData["utc"] = liveData->params.currentTime;
    jsonData["soc"] = liveData->params.socPerc;
    jsonData["power"] = liveData->params.batPowerKw * -1;
    jsonData["is_parked"] = (liveData->params.parkModeOrNeutral) ? 1 : 0;
    if (liveData->params.speedKmhGPS > 0)
    {
      jsonData["speed"] = liveData->params.speedKmhGPS;
    }
    else
    {
      jsonData["speed"] = liveData->params.speedKmh;
    }
    jsonData["is_charging"] = (liveData->params.chargingOn) ? 1 : 0;
    if (liveData->params.chargingOn)
      jsonData["is_dcfc"] = (liveData->params.chargerDCconnected) ? 1 : 0;

    if ((liveData->settings.gpsHwSerialPort <= 2 && gps.location.isValid()) || // HW GPS or MEB GPS
        (liveData->settings.gpsHwSerialPort == 255 && liveData->params.gpsLat != -1))
    {
      jsonData["lat"] = liveData->params.gpsLat;
      jsonData["lon"] = liveData->params.gpsLon;
      jsonData["elevation"] = liveData->params.gpsAlt;
    }

    jsonData["capacity"] = liveData->params.batteryTotalAvailableKWh;
    jsonData["kwh_charged"] = liveData->params.cumulativeEnergyChargedKWh;
    jsonData["soh"] = liveData->params.sohPerc;
    jsonData["ext_temp"] = liveData->params.outdoorTemperature;
    jsonData["batt_temp"] = liveData->params.batMinC;
    jsonData["voltage"] = liveData->params.batVoltage;
    jsonData["current"] = liveData->params.batPowerAmp * -1;
    if (liveData->params.odoKm > 0)
      jsonData["odometer"] = liveData->params.odoKm;

    String payload;
    serializeJson(jsonData, payload);

    String tmpStr = "api_key="; // dev ApiKey
    tmpStr.concat(ABRP_API_KEY);
    tmpStr.concat("&token=");
    tmpStr.concat(liveData->settings.abrpApiToken); // User token
    tmpStr.concat("&tlm=");
    tmpStr.concat(payload);

    tmpStr.replace("\"", "%22");

    char dta[tmpStr.length() + 1];
    tmpStr.toCharArray(dta, tmpStr.length() + 1);

    syslog->print("Sending data: ");
    syslog->println(dta);

    rc = 0;
    if (liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI && liveData->settings.wifiEnabled == 1)
    {
      WiFiClient client;
      HTTPClient http;

      // http.begin(client, "api.iternio.com", 443, "/1/tlm/send", true);
      http.begin(client, "http://api.iternio.com/1/tlm/send");
      http.setConnectTimeout(500);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      rc = http.POST(dta);
      http.end();
    }
    else if (liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_SIM800L)
    {
      // SIM800L
      rc = sim800l->doPost("http://api.iternio.com/1/tlm/send", "application/x-www-form-urlencoded", dta, SIM800L_SND_TIMEOUT * 1000);
    }

    if (rc == 200)
    {
      syslog->println("HTTP POST send successful");
      liveData->params.sim800l_lastOkSendTime = liveData->params.currentTime;
    }
    else
    {
      // Failed...
      syslog->print("HTTP POST error: ");
      syslog->println(rc);
    }
  }
  else
  {
    syslog->println("Well... This not gonna happen... (Board320_240::sim800lSendData();)"); // Just for debug reasons...
  }

  return true;
}

void Board320_240::initGPS()
{
  syslog->print("GPS initialization on hwUart: ");
  syslog->println(liveData->settings.gpsHwSerialPort);

  gpsHwUart = new HardwareSerial(liveData->settings.gpsHwSerialPort);

  if (liveData->settings.gpsHwSerialPort == 2)
  {
    gpsHwUart->begin(9600, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);
  }
  else
  {
    gpsHwUart->begin(9600);
  }

  // Enable static hold
  // https://www.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_%28UBX-13003221%29.pdf
  // https://github.com/noerw/mobile-sensebox/blob/master/esp8266-gps/gps.h
  // uBlox NEO-7M can't persist settings, so we update them on runtime to get a higher update rate
  // commands extracted via u-center (https://www.youtube.com/watch?v=iWd0gCOYsdo)
  uint8_t ubloxconfig[] = {
      // disable sleep mode
      0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x37,
      // enable GPGGA & RMC sentences (only these are evaluated by TinyGPS++)
      0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x05, 0x38, // GGA
      0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x09, 0x54, // RMC
      // disable all other NMEA sentences to save bandwith
      0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B, // GLL
      0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32, // GSA
      0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39, // GSV
      0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47, // VTG
      // setup SBAS search to EGNOS only
      0xB5, 0x62, 0x06, 0x16, 0x08, 0x00, 0x01, 0x03, 0x03, 0x00, 0x51, 0x08, 0x00, 0x00, 0x84, 0x15,
      // set NAV5 model to automotive, static hold on 0.5m/s, 3m
      0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27,
      0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x32, 0x3C, 0x00, 0x00,
      0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x78,
      // 150ms update interval
      0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x96, 0x00, 0x01, 0x00, 0x01, 0x00, 0xAC, 0x3E,
      // 100ms update interval (needs higher baudrate, whose config doesnt work?)
      // 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12,
      // uart to baud 115200 and nmea only  -> wont work?! TODO :^(
      // 0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x00, 0xC2,
      // 0x01, 0x00, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBF, 0x78,
      // save changes
      0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x03, 0x1D, 0xAB};

  gpsHwUart->write(ubloxconfig, sizeof(ubloxconfig));
}
