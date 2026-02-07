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
#include <math.h>
#include "config.h"
#include "BoardInterface.h"
#include "Board320_240.h"
#include <time.h>
#include <ArduinoJson.h>
#include "CarModelUtils.h"

#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
#include <PubSubClient.h>
#include "WebInterface.h"
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3

#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
WebInterface *webInterface = nullptr;
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3

namespace
{
  struct NetSendJob
  {
    bool sendAbrp;
  };

  constexpr uint32_t kNetRetryIntervalSec = 30;
  constexpr uint32_t kNetFailureFallbackSec = 180;
  constexpr uint16_t kNetFailureFallbackCount = 3;
  constexpr size_t kAbrpPayloadBufferSize = 768;
  constexpr size_t kAbrpFormBufferSize = 1536;
  constexpr float kGpsMaxSpeedKmh = 250.0f;
  constexpr float kGpsJitterMeters = 200.0f;
  constexpr float kGpsMaxJumpMetersShort = 2000.0f;
  constexpr uint32_t kGpsShortJumpWindowSec = 5;
  constexpr uint32_t kGpsReacquireAfterSec = 900;
  constexpr uint32_t kMotionWakeResetSec = 900;

  struct AbrpLogEntry
  {
    char payload[kAbrpPayloadBufferSize];
    size_t length;
    time_t currentTime;
    uint64_t operationTimeSec;
    bool timeSyncWithGps;
  };

  size_t encodeQuotes(char *dest, size_t destSize, const char *source)
  {
    size_t written = 0;
    while (*source != '\0')
    {
      if (*source == '"')
      {
        if (written + 3 >= destSize)
        {
          break;
        }
        dest[written++] = '%';
        dest[written++] = '2';
        dest[written++] = '2';
      }
      else
      {
        if (written + 1 >= destSize)
        {
          break;
        }
        dest[written++] = *source;
      }
      source++;
    }
    dest[written] = '\0';
    return written;
  }

  bool isGpsCoordSane(float lat, float lon)
  {
    if (!isfinite(lat) || !isfinite(lon))
    {
      return false;
    }
    if (fabsf(lat) < 0.0001f || fabsf(lon) < 0.0001f)
    {
      return false;
    }
    if (lat < -90.0f || lat > 90.0f)
    {
      return false;
    }
    if (lon < -180.0f || lon > 180.0f)
    {
      return false;
    }
    return true;
  }

  float gpsDistanceMeters(float lat1, float lon1, float lat2, float lon2)
  {
    constexpr float kEarthRadiusMeters = 6371000.0f;
    constexpr float kDegToRad = 0.017453292519943295f;
    float dLat = (lat2 - lat1) * kDegToRad;
    float dLon = (lon2 - lon1) * kDegToRad;
    float lat1Rad = lat1 * kDegToRad;
    float lat2Rad = lat2 * kDegToRad;
    float sinLat = sinf(dLat * 0.5f);
    float sinLon = sinf(dLon * 0.5f);
    float a = (sinLat * sinLat) + (cosf(lat1Rad) * cosf(lat2Rad) * sinLon * sinLon);
    float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
    return kEarthRadiusMeters * c;
  }

  bool isGpsFixUsable(const LiveData *liveData)
  {
    if (!liveData->params.gpsValid)
    {
      return false;
    }
    if (liveData->params.gpsLat == -1.0f || liveData->params.gpsLon == -1.0f)
    {
      return false;
    }
    return isGpsCoordSane(liveData->params.gpsLat, liveData->params.gpsLon);
  }
} // namespace

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
  if (getLocalTime(&tm, 0))
  {
    liveData->params.currentTime = mktime(&tm);
  }
  else
  {
    liveData->params.currentTime = tv.tv_sec;
  }
  liveData->params.chargingStartTime = liveData->params.currentTime;
}

/**
   After setup device
*/
void Board320_240::afterSetup()
{
  // Check if board was sleeping
  bool afterSetup = false;

  syslog->print("SleepMode: ");
  syslog->println(liveData->settings.sleepModeLevel);

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

  // Init display
  syslog->println("Init TFT display");
  tft.begin();
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
  BoardInterface::afterSetup();
  printHeapMemory();
  tft.fillScreen(TFT_SILVER);

  // Threading
  // Comm via thread (ble/can)
  if (liveData->settings.threading)
  {
    syslog->println("xTaskCreate/xTaskCommLoop - COMM via thread (ble/can)");
    if (commMutex == nullptr)
    {
      commMutex = xSemaphoreCreateMutex();
      if (commMutex == nullptr)
      {
        syslog->println("Failed to create comm mutex");
      }
    }
#if defined(ESP32) && defined(ARDUINO_RUNNING_CORE)
    const BaseType_t taskResult = xTaskCreatePinnedToCore(
        xTaskCommLoop,
        "xTaskCommLoop",
        32768,
        (void *)this,
        2,
        &commTaskHandle,
        (ARDUINO_RUNNING_CORE == 0) ? 1 : 0);
    if (taskResult != pdPASS)
    {
      syslog->println("Failed to create comm task (pinned)");
    }
#else
    xTaskCreate(xTaskCommLoop, "xTaskCommLoop", 32768, (void *)this, 2, &commTaskHandle);
#endif
  }
  else
  {
    syslog->println("COMM without threading (ble/can)");
  }

  if (netSendQueue == nullptr)
  {
    netSendQueue = xQueueCreate(4, sizeof(NetSendJob));
    if (netSendQueue != nullptr)
    {
      syslog->println("xTaskCreate/xTaskNetSendLoop - NET send via thread");
      xTaskCreate(xTaskNetSendLoop, "xTaskNetSendLoop", 16384, (void *)this, 1, &netSendTaskHandle);
    }
    else
    {
      syslog->println("Failed to create net send queue");
    }
  }

  if (abrpSdLogQueue == nullptr)
  {
    abrpSdLogQueue = xQueueCreate(4, sizeof(AbrpLogEntry));
    if (abrpSdLogQueue != nullptr)
    {
      syslog->println("xTaskCreate/xTaskAbrpSdLogLoop - ABRP SD log via thread");
      xTaskCreate(xTaskAbrpSdLogLoop, "xTaskAbrpSdLogLoop", 8192, (void *)this, 1, &abrpSdLogTaskHandle);
    }
    else
    {
      syslog->println("Failed to create ABRP SD log queue");
    }
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
  M5.Lcd.setBrightness(0);
  M5.Axp.SetLcdVoltage(0);
#endif // BOARD_M5STACK_CORE2
#ifdef BOARD_M5STACK_CORES3
  /*CoreS3.Display.setBrightness;
  M5.Axp.SetLcdVoltage(2500);*/
#endif // BOARD_M5STACK_CORES3
}

/**
 * sprSetFont
 */
#ifdef BOARD_M5STACK_CORE2
static bool isEquivalentCore2Font(const GFXfont *a, const GFXfont *b)
{
  if (a == b)
  {
    return true;
  }
  if (a == nullptr || b == nullptr)
  {
    return false;
  }
  // GFX fonts in the M5 headers are header-defined constants (one copy per TU),
  // so pointer equality may fail across .cpp files. Match by stable metadata.
  return (a->first == b->first) && (a->last == b->last) && (a->yAdvance == b->yAdvance);
}

void Board320_240::sprSetFont(const GFXfont *f)
{
  if (isEquivalentCore2Font(f, fontFont7))
  {
    lastFont = fontFont7;
    spr.setTextFont(7);
    return;
  }
  if (isEquivalentCore2Font(f, fontFont2))
  {
    lastFont = fontFont2;
    spr.setTextFont(2);
    return;
  }

  lastFont = f;
  if (f != nullptr)
  {
    spr.setFont(f);
  }
}
#endif // BOARD_M5STACK_CORE2
#ifdef BOARD_M5STACK_CORES3
static bool isEquivalentCoreS3Font(const lgfx::GFXfont *a, const lgfx::GFXfont *b)
{
  if (a == b)
  {
    return true;
  }
  if (a == nullptr || b == nullptr)
  {
    return false;
  }
  // Same reason as Core2 variant above.
  return (a->first == b->first) && (a->last == b->last) && (a->yAdvance == b->yAdvance);
}

void Board320_240::sprSetFont(const lgfx::GFXfont *f)
{
  if (isEquivalentCoreS3Font(f, fontFont7))
  {
    lastFont = fontFont7;
    spr.setFont(fontFont7bmp);
    return;
  }
  if (isEquivalentCoreS3Font(f, fontFont2))
  {
    lastFont = fontFont2;
    spr.setFont(fontFont2bmp);
    return;
  }

  lastFont = f;
  if (f != nullptr)
  {
    spr.setFont(f);
  }
}
#endif // BOARD_M5STACK_CORES3

/**
 * drawString
 */
void Board320_240::sprDrawString(const char *string, int32_t poX, int32_t poY)
{
#ifdef BOARD_M5STACK_CORE2
  spr.drawString(string, poX, poY, lastFont == fontFont7 ? 7 : (lastFont == fontFont2 ? 2 : GFXFF));
#endif // BOARD_M5STACK_CORE2
#ifdef BOARD_M5STACK_CORES3
  spr.drawString(string, poX, poY);
#endif // BOARD_M5STACK_CORES3
}

/**
 * drawString
 */
void Board320_240::tftDrawStringFont7(const char *string, int32_t poX, int32_t poY)
{
#ifdef BOARD_M5STACK_CORE2
  tft.drawString(string, poX, poY, 7);
#endif // BOARD_M5STACK_CORE2
#ifdef BOARD_M5STACK_CORES3
  tft.drawString(string, poX, poY);
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
  if (liveData->params.stopCommandQueue)
  {
    lcdBrightnessPerc = 20;
  }

  if (currentBrightness == lcdBrightnessPerc)
    return;

  syslog->print("Set brightness: ");
  syslog->println(lcdBrightnessPerc);
  currentBrightness = lcdBrightnessPerc;
#ifdef BOARD_M5STACK_CORE2
  // M5.Lcd.setBrightness(lcdBrightnessPerc * 2.54);
  // M5.Axp.SetDCDC3(true);
  // M5.Axp.SetLDOEnable(2, true);
  // M5.Axp.SetLDOEnable(3, true);
  M5.Axp.SetDCDC3(true);
  uint16_t lcdVolt = map(lcdBrightnessPerc, 0, 100, 2500, 3300);
  M5.Axp.SetLcdVoltage(lcdVolt);
#endif // BOARD_M5STACK_CORE2
#ifdef BOARD_M5STACK_CORES3
  CoreS3.Display.setBrightness(lcdBrightnessPerc * 1.27);
#endif // BOARD_M5STACK_CORES3
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
    sprSetFont(fontRobotoThin24);
    spr.setTextDatum(BL_DATUM);
    sprDrawString(row1, 0, height / 2);
    sprDrawString(row2, 0, (height / 2) + 30);
    spr.pushSprite(0, 0);
  }
  else
  {
    tft.fillRect(0, (height / 2) - 45, tft.width(), 90, TFT_NAVY);
    // tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    tft.setFont(fontRobotoThin24);
    tft.setTextDatum(BL_DATUM);
    tft.drawString(row1, 0, height / 2);
    tft.drawString(row2, 0, (height / 2) + 30);
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
  sprSetFont(fontRobotoThin24);
  spr.setTextDatum(BL_DATUM);
  sprDrawString(row1, 0, height / 2);
  sprDrawString(row2, 0, (height / 2) + 30);
  spr.fillRect(0, height - 50, 100, 50, TFT_NAVY);
  spr.fillRect(tft.width() - 100, height - 50, 100, 50, TFT_NAVY);
  spr.setTextDatum(BL_DATUM);
  sprDrawString("YES", 10, height - 10);
  spr.setTextDatum(BR_DATUM);
  sprDrawString("NO", tft.width() - 10, height - 10);
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
 * Redraws the screen based on the current display mode and parameters.
 * Handles switching between different display screens and rendering status indicators.
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
  sprSetFont(fontFont2);
  sprDrawString(desc, posx, posy);

  // Big 2x2 cell in the middle of screen
  if (w == 2 && h == 2)
  {

    // Bottom 2 numbers with charged/discharged kWh from start
    posx = (x * 80) + 5;
    posy = ((y + h) * 60) - 32;
    sprintf(tmpStr3, "-%01.01f", liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
    sprSetFont(fontRobotoThin24);
    spr.setTextDatum(TL_DATUM);
    sprDrawString(tmpStr3, posx, posy);

    posx = ((x + w) * 80) - 8;
    sprintf(tmpStr3, "+%01.01f", liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
    spr.setTextDatum(TR_DATUM);
    sprDrawString(tmpStr3, posx, posy);

    // Main number - kwh on roads, amps on charges
    posy = (y * 60) + 24;
    spr.setTextColor(fgColor);
    // sprSetFont(fontOrbitronLight32);
    sprSetFont(fontFont7);
    sprDrawString(text, posx, posy);
  }
  else
  {
    // All others 1x1 cells
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(fgColor);
    sprSetFont(fontOrbitronLight24);
    //, (w == 2 ? 7 : GFXFF)
    posx = (x * 80) + (w * 80 / 2) - 3;
    posy = (y * 60) + (h * 60 / 2) + 4;
    sprDrawString(text, posx, posy);
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
  sprSetFont(fontFont2);
  sprDrawString(desc, posx, posy);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(fgColor);
  posx = (x * 80) + (w * 80 / 2) - 3;
  sprSetFont(fontFont2);
  sprDrawString(text, posx, posy + 14);
}

/**
 * Draws tire pressure and temperature info in a 2x2 grid cell.
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
  sprSetFont(fontFont2);
  posx = (x * 80) + 4;
  posy = (y * 60) + 0;
  sprDrawString(topleft, posx, posy);
  posy = (y * 60) + 14;
  sprDrawString(bottomleft, posx, posy);

  spr.setTextDatum(TR_DATUM);
  sprSetFont(fontFont2);
  posx = ((x + w) * 80) - 4;
  posy = (y * 60) + 0;
  sprDrawString(topright, posx, posy);
  posy = (y * 60) + 14;
  sprDrawString(bottomright, posx, posy);
}


/**
 * Modify menu item text
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
    sprSetFont(fontOrbitronLight32);
    spr.setTextColor(TFT_WHITE);
    spr.setTextDatum(MC_DATUM);
    sprDrawString("! LIGHTS OFF !", 160, 120);
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
    const int gpsIndicatorX = 168;
    spr.fillCircle(gpsIndicatorX, 10, 8, (liveData->params.gpsValid) ? TFT_GREEN : TFT_RED);
    spr.fillCircle(gpsIndicatorX, 10, 6, TFT_BLACK);
    spr.setTextSize(1);
    spr.setTextColor((liveData->params.gpsValid) ? TFT_GREEN : TFT_WHITE);
    spr.setTextDatum(TL_DATUM);
    sprintf(tmpStr1, "%d", liveData->params.gpsSat);
    sprSetFont(fontFont2);
    sprDrawString(tmpStr1, gpsIndicatorX + 14, 2);
  }

  // SDCARD recording
  // liveData->params.sdcardRecording
  if (liveData->settings.sdcardEnabled == 1 && (liveData->params.queueLoopCounter & 1) == 1)
  {
    const int sdIndicatorX = (liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED) ? 168 : 310;
    spr.fillCircle(sdIndicatorX, 10, 4, TFT_BLACK);
    spr.fillCircle(sdIndicatorX, 10, 3,
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
  if (liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED)
  {
    if (liveData->params.gyroSensorMotion)
    {
      spr.fillRect(128, 3, 4, 18, TFT_ORANGE);
    }
    if (liveData->params.ignitionOn)
    {
      spr.fillRect(130, 4, 2, 14, TFT_GREEN);
    }
  }

  // WiFi Status
  if (liveData->settings.wifiEnabled == 1 && liveData->settings.remoteUploadModuleType == 1)
  {
    if (liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED)
    {
      const uint16_t wifiColor =
          (WiFi.status() == WL_CONNECTED)
              ? ((liveData->params.lastSuccessNetSendTime + tmp_send_interval >= liveData->params.currentTime) ? TFT_GREEN /* last request was 200 OK */ : TFT_YELLOW /* wifi connected but not send */)
              : TFT_RED; /* wifi not connected */

      // WiFi icon (upper arcs + center dot) in place of the previous square.
      const int wifiCx = 146;
      const int wifiCy = 11;
      spr.drawCircle(wifiCx, wifiCy, 6, wifiColor);
      spr.drawCircle(wifiCx, wifiCy, 4, wifiColor);
      spr.drawCircle(wifiCx, wifiCy, 2, wifiColor);
      spr.fillRect(wifiCx - 7, wifiCy, 15, 8, TFT_BLACK); // keep only upper arcs
      spr.fillCircle(wifiCx, wifiCy + 1, 1, wifiColor);

      if (liveData->params.isWifiBackupLive)
      {
        // Backup link badge.
        spr.fillCircle(wifiCx + 7, 2, 2, wifiColor);
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
      spr.fillRect(40 + 5, 40, 50 - 10, 90, 0x4208);
      spr.fillRect(40, 40 + 5, 50, 90 - 10, 0x4208);
      spr.fillCircle(40 + 5, 40 + 5, 5, 0x4208);
      spr.fillCircle(40 + 50 - 6, 40 + 5, 5, 0x4208);
      spr.fillCircle(40 + 5, 40 + 90 - 6, 5, 0x4208);
      spr.fillCircle(40 + 50 - 6, 40 + 90 - 6, 5, 0x4208);
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

  bool statusBoxUsed = false;

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
    sprSetFont(fontFont2);
    sprDrawString(tmpStr1, 10, 190);
    sprDrawString(commInterface->getConnectStatus().c_str(), 10, 210);
    statusBoxUsed = true;
  }
  else
    // CAN not connected
    if (liveData->settings.commType == COMM_TYPE_CAN_COMMU && commInterface->getConnectStatus() != "")
    {
      // Print message
      spr.fillRect(0, 185, 320, 50, TFT_BLACK);
      spr.drawRect(0, 185, 320, 50, TFT_WHITE);
      spr.setTextSize(1);
      spr.setTextDatum(TL_DATUM);
      spr.setTextColor(TFT_WHITE);
      sprSetFont(fontRobotoThin24);
      sprintf(tmpStr1, "CAN #%d %s%d", commInterface->getConnectAttempts(), (liveData->params.stopCommandQueue ? "QS" : "QR"), liveData->params.queueLoopCounter, " ", liveData->currentAtshRequest);
      sprSetFont(fontFont2);
      sprDrawString(tmpStr1, 10, 190);
      sprDrawString(commInterface->getConnectStatus().c_str(), 10, 210);
      statusBoxUsed = true;
    }

  if (!statusBoxUsed && liveData->settings.wifiEnabled == 1 &&
      WiFi.status() == WL_CONNECTED && !liveData->params.netAvailable)
  {
    spr.fillRect(0, 185, 320, 50, TFT_BLACK);
    spr.drawRect(0, 185, 320, 50, TFT_WHITE);
    spr.setTextSize(1);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(TFT_WHITE);
    sprSetFont(fontFont2);
    sprDrawString("WiFi OK", 10, 190);
    sprDrawString("Net temporarily unavailable", 10, 210);
    statusBoxUsed = true;
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
    Board320_240 *board = static_cast<Board320_240 *>(boardObj);
    if (board->commMutex == nullptr || xSemaphoreTake(board->commMutex, pdMS_TO_TICKS(20)) == pdTRUE)
    {
      boardObj->commLoop();
      if (board->commMutex != nullptr)
      {
        xSemaphoreGive(board->commMutex);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

/**
 * Task that performs network sends outside the UI loop.
 *
 * @param pvParameters Pointer to the Board320_240 instance.
 */
void Board320_240::xTaskNetSendLoop(void *pvParameters)
{
  Board320_240 *boardObj = (Board320_240 *)pvParameters;
  NetSendJob job{};
  while (1)
  {
    if (boardObj->netSendQueue != nullptr && xQueueReceive(boardObj->netSendQueue, &job, portMAX_DELAY) == pdTRUE)
    {
      boardObj->netSendInProgress = true;
      boardObj->maxMainLoopDuringNetSendMs = 0;
      int64_t startTime = esp_timer_get_time();
      boardObj->netSendData(job.sendAbrp);
      int64_t endTime = esp_timer_get_time();
      boardObj->lastNetSendDurationMs = static_cast<uint32_t>((endTime - startTime) / 1000);
      boardObj->netSendInProgress = false;
      syslog->info(DEBUG_COMM, "Net send done");
      syslog->info(DEBUG_COMM, "Net send duration (ms): " + String(boardObj->lastNetSendDurationMs));
      syslog->info(DEBUG_COMM, "Max mainLoop during send (ms): " + String(boardObj->maxMainLoopDuringNetSendMs));
    }
  }
}

/**
 * Task that writes ABRP JSON logs to SD card.
 *
 * @param pvParameters Pointer to the Board320_240 instance.
 */
void Board320_240::xTaskAbrpSdLogLoop(void *pvParameters)
{
  Board320_240 *boardObj = (Board320_240 *)pvParameters;
  AbrpLogEntry entry{};
  while (1)
  {
    if (boardObj->abrpSdLogQueue != nullptr && xQueueReceive(boardObj->abrpSdLogQueue, &entry, portMAX_DELAY) == pdTRUE)
    {
      if (boardObj->liveData->params.stopCommandQueue)
      {
        continue;
      }
      if (!boardObj->liveData->params.sdcardInit || !boardObj->liveData->params.sdcardRecording)
      {
        continue;
      }

      struct tm now;
      time_t logTime = entry.currentTime;
      localtime_r(&logTime, &now);

      if (entry.operationTimeSec > 0 && strlen(boardObj->liveData->params.sdcardAbrpFilename) == 0)
      {
        sprintf(boardObj->liveData->params.sdcardAbrpFilename, "/%llu.abrp.json", entry.operationTimeSec / 60);
      }
      if (entry.timeSyncWithGps && strlen(boardObj->liveData->params.sdcardAbrpFilename) < 20)
      {
        strftime(boardObj->liveData->params.sdcardAbrpFilename, sizeof(boardObj->liveData->params.sdcardAbrpFilename), "/%y%m%d%H%M.abrp.json", &now);
      }

      if (strlen(boardObj->liveData->params.sdcardAbrpFilename) != 0)
      {
        File file = SD.open(boardObj->liveData->params.sdcardAbrpFilename, FILE_APPEND);
        if (!file)
        {
          syslog->println("Failed to open file for appending");
          file = SD.open(boardObj->liveData->params.sdcardAbrpFilename, FILE_WRITE);
        }
        if (!file)
        {
          syslog->println("Failed to create file");
        }
        if (file)
        {
          syslog->info(DEBUG_SDCARD, "Save buffer to SD card");
          file.write((const uint8_t *)entry.payload, entry.length);
          file.print(",\n");
          file.close();
        }
      }
    }
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
  if (liveData->params.stopCommandQueue || commInterface->isSuspended())
  {
    return;
  }

  // Start timing
  int64_t startTime3 = esp_timer_get_time();

  // Read data from BLE/CAN
  commInterface->mainLoop();

  int64_t endTime3 = esp_timer_get_time();

  // Calculate duration
  int64_t duration3 = endTime3 - startTime3;

  // Print the duration using syslog
  // Use String constructor to convert int64_t to String
  // syslog->println("Time taken by function: commLoop() " + String(duration3) + " microseconds");
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
  const uint32_t loopDurationMs = (millis() - mainLoopStart);
  displayFps = (loopDurationMs == 0 ? 0 : (1000.0f / loopDurationMs));
  mainLoopStart = millis();
  if (netSendInProgress && loopDurationMs > maxMainLoopDuringNetSendMs)
  {
    maxMainLoopDuringNetSendMs = loopDurationMs;
  }

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

  const bool allowGpsProcessing = !(liveData->params.stopCommandQueue && liveData->settings.voltmeterEnabled == 1);
  if (allowGpsProcessing)
  {
    // GPS process
    // Start timing
    int64_t startTime4 = esp_timer_get_time();

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

    int64_t endTime4 = esp_timer_get_time();
    // Calculate duration
    int64_t duration4 = endTime4 - startTime4;

    // Print the duration using syslog
    // Use String constructor to convert int64_t to String
    // syslog->println("Time taken by function: GPS loop " + String(duration4) + " microseconds");
  }

  // currentTime
  struct tm now = cachedNow;
  const uint32_t nowMs = millis();
  if (lastTimeUpdateMs == 0 || (nowMs - lastTimeUpdateMs) >= 1000)
  {
    if (getLocalTime(&now, 0))
    {
      cachedNow = now;
      cachedNowEpoch = mktime(&cachedNow);
    }
    else if (cachedNowEpoch != 0 && lastTimeUpdateMs != 0)
    {
      const uint32_t deltaSec = (nowMs - lastTimeUpdateMs) / 1000U;
      if (deltaSec > 0)
      {
        cachedNowEpoch += deltaSec;
        localtime_r(&cachedNowEpoch, &cachedNow);
      }
    }
    if (cachedNowEpoch != 0)
    {
      liveData->params.currentTime = cachedNowEpoch;
    }
    else
    {
      // Fallback to uptime seconds when RTC/NTP/GPS time isn't available yet.
      liveData->params.currentTime = nowMs / 1000U;
    }
    lastTimeUpdateMs = nowMs;
  }

  // Periodic automatic brightness recalculation (handles sunrise/sunset without new GPS fix)
  if (liveData->settings.lcdBrightness == 0 &&
      liveData->params.gpsLat != -1.0 &&
      liveData->params.gpsLon != -1.0 &&
      liveData->params.currentTime != 0)
  {
    static time_t lastAutoBrightnessCalc = 0;
    const time_t nowTime = liveData->params.currentTime;
    if (lastAutoBrightnessCalc == 0 || (nowTime - lastAutoBrightnessCalc) >= 60)
    {
      lastAutoBrightnessCalc = nowTime;
      calcAutomaticBrightnessLatLon();
    }
  }

  // Check and eventually reconnect WIFI connection
  const bool wifiEnabled = (liveData->settings.wifiEnabled == 1);
  const bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  const bool allowWifiFallback = (!liveData->params.stopCommandQueue &&
                                  liveData->settings.commType != COMM_TYPE_OBD2_WIFI &&
                                  !liveData->params.wifiApMode &&
                                  wifiEnabled &&
                                  liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI);
  if (allowWifiFallback)
  {
    const bool disconnectedTooLong = (!wifiConnected &&
                                      liveData->params.currentTime - liveData->params.wifiLastConnectedTime > 60);
    const bool netFailedTooLong = (wifiConnected &&
                                   liveData->params.netFailureStartTime != 0 &&
                                   liveData->params.netFailureCount >= kNetFailureFallbackCount &&
                                   (liveData->params.currentTime - liveData->params.netFailureStartTime) > kNetFailureFallbackSec);
    if (disconnectedTooLong || netFailedTooLong)
    {
      wifiFallback();
    }
  }

  // SIM800L, WiFI remote upload, ABRP remote upload, MQTT
  netLoop();

  // SD card recording
  int64_t startTime5 = esp_timer_get_time();
  if (!liveData->params.stopCommandQueue && liveData->params.sdcardInit && liveData->params.sdcardRecording && liveData->params.sdcardCanNotify &&
      (liveData->params.odoKm != -1 && liveData->params.socPerc != -1))
  {
    const size_t sdcardFlushSize = 2048;
    const uint32_t sdcardIntervalMs = static_cast<uint32_t>(liveData->settings.sdcardLogIntervalSec) * 1000U;
    // create filename
    if (liveData->params.operationTimeSec > 0 && strlen(liveData->params.sdcardFilename) == 0)
    {
      sprintf(liveData->params.sdcardFilename, "/%llu.json", uint64_t(liveData->params.operationTimeSec / 60));
      syslog->print("Log filename by opTimeSec: ");
      syslog->println(liveData->params.sdcardFilename);
    }
    if (liveData->params.currTimeSyncWithGps && strlen(liveData->params.sdcardFilename) < 15)
    {
      if (cachedNowEpoch == 0)
      {
        getLocalTime(&now, 0);
      }
      strftime(liveData->params.sdcardFilename, sizeof(liveData->params.sdcardFilename), "/%y%m%d%H%M.json", &now);
      syslog->print("Log filename by GPS: ");
      syslog->println(liveData->params.sdcardFilename);
    }

    // append buffer, clear buffer & notify state
    if (strlen(liveData->params.sdcardFilename) != 0)
    {
      liveData->params.sdcardCanNotify = false;
      String jsonLine;
      serializeParamsToJson(jsonLine);
      jsonLine += ",\n";
      sdcardRecordBuffer += jsonLine;

      const bool timeToFlush = (sdcardIntervalMs > 0U) && ((nowMs - liveData->params.sdcardLastFlushMs) >= sdcardIntervalMs);
      const bool sizeToFlush = sdcardRecordBuffer.length() >= sdcardFlushSize;
      if (timeToFlush || sizeToFlush)
      {
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
          file.print(sdcardRecordBuffer);
          file.close();
          sdcardRecordBuffer = "";
          liveData->params.sdcardLastFlushMs = nowMs;
        }
      }
    }
  }

  int64_t endTime5 = esp_timer_get_time();

  // Calculate duration
  int64_t duration5 = endTime5 - startTime5;

  // Print the duration using syslog
  // Use String constructor to convert int64_t to String
  // syslog->println("Time taken by function: SD card write loop " + String(duration5) + " microseconds");

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

  // Reset sentry session when car becomes active
  if (liveData->params.ignitionOn || liveData->params.chargingOn)
  {
    if (liveData->params.sentrySessionActive)
    {
      liveData->params.sentrySessionActive = false;
      liveData->params.motionWakeLocked = false;
      liveData->params.gpsWakeCount = 0;
      liveData->params.gyroWakeCount = 0;
      liveData->params.motionWakeLastTime = 0;
    }
  }

  // Wake up from stopped command queue
  //  - ignitions on and aux >= 11.5v
  //  - ina3221 & voltage is >= 14V (DCDC is running)
  //  - gps speed >= 5kmh & 4+ satellites (only when voltmeter is disabled)
  //  - gyro motion (only when voltmeter is disabled)
  const bool queueSleeping = (liveData->params.stopCommandQueue || liveData->params.stopCommandQueueTime != 0);
  const bool allowMotionWake = (liveData->settings.voltmeterEnabled == 0);
  if (queueSleeping)
  {
    if (liveData->params.motionWakeLastTime == 0)
    {
      liveData->params.motionWakeLastTime = liveData->params.currentTime;
    }
    else if (liveData->params.currentTime - liveData->params.motionWakeLastTime >= kMotionWakeResetSec)
    {
      liveData->params.gpsWakeCount = 0;
      liveData->params.gyroWakeCount = 0;
      liveData->params.motionWakeLocked = false;
      liveData->params.motionWakeLastTime = liveData->params.currentTime;
    }
  }
  const uint16_t maxGpsWakePerSession = 1;
  const uint16_t maxGyroWakePerSession = 5;
  const bool gpsWakeRemaining = (liveData->params.gpsWakeCount < maxGpsWakePerSession);
  const bool gyroWakeRemaining = (liveData->params.gyroWakeCount < maxGyroWakePerSession);
  const bool motionWakeLocked = (!gpsWakeRemaining && !gyroWakeRemaining);
  liveData->params.motionWakeLocked = motionWakeLocked;
  const bool motionWakeAllowed = allowMotionWake && !motionWakeLocked;
  const bool gpsWake =
      motionWakeAllowed && gpsWakeRemaining &&
      (liveData->params.gpsValid && liveData->params.speedKmhGPS >= 5 && liveData->params.gpsSat >= 4); // 5 floor parking house, satelites 5 & gps speed = 274kmh :/
  const bool gyroWake = motionWakeAllowed && gyroWakeRemaining && liveData->params.gyroSensorMotion;
  const bool motionWake = gpsWake || gyroWake;
  if (queueSleeping &&
      ((liveData->params.ignitionOn && (liveData->params.auxVoltage <= 3 || liveData->params.auxVoltage >= 11.5)) ||
       (liveData->settings.voltmeterEnabled == 1 && liveData->params.auxVoltage > 14.0) ||
       motionWake))
  {
    if (motionWake)
    {
      if (gpsWake)
      {
        liveData->params.gpsWakeCount++;
      }
      if (gyroWake)
      {
        liveData->params.gyroWakeCount++;
      }
      liveData->params.motionWakeLastTime = liveData->params.currentTime;
      liveData->params.motionWakeLocked =
          (liveData->params.gpsWakeCount >= maxGpsWakePerSession &&
           liveData->params.gyroWakeCount >= maxGyroWakePerSession);
    }
    liveData->continueWithCommandQueue();
    if (commInterface->isSuspended())
    {
      if (commMutex == nullptr || xSemaphoreTake(commMutex, pdMS_TO_TICKS(20)) == pdTRUE)
      {
        commInterface->resumeDevice();
        if (commMutex != nullptr)
        {
          xSemaphoreGive(commMutex);
        }
      }
    }
  }
  // Stop command queue
  //  - automatically turns off CAN scanning after 1-2 minutes of inactivity
  //  - ignition is off
  //  - AUX voltage is under 11.5V
  const time_t doorStateStaleAfterSec = 15;
  const bool doorStateStale = (liveData->params.currentTime - liveData->params.lastCanbusResponseTime > doorStateStaleAfterSec);
  const bool doorsClosed = (!liveData->params.leftFrontDoorOpen &&
                            !liveData->params.rightFrontDoorOpen &&
                            !liveData->params.trunkDoorOpen);
  const bool doorsOk = (doorsClosed || doorStateStale);
  if (liveData->settings.commandQueueAutoStop == 1 &&
      liveData->params.getValidResponse &&
      ((!liveData->params.ignitionOn &&
        doorsOk &&
        !liveData->params.chargingOn) ||
       (liveData->params.auxVoltage > 3 && liveData->params.auxVoltage < 11.5)))
  {
    liveData->prepareForStopCommandQueue();
  }
  if (!liveData->params.stopCommandQueue &&
      ((liveData->params.stopCommandQueueTime != 0 && liveData->params.currentTime - liveData->params.stopCommandQueueTime > 60) ||
       (liveData->params.auxVoltage > 3 && liveData->params.auxVoltage < 11.0)))
  {
    liveData->params.stopCommandQueue = true;
    if (commMutex == nullptr || xSemaphoreTake(commMutex, pdMS_TO_TICKS(20)) == pdTRUE)
    {
      commInterface->suspendDevice();
      if (commMutex != nullptr)
      {
        xSemaphoreGive(commMutex);
      }
    }
    syslog->println("CAN Command queue stopped...");
  }

  // Descrease loop fps
  if (liveData->params.stopCommandQueue)
  {
    delay(1000);
  }

  // Automatic sleep after inactivity
  if (liveData->params.currentTime - liveData->params.lastIgnitionOnTime > 10 &&
      liveData->settings.sleepModeLevel >= SLEEP_MODE_SCREEN_ONLY &&
      liveData->params.currentTime - liveData->params.lastButtonPushedTime > 30 &&
      (liveData->params.currentTime - liveData->params.wakeUpTime > 60))
  {
    turnOffScreen();
  }
  else
  {
    setBrightness();
  }

  // Read data from BLE/CAN
  if (!liveData->settings.threading)
  {
    if (!liveData->params.stopCommandQueue && !commInterface->isSuspended())
    {
      commInterface->mainLoop();
    }
  }

  // force redraw (min 1 sec update; slower while in Sentry)
  const time_t redrawIntervalSec = liveData->params.stopCommandQueue ? 2 : 1;
  if (liveData->params.currentTime - lastRedrawTime >= redrawIntervalSec || liveData->redrawScreenRequested)
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
    liveData->params.avgSpeedKmh = /*(double)*/ (liveData->params.odoKm - forwardDriveOdoKmStart) /
                                   (/*(double)*/ liveData->params.timeInForwardDriveMode / 3600.0);
  }

  // Automatic reset charging or drive data after switch between drive / charging or longer standing (1800 seconds)
  if (liveData->params.chargingOn && !lastChargingOn)
  {
    liveData->params.chargingStartTime = liveData->params.currentTime;
  }
  lastChargingOn = liveData->params.chargingOn;

  if (liveData->params.chargingOn && liveData->params.carMode != CAR_MODE_CHARGING)
  {
    liveData->clearDrivingAndChargingStats(CAR_MODE_CHARGING);
  }
  else if (liveData->params.forwardDriveMode &&
           (liveData->params.speedKmh > 15 || (liveData->params.speedKmhGPS > 15 && liveData->params.gpsSat >= 4)) && liveData->params.carMode != CAR_MODE_DRIVE)
  {
    liveData->clearDrivingAndChargingStats(CAR_MODE_DRIVE);
  }
  /*else if (!liveData->params.chargingOn && !liveData->params.forwardDriveMode && liveData->params.carMode != CAR_MODE_NONE &&
           (!(liveData->params.speedKmh > 15 || (liveData->params.speedKmhGPS > 15 && liveData->params.gpsSat >= 4))) && liveData->params.currentTime - liveData->params.carModeChanged > 1800 &&
           liveData->params.currentTime - liveData->params.carModeChanged < 10 * 24 * 3600)
  {
    liveData->clearDrivingAndChargingStats(CAR_MODE_NONE);
  }*/
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

  struct tm tm = {0};
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = seconds;
  time_t t = mktime(&tm);
  printf("%02d%02d%02d%02d%02d%02d\n", year - 2000, month, day, hour, minute, seconds);
  struct timeval now = {.tv_sec = t};
  settimeofday(&now, NULL);
  syncTimes(t);

#ifdef BOARD_M5STACK_CORE2
  RTC_TimeTypeDef RTCtime = {hour, minute, seconds};
  RTC_DateTypeDef RTCdate = {year, month, day};
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
  time_t *timeParams[] = {
      &liveData->params.chargingStartTime,
      &liveData->params.lastRemoteApiSent,
      &liveData->params.lastAbrpSent,
      &liveData->params.lastContributeSent,
      &liveData->params.lastSuccessNetSendTime,
      &liveData->params.lastButtonPushedTime,
      &liveData->params.wakeUpTime,
      &liveData->params.lastIgnitionOnTime,
      &liveData->params.stopCommandQueueTime,
      &liveData->params.motionWakeLastTime,
      &liveData->params.lastChargingOnTime,
      &liveData->params.lastVoltageReadTime,
      &liveData->params.lastVoltageOkTime,
      &liveData->params.lastCanbusResponseTime};

  for (time_t *param : timeParams)
  {
    if (*param != 0)
    {
      *param = newTime - (liveData->params.currentTime - *param);
    }
  }

  // Reset avg speed counter
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
  // Check if SD card is already initialized
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
    liveData->params.sdcardLastFlushMs = millis();
  }
  else
  {
    if (sdcardRecordBuffer.length() > 0 && strlen(liveData->params.sdcardFilename) != 0)
    {
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
        file.print(sdcardRecordBuffer);
        file.close();
      }
      sdcardRecordBuffer = "";
    }
    String tmpStr = "";
    tmpStr.toCharArray(liveData->params.sdcardFilename, tmpStr.length() + 1);
  }
}

/**
 * Synchronizes GPS data with the liveData structure.
 *
 * This function updates GPS-related parameters such as latitude, longitude,
 * altitude, satellite count, speed, and time synchronization if valid data is received.
 */
void Board320_240::syncGPS()
{
  if (gps.satellites.isValid())
  {
    liveData->params.gpsSat = gps.satellites.value();
  }

  bool accepted = false;
  float newLat = 0.0f;
  float newLon = 0.0f;

  if (gps.location.isValid())
  {
    newLat = gps.location.lat();
    newLon = gps.location.lng();

    if (isGpsCoordSane(newLat, newLon))
    {
      bool hasLastFix = (liveData->params.gpsLat != -1.0f && liveData->params.gpsLon != -1.0f &&
                         (liveData->params.gpsLastFixTime != 0 || liveData->params.gpsLastFixMs != 0));

      if (!hasLastFix)
      {
        accepted = true;
      }
      else
      {
        uint32_t dtSec = 0;
        if (liveData->params.currentTime > 0 && liveData->params.gpsLastFixTime > 0)
        {
          dtSec = (liveData->params.currentTime >= liveData->params.gpsLastFixTime)
                      ? static_cast<uint32_t>(liveData->params.currentTime - liveData->params.gpsLastFixTime)
                      : 0;
        }
        else if (liveData->params.gpsLastFixMs != 0)
        {
          uint32_t nowMs = millis();
          dtSec = (nowMs - liveData->params.gpsLastFixMs) / 1000;
        }

        if (dtSec == 0)
        {
          dtSec = 1;
        }

        float maxSpeedKmh = 130.0f;
        if (liveData->params.speedKmh > 0)
        {
          float candidate = liveData->params.speedKmh + 30.0f;
          if (candidate > maxSpeedKmh)
          {
            maxSpeedKmh = candidate;
          }
        }
        if (gps.speed.isValid())
        {
          float candidate = gps.speed.kmph() + 30.0f;
          if (candidate > maxSpeedKmh)
          {
            maxSpeedKmh = candidate;
          }
        }
        if (maxSpeedKmh > kGpsMaxSpeedKmh)
        {
          maxSpeedKmh = kGpsMaxSpeedKmh;
        }

        float distanceMeters = gpsDistanceMeters(liveData->params.gpsLat, liveData->params.gpsLon, newLat, newLon);
        float allowedMeters = (maxSpeedKmh * dtSec / 3.6f) + kGpsJitterMeters;
        bool shortJump = (dtSec <= kGpsShortJumpWindowSec && distanceMeters > kGpsMaxJumpMetersShort);
        bool speedJump = distanceMeters > allowedMeters;

        if (!shortJump && !speedJump)
        {
          accepted = true;
        }
        else if (dtSec >= kGpsReacquireAfterSec)
        {
          accepted = true;
        }
      }
    }
  }

  if (accepted)
  {
    liveData->params.gpsValid = true;
    liveData->params.gpsLat = newLat;
    liveData->params.gpsLon = newLon;
    liveData->params.gpsAlt = gps.altitude.meters();
    liveData->params.gpsLastFixTime = liveData->params.currentTime;
    liveData->params.gpsLastFixMs = millis();
    calcAutomaticBrightnessLatLon(); // Adjust screen brightness based on location
  }
  else
  {
    liveData->params.gpsValid = false;
  }

  // Update GPS speed if valid and enough satellites are available
  if (gps.speed.isValid() && liveData->params.gpsSat >= 4)
  {
    liveData->params.speedKmhGPS = gps.speed.kmph();
  }
  else
  {
    liveData->params.speedKmhGPS = -1; // Invalid GPS speed
  }
  if (liveData->params.speedKmh == 0)
  {
    liveData->params.speedKmhGPS = 0;
  }
  if (gps.course.isValid() && liveData->params.gpsSat >= 4)
  {
    liveData->params.gpsHeadingDeg = gps.course.deg();
  }
  else
  {
    liveData->params.gpsHeadingDeg = -1;
  }

  // Synchronize time with GPS if it has not been synchronized yet
  if (!liveData->params.currTimeSyncWithGps && gps.date.isValid() && gps.time.isValid())
  {
    setGpsTime(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
  }
}

/**
 * Initializes and connects to WiFi using the stored SSID and password.
 *
 * Enables STA mode, starts the connection, and updates the last connected time.
 *
 * @return True if WiFi initialization and connection succeeded, false otherwise.
 */
bool Board320_240::wifiSetup()
{
  syslog->print("Initializing WiFi with SSID: ");
  syslog->println(liveData->settings.wifiSsid);

  // Enable Station mode and start connection
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(liveData->settings.wifiSsid, liveData->settings.wifiPassword);

  // Update the last connected time
  liveData->params.wifiLastConnectedTime = liveData->params.currentTime;

  return true;
}

/**
 * Handles switching between main and backup WiFi networks.
 *
 * Disconnects from the current WiFi network. If a backup network is configured, it switches to the backup
 * network or restores the main network otherwise. Useful for maintaining connectivity during interruptions.
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
    // Attempt reconnection to the main WiFi if no backup is configured
    wifiSwitchToMain();
  }
}

/**
 * Switches to the backup WiFi network.
 *
 * Updates relevant parameters and attempts to connect using the backup credentials.
 */
void Board320_240::wifiSwitchToBackup()
{
  syslog->print("Switching to backup WiFi: ");
  syslog->println(liveData->settings.wifiSsid2);
  WiFi.begin(liveData->settings.wifiSsid2, liveData->settings.wifiPassword2);
  liveData->params.isWifiBackupLive = true;
  liveData->params.wifiBackupUptime = liveData->params.currentTime;
  liveData->params.wifiLastConnectedTime = liveData->params.currentTime;
}

/**
 * Restores the main WiFi connection.
 *
 * Reverts to the main WiFi credentials and updates status parameters.
 */
void Board320_240::wifiSwitchToMain()
{
  syslog->print("Switching to main WiFi: ");
  syslog->println(liveData->settings.wifiSsid);
  WiFi.begin(liveData->settings.wifiSsid, liveData->settings.wifiPassword);
  liveData->params.isWifiBackupLive = false;
  liveData->params.wifiLastConnectedTime = liveData->params.currentTime;
}

void Board320_240::updateNetAvailability(bool success)
{
  if (success)
  {
    liveData->params.netAvailable = true;
    liveData->params.netLastFailureTime = 0;
    liveData->params.netFailureStartTime = 0;
    liveData->params.netFailureCount = 0;
  }
  else
  {
    liveData->params.netAvailable = false;
    liveData->params.netLastFailureTime = liveData->params.currentTime;
    if (liveData->params.netFailureStartTime == 0)
    {
      liveData->params.netFailureStartTime = liveData->params.currentTime;
    }
    if (liveData->params.netFailureCount < 0xFFFFU)
    {
      liveData->params.netFailureCount++;
    }
  }
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
  if (!wifiReady)
  {
    liveData->params.netAvailable = true;
    liveData->params.netLastFailureTime = 0;
    liveData->params.netFailureStartTime = 0;
    liveData->params.netFailureCount = 0;
  }
  bool netBackoffActive = (!liveData->params.netAvailable &&
                           liveData->params.netLastFailureTime != 0 &&
                           (liveData->params.currentTime - liveData->params.netLastFailureTime) < kNetRetryIntervalSec);
  bool netReady = wifiReady && !netBackoffActive;

  // Sync NTP firsttime
  if (netReady && !liveData->params.ntpTimeSet && liveData->settings.ntpEnabled)
  {
    ntpSync();
  }

  // Upload to custom API
  if (netReady && liveData->params.currentTime - liveData->params.lastRemoteApiSent > liveData->settings.remoteUploadIntervalSec &&
      liveData->settings.remoteUploadIntervalSec != 0)
  {
    liveData->params.lastRemoteApiSent = liveData->params.currentTime;
    syslog->info(DEBUG_COMM, "Remote send tick");
    if (netSendQueue != nullptr)
    {
      NetSendJob job{false};
      if (xQueueSend(netSendQueue, &job, 0) != pdTRUE)
      {
        NetSendJob discard{};
        xQueueReceive(netSendQueue, &discard, 0);
        if (xQueueSend(netSendQueue, &job, 0) != pdTRUE)
        {
          syslog->info(DEBUG_COMM, "Net send queue full, dropping remote send");
        }
      }
    }
  }

  // Upload to ABRP
  if (netReady && liveData->params.currentTime - liveData->params.lastAbrpSent > liveData->settings.remoteUploadAbrpIntervalSec &&
      liveData->settings.remoteUploadAbrpIntervalSec != 0)
  {
    syslog->info(DEBUG_COMM, "ABRP send tick");
    if (netSendQueue != nullptr)
    {
      const bool netSendBusy = netSendInProgress || (uxQueueMessagesWaiting(netSendQueue) > 0);
      if (netSendBusy)
      {
        syslog->info(DEBUG_COMM, "Net send busy, skipping ABRP enqueue");
      }
      else
      {
        NetSendJob job{true};
        if (xQueueSend(netSendQueue, &job, 0) == pdTRUE)
        {
          liveData->params.lastAbrpSent = liveData->params.currentTime;
        }
        else
        {
          syslog->info(DEBUG_COMM, "Net send queue full, dropping ABRP send");
        }
      }
    }
  }

  // Contribute anonymous data
  if (netReady && liveData->settings.contributeData == 1 &&
      liveData->params.contributeStatus == CONTRIBUTE_NONE && liveData->params.currentTime - liveData->params.lastContributeSent > 60)
  {
    liveData->params.lastContributeSent = liveData->params.currentTime;
    liveData->params.contributeStatus = CONTRIBUTE_WAITING;
  }
  if (netReady && liveData->settings.contributeData == 1 &&
      liveData->params.contributeStatus == CONTRIBUTE_READY_TO_SEND)
  {
    netContributeData();
  }
}

/**
 * Send data
 **/
bool Board320_240::netSendData(bool sendAbrp)
{
  int64_t startTime2 = esp_timer_get_time();
  uint16_t rc = 0;
  const bool netDebug = liveData->settings.debugLevel >= DEBUG_GSM;
  const bool wifiReady = (liveData->settings.wifiEnabled == 1 && WiFi.status() == WL_CONNECTED);

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

  if (!wifiReady)
  {
    syslog->println("WiFi not connected, skipping data send");
    return false;
  }

  syslog->println("Start HTTP POST...");

  if (!sendAbrp && liveData->settings.remoteUploadIntervalSec != 0)
  {
    if (strlen(liveData->settings.remoteApiUrl) == 0 ||
        strcmp(liveData->settings.remoteApiUrl, "not_set") == 0 ||
        strstr(liveData->settings.remoteApiUrl, "http") == nullptr)
    {
      syslog->println("Remote API URL not set, skipping send");
      return false;
    }

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
    if (isGpsFixUsable(liveData))
    {
      jsonData["gpsLat"] = liveData->params.gpsLat;
      jsonData["gpsLon"] = liveData->params.gpsLon;
      jsonData["gpsAlt"] = liveData->params.gpsAlt;
      jsonData["gpsSpeed"] = liveData->params.speedKmhGPS;
      if (liveData->params.gpsHeadingDeg >= 0)
        jsonData["gpsHeading"] = liveData->params.gpsHeadingDeg;
    }

    char payload[768];
    serializeJson(jsonData, payload);

    if (netDebug)
    {
      syslog->print("Sending payload: ");
      syslog->println(payload);

      syslog->print("Remote API server: ");
      syslog->println(liveData->settings.remoteApiUrl);
    }
    else
    {
      syslog->println("Sending data to remote API");
    }

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
          if (isGpsFixUsable(liveData))
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

            if (liveData->params.gpsHeadingDeg >= 0)
            {
              strcpy(topic + strlen(liveData->settings.mqttPubTopic), "/gpsHeading");
              dtostrf(liveData->params.gpsHeadingDeg, 1, 2, tmpVal);
              client.publish(topic, tmpVal);
            }
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
      updateNetAvailability(true);
    }
    else
    {
      // Failed...
      syslog->print("HTTP POST error: ");
      syslog->println(rc);
      updateNetAvailability(false);
    }
  }
  else if (sendAbrp && liveData->settings.remoteUploadAbrpIntervalSec != 0)
  {
    if (strlen(liveData->settings.abrpApiToken) == 0 ||
        strcmp(liveData->settings.abrpApiToken, "empty") == 0 ||
        strcmp(liveData->settings.abrpApiToken, "not_set") == 0)
    {
      syslog->println("ABRP token not set, skipping send");
      return false;
    }

    StaticJsonDocument<768> jsonData;

    jsonData["car_model"] = getCarModelAbrpStr(liveData->settings.carType);
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

    if (isGpsFixUsable(liveData))
    {
      jsonData["lat"] = liveData->params.gpsLat;
      jsonData["lon"] = liveData->params.gpsLon;
      jsonData["elevation"] = liveData->params.gpsAlt;
      if (liveData->params.gpsHeadingDeg >= 0)
        jsonData["heading"] = liveData->params.gpsHeadingDeg;
    }

    jsonData["capacity"] = liveData->params.batteryTotalAvailableKWh;
    jsonData["kwh_charged"] = liveData->params.cumulativeEnergyChargedKWh;
    jsonData["soh"] = liveData->params.sohPerc;
    jsonData["ext_temp"] = liveData->params.outdoorTemperature;
    if (liveData->params.indoorTemperature != -100)
    {
      jsonData["cabin_temp"] = liveData->params.indoorTemperature;
    }
    jsonData["batt_temp"] = liveData->params.batMinC;
    jsonData["voltage"] = liveData->params.batVoltage;
    jsonData["current"] = liveData->params.batPowerAmp * -1;
    if (liveData->params.odoKm > 0)
      jsonData["odometer"] = liveData->params.odoKm;

    char payload[kAbrpPayloadBufferSize];
    size_t payloadLength = serializeJson(jsonData, payload, sizeof(payload));
    if (payloadLength == 0)
    {
      syslog->println("Failed to serialize ABRP payload");
      return false;
    }

    if (liveData->settings.abrpSdcardLog != 0 && liveData->settings.remoteUploadAbrpIntervalSec > 0)
    {
      queueAbrpSdLog(payload, payloadLength, liveData->params.currentTime, liveData->params.operationTimeSec, liveData->params.currTimeSyncWithGps);
    }

    char encodedPayload[kAbrpFormBufferSize];
    encodeQuotes(encodedPayload, sizeof(encodedPayload), payload);

    char dta[kAbrpFormBufferSize];
    int dtaLength = snprintf(dta, sizeof(dta), "api_key=%s&token=%s&tlm=%s", ABRP_API_KEY, liveData->settings.abrpApiToken, encodedPayload);
    if (dtaLength < 0 || static_cast<size_t>(dtaLength) >= sizeof(dta))
    {
      syslog->println("ABRP payload too large, skipping send");
      return false;
    }

    if (netDebug)
    {
      syslog->print("Sending data: ");
      syslog->println(dta); // dta is total string sent to ABRP API including api-key and user-token (could be sensitive data to log)
    }
    else
    {
      syslog->println("Sending data to ABRP");
    }

    // Code for sending https data to ABRP api server
    rc = 0;
    if (liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI && liveData->settings.wifiEnabled == 1)
    {
      WiFiClientSecure client;
      HTTPClient http;

      client.setInsecure();
      http.begin(client, "https://api.iternio.com/1/tlm/send");
      http.setConnectTimeout(1000);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      const size_t bodyLength = static_cast<size_t>(dtaLength);
      rc = http.POST((uint8_t *)dta, bodyLength);

      if (rc == HTTP_CODE_OK)
      {
        // Request successful
        if (netDebug)
        {
          String payload = http.getString();
          syslog->println("HTTP Response: " + payload);
        }
      }
      else
      {
        // Handle different HTTP status codes
        syslog->println("HTTP Request failed with code: " + String(rc));
      }

      http.end();
      client.stop();
    }

    if (rc == 200)
    {
      syslog->println("HTTP POST send successful");
      liveData->params.lastSuccessNetSendTime = liveData->params.currentTime;
      updateNetAvailability(true);
    }
    else
    {
      // Failed...
      syslog->print("HTTP POST error: ");
      syslog->println(rc);
      updateNetAvailability(false);
    }
  }
  else
  {
    syslog->println("Well... This not gonna happen... (Board320_240::netSendData();)"); // Just for debug reasons...
  }
  // next three rows are for time measurement of this function
  int64_t endTime2 = esp_timer_get_time();
  int64_t duration2 = endTime2 - startTime2;
  // syslog->println("Time taken by function: netSendData() " + String(duration2) + " microseconds");

  return true;
}

void Board320_240::queueAbrpSdLog(const char *payload, size_t length, time_t currentTime, uint64_t operationTimeSec, bool timeSyncWithGps)
{
  if (abrpSdLogQueue == nullptr || payload == nullptr)
  {
    return;
  }
  if (liveData->params.stopCommandQueue)
  {
    return;
  }

  AbrpLogEntry entry{};
  entry.length = (length >= sizeof(entry.payload)) ? sizeof(entry.payload) - 1 : length;
  memcpy(entry.payload, payload, entry.length);
  entry.payload[entry.length] = '\0';
  entry.currentTime = currentTime;
  entry.operationTimeSec = operationTimeSec;
  entry.timeSyncWithGps = timeSyncWithGps;

  if (xQueueSend(abrpSdLogQueue, &entry, 0) != pdTRUE)
  {
    AbrpLogEntry discard{};
    xQueueReceive(abrpSdLogQueue, &discard, 0);
    if (xQueueSend(abrpSdLogQueue, &entry, 0) != pdTRUE)
    {
      syslog->info(DEBUG_SDCARD, "ABRP SD log queue full, dropping log entry");
    }
  }
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
  if (liveData->settings.wifiEnabled == 1 && WiFi.status() == WL_CONNECTED &&
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
      liveData->contributeDataJson += "\"carType\": \"" + getCarModelAbrpStr(liveData->settings.carType) + "\", ";
      liveData->contributeDataJson += "\"carVin\": \"" + String(liveData->params.carVin) + "\", ";
      liveData->contributeDataJson += "\"stoppedQueue\": " + String(liveData->params.stopCommandQueue) + ", ";
      liveData->contributeDataJson += "\"ignitionOn\": " + String(liveData->params.ignitionOn) + ", ";
      liveData->contributeDataJson += "\"chargingOn\": " + String(liveData->params.chargingOn) + ", ";
      liveData->contributeDataJson += "\"socPerc\": " + String(liveData->params.socPerc, 0) + ", ";
      if (liveData->params.socPercBms != -1)
        liveData->contributeDataJson += "\"socPercBms\": " + String(liveData->params.socPercBms, 0) + ", ";
      liveData->contributeDataJson += "\"sohPerc\": " + String(liveData->params.sohPerc, 0) + ", ";
      liveData->contributeDataJson += "\"batPowerKw\": " + String(liveData->params.batPowerKw, 3) + ", ";
      liveData->contributeDataJson += "\"batPowerKwh100\": " + String(liveData->params.batPowerKwh100, 3) + ", ";
      liveData->contributeDataJson += "\"batVoltage\": " + String(liveData->params.batVoltage, 0) + ", ";
      liveData->contributeDataJson += "\"batCurrentAmp\": " + String(liveData->params.batPowerAmp, 0) + ", ";
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
      if (isGpsFixUsable(liveData))
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
      updateNetAvailability(true);

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
      updateNetAvailability(false);
    }
  }
#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3

  return true;
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
  auto beginGpsUart = [&](unsigned long baud) {
    if (liveData->settings.gpsHwSerialPort == 2)
    {
      gpsHwUart->begin(baud, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);
    }
    else
    {
      gpsHwUart->begin(baud);
    }
    delay(120);
    while (gpsHwUart->available())
    {
      gpsHwUart->read();
    }
  };

  auto hasValidNmea = [&](uint32_t timeoutMs) -> bool {
    TinyGPSPlus probeGps;
    const uint32_t startMs = millis();
    while (millis() - startMs < timeoutMs)
    {
      while (gpsHwUart->available())
      {
        int ch = gpsHwUart->read();
        if (ch >= 0)
        {
          probeGps.encode(static_cast<char>(ch));
        }
      }
      if (probeGps.passedChecksum() > 0)
      {
        return true;
      }
      delay(5);
    }
    return false;
  };

  unsigned long detectedBaud = liveData->settings.gpsSerialPortSpeed;
  unsigned long baudCandidates[4] = {0, 0, 0, 0};
  uint8_t baudCandidateCount = 0;

  auto pushBaudCandidate = [&](unsigned long baud) {
    if (baud == 0)
    {
      return;
    }
    for (uint8_t i = 0; i < baudCandidateCount; i++)
    {
      if (baudCandidates[i] == baud)
      {
        return;
      }
    }
    if (baudCandidateCount < (sizeof(baudCandidates) / sizeof(baudCandidates[0])))
    {
      baudCandidates[baudCandidateCount++] = baud;
    }
  };

  pushBaudCandidate(liveData->settings.gpsSerialPortSpeed);
  if (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_M5_GNSS)
  {
    pushBaudCandidate(38400);
    pushBaudCandidate(115200);
    pushBaudCandidate(9600);
  }
  else if (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_NEO_M8N)
  {
    pushBaudCandidate(9600);
    pushBaudCandidate(38400);
    pushBaudCandidate(115200);
  }
  else
  {
    pushBaudCandidate(9600);
    pushBaudCandidate(38400);
    pushBaudCandidate(115200);
  }

  bool baudDetected = false;
  for (uint8_t i = 0; i < baudCandidateCount; i++)
  {
    beginGpsUart(baudCandidates[i]);
    if (hasValidNmea(1500))
    {
      detectedBaud = baudCandidates[i];
      baudDetected = true;
      break;
    }
  }

  if (!baudDetected)
  {
    detectedBaud = liveData->settings.gpsSerialPortSpeed;
    beginGpsUart(detectedBaud);
    syslog->print("GPS baud auto-detect failed, using configured speed: ");
    syslog->println(detectedBaud);
  }
  else
  {
    if (detectedBaud != liveData->settings.gpsSerialPortSpeed)
    {
      syslog->print("GPS baud auto-detected: ");
      syslog->println(detectedBaud);
      liveData->settings.gpsSerialPortSpeed = detectedBaud;
    }
    else
    {
      syslog->print("GPS baud confirmed: ");
      syslog->println(detectedBaud);
    }
  }

  // M5 GPS MODULE with int.&ext. antenna (u-blox NEO-M8N)
  // https://shop.m5stack.com/products/gps-module
  if (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_NEO_M8N)
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
  if (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_M5_GNSS)
  {
    uint8_t ubloxconfig[] = {
        // Disable sleep mode
        0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x37,
        // Enable GPGGA & RMC sentences (only these are evaluated by TinyGPS++)
        0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x05, 0x38, // GGA
        0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x09, 0x54, // RMC
        // Disable all other NMEA sentences to save bandwidth
        0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B, // GLL
        0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32, // GSA
        0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39, // GSV
        0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47, // VTG
        // Enable GPS, GLONASS, Galileo, BeiDou
        0xB5, 0x62, 0x06, 0x3E, 0x24, 0x00, 0x00, 0x20, 0x20, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01,
        0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01,
        0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0xA4, 0x47,
        // Set NAV5 model to automotive, static hold on 0.5m/s, 3m
        0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27,
        0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x32, 0x3C, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x78,
        // Dynamic Model: Automotive
        0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27,
        0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x32, 0x3C, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x78,
        // Disable SBAS
        0xB5, 0x62, 0x06, 0x16, 0x08, 0x00, 0x00, 0x03, 0x03, 0x00, 0x51, 0x08, 0x00, 0x00, 0x84, 0x15,
        // 100ms update interval for higher refresh rate
        0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12,
        // Save changes
        0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0x1D, 0xAB};

    gpsHwUart->write(ubloxconfig, sizeof(ubloxconfig));
  }
}

/**
 * This function uploads log files from the SD card to the EvDash server.
 */
void Board320_240::uploadSdCardLogToEvDashServer()
{
  syslog->println("uploadSdCardLogToEvDashServer");
  if (!liveData->params.sdcardInit)
  {
    syslog->println("SD card not initialized");
    displayMessage("SDCARD", "Not mounted");
    return;
  }
  if (!(liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI && liveData->settings.wifiEnabled == 1))
  {
    displayMessage("Error", "Wifi not enabled");
    return;
  }

  File dir = SD.open("/");
  String fileName;
  WiFiClientSecure client;
  HTTPClient http;
  String uploadedStr;
  uint16_t rc = 0;
  uint32_t part, sz, uploaded, cntLogs, cntUploaded;
  bool errorUploadFile;
  char *buff = (char *)malloc(16384);

  displayMessage("Upload logs to server", "No files found");
  cntLogs = cntUploaded = 0;

  while (true)
  {
    uploaded = part = 0;
    errorUploadFile = true;
    File entry = dir.openNextFile(FILE_READ);
    if (!entry)
      break;
    if (!entry.isDirectory())
    {
      fileName = "/";
      fileName += entry.name();
      if (fileName.indexOf(".json") != -1 && fileName.indexOf(liveData->params.sdcardFilename) == -1)
      {
        cntLogs++;
        displayMessage(fileName.c_str(), "Uploading...");
        memset(buff, 0, sizeof(buff));
        while ((sz = entry.readBytes(buff, 16383)) > 0)
        {
          errorUploadFile = false;
          syslog->println("Part:" + String(part) + "\tSize:" + String(sz));
          uploadedStr = "Uploading... " + String(uploaded / 1024) + " / " + String(entry.size() / 1024) + "kB";
          displayMessage(fileName.c_str(), uploadedStr.c_str());

          client.setInsecure();
          http.begin(client, "https://evdash.next176.sk/api/upload.php?token=" + String(liveData->settings.contributeToken) +
                                 "&filename=" + String(entry.name()) + "&part=" + String(part));
          http.setConnectTimeout(1000);
          rc = http.POST((uint8_t *)buff, sz);
          uploaded += sz;
          if (rc == HTTP_CODE_OK)
          {
            String payload = http.getString();
            syslog->println(payload);
            if (payload.indexOf("status\":\"ok") == -1)
            {
              errorUploadFile = true;
              break;
            }
          }
          else
          {
            syslog->println("HTTP code: " + String(rc));
            errorUploadFile = true;
            break;
          }
          part++;
          memset(buff, 0, sizeof(buff));
        }

        displayMessage(fileName.c_str(), (errorUploadFile ? "Upload error..." : "Uploaded..."));
        if (!errorUploadFile)
        {
          SD.remove(fileName);
          cntUploaded++;
        }
      }
    }
    entry.close();
  }

  uploadedStr = String(cntUploaded) + " of " + String(cntLogs);
  displayMessage("Uploaded", uploadedStr.c_str());
  free(buff);
}
