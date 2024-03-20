/**
 * Initializes the board hardware and peripherals.
 * Sets up pins, time, display, etc.
 *
This code initializes and configures the hardware and peripherals for a display board.

@src\Board320_240.cpp:1-5124 initializes a 320x240 pixel display board.

It first includes necessary libraries and header files. It gets the current date and time from the real time clock module and sets it using the time library functions.

It initializes the display driver and sets the rotation, brightness and color depth based on configuration settings. It also initializes a sprite graphics buffer to hold images to display.

The initBoard() function configures the input buttons, gets the current time, and increments a boot counter.

afterSetup() checks if the board is waking from sleep mode, initializes the voltmeter module if enabled, and calls additional setup functions from a board interface class.

It prints debug messages, checks sleep mode settings, and decides whether to go back to sleep or wake up the board.

The main purpose is to initialize all the hardware like the screen, buttons, voltmeter, etc. and configure settings like brightness and rotation. It gets the current time and date and stores it.
It also handles waking from sleep and going back to sleep.

The key inputs are the real time clock data, configuration settings, and sleep mode/wakeup logic.

The outputs are an initialized display, sprite buffer, time, debug messages, and sleep management.

The main logic flow is:

1. Include libraries
2. Get time and date
3. Initialize display and sprites
4. Configure input buttons
5. Increment boot count
6. Check sleep mode and wake/sleep logic
7. Initialize other devices like voltmeter
8. Call additional setup functions

So in summary, it initializes the core display and hardware functionality, retrieves time and settings, handles wake/sleep logic, and prepares the board for operation.

 */
#include <FS.h>
#include <analogWrite.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> //To be able to use https with ABRP api server
#include <Update.h>
#include "config.h"
#include "BoardInterface.h"
#include "Board320_240.h"
#include <time.h>
#include <ArduinoJson.h>

#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
#include <PubSubClient.h>
#include "WebInterface.h"
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3

RTC_DATA_ATTR unsigned int bootCount = 0;
RTC_DATA_ATTR unsigned int sleepCount = 0;

#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
WebInterface *webInterface = nullptr;
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3

#ifdef BOARD_M5STACK_CORES3
// SD card
#define TFCARD_CS_PIN 4
#endif // BOARD_M5STACK_CORES3

/**
   Init board
*/
void Board320_240::initBoard()
{
  liveData->params.booting = true;

  // Init time library
  struct timeval tv;

#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
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
#endif // BOARD_M5STACK_CORE2
#ifdef BOARD_M5STACK_CORES3
  auto dt = CoreS3.Rtc.getDateTime();
  if (dt.date.year > 2020)
  {
    struct tm tm_tmp;
    tm_tmp.tm_year = dt.date.year - 1900;
    tm_tmp.tm_mon = dt.date.month - 1;
    tm_tmp.tm_mday = dt.date.date;
    tm_tmp.tm_hour = dt.time.hours;
    tm_tmp.tm_min = dt.time.minutes;
    tm_tmp.tm_sec = dt.time.seconds;

    time_t t = mktime(&tm_tmp);
    tv.tv_sec = t;
  }
  else
  {
    tv.tv_sec = 1589011873;
  }
#endif // BOARD_M5STACK_CORES3
#else
  tv.tv_sec = 1589011873;
#endif

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
    syslog->println("V");
    syslog->print("ch1:");
    syslog->print(ina3221.getCurrent_mA(1));
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
  tft.fillScreen(TFT_RED);

  bool psramUsed = false; // 320x240 16bpp sprites requires psram
#if defined(ESP32) && defined(CONFIG_SPIRAM_SUPPORT)
  if (psramFound())
    psramUsed = true;
#endif
  printHeapMemory();
  syslog->println((psramUsed) ? "Sprite 16" : "Sprite 8");
  spr.setColorDepth((psramUsed) ? 16 : 8);
  spr.createSprite(320, 240);
  liveData->params.spriteInit = true;
  printHeapMemory();
  tft.fillScreen(TFT_PURPLE);

  // Show test data on right button during boot device
  liveData->params.displayScreen = liveData->settings.defaultScreen;
  if (isButtonPressed(pinButtonRight))
  {
    syslog->printf("Right button pressed");
    loadTestData();
    printHeapMemory();
  }
  tft.fillScreen(TFT_YELLOW);

  // Wifi
  // Starting Wifi after BLE prevents reboot loop
  if (!liveData->params.wifiApMode && liveData->settings.wifiEnabled == 1)
  {
    wifiSetup();
    printHeapMemory();
  }
  tft.fillScreen(TFT_BLUE);

  // Init GPS
  if (liveData->settings.gpsHwSerialPort <= 2)
  {
    initGPS();
    printHeapMemory();
  }
  tft.fillScreen(TFT_SKYBLUE);

  // SD card
  if (liveData->settings.sdcardEnabled == 1)
  {
    if (sdcardMount() && liveData->settings.sdcardAutstartLog == 1)
    {
      syslog->println("Toggle recording on SD card");
      sdcardToggleRecording();
      printHeapMemory();
    }
  }
  tft.fillScreen(TFT_ORANGE);

  // Init comm device
  if (!afterSetup)
  {
    BoardInterface::afterSetup();
    printHeapMemory();
  }
  tft.fillScreen(TFT_SILVER);

  // Threading
  // Comm via thread (ble/can)
  if (liveData->settings.threading)
  {
    syslog->println("xTaskCreate/xTaskCommLoop - COMM via thread (ble/can)");
    xTaskCreate(xTaskCommLoop, "xTaskCommLoop", 32768, (void *)this, 0, NULL);
  }
  else
  {
    syslog->println("COMM without threading (ble/can)");
  }

  showTime();
  tft.fillScreen(TFT_GREEN);

  liveData->params.wakeUpTime = liveData->params.currentTime;
  liveData->params.lastCanbusResponseTime = liveData->params.currentTime;
  liveData->params.booting = false;
}

/**
 * OTA Update
 */
void Board320_240::otaUpdate()
{
// Only for core2
#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
#include "raw_githubusercontent_com.h" // the root certificate is now in const char * root_cert

  printHeapMemory();

  String url = "https://raw.githubusercontent.com/nickn17/evDash/master/dist/m5stack-core2/evDash.ino.bin";
  // url = "https://raw.githubusercontent.com/nickn17/evDash/master/dist/ttgo-t4-v13/evDash.ino.bin";

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
    printHeapMemory();
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

  /**
   * Processes the HTTP response from the server.
   * Checks if the response code is 200 OK, otherwise prints the error response.
   * Breaks out of the loop when an empty line is reached, indicating the end of the headers.
   */
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

  /**
   * Attempts to begin an OTA update, checking for errors.
   * Displays an error message if there is not enough space to begin the update.
   */
  displayMessage("Installing OTA...", "");
  if (!Update.begin(contentLength))
  {
    printHeapMemory();
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

#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3
}

/**
 * Puts the board into sleep mode for a number of seconds.
 *
 * If SIM800L is enabled, disconnects from GPRS before sleeping.
 * Puts SIM800L into sleep mode.
 *
 * If GPS is enabled, sends command to put into sleep mode.
 *
 * Determines sleep duration based on settings:
 * - sleepModeLevel: SLEEP_MODE_DEEP_SLEEP to use interval from settings.
 * - sleepModeIntervalSec: seconds to sleep.
 * - sleepModeShutdownHrs: max hours to sleep before shutting down.
 * Increments counter of sleep cycles.
 *
 * Calls enterSleepMode() with determined duration.
 */
void Board320_240::goToSleep()
{
  syslog->println("Going to sleep.");

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

/**
 * Wake up board from deep sleep and determine if car is charging or ignition is on.
 * Checks wakeup reason and returns if external wakeup pin was triggered.
 * Checks voltage and returns if above wakeup threshold.
 * Otherwise queues commands to check for valid response indicating car is active.
 * If no valid response after timeout, enters deep sleep again.
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

#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
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
 * Turns off the screen by setting the brightness to 0.
 * Checks if screen is already off before doing anything.
 * Has debug mode to print messages.
 * Uses hardware-specific code based on #defines to actually
 * control the backlight/screen brightness.
 */
void Board320_240::turnOffScreen()
{
  bool debugTurnOffScreen = false;
  if (debugTurnOffScreen)
    displayMessage("Turn off screen", (liveData->params.stopCommandQueue ? "Command queue stopped" : "Queue is running"));
  if (currentBrightness == 0)
    return;

  syslog->println("Turn off screen");
  currentBrightness = 0;

  if (debugTurnOffScreen)
  {
    return;
  }

#ifdef BOARD_M5STACK_CORE2
  M5.Axp.SetDCDC3(false);
  M5.Axp.SetLcdVoltage(2500);
#endif // BOARD_M5STACK_CORE2
#ifdef BOARD_M5STACK_CORES3
  /*CoreS3.Display.setBrightness;
  M5.Axp.SetLcdVoltage(2500);*/
#endif // BOARD_M5STACK_CORES3
}

/**
 * Set brightness level
 */
void Board320_240::setBrightness()
{
  uint8_t lcdBrightnessPerc;

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
  if (liveData->params.stopCommandQueue) { 
    lcdBrightnessPerc = 20;
  }

  if (currentBrightness == lcdBrightnessPerc)
    return;

  syslog->print("Set brightness: ");
  syslog->println(lcdBrightnessPerc);
  currentBrightness = lcdBrightnessPerc;

#ifdef BOARD_M5STACK_CORE2
  M5.Axp.SetDCDC3(true);
  uint16_t lcdVolt = map(lcdBrightnessPerc, 0, 100, 2500, 3300);
  M5.Axp.SetLcdVoltage(lcdVolt);
#endif // BOARD_M5STACK_CORE2
}

/**
 * Displays a message dialog on the screen.
 *
 * Draws a filled rectangle as the dialog background, then draws the two message
 * strings centered vertically and horizontally. Uses direct drawing to the screen
 * buffer instead of sprites if sprites are not initialized.
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
 * Displays a confirmation dialog with Yes/No buttons and waits for user input.
 *
 * Draws a filled rectangle as the dialog background, displays the confirmation
 * message strings centered vertically and horizontally, and "YES"/"NO" buttons
 * at bottom left/right. Waits up to 2 seconds for left/right buttons to be pressed.
 *
 * @param row1 First message string
 * @param row2 Second message string
 * @return True if Yes was selected, false if No or timeout
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
  for (uint16_t i = 0; i < 2000 * 100; i++)
  {
    boardLoop();
    if (isButtonPressed(pinButtonLeft))
    {
      res = true;
      break;
    }
    if (isButtonPressed(pinButtonRight))
    {
      res = false;
      break;
    }
  }
  return res;
}

/**
 * Draws a cell on the dashboard display.
 *
 * Cells are 80x60 pixel blocks that make up the dashboard grid.
 * This handles drawing the cell background, border, and centered text.
 * It handles large 2x2 cells differently, drawing kWh and amp
 * values instead of generic text.
 *
 * @param x The X grid position of the cell
 * @param y The Y grid position of the cell
 * @param w The width of the cell in grid blocks
 * @param h The height of the cell in grid blocks
 * @param text The text/value to draw in the cell
 * @param desc Small description text
 * @param bgColor The background color of the cell
 * @param fgColor The foreground color of the text
 */
void Board320_240::drawBigCell(int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const char *desc, uint16_t bgColor, uint16_t fgColor)
{

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 60) + 1;

  spr.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1, bgColor);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);
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
 * Draw small rect 80x30
 */
void Board320_240::drawSmallCell(int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const char *desc, int16_t bgColor, int16_t fgColor)
{
  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 32) + 1;

  spr.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32), bgColor);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 32) - 1, h * 32, TFT_BLACK);
  spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 32) - 1, w * 80, TFT_BLACK);
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
 * Draws tire pressure and temperature info in a 2x2 grid cell.
 *
 * @param x The x grid position
 * @param y The y grid position
 * @param w The width in grid cells
 * @param h The height in grid cells
 * @param topleft The top left text
 * @param topright The top right text
 * @param bottomleft The bottom left text
 * @param bottomright The bottom right text
 * @param color The background color
 */
void Board320_240::showTires(int32_t x, int32_t y, int32_t w, int32_t h, const char *topleft, const char *topright, const char *bottomleft, const char *bottomright, uint16_t color)
{
  int32_t posx, posy;

  spr.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1, color);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);

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
 * Draws the main screen (screen 1) with live vehicle data.
 *
 * This shows the most important live data values in a grid layout.
 * It includes:
 * - Tire pressures and temperatures
 * - Battery power usage
 * - State of charge percentage
 * - Battery current
 * - Battery voltage
 * - Minimum battery cell voltage
 * - Battery temperature
 * - Auxiliary battery percentage
 * - Auxiliary battery current
 * - Auxiliary battery voltage
 * - Indoor and outdoor temperatures
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
  if (liveData->params.speedKmh > 20 ||
      (liveData->params.speedKmh == -1 && liveData->params.speedKmhGPS > 20 && liveData->params.gpsSat >= 4))
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
 * Draws the speed and energy usage related screen. (Screen 2)
 *
 * This displays the current speed, average speed, energy usage,
 * odometer, trip meter, and other driving related metrics.
 *
 * It handles formatting and coloring based on value thresholds.
 */
void Board320_240::drawSceneSpeed()
{
  int32_t posx, posy;

  posx = 320 / 2;
  posy = 40;
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor(TFT_WHITE);

  // Stopped CAN command queue
  if (liveData->params.stopCommandQueueTime != 0)
  {
    sprintf(tmpStr1, "%s%d", (liveData->params.stopCommandQueue ? "QS " : "QR "), (liveData->params.currentTime - liveData->params.stopCommandQueueTime));
    spr.drawString(tmpStr1, 0, 40, 2);
  }
  if (liveData->params.stopCommandQueue) 
  {
    spr.setFreeFont(&Roboto_Thin_24);
    spr.setTextColor(TFT_RED);
    spr.setTextDatum(TL_DATUM);
    spr.drawString("SENTRY ON", 90, 100, GFXFF);
    return;
  }

  // Speed or charging data
  if (liveData->params.chargingOn)
  {
    // Charging voltage, current, time
    posy = 40;
    spr.setFreeFont(&Roboto_Thin_24);
    spr.setTextDatum(TR_DATUM); // Top center
    spr.setTextColor(TFT_WHITE);
    time_t diffTime = liveData->params.currentTime - liveData->params.chargingStartTime;
    if ((diffTime / 60) > 99)
      sprintf(tmpStr3, "%02ld:%02ld:%02ld", (diffTime / 3600) % 24, (diffTime / 60) % 60, diffTime % 60);
    else
      sprintf(tmpStr3, "%02ld:%02ld", (diffTime / 60), diffTime % 60);
    spr.drawString(tmpStr3, 200, posy, GFXFF);
    posy += 24;
    sprintf(tmpStr3, (liveData->params.batVoltage == -1000) ? "n/a V" : "%01.01f V", liveData->params.batVoltage);
    spr.drawString(tmpStr3, 200, posy, GFXFF);
    posy += 24;
    sprintf(tmpStr3, (liveData->params.batPowerAmp == -1000) ? "n/a A" : "%01.01f A", liveData->params.batPowerAmp);
    spr.drawString(tmpStr3, 200, posy, GFXFF);
    posy += 24;
    if (diffTime > 5) { 
      sprintf(tmpStr3, "~%01.01f kW", ((liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart) * (3600 / diffTime)));
      spr.drawString(tmpStr3, 200, posy, GFXFF);
    }
    
  }
  else
  {
    // Speed
    spr.setTextSize(2);
    sprintf(tmpStr3, "%01.00f", liveData->km2distance(
      (((liveData->params.speedKmhGPS > 10 && liveData->settings.carSpeedType == CAR_SPEED_TYPE_AUTO) || liveData->settings.carSpeedType == CAR_SPEED_TYPE_GPS) ? 
          liveData->params.speedKmhGPS : 
          ((((liveData->params.speedKmh > 10 && liveData->settings.carSpeedType == CAR_SPEED_TYPE_AUTO) || liveData->settings.carSpeedType == CAR_SPEED_TYPE_CAR)) ? 
             liveData->params.speedKmh : 0))
      ));
    spr.drawString(tmpStr3, 200, posy, 7);
  }

  posy = 140;
  spr.setTextDatum(TR_DATUM); // Top center
  spr.setTextSize(1);
  if ((liveData->params.speedKmh > 25 || (liveData->params.speedKmhGPS > 25 && liveData->params.gpsSat >= 4)) && liveData->params.batPowerKw < 0)
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
  sprintf(tmpStr3, "N");
  if (liveData->params.forwardDriveMode)
  {
    if (liveData->params.odoKm > 0 && liveData->params.odoKmStart > 0 &&
        (liveData->params.odoKm - liveData->params.odoKmStart) > 0 && liveData->params.speedKmhGPS < 150)
    {
      sprintf(tmpStr3, ((liveData->settings.distanceUnit == 'k') ? "%01.00f" : "%01.00f"), liveData->km2distance(liveData->params.odoKm - liveData->params.odoKmStart));
    }
    else
    {
      sprintf(tmpStr3, "D");
    }
  }
  else if (liveData->params.reverseDriveMode)
  {
    sprintf(tmpStr3, "R");
  }
  if (liveData->params.chargingOn)
  {
    sprintf(tmpStr3, "AC/DC");
  }
  if (liveData->params.chargerACconnected)
  {
    sprintf(tmpStr3, "AC");
  }
  if (liveData->params.chargerDCconnected)
  {
    sprintf(tmpStr3, "DC");
  }
  spr.drawString(tmpStr3, posx, posy + 20, GFXFF);

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

  // Avg speed
  posy = 60;
  sprintf(tmpStr3, "%01.00f", liveData->params.avgSpeedKmh);
  spr.setTextDatum(TL_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  spr.drawString("avg.km/h", 0, posy + 20, 2);
  posy += 40;

  // AUX voltage
  if (liveData->params.auxVoltage > 5 && liveData->params.speedKmh < 100 && liveData->params.speedKmhGPS < 100)
  {
    if (liveData->params.auxPerc != -1)
    {
      sprintf(tmpStr3, "%01.00f", liveData->params.auxPerc);
      spr.setTextDatum(TL_DATUM);
      spr.drawString(tmpStr3, posx, posy, GFXFF);
      spr.drawString("aux %", 0, posy + 20, 2);
      posy += 40;
    }
    sprintf(tmpStr3, "%01.01f", liveData->params.auxVoltage);
    spr.setTextDatum(TL_DATUM);
    spr.drawString(tmpStr3, posx, posy, GFXFF);
    spr.drawString("aux V", 0, posy + 20, 2);
    posy += 40;
  }

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

/**
 * Draws the heads-up display (HUD) scene on the board's 320x240 display.
 *
 * The HUD shows key driving information like speed, power, SOC. It is
 * optimized for vertical orientation while driving.
 */
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
  if (liveData->params.speedKmhGPS > 10 && liveData->params.gpsSat >= 4)
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
 * Draws the battery cells screen. (Screen 3)
 *
 * Displays the heater, inlet, and module temperatures.
 * Then displays the voltage for each cell.
 * Cell colors indicate temperature or min/max voltage.
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
 * Charging graph (Screen 4)
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
 * Draws the SOC 10% table screen. (screen 5)
 *
 * Displays discharge consumption and characteristics from 100% to 4% SOC,
 * in 10% increments. Calculates differences between SOC levels for discharge
 * energy consumed, charge energy consumed, odometer distance, and speed.
 * Prints totals for 0-100% and estimated available capacity.
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
 * Draws debug information to the screen. (screen 6)
 *
 * Displays diagnostic values for debugging purposes:
 * - App version
 * - Settings version
 * - Frames per second
 * - Wifi status
 * - Remote upload module status
 * - SD card status
 * - Voltmeter status
 * - Sleep mode
 * - GPS status
 * - Current time
 */
void Board320_240::drawSceneDebug()
{
  String tmpStr;

  spr.setTextFont(2);
  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextColor(TFT_SILVER);
  spr.setTextDatum(TL_DATUM);

  spr.setCursor(0, 0, 2);

  /* Spotzify [SE]: Diagnostic values I would love to have :
FPS, or more specific time for one total loop of program.
COMMU stats
GPS stats
Time and date
GSM status
ABRP status
SD status*/

  // BASIC INFO
  spr.print("APP ");
  spr.print(APP_VERSION);
  spr.print(" | Settings v");
  spr.print(liveData->settings.settingsVersion);
  spr.print(" | FPS ");
  spr.println(displayFps);

  // TODO Cartype liveData->settings.carType - translate car number to string
  // TODO Adapter type liveData->settings.commType == COMM_TYPE_OBD2BLE4
  // TODO data from adapter

  // WIFI
  spr.print("WIFI ");
  spr.print(liveData->settings.wifiEnabled == 1 ? "ON" : "OFF");
  // if (liveData->params.isWifiBackupLive == true)
  spr.print(" IP ");
  spr.print(WiFi.localIP().toString());
  spr.print(" SSID ");
  spr.println(liveData->settings.wifiSsid);

  // REMOTE UPLOAD
  spr.print("REMOTE UPLOAD ");
  switch (liveData->settings.remoteUploadModuleType)
  {
  case REMOTE_UPLOAD_OFF:
    spr.print("OFF");
    break;
  case REMOTE_UPLOAD_WIFI:
    spr.print("WIFI");
    break;
  default:
    spr.print("unknown");
  }

  spr.println("");
  spr.print("CarMode: ");
  spr.println(liveData->params.carMode);

  spr.print("Power (kW): ");
  spr.println(liveData->params.batPowerKw * -1);

  spr.print("ignitionOn: ");
  spr.println(liveData->params.ignitionOn == 1 ? "ON" : "OFF");

  spr.print("chargingOn: ");
  spr.println(liveData->params.chargingOn == 1 ? "ON" : "OFF");

  spr.print("AC charger connected: ");
  spr.println(liveData->params.chargerACconnected == 1 ? "ON" : "OFF");

  spr.print("DC charger connected: ");
  spr.println(liveData->params.chargerDCconnected == 1 ? "ON" : "OFF");

  spr.print("Forward drive mode: : ");
  spr.println(liveData->params.forwardDriveMode == 1 ? "ON" : "OFF");

  spr.print("Reverse drive mode: : ");
  spr.println(liveData->params.reverseDriveMode == 1 ? "ON" : "OFF");

  /*
    jsonData["power"] = liveData->params.batPowerKw * -1;
      jsonData["is_parked"] = (liveData->params.parkModeOrNeutral) ? 1 : 0;

    bool ignitionOn;
    bool chargingOn;
    bool chargerACconnected;
    bool chargerDCconnected;
    bool forwardDriveMode;
    bool reverseDriveMode;
    bool parkModeOrNeutral;
    */

  // TODO sent status, ms from last sent
  // spr.println("");

  // SDCARD
  spr.print("SDCARD ");
  spr.print((liveData->settings.sdcardEnabled == 0) ? "OFF" : (strlen(liveData->params.sdcardFilename) != 0) ? "WRITE"
                                                          : (liveData->params.sdcardInit)                    ? "READY"
                                                                                                             : "MOUNTED");
  spr.print(" used ");
  spr.print(SD.usedBytes() / 1048576);
  spr.print("/");
  spr.print(SD.totalBytes() / 1048576);
  spr.println("MB");

  // VOLTMETER INA3221
  spr.print("VOLTMETER ");
  spr.println(liveData->settings.voltmeterEnabled == 1 ? "ON" : "OFF");
  if (liveData->settings.voltmeterEnabled == 1)
  {
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

  // SLEEP MODE
  spr.print("SLEEP MODE ");
  switch (liveData->settings.sleepModeLevel)
  {
  case SLEEP_MODE_OFF:
    spr.print("OFF");
    break;
  case SLEEP_MODE_SCREEN_ONLY:
    spr.print("SCREEN ONLY");
    break;
  case SLEEP_MODE_DEEP_SLEEP:
    spr.print("DEEP SLEEP");
    break;
  case SLEEP_MODE_SHUTDOWN:
    spr.print("SHUTDOWN");
    break;
  default:
    spr.print("UNKNOWN");
  }
  spr.println("");

  // GPS
  spr.print("GPS ");
  spr.print(liveData->params.gpsValid ? "OK" : "-");
  if (liveData->params.gpsValid)
  {
    spr.print(" ");
    spr.print(liveData->params.gpsLat);
    spr.print("/");
    spr.print(liveData->params.gpsLon);
    spr.print(" alt ");
    spr.print(liveData->params.gpsAlt);
    spr.print("m sat ");
    spr.print(liveData->params.gpsSat);
  }
  spr.println("");

  // CURRENT TIME
  spr.print("TIME ");
  spr.print(liveData->params.ntpTimeSet == 1 ? " NTP " : "");
  spr.print(liveData->params.currTimeSyncWithGps == 1 ? "GPS " : "");
  struct tm now;
  getLocalTime(&now);
  sprintf(tmpStr1, "%02d-%02d-%02d %02d:%02d:%02d", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
  spr.println(tmpStr1);
}

/**
 * Modify menu item text
 */
String Board320_240::menuItemText(int16_t menuItemId, String title)
{
  String prefix = "", suffix = "";
  uint8_t perc = 0;

  switch (menuItemId)
  {
  // Set vehicle type
  case MENU_VEHICLE_TYPE:
    suffix = getCarModelAbrpStr();
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
  case VEHICLE_TYPE_HYUNDAI_IONIQ5_58:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_58) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ5_72:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_72) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ5_77:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_77) ? ">" : "";
    break;
  case VEHICLE_TYPE_HYUNDAI_IONIQ6_77:
    prefix = (liveData->settings.carType == CAR_HYUNDAI_IONIQ6_77) ? ">" : "";
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
  spr.setFreeFont(&Roboto_Thin_24);

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
        spr.drawString(menuItemText(liveData->menuItems[i].id, liveData->menuItems[i].title), 0, posY + off, GFXFF);
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
    case VEHICLE_TYPE_HYUNDAI_IONIQ6_77:
      liveData->settings.carType = CAR_HYUNDAI_IONIQ6_77;
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
    case MENU_GPS_MODULE_TYPE:
      liveData->settings.gpsModuleType = (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_M5_GNSS) ? GPS_MODULE_TYPE_NONE : liveData->settings.gpsModuleType + 1;
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

/**
 * Redraws the screen based on the current display mode and parameters.
 * Handles switching between different display screens and rendering status indicators.
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
    else if (liveData->params.speedKmh > 15 || (liveData->params.speedKmhGPS > 15 && liveData->params.gpsSat >= 4))
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
  // 7. Debug screen
  case SCREEN_DEBUG:
    drawSceneDebug();
    break;
  // 8. HUD
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
                   (liveData->params.sdcardInit) ? (liveData->params.sdcardRecording) ? (strlen(liveData->params.sdcardFilename) != 0) ? TFT_GREEN /* assigned filename (opsec from bms or gsm/gps timestamp */ : TFT_BLUE /* recording started but waiting for data */ : TFT_ORANGE /* sdcard init ready but recording not started*/ : TFT_YELLOW /* failed to initialize sdcard */
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

  // Ignition ON, Gyro motion
  if (liveData->params.ignitionOn) 
  {
    spr.fillRect(130, 0, 2, 14, TFT_GREEN);
  }
  if (liveData->params.gyroSensorMotion) 
  {
    spr.fillRect(120, 0, 4, 20, TFT_ORANGE);
  }

  // WiFi Status
  if (liveData->settings.wifiEnabled == 1 && liveData->settings.remoteUploadModuleType == 1)
  {
    if (liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED)
    {
      spr.fillRect(140, 7, 7, 7,
                   (WiFi.status() == WL_CONNECTED) ? (liveData->params.lastSuccessNetSendTime + tmp_send_interval >= liveData->params.currentTime) ? TFT_GREEN /* last request was 200 OK */ : TFT_YELLOW /* wifi connected but not send */ : TFT_RED /* wifi not connected */
      );
      if (liveData->params.isWifiBackupLive)
      {
        spr.fillRect(140, 0, 7, 3,
                     (WiFi.status() == WL_CONNECTED) ? (liveData->params.lastSuccessNetSendTime + tmp_send_interval >=
                                                        liveData->params.currentTime)
                                                           ? TFT_GREEN  /* last request was 200 OK */
                                                           : TFT_YELLOW /* wifi connected but not send */
                                                     : TFT_RED          /* wifi not connected */
        );
      }
    }
    else if (liveData->params.displayScreen != SCREEN_BLANK)
    {
      spr.fillRect(308, 0, 5, 5,
                   (WiFi.status() == WL_CONNECTED) ? (liveData->params.lastSuccessNetSendTime + tmp_send_interval >= liveData->params.currentTime) ? TFT_GREEN /* last request was 200 OK */ : TFT_YELLOW /* wifi connected but not send */ : TFT_RED /* wifi not connected */
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
  if ((liveData->settings.commType == COMM_TYPE_OBD2_BLE4 ||
       liveData->settings.commType == COMM_TYPE_OBD2_WIFI) &&
      !liveData->commConnected && liveData->obd2ready)
  {
    // Print message
    spr.fillRect(0, 185, 220, 50, TFT_BLACK);
    spr.drawRect(0, 185, 220, 50, TFT_WHITE);
    spr.setTextSize(1);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(TFT_WHITE);
    sprintf(tmpStr1, "OBDII not connected #%d", commInterface->getConnectAttempts());
    spr.drawString(tmpStr1, 10, 190, 2);
    spr.drawString(commInterface->getConnectStatus(), 10, 210, 2);
  }
  else
    // CAN not connected
    if (liveData->settings.commType == COMM_TYPE_CAN_COMMU && commInterface->getConnectStatus() != "")
    {
      // Print message
      spr.fillRect(0, 185, 160, 50, TFT_BLACK);
      spr.drawRect(0, 185, 160, 50, TFT_WHITE);
      spr.setTextSize(1);
      spr.setTextDatum(TL_DATUM);
      spr.setTextColor(TFT_WHITE);
      sprintf(tmpStr1, "CAN #%d %s%d", commInterface->getConnectAttempts(), (liveData->params.stopCommandQueue ? "QS" : "QR"), liveData->params.queueLoopCounter);
      spr.drawString(tmpStr1, 10, 190, 2);
      spr.drawString(commInterface->getConnectStatus(), 10, 210, 2);
    }

  spr.pushSprite(0, 0);
  redrawScreenIsRunning = false;
}

/**
 * Parse test data
 */
void Board320_240::loadTestData()
{
  syslog->println("Loading test data");

  testDataMode = true; // skip lights off message
  carInterface->loadTestData();
  redrawScreen();
}

/**
 * Print heap memory to serial console
 */
void Board320_240::printHeapMemory()
{
  syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());
}

/**
 * Task that continuously calls the communication loop function.
 * This allows the communication code to run asynchronously in the background.
 *
 * @param pvParameters Pointer to the BoardInterface instance.
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
 * commLoop function - This function runs the main communication loop.
 * It calls the commInterface's mainLoop() method to read data from
 * BLE and CAN interfaces. This allows the communication code to run
 * continuously in the background.
 */
void Board320_240::commLoop()
{
  // Read data from BLE/CAN
  commInterface->mainLoop();
}

/**
 * Board loop function.
 * This function runs in the main loop to handle touch events
 * and other board specific tasks.
 */
void Board320_240::boardLoop()
{
  // touch events, m5.update
#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
  if (webInterface != nullptr)
    webInterface->mainLoop();
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3
}

/**
 * Main loop - primary thread
 */
void Board320_240::mainLoop()
{
  // Calculate FPS
  float timeDiff = (millis() - mainLoopStart);
  displayFps = (timeDiff == 0 ? 0 : (1000 / (millis() - mainLoopStart)));
  mainLoopStart = millis();

  // board loop
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
    if (gpsHwUart->available())
    {
      do
      {
        int ch = gpsHwUart->read();
        if (ch != -1)
          syslog->infoNolf(DEBUG_GPS, char(ch));
        gps.encode(ch);
      } while (gpsHwUart->available());
      syncGPS();
    }
  }
  else
  {
    // MEB CAR GPS
    if (liveData->params.gpsValid && liveData->params.gpsLat != -1.0 && liveData->params.gpsLon != -1.0)
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
  if (!liveData->params.stopCommandQueue && liveData->settings.commType != COMM_TYPE_OBD2_WIFI && !liveData->params.wifiApMode && liveData->settings.wifiEnabled == 1 && 
      WiFi.status() != WL_CONNECTED && liveData->params.currentTime - liveData->params.wifiLastConnectedTime > 60 && liveData->settings.remoteUploadModuleType == 1)
  {
      wifiFallback();
  }

  // SIM800L, WiFI remote upload, ABRP remote upload, MQTT
  netLoop();

  // SD card recording
  if (!liveData->params.stopCommandQueue && liveData->params.sdcardInit && liveData->params.sdcardRecording && liveData->params.sdcardCanNotify &&
      (liveData->params.odoKm != -1 && liveData->params.socPerc != -1))
  {
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
  if (liveData->settings.voltmeterEnabled == 1 && liveData->params.currentTime - liveData->params.lastVoltageReadTime > 5)
  {
    liveData->params.auxVoltage = ina3221.getBusVoltage_V(1);
    liveData->params.lastVoltageReadTime = liveData->params.currentTime;
    if (liveData->params.auxVoltage > liveData->settings.voltmeterSleep)
    {
      liveData->params.lastVoltageOkTime = liveData->params.currentTime;
    }

    // Protect AUX battery in screen only mode
    if (liveData->settings.sleepModeLevel == SLEEP_MODE_SCREEN_ONLY &&
        liveData->params.auxVoltage > 5 && liveData->params.auxVoltage < liveData->settings.voltmeterCutOff)
    {
      syslog->print("AUX voltage under cut-off voltage: ");
      syslog->println(liveData->settings.voltmeterCutOff);
      shutdownDevice();
    }

    // Calculate AUX perc for ioniq2018
    if (liveData->settings.carType == CAR_HYUNDAI_IONIQ_2018)
    {
      float tmpAuxPerc = (float)(liveData->params.auxVoltage - 11.6) * 100 / (float)(12.8 - 11.6); // min 11.6V; max: 12.8V
      liveData->params.auxPerc = ((tmpAuxPerc > 100) ? 100 : ((tmpAuxPerc < 0) ? 0 : tmpAuxPerc));
    }
  }

  // Wake up from stopped command queue
  //  - any touch on display
  //  - ignitions on and aux >= 11.5v
  //  - ina3221 & voltage is >= 14V (DCDC is running) 
  //  - gps speed 20 - 60kmh & 8+ satellites
  if (
      (liveData->params.ignitionOn && (liveData->params.auxVoltage <= 3 || liveData->params.auxVoltage >= 11.5)) ||
      (liveData->settings.voltmeterEnabled == 1 && liveData->params.auxVoltage > 14.0) ||
      (liveData->params.gyroSensorMotion) ||
      (liveData->params.speedKmhGPS >= 20 && liveData->params.speedKmhGPS <= 60 && liveData->params.gpsSat >= 8) // 5 floor parking house, satelites 5 & gps speed = 274kmh :/
      ) 
  {
    liveData->continueWithCommandQueue();
  }
  // Stop command queue
  //  - automatically turns off CAN scanning after 1-2 minutes of inactivity
  //  - ignition is off
  //  - AUX voltage is under 11.5V
  if (liveData->settings.commandQueueAutoStop == 1 &&
      ((!liveData->params.ignitionOn && !liveData->params.chargingOn) || (liveData->params.auxVoltage > 3 && liveData->params.auxVoltage < 11.5))
      )
  {
    liveData->prepareForStopCommandQueue();
  }
  if (!liveData->params.stopCommandQueue &&
      (
        (liveData->params.stopCommandQueueTime != 0 && liveData->params.currentTime - liveData->params.stopCommandQueueTime > 60) || 
        (liveData->params.auxVoltage > 3 && liveData->params.auxVoltage < 11.0)
      )
     )
  {
    liveData->params.stopCommandQueue = true;
    syslog->println("CAN Command queue stopped...");
  }
  // Descrease loop fps
  if (liveData->params.stopCommandQueue) {
    delay(250);
  }

  // Automatic sleep after inactivity
  if (liveData->params.currentTime - liveData->params.lastIgnitionOnTime > 10 &&
      liveData->settings.sleepModeLevel >= SLEEP_MODE_SCREEN_ONLY &&
      liveData->params.currentTime - liveData->params.lastButtonPushedTime > 30 &&
      (liveData->params.currentTime - liveData->params.wakeUpTime > 60 || bootCount > 1))
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
  if (liveData->settings.voltmeterEnabled == 1 && liveData->settings.voltmeterBasedSleep == 1 &&
      liveData->params.auxVoltage > 0 && liveData->params.currentTime - liveData->params.lastVoltageOkTime > 30 &&
      liveData->params.currentTime - liveData->params.wakeUpTime > 30 && liveData->params.currentTime - liveData->params.lastButtonPushedTime > 10)
  {
    // eGMP Ioniq6 DC2DC is sometimes not active in forward driving mode and evDash then turn off screen
    if (!liveData->params.forwardDriveMode && !liveData->params.reverseDriveMode)
    {
      if (liveData->settings.sleepModeLevel >= SLEEP_MODE_DEEP_SLEEP)
        goToSleep();
    }
  }

  // Read data from BLE/CAN
  if (!liveData->settings.threading)
  {
    commInterface->mainLoop();
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
  else if (liveData->params.forwardDriveMode &&
           (liveData->params.speedKmh > 15 || (liveData->params.speedKmhGPS > 15 && liveData->params.gpsSat >= 4)) && liveData->params.carMode != CAR_MODE_DRIVE)
  {
    liveData->clearDrivingAndChargingStats(CAR_MODE_DRIVE);
  }
  else if (!liveData->params.chargingOn && !liveData->params.forwardDriveMode && liveData->params.carMode != CAR_MODE_NONE &&
           liveData->params.currentTime - liveData->params.carModeChanged > 1800 &&
           liveData->params.currentTime - liveData->params.carModeChanged < 10 * 24 * 3600)
  {
    liveData->clearDrivingAndChargingStats(CAR_MODE_NONE);
  }
}

/**
 * Set the RTC time using the provided GPS time.
 * Converts the provided date/time components into a UNIX timestamp,
 * sets the system time using settimeofday(), and syncs other times.
 * Also sets the time on the M5Stack Core2 RTC module if being used.
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
#endif // BOARD_M5STACK_CORE2
#ifdef BOARD_M5STACK_CORES3
  CoreS3.Rtc.setDateTime(gmtime(&t));
#endif // BOARD_M5STACK_CORES3
}

/**
 * Syncs related timestamp variables to the provided new time.
 * This is called after setting the system time, to update timestamp
 * variables that are relative to the current time. It calculates the
 * offset between the old current time and new current time, and adjusts
 * each timestamp variable by that offset.
 */
void Board320_240::syncTimes(time_t newTime)
{
  if (liveData->params.chargingStartTime != 0)
    liveData->params.chargingStartTime = newTime - (liveData->params.currentTime - liveData->params.chargingStartTime);

  if (liveData->params.lastDataSent != 0)
    liveData->params.lastDataSent = newTime - (liveData->params.currentTime - liveData->params.lastDataSent);

  if (liveData->params.lastContributeSent != 0)
    liveData->params.lastContributeSent = newTime - (liveData->params.currentTime - liveData->params.lastContributeSent);

  if (liveData->params.lastSuccessNetSendTime != 0)
    liveData->params.lastSuccessNetSendTime = newTime - (liveData->params.currentTime - liveData->params.lastSuccessNetSendTime);

  if (liveData->params.lastButtonPushedTime != 0)
    liveData->params.lastButtonPushedTime = newTime - (liveData->params.currentTime - liveData->params.lastButtonPushedTime);

  if (liveData->params.wakeUpTime != 0)
    liveData->params.wakeUpTime = newTime - (liveData->params.currentTime - liveData->params.wakeUpTime);

  if (liveData->params.lastIgnitionOnTime != 0)
    liveData->params.lastIgnitionOnTime = newTime - (liveData->params.currentTime - liveData->params.lastIgnitionOnTime);

  if (liveData->params.stopCommandQueueTime != 0)
    liveData->params.stopCommandQueueTime = newTime - (liveData->params.currentTime - liveData->params.stopCommandQueueTime);

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
 * skipAdapterScan
 */
bool Board320_240::skipAdapterScan()
{
  return isButtonPressed(pinButtonMiddle) || isButtonPressed(pinButtonLeft) || isButtonPressed(pinButtonRight);
}

/**
 * Attempts to mount the SD card.
 *
 * Tries initializing the SD card multiple times if needed.
 * Checks the card type and size once mounted.
 * Updates liveData->params.sdcardInit if successful.
 *
 * @returns true if the SD card was mounted successfully, false otherwise.
 */
bool Board320_240::sdcardMount()
{
  if (liveData->params.sdcardInit)
  {
    syslog->println("SD card already mounted...");
    return true;
  }

  bool SdState = false;
  syslog->print("Initializing SD card...");
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

  return false;
}

/**
 * Toggles SD card recording on/off.
 *
 * Checks if SD card is initialized first.
 * Updates sdcardRecording parameter and filename as needed.
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
 * Syncs GPS data from the GPS module to the live data parameters.
 *
 * Updates validity flags and values for GPS location, altitude, speed,
 * satellite info. Handles syncing time from GPS if needed.
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
  if (gps.speed.isValid() && liveData->params.gpsSat >= 4)
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
 * Sets up the WiFi connection using the stored SSID and password.
 * Enables STA mode, connects to the network, and records the time of
 * the last successful connection.
 *
 * @returns True if the WiFi connection setup succeeded, false otherwise.
 */
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
 * Switches between the main and backup WiFi networks.
 *
 * Disconnects from the current WiFi network.
 * If a backup network is configured, switches to
 * the other network. Otherwise, attempts to
 * reconnect to the main network.
 */
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

/**
 * Switch to backup wifi
 */
void Board320_240::wifiSwitchToBackup()
{
  syslog->print("WiFi switchover to backup: ");
  syslog->println(liveData->settings.wifiSsid2);
  WiFi.begin(liveData->settings.wifiSsid2, liveData->settings.wifiPassword2);
  liveData->params.isWifiBackupLive = true;
  liveData->params.wifiBackupUptime = liveData->params.currentTime;
  liveData->params.wifiLastConnectedTime = liveData->params.currentTime;
}

/**
 * Restore main wifi connection
 */
void Board320_240::wifiSwitchToMain()
{
  syslog->print("WiFi switchover to main: ");
  syslog->println(liveData->settings.wifiSsid);
  WiFi.begin(liveData->settings.wifiSsid, liveData->settings.wifiPassword);
  liveData->params.isWifiBackupLive = false;
  liveData->params.wifiLastConnectedTime = liveData->params.currentTime;
}

/**
 * Net loop, send data over net
 * Checks if WiFi is connected, syncs NTP if needed, sends data to remote API if interval elapsed,
 * sends data to ABRP if interval elapsed, contributes anonymous data if enabled.
 */
void Board320_240::netLoop()
{
  if (liveData->params.wifiApMode)
  {
    return;
  }

  bool wifiReady = (liveData->settings.wifiEnabled == 1 && WiFi.status() == WL_CONNECTED);

  // Sync NTP firsttime
  if (wifiReady && !liveData->params.ntpTimeSet && liveData->settings.ntpEnabled)
  {
    ntpSync();
  }

  // Upload to custom API
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

  // Contribute anonymous data
  if (wifiReady && liveData->params.contributeStatus == CONTRIBUTE_NONE && liveData->params.currentTime - liveData->params.lastContributeSent > 60)
  {
    liveData->params.lastContributeSent = liveData->params.currentTime;
    liveData->params.contributeStatus = CONTRIBUTE_WAITING;
  }
  if (wifiReady && liveData->params.contributeStatus == CONTRIBUTE_READY_TO_SEND)
  {
    netContributeData();
  }
}

/**
 * Send data
 **/
bool Board320_240::netSendData()
{
  uint16_t rc = 0;

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
  else
  {
    if (liveData->settings.remoteUploadModuleType != 0)
    {
      syslog->println("Unsupported module");
    }
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
    if ((liveData->settings.gpsHwSerialPort <= 2 && gps.location.isValid() && liveData->params.gpsSat >= 3) || // HW GPS or MEB GPS
        (liveData->settings.gpsHwSerialPort == 255 && liveData->params.gpsLat != -1.0 && liveData->params.gpsLon != -1.0))
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

    // WIFI remote upload
    rc = 0;
    if (liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI && liveData->settings.wifiEnabled == 1)
    {
      // MQTT
      WiFiClient wClient;
      if (liveData->settings.mqttEnabled == 1)
      {
        PubSubClient client(wClient);
        client.setServer(liveData->settings.mqttServer, 1883);
        if (client.connect(liveData->settings.mqttId, liveData->settings.mqttUsername, liveData->settings.mqttPassword))
        {
          char topic[80];
          char tmpVal[20];
          strcpy(topic, liveData->settings.mqttPubTopic);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/socPerc");
          dtostrf(liveData->params.socPerc, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/chargingOn");
          dtostrf(liveData->params.chargingOn, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/batPowerKw");
          dtostrf(liveData->params.batPowerKw, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/batPowerAmp");
          dtostrf(liveData->params.batPowerAmp, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/batVoltage");
          dtostrf(liveData->params.batVoltage, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/auxVoltage");
          dtostrf(liveData->params.auxVoltage, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/batMinC");
          dtostrf(liveData->params.batMinC, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/batMaxC");
          dtostrf(liveData->params.batMaxC, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/extTemp");
          dtostrf(liveData->params.outdoorTemperature, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/speedKmh");
          dtostrf(liveData->params.speedKmh, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/odoKm");
          dtostrf(liveData->params.odoKm, 1, 2, tmpVal);
          client.publish(topic, tmpVal);
          // Send GPS data via GPRS (if enabled && valid)
          if ((liveData->settings.gpsHwSerialPort <= 2 && gps.location.isValid() && liveData->params.gpsSat >= 3) || // HW GPS or MEB GPS
              (liveData->settings.gpsHwSerialPort == 255 && liveData->params.gpsLat != -1.0 && liveData->params.gpsLon != -1.0))
          {
            strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/gpsLat");
            dtostrf(liveData->params.gpsLat, 1, 2, tmpVal);
            client.publish(topic, tmpVal);
            strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/gpsLon");
            dtostrf(liveData->params.gpsLon, 1, 2, tmpVal);
            client.publish(topic, tmpVal);

            strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/gpsSpeed");
            dtostrf(liveData->params.speedKmhGPS, 1, 2, tmpVal);
            client.publish(topic, tmpVal);

            strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/gpsAlt");
            dtostrf(liveData->params.gpsAlt, 1, 2, tmpVal);
            client.publish(topic, tmpVal);
          }
          rc = 200;
        }
      }
      else
      {
        // Standard http post
        HTTPClient http;

        http.begin(wClient, liveData->settings.remoteApiUrl);
        http.setConnectTimeout(500);
        http.addHeader("Content-Type", "application/json");
        rc = http.POST(payload);
        http.end();
      }
    }

    if (rc == 200)
    {
      syslog->println("HTTP POST send successful");
      liveData->params.lastSuccessNetSendTime = liveData->params.currentTime;
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

    jsonData["car_model"] = getCarModelAbrpStr();
    if (strcmp(jsonData["car_model"], "n/a") == 0)
    {
      syslog->println("Car not supported by ABRP Uploader");
      return false;
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
        (liveData->settings.gpsHwSerialPort == 255 && liveData->params.gpsLat != -1.0 && liveData->params.gpsLon != -1.0))
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

    // Log ABRP jsonData to SD card
    struct tm now;
    getLocalTime(&now);
    if (liveData->settings.abrpSdcardLog != 0 && liveData->settings.remoteUploadAbrpIntervalSec > 0 && liveData->params.sdcardInit && liveData->params.sdcardRecording)
    {
      // create filename
      if (liveData->params.operationTimeSec > 0 && strlen(liveData->params.sdcardAbrpFilename) == 0)
      {
        sprintf(liveData->params.sdcardAbrpFilename, "/%llu.abrp.json", uint64_t(liveData->params.operationTimeSec / 60));
      }
      if (liveData->params.currTimeSyncWithGps && strlen(liveData->params.sdcardAbrpFilename) < 20)
      {
        strftime(liveData->params.sdcardAbrpFilename, sizeof(liveData->params.sdcardAbrpFilename), "/%y%m%d%H%M.abrp.json", &now);
      }

      // append buffer, clear buffer & notify state
      if (strlen(liveData->params.sdcardAbrpFilename) != 0)
      {
        File file = SD.open(liveData->params.sdcardAbrpFilename, FILE_APPEND);
        if (!file)
        {
          syslog->println("Failed to open file for appending");
          File file = SD.open(liveData->params.sdcardAbrpFilename, FILE_WRITE);
        }
        if (!file)
        {
          syslog->println("Failed to create file");
        }
        if (file)
        {
          syslog->info(DEBUG_SDCARD, "Save buffer to SD card");
          serializeJson(jsonData, file);
          file.print(",\n");
          file.close();
        }
      }
    }
    // End of ABRP SD card log

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
    syslog->println(dta); // dta is total string sent to ABRP API including api-key and user-token (could be sensitive data to log)

    // Code for sending https data to ABRP api server
    rc = 0;
    if (liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI && liveData->settings.wifiEnabled == 1)
    {
      WiFiClientSecure client;
      HTTPClient http;

      client.setInsecure();

      // http.begin(client, "api.iternio.com", 443, "/1/tlm/send", true);
      http.begin(client, "https://api.iternio.com/1/tlm/send");
      // http.begin(client, "https://api.iternio.com/1/tlm/send");   // test of SSL via HTTPS to API endpoint
      http.setConnectTimeout(1000);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      rc = http.POST(dta);

      if (rc == HTTP_CODE_OK)
      {
        // Request successful
        String payload = http.getString();
        syslog->println("HTTP Response: " + payload);
      }
      else
      {
        // Handle different HTTP status codes
        syslog->println("HTTP Request failed with code: " + String(rc));
      }

      http.end();
    }

    if (rc == 200)
    {
      syslog->println("HTTP POST send successful");
      liveData->params.lastSuccessNetSendTime = liveData->params.currentTime;
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
    syslog->println("Well... This not gonna happen... (Board320_240::netSendData();)"); // Just for debug reasons...
  }

  return true;
}

/**
 * Contributes anonymous usage data if enabled in settings.
 * Sends data via WiFi to https://evdash.next176.sk/api/index.php.
 * Data includes vehicle info, location, temps, voltages, etc.
 * Receives a contribute token if successful.
 * Only called for M5Stack Core2 boards.
 **/
bool Board320_240::netContributeData()
{
  uint16_t rc = 0;

// Only for core2
#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
  // Contribute anonymous data (evdash.next176.sk/api) to project author (nick.n17@gmail.com)
  if (liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI && liveData->settings.wifiEnabled == 1 &&
      liveData->params.contributeStatus == CONTRIBUTE_READY_TO_SEND)
  {
    syslog->println("Contributing anonymous data...");

    WiFiClientSecure client;
    HTTPClient http;
    client.setInsecure();
    http.begin(client, "https://evdash.next176.sk/api/index.php");
    http.setConnectTimeout(1000);
    http.addHeader("Content-Type", "application/json");

    if (liveData->contributeDataJson.charAt(liveData->contributeDataJson.length() - 1) != '}')
    {

      liveData->contributeDataJson += "\"apikey\": \"" + String(liveData->settings.remoteApiKey) + "\", ";
      liveData->contributeDataJson += "\"carType\": \"" + getCarModelAbrpStr() + "\", ";
      liveData->contributeDataJson += "\"ignitionOn\": " + String(liveData->params.ignitionOn) + ", ";
      liveData->contributeDataJson += "\"chargingOn\": " + String(liveData->params.chargingOn) + ", ";
      liveData->contributeDataJson += "\"socPerc\": " + String(liveData->params.socPerc, 0) + ", ";
      if (liveData->params.socPercBms != -1)
        liveData->contributeDataJson += "\"socPercBms\": " + String(liveData->params.socPercBms, 0) + ", ";
      liveData->contributeDataJson += "\"sohPerc\": " + String(liveData->params.sohPerc, 0) + ", ";
      liveData->contributeDataJson += "\"batPowerKw\": " + String(liveData->params.batPowerKw, 3) + ", ";
      liveData->contributeDataJson += "\"batPowerKwh100\": " + String(liveData->params.batPowerKwh100, 3) + ", ";
      liveData->contributeDataJson += "\"batVoltage\": " + String(liveData->params.batVoltage, 0) + ", ";
      liveData->contributeDataJson += "\"auxVoltage\": " + String(liveData->params.auxVoltage, 0) + ", ";
      liveData->contributeDataJson += "\"auxCurrentAmp\": " + String(liveData->params.auxCurrentAmp, 0) + ", ";
      liveData->contributeDataJson += "\"batMinC\": " + String(liveData->params.batMinC, 0) + ", ";
      liveData->contributeDataJson += "\"batMaxC\": " + String(liveData->params.batMaxC, 0) + ", ";
      liveData->contributeDataJson += "\"inTemp\": " + String(liveData->params.indoorTemperature, 0) + ", ";
      liveData->contributeDataJson += "\"extTemp\": " + String(liveData->params.outdoorTemperature, 0) + ", ";
      liveData->contributeDataJson += "\"speedKmh\": " + String(liveData->params.speedKmh, 0) + ", ";
      liveData->contributeDataJson += "\"odoKm\": " + String(liveData->params.odoKm, 0) + ", ";
      liveData->contributeDataJson += "\"cecKWh\": " + String(liveData->params.cumulativeEnergyChargedKWh, 3) + ", ";
      liveData->contributeDataJson += "\"cedKWh\": " + String(liveData->params.cumulativeEnergyDischargedKWh, 3) + ", ";
      // Send GPS data via GPRS (if enabled && valid)
      if ((liveData->settings.gpsHwSerialPort <= 2 && gps.location.isValid() && liveData->params.gpsSat >= 3) || // HW GPS or MEB GPS
          (liveData->settings.gpsHwSerialPort == 255 && liveData->params.gpsLat != -1.0 && liveData->params.gpsLon != -1.0))
      {
        liveData->contributeDataJson += "\"gpsLat\": " + String(liveData->params.gpsLat, 5) + ", ";
        liveData->contributeDataJson += "\"gpsLon\": " + String(liveData->params.gpsLon, 5) + ", ";
        liveData->contributeDataJson += "\"gpsAlt\": \"" + String(liveData->params.gpsAlt, 0) + "\", ";
        liveData->contributeDataJson += "\"gpsSpeed\": " + String(liveData->params.speedKmhGPS, 0) + ", ";
      }

      liveData->contributeDataJson += "\"token\": \"" + String(liveData->settings.contributeToken) + "\"";
      liveData->contributeDataJson += "}";
    }

    rc = http.POST(liveData->contributeDataJson);
    if (rc == HTTP_CODE_OK)
    {
      // Request successful
      liveData->params.contributeStatus = CONTRIBUTE_NONE;
      liveData->contributeDataJson = "{"; // begin json
      String payload = http.getString();
      syslog->println("HTTP Response (api.next176.sk): " + payload);

      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error)
      {
        // const char* t = root["token"];
        const char *token = doc["token"];
        if (strcmp(liveData->settings.contributeToken, token) != 0 && strlen(token) > 10)
        {
          syslog->print("Assigned token: ");
          syslog->println(token);
          strcpy(liveData->settings.contributeToken, doc["token"]);
          saveSettings();
        }
  
        liveData->params.lastSuccessNetSendTime = liveData->params.currentTime;
      }
    }
    else
    {
      // Failed...
      syslog->print("HTTP POST error: ");
      syslog->println(rc);
    }
  }
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3

  return true;
}

/**
 * Returns the ABRP car model string for the selected car type.
 *
 * Maps the internal car type enum to the corresponding ABRP car model string.
 * This allows looking up the car capabilities on ABRP.
 *
 * @param carType The internal car type enum.
 * @return The ABRP car model string.
 */
String Board320_240::getCarModelAbrpStr()
{
  switch (liveData->settings.carType)
  {
  case CAR_HYUNDAI_KONA_2020_39:
    return "hyundai:kona:19:39:other";
  case CAR_HYUNDAI_IONIQ_2018:
    return "hyundai:ioniq:17:28:other";
  case CAR_HYUNDAI_IONIQ_PHEV:
    return "hyundai:phev:17:28:other";
  case CAR_HYUNDAI_IONIQ5_58:
    return "hyundai:ioniq5:21:58:mr";
  case CAR_HYUNDAI_IONIQ5_72:
    return "hyundai:ioniq5:21:72:lr";
  case CAR_HYUNDAI_IONIQ5_77:
    return "hyundai:ioniq5:21:77:lr";
  case CAR_HYUNDAI_IONIQ6_77:
    return "hyundai:ioniq6:23:77:lr";
  case CAR_KIA_ENIRO_2020_64:
    return "kia:niro:19:64:other";
  case CAR_KIA_ESOUL_2020_64:
    return "kia:soul:19:64:other";
  case CAR_HYUNDAI_KONA_2020_64:
    return "hyundai:kona:19:64:other";
  case CAR_KIA_ENIRO_2020_39:
    return "kia:niro:19:39:other";
  case CAR_KIA_EV6_58:
    return "kia:ev6:22:58:mr";
  case CAR_KIA_EV6_77:
    return "kia:ev6:22:77:lr";
  case CAR_AUDI_Q4_35:
    return "audi:q4:21:52:meb";
  case CAR_AUDI_Q4_40:
    return "audi:q4:21:77:meb";
  case CAR_AUDI_Q4_45:
    return "audi:q4:21:77:meb";
  case CAR_AUDI_Q4_50:
    return "audi:q4:21:77:meb";
  case CAR_SKODA_ENYAQ_55:
    return "skoda:enyaq:21:52:meb";
  case CAR_SKODA_ENYAQ_62:
    return "skoda:enyaq:21:55:meb";
  case CAR_SKODA_ENYAQ_82:
    return "skoda:enyaq:21:77:meb";
  case CAR_VW_ID3_2021_45:
    return "volkswagen:id3:20:45:sr";
  case CAR_VW_ID3_2021_58:
    return "volkswagen:id3:20:58:mr";
  case CAR_VW_ID3_2021_77:
    return "volkswagen:id3:20:77:lr";
  case CAR_VW_ID4_2021_45:
    return "volkswagen:id4:20:45:sr"; // not valid in the iterno list of cars
  case CAR_VW_ID4_2021_58:
    return "volkswagen:id4:21:52";
  case CAR_VW_ID4_2021_77:
    return "volkswagen:id4:21:77";
  case CAR_RENAULT_ZOE:
    return "renault:zoe:r240:22:other";
  case CAR_BMW_I3_2014:
    return "bmw:i3:14:22:other";
  default:
    return "n/a";
  }
}

/**
 * Initializes the hardware GPS module.
 * Configures the hardware serial port and baud rate.
 * Sends configuration commands to the GPS module to enable desired sentences,
 * set update rate, enable SBAS, and configure navigation model.
 */
void Board320_240::initGPS()
{
  syslog->print("GPS initialization on hwUart: ");
  syslog->println(liveData->settings.gpsHwSerialPort);

  gpsHwUart = new HardwareSerial(liveData->settings.gpsHwSerialPort);
  if (liveData->settings.gpsHwSerialPort == 2)
  {
    gpsHwUart->begin(liveData->settings.gpsSerialPortSpeed, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);
  }
  else
  {
    gpsHwUart->begin(liveData->settings.gpsSerialPortSpeed);
  }

  // M5 GPS MODULE with int.&ext. antenna (u-blox NEO-M8N)
  // https://shop.m5stack.com/products/gps-module
  if (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_NEO_M8N || liveData->settings.gpsModuleType == GPS_MODULE_TYPE_M5_GNSS)
  {
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
}
