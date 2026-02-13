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
#include <esp_heap_caps.h>
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
  constexpr uint32_t kNetRetryIntervalSec = 30;
  constexpr uint32_t kNetFailureStaleResetSec = 300;
  constexpr uint32_t kNetFailureFallbackSec = 180;
  constexpr uint16_t kNetFailureFallbackCount = 3;
  constexpr size_t kAbrpPayloadBufferSize = 768;
  constexpr size_t kAbrpFormBufferSize = 1536;
  constexpr uint16_t kAbrpHttpsConnectTimeoutMs = 1000;
  constexpr uint16_t kAbrpHttpsIoTimeoutMs = 2500;
  constexpr float kGpsMaxSpeedKmh = 250.0f;
  constexpr float kGpsJitterMeters = 200.0f;
  constexpr float kGpsMaxJumpMetersShort = 2000.0f;
  constexpr uint32_t kGpsShortJumpWindowSec = 5;
  constexpr uint32_t kGpsReacquireAfterSec = 900;
  constexpr uint32_t kMotionWakeResetSec = 900;
  constexpr size_t kContributeJsonDocCapacity = 12288;
  constexpr uint8_t kContributeRawFrameUploadMax = 24;
  constexpr bool kContributeIncludeRawLatency = false;
  constexpr bool kContributeRetryOnceOnFail = false;
  constexpr bool kContributeRawTlsFallbackOnTlsMem = false;
  constexpr bool kContributeEnableTcpProbe = false;
  constexpr bool kContributeHttpFallbackOnTlsMem = false;
  constexpr uint16_t kContributeHttpsConnectTimeoutMs = 4000;
  constexpr uint16_t kContributeHttpsIoTimeoutMs = 4500;
  constexpr uint16_t kContributeHttpConnectTimeoutMs = 2000;
  constexpr uint16_t kContributeHttpReadTimeoutMs = 3500;
  constexpr size_t kContributeResponseBufferCap = 2048;
  constexpr uint32_t kFirmwareVersionCheckCooldownMs = 30000;
  constexpr uint16_t kFirmwareVersionHttpTimeoutMs = 4500;
  constexpr uint32_t kPairStatusPollIntervalMs = 8000;
  constexpr uint16_t kPairHttpTimeoutMs = 4500;

  // ABRP upload runs in the main loop, so avoid large temporary stack buffers here.
  // These static buffers are only used from the single-threaded board loop path.
  static char gAbrpPayloadBuffer[kAbrpPayloadBufferSize];
  static char gAbrpEncodedPayloadBuffer[kAbrpFormBufferSize];
  static char gAbrpFormBuffer[kAbrpFormBufferSize];

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

  float roundToPrecision(float value, float multiplier)
  {
    if (!isfinite(value))
    {
      return value;
    }
    return roundf(value * multiplier) / multiplier;
  }

  float normalizeHeadingDeg(float headingDeg)
  {
    if (!isfinite(headingDeg))
    {
      return -1.0f;
    }
    // Some GPS modules provide tenths without decimal point (e.g. 1715 => 171.5 deg).
    if (headingDeg > 360.0f && headingDeg <= 3600.0f)
    {
      headingDeg /= 10.0f;
    }
    while (headingDeg < 0.0f)
    {
      headingDeg += 360.0f;
    }
    while (headingDeg >= 360.0f)
    {
      headingDeg -= 360.0f;
    }
    return headingDeg;
  }

  String formatTimestampYyMmDdHhIiSs(time_t timestamp)
  {
    if (timestamp <= 0)
    {
      return "";
    }
    struct tm tmValue;
    if (localtime_r(&timestamp, &tmValue) == nullptr)
    {
      return "";
    }
    char out[16] = {0};
    snprintf(out, sizeof(out), "%02d%02d%02d%02d%02d%02d",
             (tmValue.tm_year + 1900) % 100,
             tmValue.tm_mon + 1,
             tmValue.tm_mday,
             tmValue.tm_hour,
             tmValue.tm_min,
             tmValue.tm_sec);
    return String(out);
  }

  String normalizeDeviceIdForApi(const String &deviceId)
  {
    String normalized = "";
    normalized.reserve(deviceId.length());
    for (size_t i = 0; i < deviceId.length(); i++)
    {
      const char ch = deviceId.charAt(i);
      if (ch >= '0' && ch <= '9')
      {
        normalized += ch;
      }
      else if (ch >= 'A' && ch <= 'F')
      {
        normalized += ch;
      }
      else if (ch >= 'a' && ch <= 'f')
      {
        normalized += static_cast<char>(ch - ('a' - 'A'));
      }
    }
    if (normalized.length() == 32)
    {
      return normalized;
    }
    return deviceId;
  }

  bool hasContributeRawFrames(const LiveData *liveData)
  {
    if (liveData == nullptr)
    {
      return false;
    }
    for (uint8_t i = 0; i < liveData->contributeRawFrameCount; i++)
    {
      const LiveData::ContributeRawFrame &raw = liveData->contributeRawFrames[i];
      if (raw.key[0] != '\0' && raw.value[0] != '\0')
      {
        return true;
      }
    }
    return false;
  }

  bool isContributeV2SnapshotEffectivelyEmpty(const LiveData *liveData)
  {
    if (liveData == nullptr)
    {
      return true;
    }
    if (liveData->params.getValidResponse)
    {
      return false;
    }
    if (isGpsFixUsable(liveData))
    {
      return false;
    }
    if (liveData->params.ignitionOn || liveData->params.chargingOn ||
        liveData->params.chargerACconnected || liveData->params.chargerDCconnected)
    {
      return false;
    }
    if (hasContributeRawFrames(liveData))
    {
      return false;
    }
    if (liveData->params.socPerc >= 0 || liveData->params.sohPerc >= 0 ||
        liveData->params.batPowerKw > -999.0f || liveData->params.batPowerKwh100 >= 0 ||
        liveData->params.batVoltage >= 0 || liveData->params.batPowerAmp > -999.0f ||
        liveData->params.auxVoltage >= 0 || liveData->params.auxCurrentAmp > -999.0f ||
        liveData->params.batMinC > -100.0f || liveData->params.batMaxC > -100.0f ||
        liveData->params.indoorTemperature > -100.0f || liveData->params.outdoorTemperature > -100.0f ||
        liveData->params.speedKmh >= 0 || liveData->params.odoKm >= 0 ||
        liveData->params.batCellMinV >= 0 || liveData->params.batCellMaxV >= 0 ||
        liveData->params.batCellMinVNo != 255 ||
        liveData->params.cumulativeEnergyChargedKWh >= 0 ||
        liveData->params.cumulativeEnergyDischargedKWh >= 0)
    {
      return false;
    }
    return true;
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
  showBootProgress("Display initialization...", "Preparing resources", TFT_RED);

  bool psramUsed = false; // 320x240 16bpp sprites requires psram
#if defined(ESP32) && defined(CONFIG_SPIRAM_SUPPORT)
  if (psramFound())
    psramUsed = true;
#endif
  printHeapMemory();
  syslog->println((psramUsed) ? "Sprite 16" : "Sprite 8");
  spriteColorDepth = (psramUsed) ? 16 : 8;
  spr.setColorDepth(spriteColorDepth);
  spr.createSprite(320, 240);
  menuBackbufferActive = false;
  liveData->params.spriteInit = true;
  printHeapMemory();
  showBootProgress("Display initialization...", "Framebuffer ready", TFT_PURPLE);

  // Show test data on right button during boot device
  liveData->params.displayScreen = liveData->settings.defaultScreen;
  showBootProgress("Checking test mode...", "Hold right button for demo", TFT_YELLOW);
  if (isButtonPressed(pinButtonRight))
  {
    syslog->printf("Right button pressed");
    loadTestData();
    printHeapMemory();
    showBootProgress("Demo mode enabled", "Using static test data", TFT_YELLOW);
  }

  // Wifi
  // Starting Wifi after BLE prevents reboot loop
  const bool wifiRequiredByAdapter = (liveData->settings.commType == COMM_TYPE_OBD2_WIFI);
  if (!liveData->params.wifiApMode &&
      (liveData->settings.wifiEnabled == 1 || wifiRequiredByAdapter))
  {
    showBootProgress("WiFi initialization...", "Connecting to configured AP", TFT_BLUE);
    wifiSetup();
    printHeapMemory();
  }
  else
  {
    showBootProgress("WiFi initialization...", "Skipped (disabled)", TFT_BLUE);
  }

  // Init GPS
  if (liveData->settings.gpsHwSerialPort <= 2)
  {
    showBootProgress("GPS initialization...", "Starting GPS serial", TFT_SKYBLUE);
    initGPS();
    printHeapMemory();
  }
  else
  {
    showBootProgress("GPS initialization...", "Skipped (not configured)", TFT_SKYBLUE);
  }

  // SD card
  if (liveData->settings.sdcardEnabled == 1)
  {
    showBootProgress("SD card initialization...", "Mounting storage", TFT_ORANGE);
    if (sdcardMount() && liveData->settings.sdcardAutstartLog == 1)
    {
      syslog->println("Toggle recording on SD card");
      sdcardToggleRecording();
      printHeapMemory();
      showBootProgress("SD card initialization...", "Autolog enabled", TFT_ORANGE);
    }
  }
  else
  {
    showBootProgress("SD card initialization...", "Skipped (disabled)", TFT_ORANGE);
  }

  // Init comm device
  showBootProgress("Adapter initialization...", "Starting OBD2/CAN comm", TFT_SILVER);
  BoardInterface::afterSetup();
  printHeapMemory();

  syslog->println("COMM in main loop (threading removed)");
  syslog->println("NET send in main loop (queue/task removed)");

  showBootProgress("Finalizing startup...", "Syncing clocks", TFT_GREEN);
  showTime();
  showBootProgress("Startup completed", "Loading main screen", TFT_GREEN);

  liveData->params.wakeUpTime = liveData->params.currentTime;
  liveData->params.lastCanbusResponseTime = liveData->params.currentTime;
  liveData->params.booting = false;
}

void Board320_240::showBootProgress(const char *step, const char *detail, uint16_t bgColor)
{
  tft.fillScreen(bgColor);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setTextDatum(TL_DATUM);
  tft.setFont(fontFont2);
  tft.drawString("Boot:", 12, 90);
  if (step != nullptr && step[0] != '\0')
    tft.drawString(step, 12, 114);
  if (detail != nullptr && detail[0] != '\0')
    tft.drawString(detail, 12, 138);
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

bool Board320_240::ensureMenuBackbuffer()
{
  if (menuBackbufferActive)
  {
    return true;
  }

  spr.deleteSprite();
  spr.setColorDepth(spriteColorDepth);
  if (spr.createSprite(320, 240 + (menuBackbufferOverscanPx * 2)) == nullptr)
  {
    // Fall back to normal-size sprite when overscan buffer cannot be allocated.
    spr.deleteSprite();
    spr.setColorDepth(spriteColorDepth);
    if (spr.createSprite(320, 240) == nullptr)
    {
      liveData->params.spriteInit = false;
    }
    return false;
  }

  menuBackbufferActive = true;
  return true;
}

void Board320_240::releaseMenuBackbuffer()
{
  if (!menuBackbufferActive)
  {
    return;
  }

  spr.deleteSprite();
  spr.setColorDepth(spriteColorDepth);
  if (spr.createSprite(320, 240) == nullptr)
  {
    liveData->params.spriteInit = false;
  }
  menuBackbufferActive = false;
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
  static uint32_t blankScreenStartMs = 0;
  uint8_t lcdBrightnessPerc;

  lcdBrightnessPerc = liveData->settings.lcdBrightness;
  if (lcdBrightnessPerc == 0)
  { // automatic brightness (based on gps&and sun angle)
    lcdBrightnessPerc = ((liveData->params.lcdBrightnessCalc == -1) ? 100 : liveData->params.lcdBrightnessCalc);
  }
  if (liveData->params.displayScreen == SCREEN_BLANK)
  {
    const uint32_t nowMs = millis();
    if (blankScreenStartMs == 0)
    {
      blankScreenStartMs = nowMs;
      tft.fillScreen(TFT_BLACK);
    }
    if (nowMs - blankScreenStartMs >= 5000U)
    {
      turnOffScreen();
    }
    return;
  }
  blankScreenStartMs = 0;
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
  displayMessage(row1, row2, nullptr);
}

void Board320_240::displayMessage(const char *row1, const char *row2, const char *row3)
{
  const uint16_t height = tft.height();
  const uint16_t width = tft.width();
  const char *rows[3] = {row1, row2, row3};
  uint8_t lineCount = 0;
  for (uint8_t i = 0; i < 3; i++)
  {
    if (rows[i] != nullptr && rows[i][0] != '\0')
      lineCount++;
  }
  if (lineCount == 0)
    return;
  const int16_t dialogW = width - 24;
  const int16_t dialogH = 82 + ((lineCount > 0 ? lineCount - 1 : 0) * 24);
  const int16_t dialogX = (width - dialogW) / 2;
  int16_t dialogY = (height - dialogH) / 2 - 12;
  if (dialogY < 6)
    dialogY = 6;
  const int16_t radius = 12;
  const uint16_t bg = 0x18C3; // darker panel for better contrast
  const uint16_t border = TFT_DARKGREY;
  const uint16_t textColor = TFT_WHITE;

  syslog->print("Message: ");
  syslog->print(row1);
  syslog->print(" ");
  syslog->println(row2);
  messageDialogVisible = true;

  if (liveData->params.spriteInit)
  {
    spr.fillRoundRect(dialogX, dialogY, dialogW, dialogH, radius, bg);
    spr.drawRoundRect(dialogX, dialogY, dialogW, dialogH, radius, border);
    spr.setTextColor(textColor, bg);
    spr.setTextDatum(TC_DATUM);
    sprSetFont(fontRobotoThin24);
    int16_t textY = dialogY + 24;
    for (uint8_t i = 0; i < 3; i++)
    {
      if (rows[i] != nullptr && rows[i][0] != '\0')
      {
        sprDrawString(rows[i], width / 2, textY);
        textY += 32;
      }
    }
    spr.pushSprite(0, 0);
  }
  else
  {
    tft.fillRoundRect(dialogX, dialogY, dialogW, dialogH, radius, bg);
    tft.drawRoundRect(dialogX, dialogY, dialogW, dialogH, radius, border);
    tft.setTextColor(textColor, bg);
    tft.setTextDatum(TC_DATUM);
    tft.setFont(fontRobotoThin24);
    int16_t textY = dialogY + 24;
    for (uint8_t i = 0; i < 3; i++)
    {
      if (rows[i] != nullptr && rows[i][0] != '\0')
      {
        tft.drawString(rows[i], width / 2, textY);
        textY += 32;
      }
    }
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
  const uint16_t height = tft.height();
  const uint16_t width = tft.width();
  const int16_t dialogW = width - 24;
  const int16_t dialogH = 142;
  const int16_t dialogX = (width - dialogW) / 2;
  int16_t dialogY = (height - dialogH) / 2 - 12;
  if (dialogY < 6)
    dialogY = 6;
  const int16_t radius = 12;
  const uint16_t bg = 0x18C3; // darker panel for better contrast
  const uint16_t border = TFT_DARKGREY;
  const uint16_t textColor = TFT_WHITE;
  const uint16_t yesBg = TFT_DARKGREEN;
  const uint16_t noBg = TFT_DARKRED;
  const int16_t btnW = 90;
  const int16_t btnH = 42; // ~30% higher than original 32px
  const int16_t btnY = dialogY + dialogH - btnH - 10;
  const int16_t btnYesX = dialogX + 16;
  const int16_t btnNoX = dialogX + dialogW - btnW - 16;
  syslog->print("Confirm: ");
  syslog->print(row1);
  syslog->print(" ");
  syslog->println(row2);
  messageDialogVisible = false;

  if (liveData->params.spriteInit)
  {
    spr.fillRoundRect(dialogX, dialogY, dialogW, dialogH, radius, bg);
    spr.drawRoundRect(dialogX, dialogY, dialogW, dialogH, radius, border);
    spr.setTextColor(textColor, bg);
    spr.setTextDatum(TC_DATUM);
    sprSetFont(fontRobotoThin24);
    sprDrawString(row1, width / 2, dialogY + 22);
    sprDrawString(row2, width / 2, dialogY + 56);

    spr.fillRoundRect(btnYesX, btnY, btnW, btnH, 8, yesBg);
    spr.drawRoundRect(btnYesX, btnY, btnW, btnH, 8, border);
    spr.fillRoundRect(btnNoX, btnY, btnW, btnH, 8, noBg);
    spr.drawRoundRect(btnNoX, btnY, btnW, btnH, 8, border);

    spr.setTextColor(TFT_WHITE, yesBg);
    spr.setTextDatum(MC_DATUM);
    sprDrawString("YES", btnYesX + btnW / 2, btnY + btnH / 2 + 1);
    spr.setTextColor(TFT_WHITE, noBg);
    sprDrawString("NO", btnNoX + btnW / 2, btnY + btnH / 2 + 1);
    spr.pushSprite(0, 0);
  }
  else
  {
    tft.fillRoundRect(dialogX, dialogY, dialogW, dialogH, radius, bg);
    tft.drawRoundRect(dialogX, dialogY, dialogW, dialogH, radius, border);
    tft.setTextColor(textColor, bg);
    tft.setTextDatum(TC_DATUM);
    tft.setFont(fontRobotoThin24);
    tft.drawString(row1, width / 2, dialogY + 22);
    tft.drawString(row2, width / 2, dialogY + 56);

    tft.fillRoundRect(btnYesX, btnY, btnW, btnH, 8, yesBg);
    tft.drawRoundRect(btnYesX, btnY, btnW, btnH, 8, border);
    tft.fillRoundRect(btnNoX, btnY, btnW, btnH, 8, noBg);
    tft.drawRoundRect(btnNoX, btnY, btnW, btnH, 8, border);

    tft.setTextColor(TFT_WHITE, yesBg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("YES", btnYesX + btnW / 2, btnY + btnH / 2 + 1);
    tft.setTextColor(TFT_WHITE, noBg);
    tft.drawString("NO", btnNoX + btnW / 2, btnY + btnH / 2 + 1);
  }

  bool res = false;
  modalDialogActive = true;
  for (uint16_t i = 0; i < 2000 * 100; i++)
  {
    boardLoop();
    int16_t tx = 0, ty = 0;
    if (getTouch(tx, ty))
    {
      if (tx >= btnYesX && tx <= (btnYesX + btnW) && ty >= btnY && ty <= (btnY + btnH))
      {
        res = true;
        break;
      }
      if (tx >= btnNoX && tx <= (btnNoX + btnW) && ty >= btnY && ty <= (btnY + btnH))
      {
        res = false;
        break;
      }
    }
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
  modalDialogActive = false;
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
 * Draw currently selected screen into `spr` without pushing it to TFT.
 */
bool Board320_240::drawActiveScreenToSprite()
{

  // Headlights reminders
  if (!testDataMode && liveData->settings.headlightsReminder == 1 && liveData->params.forwardDriveMode &&
      !liveData->params.headLights && !liveData->params.autoLights)
  {
    spr.fillSprite(TFT_RED);
    sprSetFont(fontOrbitronLight32);
    spr.setTextColor(TFT_WHITE);
    spr.setTextDatum(MC_DATUM);
    sprDrawString("! LIGHTS OFF !", 160, 120);
    return true;
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
    return false;
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

  const auto remoteApiConfiguredForWifi = [this]() -> bool {
    if (liveData->settings.remoteUploadIntervalSec == 0)
    {
      return false;
    }
    const char *url = liveData->settings.remoteApiUrl;
    if (url == nullptr || url[0] == '\0')
    {
      return false;
    }
    if (strcmp(url, "not_set") == 0)
    {
      return false;
    }
    return strstr(url, "http") != nullptr;
  }();

  const auto abrpConfiguredForWifi = [this]() -> bool {
    if (liveData->settings.remoteUploadAbrpIntervalSec == 0)
    {
      return false;
    }
    const char *token = liveData->settings.abrpApiToken;
    if (token == nullptr || token[0] == '\0')
    {
      return false;
    }
    if (strcmp(token, "empty") == 0 || strcmp(token, "not_set") == 0)
    {
      return false;
    }
    return true;
  }();

  const bool contributeOnlyConfigured =
      (liveData->settings.contributeData == 1) &&
      !remoteApiConfiguredForWifi &&
      !abrpConfiguredForWifi;

  const uint8_t wifiOkWindowSec = contributeOnlyConfigured ? 75 : 15;

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
              ? ((liveData->params.lastSuccessNetSendTime + wifiOkWindowSec >= liveData->params.currentTime) ? TFT_GREEN /* last request was 200 OK */ : TFT_YELLOW /* wifi connected but not send */)
              : TFT_RED; /* wifi not connected */

      // WiFi icon (upper arcs + center dot), tuned to be ~20% larger.
      const int wifiCx = 146;
      const int wifiCy = 11;
      const int wifiOuterR = 10;
      const int wifiMidR = 6;
      const int wifiInnerR = 4;
      spr.drawCircle(wifiCx, wifiCy, wifiOuterR, wifiColor);
      spr.drawCircle(wifiCx, wifiCy, wifiMidR, wifiColor);
      spr.drawCircle(wifiCx, wifiCy, wifiInnerR, wifiColor);
      spr.fillRect(wifiCx - (wifiOuterR + 1), wifiCy, (wifiOuterR + 1) * 2 + 1, wifiOuterR + 2, TFT_BLACK); // keep only upper arcs
      spr.fillCircle(wifiCx, wifiCy + 3, 2, wifiColor);

      if (liveData->params.isWifiBackupLive)
      {
        // Backup link badge.
        spr.fillCircle(wifiCx + 11, 2, 2, wifiColor);
      }
    }
    else if (liveData->params.displayScreen != SCREEN_BLANK)
    {
      spr.fillRect(308, 0, 5, 5,
                   (WiFi.status() == WL_CONNECTED) ? (liveData->params.lastSuccessNetSendTime + wifiOkWindowSec >= liveData->params.currentTime) ? TFT_GREEN /* last request was 200 OK */ : TFT_YELLOW /* wifi connected but not send */ : TFT_RED /* wifi not connected */
      );
    }
  }

  // Door status
  if (liveData->params.displayScreen == SCREEN_SPEED || liveData->params.displayScreenAutoMode == SCREEN_SPEED)
  {
    const bool showCarIcon = (liveData->params.trunkDoorOpen || liveData->params.leftFrontDoorOpen || liveData->params.rightFrontDoorOpen ||
                              liveData->params.leftRearDoorOpen || liveData->params.rightRearDoorOpen || liveData->params.hoodDoorOpen);
    if (showCarIcon)
    {
      const int16_t carX = 40;
      const int16_t carY = 40;
      const int16_t carW = 50;
      const int16_t carH = 90;
      const int16_t bodyX = carX + 3;
      const int16_t bodyY = carY + 6;
      const int16_t bodyW = carW - 6;
      const int16_t bodyH = carH - 12;
      const uint16_t bodyColor = 0x4208;
      const uint16_t cabinColor = 0x3186;
      const uint16_t glassColor = 0x7BEF;
      const uint16_t lineColor = 0x5AEB;

      // Top-view silhouette
      spr.fillRoundRect(bodyX, bodyY, bodyW, bodyH, 10, bodyColor);
      spr.drawRoundRect(bodyX, bodyY, bodyW, bodyH, 10, lineColor);
      spr.fillRoundRect(bodyX + 8, bodyY + 14, bodyW - 16, bodyH - 28, 8, cabinColor);
      spr.drawRoundRect(bodyX + 8, bodyY + 14, bodyW - 16, bodyH - 28, 8, lineColor);
      spr.fillRoundRect(bodyX + 10, bodyY + 18, bodyW - 20, 12, 4, glassColor);
      spr.fillRoundRect(bodyX + 10, bodyY + bodyH - 30, bodyW - 20, 12, 4, glassColor);
      spr.drawFastVLine(bodyX + (bodyW / 2), bodyY + 20, bodyH - 40, lineColor);
      spr.fillRoundRect(bodyX - 3, bodyY + 22, 3, 10, 2, lineColor);
      spr.fillRoundRect(bodyX + bodyW, bodyY + 22, 3, 10, 2, lineColor);
      spr.drawFastHLine(bodyX + 2, bodyY + 24, 6, lineColor);
      spr.drawFastHLine(bodyX + 2, bodyY + bodyH - 24, 6, lineColor);
      spr.drawFastHLine(bodyX + bodyW - 8, bodyY + 24, 6, lineColor);
      spr.drawFastHLine(bodyX + bodyW - 8, bodyY + bodyH - 24, 6, lineColor);

      // Light indicators
      if (liveData->params.brakeLights)
      {
        spr.fillRoundRect(bodyX + 5, bodyY + bodyH, 10, 6, 2, TFT_RED);
        spr.fillRoundRect(bodyX + bodyW - 15, bodyY + bodyH, 10, 6, 2, TFT_RED);
      }
      if (liveData->params.headLights || liveData->params.dayLights)
      {
        const uint16_t headColor = liveData->params.headLights ? TFT_YELLOW : TFT_GOLD;
        spr.fillRoundRect(bodyX + 5, bodyY - 6, 10, 6, 2, headColor);
        spr.fillRoundRect(bodyX + bodyW - 15, bodyY - 6, 10, 6, 2, headColor);
      }

      // Charging door
      if (liveData->params.chargerACconnected || liveData->params.chargerDCconnected)
      {
        uint16_t doorColor = TFT_GREEN;
        if (liveData->params.chargerACconnected && liveData->params.chargerDCconnected)
          doorColor = TFT_MAGENTA;
        else if (liveData->params.chargerDCconnected)
          doorColor = TFT_BLUE;
        const int16_t doorX = bodyX + bodyW + 2;
        const int16_t doorY = bodyY + bodyH / 2 - 6;
        spr.fillRoundRect(doorX, doorY, 10, 12, 2, doorColor);
        spr.drawRoundRect(doorX, doorY, 10, 12, 2, lineColor);
        if (liveData->params.chargingOn)
        {
          spr.drawFastVLine(doorX + 5, doorY + 2, 8, TFT_WHITE);
          spr.drawFastHLine(doorX + 3, doorY + 6, 4, TFT_WHITE);
        }
      }

      if (liveData->params.trunkDoorOpen)
        spr.fillRoundRect(bodyX + 6, bodyY - 10, bodyW - 12, 10, 3, TFT_GOLD);
      if (liveData->params.leftFrontDoorOpen)
        spr.fillRoundRect(bodyX - 14, bodyY + 20, 12, 18, 3, TFT_GOLD);
      if (liveData->params.rightFrontDoorOpen)
        spr.fillRoundRect(bodyX - 14, bodyY + bodyH - 38, 12, 18, 3, TFT_GOLD);
      if (liveData->params.leftRearDoorOpen)
        spr.fillRoundRect(bodyX + bodyW + 2, bodyY + 20, 12, 18, 3, TFT_GOLD);
      if (liveData->params.rightRearDoorOpen)
        spr.fillRoundRect(bodyX + bodyW + 2, bodyY + bodyH - 38, 12, 18, 3, TFT_GOLD);
      if (liveData->params.hoodDoorOpen)
        spr.fillRoundRect(bodyX + 6, bodyY + bodyH, bodyW - 12, 10, 3, TFT_GOLD);
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
  constexpr int16_t statusBoxRadius = 8;

  // OBD adapter not connected
  if ((liveData->settings.commType == COMM_TYPE_OBD2_BLE4 ||
       liveData->settings.commType == COMM_TYPE_OBD2_BT3 ||
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
    if (canStatusMessageVisible())
    {
      // Print message
      spr.fillRoundRect(0, 185, 320, 50, statusBoxRadius, TFT_BLACK);
      spr.drawRoundRect(0, 185, 320, 50, statusBoxRadius, TFT_WHITE);
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
      WiFi.status() == WL_CONNECTED && netStatusMessageVisible())
  {
    spr.fillRoundRect(0, 185, 320, 50, statusBoxRadius, TFT_BLACK);
    spr.drawRoundRect(0, 185, 320, 50, statusBoxRadius, TFT_WHITE);
    spr.setTextSize(1);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(TFT_WHITE);
    sprSetFont(fontFont2);
    sprDrawString("WiFi OK", 10, 190);
    sprDrawString("Net temporarily unavailable", 10, 210);
    statusBoxUsed = true;
  }

  return true;
}

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
  messageDialogVisible = false;

  if (drawActiveScreenToSprite())
  {
    spr.pushSprite(0, 0);
  }

  redrawScreenIsRunning = false;
}

void Board320_240::showScreenSwipePreview(int16_t deltaX)
{
  static int16_t lastPreviewDeltaX = 0;
  static uint32_t lastPreviewMs = 0;
  const uint8_t firstCarouselScreen = SCREEN_DASH;
  const uint8_t lastCarouselScreen = SCREEN_DEBUG;
  if (liveData->menuVisible || currentBrightness == 0 || !liveData->params.spriteInit)
  {
    return;
  }
  if (redrawScreenIsRunning)
  {
    return;
  }
  if (liveData->params.displayScreen == SCREEN_HUD)
  {
    return;
  }

  if (deltaX > 319)
    deltaX = 319;
  else if (deltaX < -319)
    deltaX = -319;

  if (deltaX == 0)
  {
    lastPreviewDeltaX = 0;
    return;
  }

  const uint32_t nowMs = millis();
  const bool deltaChangedEnough = (abs(deltaX - lastPreviewDeltaX) >= 2);
  const bool frameElapsed = (nowMs - lastPreviewMs) >= 16;
  if (!deltaChangedEnough && !frameElapsed)
  {
    return;
  }
  lastPreviewDeltaX = deltaX;
  lastPreviewMs = nowMs;

  const uint8_t originalScreen = liveData->params.displayScreen;
  if (originalScreen < firstCarouselScreen || originalScreen > lastCarouselScreen)
  {
    return;
  }
  if ((deltaX > 0 && originalScreen == firstCarouselScreen) ||
      (deltaX < 0 && originalScreen == lastCarouselScreen))
  {
    return;
  }

  const uint8_t originalAutoMode = liveData->params.displayScreenAutoMode;
  const uint8_t adjacentScreen = (deltaX > 0) ? (originalScreen - 1) : (originalScreen + 1);

  redrawScreenIsRunning = true;

  tft.setRotation(liveData->settings.displayRotation);

  liveData->params.displayScreen = originalScreen;
  liveData->params.displayScreenAutoMode = originalAutoMode;
  if (drawActiveScreenToSprite())
  {
    spr.pushSprite(deltaX, 0);
  }

  liveData->params.displayScreen = adjacentScreen;
  liveData->params.displayScreenAutoMode = originalAutoMode;
  if (drawActiveScreenToSprite())
  {
    const int16_t adjacentX = deltaX + ((deltaX > 0) ? -320 : 320);
    spr.pushSprite(adjacentX, 0);
  }

  liveData->params.displayScreen = originalScreen;
  liveData->params.displayScreenAutoMode = originalAutoMode;
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

void Board320_240::recordContributeSample()
{
  const time_t nowTime = liveData->params.currentTime;
  if (nowTime <= 0)
  {
    return;
  }
  if (lastContributeSampleTime != 0 && (nowTime - lastContributeSampleTime) < kContributeSampleIntervalSec)
  {
    return;
  }
  lastContributeSampleTime = nowTime;

  ContributeMotionSample motionSample{};
  motionSample.time = nowTime;
  if (isGpsFixUsable(liveData))
  {
    motionSample.hasGpsFix = true;
    motionSample.lat = roundToPrecision(liveData->params.gpsLat, 100000.0f);
    motionSample.lon = roundToPrecision(liveData->params.gpsLon, 100000.0f);
  }
  float currentSpeedKmh = liveData->params.speedKmh;
  if (currentSpeedKmh < 0 && liveData->params.speedKmhGPS >= 0)
  {
    currentSpeedKmh = liveData->params.speedKmhGPS;
  }
  motionSample.speedKmh = roundToPrecision(currentSpeedKmh, 10.0f);
  if (liveData->params.gpsHeadingDeg >= 0)
  {
    motionSample.headingDeg = roundToPrecision(liveData->params.gpsHeadingDeg, 10.0f);
  }
  motionSample.cellMinV = roundToPrecision(liveData->params.batCellMinV, 1000.0f);
  motionSample.cellMaxV = roundToPrecision(liveData->params.batCellMaxV, 1000.0f);
  motionSample.cellMinNo = liveData->params.batCellMinVNo;

  contributeMotionSamples[contributeMotionSampleNext] = motionSample;
  contributeMotionSampleNext = (contributeMotionSampleNext + 1) % kContributeSampleSlots;
  if (contributeMotionSampleCount < kContributeSampleSlots)
  {
    contributeMotionSampleCount++;
  }

  if (liveData->params.chargingOn)
  {
    ContributeChargingSample chargingSample{};
    chargingSample.time = nowTime;
    chargingSample.soc = roundToPrecision(liveData->params.socPerc, 10.0f);
    chargingSample.batV = roundToPrecision(liveData->params.batVoltage, 10.0f);
    chargingSample.batA = roundToPrecision(liveData->params.batPowerAmp, 10.0f);
    chargingSample.powKw = roundToPrecision(liveData->params.batPowerKw, 1000.0f);

    contributeChargingSamples[contributeChargingSampleNext] = chargingSample;
    contributeChargingSampleNext = (contributeChargingSampleNext + 1) % kContributeSampleSlots;
    if (contributeChargingSampleCount < kContributeSampleSlots)
    {
      contributeChargingSampleCount++;
    }
  }
}

Board320_240::ContributeChargingEvent Board320_240::captureContributeChargingEventSnapshot(time_t eventTime) const
{
  ContributeChargingEvent event{};
  event.valid = true;
  event.time = eventTime;
  event.soc = roundToPrecision(liveData->params.socPerc, 10.0f);
  event.batV = roundToPrecision(liveData->params.batVoltage, 10.0f);
  event.batA = roundToPrecision(liveData->params.batPowerAmp, 10.0f);
  event.cellMinV = roundToPrecision(liveData->params.batCellMinV, 1000.0f);
  event.cellMaxV = roundToPrecision(liveData->params.batCellMaxV, 1000.0f);
  event.cellMinNo = liveData->params.batCellMinVNo;
  event.batMinC = roundToPrecision(liveData->params.batMinC, 10.0f);
  event.batMaxC = roundToPrecision(liveData->params.batMaxC, 10.0f);
  event.cecKWh = roundToPrecision(liveData->params.cumulativeEnergyChargedKWh, 1000.0f);
  event.cedKWh = roundToPrecision(liveData->params.cumulativeEnergyDischargedKWh, 1000.0f);
  return event;
}

void Board320_240::handleContributeChargingTransitions()
{
  const time_t nowTime = liveData->params.currentTime;
  ContributeChargingEvent snapshot = captureContributeChargingEventSnapshot(nowTime);

  if (liveData->params.chargingOn)
  {
    contributeLastDuringCharge = snapshot;
  }
  else
  {
    contributeLastBeforeCharge = snapshot;
  }

  if (liveData->params.chargingOn && !lastChargingOn)
  {
    ContributeChargingEvent startEvent = contributeLastBeforeCharge.valid ? contributeLastBeforeCharge : snapshot;
    startEvent.valid = true;
    startEvent.time = nowTime;
    contributeChargingStartEvent = startEvent;
  }
  if (!liveData->params.chargingOn && lastChargingOn)
  {
    ContributeChargingEvent endEvent = contributeLastDuringCharge.valid ? contributeLastDuringCharge : snapshot;
    endEvent.valid = true;
    endEvent.time = nowTime;
    contributeChargingEndEvent = endEvent;
  }
}

bool Board320_240::buildContributePayloadV2(String &outJson, bool useReadableTsForSd)
{
  (void)useReadableTsForSd;
  DynamicJsonDocument jsonData(kContributeJsonDocCapacity);
  const time_t nowTime = liveData->params.currentTime;
  const String contributeKey = ensureContributeKey();
  const String hardwareDeviceId = normalizeDeviceIdForApi(getHardwareDeviceId());
  const String readableTs = formatTimestampYyMmDdHhIiSs(nowTime);

  jsonData["ver"] = 2;
  if (readableTs.length() == 12)
  {
    jsonData["ts"] = readableTs;
  }
  else
  {
    jsonData["ts"] = "000000000000";
  }
  jsonData["key"] = contributeKey;
  jsonData["deviceId"] = hardwareDeviceId;
  jsonData["apiKey"] = liveData->settings.remoteApiKey;
  jsonData["register"] = 1;
  jsonData["carType"] = getCarModelAbrpStr(liveData->settings.carType);
  jsonData["carVin"] = liveData->params.carVin;
  jsonData["sd"] = (liveData->params.sdcardInit && liveData->params.sdcardRecording) ? 1 : 0;

  const char *gpsMode = "none";
  switch (liveData->settings.gpsModuleType)
  {
  case GPS_MODULE_TYPE_NEO_M8N:
    gpsMode = "m8n";
    break;
  case GPS_MODULE_TYPE_M5_GNSS:
    gpsMode = "gnss";
    break;
  case GPS_MODULE_TYPE_GPS_V21_GNSS:
    gpsMode = "v21";
    break;
  default:
    gpsMode = "none";
    break;
  }
  jsonData["gps"] = gpsMode;

  const char *commMode = "unknown";
  switch (liveData->settings.commType)
  {
  case COMM_TYPE_CAN_COMMU:
    commMode = "can";
    break;
  case COMM_TYPE_OBD2_BLE4:
    commMode = "ble4";
    break;
  case COMM_TYPE_OBD2_BT3:
    commMode = "bt4";
    break;
  case COMM_TYPE_OBD2_WIFI:
    commMode = "wifi";
    break;
  default:
    commMode = "unknown";
    break;
  }
  jsonData["comm"] = commMode;

  jsonData["stoppedCan"] = liveData->params.stopCommandQueue ? 1 : 0;
  jsonData["ign"] = liveData->params.ignitionOn ? 1 : 0;
  jsonData["chg"] = liveData->params.chargingOn ? 1 : 0;
  jsonData["chgAc"] = liveData->params.chargerACconnected ? 1 : 0;
  jsonData["chgDc"] = liveData->params.chargerDCconnected ? 1 : 0;
  jsonData["soc"] = roundToPrecision(liveData->params.socPerc, 10.0f);
  if (liveData->params.socPercBms != -1)
  {
    jsonData["socBms"] = roundToPrecision(liveData->params.socPercBms, 10.0f);
  }
  jsonData["soh"] = roundToPrecision(liveData->params.sohPerc, 10.0f);
  jsonData["powKw"] = roundToPrecision(liveData->params.batPowerKw, 1000.0f);
  jsonData["powKwh100"] = roundToPrecision(liveData->params.batPowerKwh100, 1000.0f);
  jsonData["batV"] = roundToPrecision(liveData->params.batVoltage, 10.0f);
  jsonData["batA"] = roundToPrecision(liveData->params.batPowerAmp, 10.0f);
  jsonData["auxV"] = roundToPrecision(liveData->params.auxVoltage, 10.0f);
  jsonData["auxA"] = roundToPrecision(liveData->params.auxCurrentAmp, 10.0f);
  jsonData["batMinC"] = roundToPrecision(liveData->params.batMinC, 10.0f);
  jsonData["batMaxC"] = roundToPrecision(liveData->params.batMaxC, 10.0f);
  jsonData["inC"] = roundToPrecision(liveData->params.indoorTemperature, 10.0f);
  jsonData["outC"] = roundToPrecision(liveData->params.outdoorTemperature, 10.0f);
  jsonData["spd"] = roundToPrecision(liveData->params.speedKmh, 10.0f);
  if (liveData->params.speedKmhGPS >= 0)
  {
    jsonData["gpsSpd"] = roundToPrecision(liveData->params.speedKmhGPS, 10.0f);
  }
  if (liveData->params.gpsHeadingDeg >= 0)
  {
    jsonData["hdg"] = roundToPrecision(liveData->params.gpsHeadingDeg, 10.0f);
  }
  jsonData["odoKm"] = roundToPrecision(liveData->params.odoKm, 10.0f);
  jsonData["cMinV"] = roundToPrecision(liveData->params.batCellMinV, 1000.0f);
  jsonData["cMaxV"] = roundToPrecision(liveData->params.batCellMaxV, 1000.0f);
  jsonData["cMinNo"] = liveData->params.batCellMinVNo;
  jsonData["cecKWh"] = roundToPrecision(liveData->params.cumulativeEnergyChargedKWh, 1000.0f);
  jsonData["cedKWh"] = roundToPrecision(liveData->params.cumulativeEnergyDischargedKWh, 1000.0f);
  if (isGpsFixUsable(liveData))
  {
    jsonData["lat"] = roundToPrecision(liveData->params.gpsLat, 100000.0f);
    jsonData["lon"] = roundToPrecision(liveData->params.gpsLon, 100000.0f);
  }
  JsonArray motion;
  const uint8_t motionStartIndex = (contributeMotionSampleCount == kContributeSampleSlots) ? contributeMotionSampleNext : 0;
  for (uint8_t i = 0; i < contributeMotionSampleCount; i++)
  {
    uint8_t sampleIndex = (motionStartIndex + i) % kContributeSampleSlots;
    const ContributeMotionSample &sample = contributeMotionSamples[sampleIndex];
    if (sample.time == 0)
    {
      continue;
    }
    const time_t sampleAge = nowTime - sample.time;
    if (sampleAge < 0 || sampleAge > kContributeSampleWindowSec)
    {
      continue;
    }
    if (!sample.hasGpsFix)
    {
      continue;
    }
    if (motion.isNull())
    {
      motion = jsonData.createNestedArray("motion");
    }
    JsonObject row = motion.createNestedObject();
    row["t"] = static_cast<int32_t>(sample.time - nowTime);
    row["lat"] = sample.lat;
    row["lon"] = sample.lon;
    row["spd"] = sample.speedKmh;
    row["hdg"] = sample.headingDeg;
    row["cMinV"] = sample.cellMinV;
    row["cMaxV"] = sample.cellMaxV;
    row["cMinNo"] = sample.cellMinNo;
  }

  if (liveData->params.chargingOn)
  {
    JsonArray charging = jsonData.createNestedArray("charging");
    const uint8_t chargingStartIndex = (contributeChargingSampleCount == kContributeSampleSlots) ? contributeChargingSampleNext : 0;
    for (uint8_t i = 0; i < contributeChargingSampleCount; i++)
    {
      uint8_t sampleIndex = (chargingStartIndex + i) % kContributeSampleSlots;
      const ContributeChargingSample &sample = contributeChargingSamples[sampleIndex];
      if (sample.time == 0)
      {
        continue;
      }
      const time_t sampleAge = nowTime - sample.time;
      if (sampleAge < 0 || sampleAge > kContributeSampleWindowSec)
      {
        continue;
      }
      JsonObject row = charging.createNestedObject();
      row["t"] = static_cast<int32_t>(sample.time - nowTime);
      row["soc"] = sample.soc;
      row["batV"] = sample.batV;
      row["batA"] = sample.batA;
      row["powKw"] = sample.powKw;
    }
  }

  if (contributeChargingStartEvent.valid)
  {
    const time_t eventAge = nowTime - contributeChargingStartEvent.time;
    if (eventAge >= 0 && eventAge <= kContributeSampleWindowSec)
    {
      JsonObject chargingStart = jsonData.createNestedObject("chargingStart");
      chargingStart["time"] = contributeChargingStartEvent.time;
      chargingStart["soc"] = contributeChargingStartEvent.soc;
      chargingStart["batV"] = contributeChargingStartEvent.batV;
      chargingStart["batA"] = contributeChargingStartEvent.batA;
      chargingStart["cMinV"] = contributeChargingStartEvent.cellMinV;
      chargingStart["cMaxV"] = contributeChargingStartEvent.cellMaxV;
      chargingStart["cMinNo"] = contributeChargingStartEvent.cellMinNo;
      chargingStart["batMinC"] = contributeChargingStartEvent.batMinC;
      chargingStart["batMaxC"] = contributeChargingStartEvent.batMaxC;
      chargingStart["cecKWh"] = contributeChargingStartEvent.cecKWh;
      chargingStart["cedKWh"] = contributeChargingStartEvent.cedKWh;
    }
  }

  if (contributeChargingEndEvent.valid)
  {
    const time_t eventAge = nowTime - contributeChargingEndEvent.time;
    if (eventAge >= 0 && eventAge <= kContributeSampleWindowSec)
    {
      JsonObject chargingEnd = jsonData.createNestedObject("chargingEnd");
      chargingEnd["time"] = contributeChargingEndEvent.time;
      chargingEnd["soc"] = contributeChargingEndEvent.soc;
      chargingEnd["batV"] = contributeChargingEndEvent.batV;
      chargingEnd["batA"] = contributeChargingEndEvent.batA;
      chargingEnd["cMinV"] = contributeChargingEndEvent.cellMinV;
      chargingEnd["cMaxV"] = contributeChargingEndEvent.cellMaxV;
      chargingEnd["cMinNo"] = contributeChargingEndEvent.cellMinNo;
      chargingEnd["batMinC"] = contributeChargingEndEvent.batMinC;
      chargingEnd["batMaxC"] = contributeChargingEndEvent.batMaxC;
      chargingEnd["cecKWh"] = contributeChargingEndEvent.cecKWh;
      chargingEnd["cedKWh"] = contributeChargingEndEvent.cedKWh;
    }
  }

  uint8_t rawAdded = 0;
  uint8_t rawDropped = 0;
  for (uint8_t i = 0; i < liveData->contributeRawFrameCount; i++)
  {
    const LiveData::ContributeRawFrame &raw = liveData->contributeRawFrames[i];
    if (raw.key[0] == '\0' || raw.value[0] == '\0')
    {
      continue;
    }

    if (rawAdded >= kContributeRawFrameUploadMax)
    {
      rawDropped++;
      continue;
    }

    jsonData[String(raw.key)] = raw.value;
    if (kContributeIncludeRawLatency)
    {
      jsonData[String(raw.key) + "_ms"] = String(raw.latencyMs);
    }
    rawAdded++;
  }

  if (rawDropped > 0)
  {
    jsonData["rawDrop"] = rawDropped;
  }

  if (jsonData.overflowed())
  {
    syslog->println("Contribute JSON overflow (stability mode): payload fields trimmed");
  }

  outJson = "";
  size_t payloadLen = serializeJson(jsonData, outJson);
  return payloadLen > 0;
}

void Board320_240::syncContributeRelativeTimes(time_t offset)
{
  if (offset == 0)
  {
    return;
  }

  for (uint8_t i = 0; i < kContributeSampleSlots; i++)
  {
    if (contributeMotionSamples[i].time != 0)
    {
      contributeMotionSamples[i].time += offset;
    }
    if (contributeChargingSamples[i].time != 0)
    {
      contributeChargingSamples[i].time += offset;
    }
  }

  auto syncEventTime = [offset](ContributeChargingEvent &event) {
    if (event.valid && event.time != 0)
    {
      event.time += offset;
    }
  };
  syncEventTime(contributeLastBeforeCharge);
  syncEventTime(contributeLastDuringCharge);
  syncEventTime(contributeChargingStartEvent);
  syncEventTime(contributeChargingEndEvent);

  if (lastContributeSampleTime != 0)
  {
    lastContributeSampleTime += offset;
  }
  if (lastContributeSdRecordTime != 0)
  {
    lastContributeSdRecordTime += offset;
  }
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
  if (wifiConnected)
  {
    liveData->params.wifiLastConnectedTime = liveData->params.currentTime;
  }
  if (wifiConnected && !lastWifiConnected)
  {
    checkFirmwareVersionOnServer();
  }
  lastWifiConnected = wifiConnected;

  pollEvdashPairingStatus();

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
  const bool sdcardJsonV2 = (liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V2);
  const bool sdcardWriteTick = sdcardJsonV2 ? true : liveData->params.sdcardCanNotify;
  if (!liveData->params.stopCommandQueue && liveData->params.sdcardInit && liveData->params.sdcardRecording && sdcardWriteTick &&
      (liveData->params.odoKm != -1 && liveData->params.socPerc != -1))
  {
    const size_t sdcardFlushSize = 2048;
    const uint32_t sdcardIntervalMs = static_cast<uint32_t>(liveData->settings.sdcardLogIntervalSec) * 1000U;
    const char *sdcardOpFilenameFmt = sdcardJsonV2 ? "/%llu_v2.json" : "/%llu.json";
    const char *sdcardGpsFilenameFmt = sdcardJsonV2 ? "/%y%m%d%H%M_v2.json" : "/%y%m%d%H%M.json";
    const size_t sdcardGpsFilenameMinLength = sdcardJsonV2 ? 18 : 15;

    // create filename
    if (liveData->params.operationTimeSec > 0 && strlen(liveData->params.sdcardFilename) == 0)
    {
      sprintf(liveData->params.sdcardFilename, sdcardOpFilenameFmt, uint64_t(liveData->params.operationTimeSec / 60));
      syslog->print("Log filename by opTimeSec: ");
      syslog->println(liveData->params.sdcardFilename);
    }
    if (liveData->params.currTimeSyncWithGps && strlen(liveData->params.sdcardFilename) < sdcardGpsFilenameMinLength)
    {
      if (cachedNowEpoch == 0)
      {
        getLocalTime(&now, 0);
      }
      strftime(liveData->params.sdcardFilename, sizeof(liveData->params.sdcardFilename), sdcardGpsFilenameFmt, &now);
      syslog->print("Log filename by GPS: ");
      syslog->println(liveData->params.sdcardFilename);
    }

    // append buffer, clear buffer & notify state
    if (strlen(liveData->params.sdcardFilename) != 0)
    {
      liveData->params.sdcardCanNotify = false;
      if (sdcardJsonV2)
      {
        const bool minuteTick = (lastContributeSdRecordTime == 0) ||
                                ((liveData->params.currentTime - lastContributeSdRecordTime) >= kContributeSampleWindowSec);
        if (minuteTick)
        {
          String jsonLine;
          if (buildContributePayloadV2(jsonLine, true))
          {
            jsonLine += ",\n";
            sdcardRecordBuffer += jsonLine;
            lastContributeSdRecordTime = liveData->params.currentTime;
          }
        }
      }
      else
      {
        String jsonLine;
        serializeParamsToJson(jsonLine);
        jsonLine += ",\n";
        sdcardRecordBuffer += jsonLine;
      }

      const bool timeToFlush = (sdcardIntervalMs > 0U) && ((nowMs - liveData->params.sdcardLastFlushMs) >= sdcardIntervalMs);
      const bool sizeToFlush = sdcardRecordBuffer.length() >= sdcardFlushSize;
      if ((timeToFlush || sizeToFlush) && sdcardRecordBuffer.length() > 0)
      {
        File file = SD.open(liveData->params.sdcardFilename, FILE_APPEND);
        if (!file)
        {
          syslog->println("Failed to open file for appending");
          file = SD.open(liveData->params.sdcardFilename, FILE_WRITE);
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
      commInterface->resumeDevice();
    }

    // Some APs drop ESP32 station during prolonged Sentry inactivity.
    // Force a reconnect immediately after wake so recovery does not require reboot.
    const bool shouldReconnectWifiAfterWake =
        (!liveData->params.wifiApMode &&
         liveData->settings.wifiEnabled == 1 &&
         liveData->settings.commType != COMM_TYPE_OBD2_WIFI &&
         WiFi.status() != WL_CONNECTED);
    if (shouldReconnectWifiAfterWake)
    {
      syslog->println("Sentry wake: WiFi disconnected, forcing reconnect.");
      WiFi.enableSTA(true);
      WiFi.mode(WIFI_STA);

      if (liveData->settings.backupWifiEnabled == 1 && liveData->params.isWifiBackupLive)
      {
        wifiSwitchToBackup();
      }
      else
      {
        wifiSwitchToMain();
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
  const bool parkingLikely =
      (!liveData->params.ignitionOn &&
       !liveData->params.chargingOn &&
       doorsOk);
  const bool lowAuxVoltage = (liveData->params.auxVoltage > 3 && liveData->params.auxVoltage < 11.5);
  const bool autoStopByCanSignals = (parkingLikely || lowAuxVoltage);

  // Fallback when CAN never yields a valid response (e.g. adapter/config issue):
  // still allow Sentry autostop after a grace period so AUX battery is protected.
  const time_t autoStopWithoutCanGraceSec = 180;
  const bool canDataMissingTooLong =
      (!liveData->params.getValidResponse &&
       (liveData->params.currentTime - liveData->params.wakeUpTime > autoStopWithoutCanGraceSec));
  const bool dcDcLikelyRunning = (liveData->params.auxVoltage >= 13.8);
  const bool autoStopFallbackSafe = (parkingLikely && !dcDcLikelyRunning);

  if (liveData->settings.commandQueueAutoStop == 1 &&
      ((liveData->params.getValidResponse && autoStopByCanSignals) ||
       (canDataMissingTooLong && autoStopFallbackSafe)))
  {
    if (canDataMissingTooLong && !liveData->params.getValidResponse && liveData->params.stopCommandQueueTime == 0)
    {
      syslog->println("Sentry fallback: no valid CAN data, preparing autostop.");
    }
    liveData->prepareForStopCommandQueue();
  }
  if (!liveData->params.stopCommandQueue &&
      ((liveData->params.stopCommandQueueTime != 0 && liveData->params.currentTime - liveData->params.stopCommandQueueTime > 60) ||
       (liveData->params.auxVoltage > 3 && liveData->params.auxVoltage < 11.0)))
  {
    liveData->params.stopCommandQueue = true;
    commInterface->suspendDevice();
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
  commLoop();

  // force redraw (min 1 sec update; slower while in Sentry)
  const time_t redrawIntervalSec = liveData->params.stopCommandQueue ? 2 : 1;
  if (!screenSwipePreviewActive &&
      (liveData->params.currentTime - lastRedrawTime >= redrawIntervalSec || liveData->redrawScreenRequested))
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
  handleContributeChargingTransitions();
  lastChargingOn = liveData->params.chargingOn;
  recordContributeSample();

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
  const time_t offset = newTime - liveData->params.currentTime;
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

  syncContributeRelativeTimes(offset);

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
    lastContributeSdRecordTime = 0;
  }
  else
  {
    if (sdcardRecordBuffer.length() > 0 && strlen(liveData->params.sdcardFilename) != 0)
    {
      File file = SD.open(liveData->params.sdcardFilename, FILE_APPEND);
      if (!file)
      {
        syslog->println("Failed to open file for appending");
        file = SD.open(liveData->params.sdcardFilename, FILE_WRITE);
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

void Board320_240::sdcardEraseLogs()
{
  if (!liveData->settings.sdcardEnabled || !sdcardMount())
  {
    displayMessage("SDCARD", "Not mounted");
    delay(2000);
    return;
  }

  if (liveData->params.sdcardRecording)
  {
    sdcardToggleRecording();
  }

  File dir = SD.open("/");
  if (!dir || !dir.isDirectory())
  {
    displayMessage("SDCARD", "Open root failed");
    delay(2000);
    return;
  }

  uint16_t removed = 0;
  uint16_t failed = 0;
  while (true)
  {
    File entry = dir.openNextFile(FILE_READ);
    if (!entry)
    {
      break;
    }

    const bool isDir = entry.isDirectory();
    String fileName = String(entry.name());
    entry.close();

    if (isDir)
    {
      continue;
    }

    if (!fileName.endsWith(".json"))
    {
      continue;
    }

    if (SD.remove(fileName.c_str()))
    {
      removed++;
    }
    else
    {
      failed++;
    }
  }
  dir.close();

  sdcardRecordBuffer = "";
  String tmpStr = "";
  tmpStr.toCharArray(liveData->params.sdcardFilename, tmpStr.length() + 1);
  tmpStr.toCharArray(liveData->params.sdcardAbrpFilename, tmpStr.length() + 1);

  String msg1 = "Logs erased: " + String(removed);
  String msg2 = (failed == 0) ? "Done" : "Failed: " + String(failed);
  displayMessage(msg1.c_str(), msg2.c_str());
  delay(2000);
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
    liveData->params.gpsHeadingDeg = normalizeHeadingDeg(gps.course.deg());
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

bool Board320_240::wifiScanToMenu()
{
  displayMessage("Scanning WiFi...", "");
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(200);

  int found = WiFi.scanNetworks();
  liveData->wifiScanCount = 0;

  for (uint16_t i = 0; i < liveData->menuItemsCount; ++i)
  {
    if (liveData->menuItems[i].id >= LIST_OF_WIFI_1 && liveData->menuItems[i].id <= LIST_OF_WIFI_10)
    {
      strlcpy(liveData->menuItems[i].title, "-", sizeof(liveData->menuItems[i].title));
    }
  }

  if (found <= 0)
  {
    displayMessage("WiFi scan", "No networks found");
    delay(1500);
    showMenu();
    return false;
  }

  for (int i = 0; i < found && liveData->wifiScanCount < 10; i++)
  {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0)
      continue;

    int8_t rssi = WiFi.RSSI(i);
    uint8_t enc = WiFi.encryptionType(i);
    uint8_t slot = liveData->wifiScanCount;

    ssid.toCharArray(liveData->wifiScanSsid[slot], sizeof(liveData->wifiScanSsid[slot]));
    liveData->wifiScanRssi[slot] = rssi;
    liveData->wifiScanEnc[slot] = enc;

    String label = ssid;
    label += " ";
    label += String(rssi);
    label += "dBm";
    if (enc != WIFI_AUTH_OPEN)
      label += " *";

    for (uint16_t m = 0; m < liveData->menuItemsCount; ++m)
    {
      if (liveData->menuItems[m].id == (LIST_OF_WIFI_1 + slot))
      {
        label.toCharArray(liveData->menuItems[m].title, sizeof(liveData->menuItems[m].title));
        break;
      }
    }

    liveData->wifiScanCount++;
  }

  liveData->menuVisible = true;
  liveData->menuCurrent = LIST_OF_WIFI_DEV;
  liveData->menuItemSelected = 0;
  liveData->menuItemOffset = 0;
  showMenu();
  return true;
}

bool Board320_240::promptKeyboard(const char *title, String &value, bool mask, uint8_t maxLen)
{
  keyboardInputActive = true;
  const int16_t screenW = tft.width();
  const int16_t screenH = tft.height();
  const int16_t topBtnY = 4;
  const int16_t topBtnH = 24;
  const int16_t topExitW = 50;
  const int16_t topExitX = 6;
  const int16_t titleY = 6;
  const int16_t titleX = topExitX + topExitW + 8;
  const int16_t inputY = 32;
  const int16_t inputH = 34;
  const int16_t keyW = 30;
  const int16_t gap = 2;
  const int16_t rowsY = inputY + inputH + 8;
  const int16_t keyboardRows = 5;
  const int16_t modeW = 48;
  const int16_t shiftW = 56;
  const int16_t spaceW = 98;
  const int16_t bkspW = 52;
  const int16_t okW = 58;
  int16_t keyH = (screenH - 6 - rowsY - (keyboardRows - 1) * gap) / keyboardRows;
  if (keyH < 26)
    keyH = 26;

  enum KeyAction : uint8_t
  {
    KEY_NONE = 0,
    KEY_CHAR,
    KEY_MODE,
    KEY_SHIFT,
    KEY_SPACE,
    KEY_DEL,
    KEY_OK,
    KEY_EXIT
  };
  struct KeyHit
  {
    KeyAction action;
    char ch;
  };

  struct KeyRows
  {
    const char *row[4];
    uint8_t len[4];
  };

  bool shift = false;
  bool numericMode = false;
  bool touchActive = false;
  KeyAction activeAction = KEY_NONE;
  char activeChar = 0;
  char previewChar = 0;
  uint32_t previewUntil = 0;
  bool needsRedraw = true;

  auto loadRows = [&](KeyRows &rows) {
    if (!numericMode)
    {
      rows.row[0] = shift ? "QWERTYUIOP" : "qwertyuiop";
      rows.len[0] = 10;
      rows.row[1] = shift ? "ASDFGHJKL" : "asdfghjkl";
      rows.len[1] = 9;
      rows.row[2] = shift ? "ZXCVBNM" : "zxcvbnm";
      rows.len[2] = 7;
      rows.row[3] = "@._-/:;!?+";
      rows.len[3] = 10;
      return;
    }

    rows.row[0] = "1234567890";
    rows.len[0] = 10;
    if (!shift)
    {
      rows.row[1] = "!@#$%^&*()";
      rows.len[1] = 10;
      rows.row[2] = "-_=+[]{}";
      rows.len[2] = 8;
      rows.row[3] = ".,:;/?\\|";
      rows.len[3] = 8;
    }
    else
    {
      rows.row[1] = "~`<>\"'()";
      rows.len[1] = 8;
      rows.row[2] = "{}[]+=-_";
      rows.len[2] = 8;
      rows.row[3] = "#$%&*@!?";
      rows.len[3] = 8;
    }
  };

  auto hitTest = [&](int16_t tx, int16_t ty) -> KeyHit {
    if (ty >= topBtnY && ty <= topBtnY + topBtnH)
    {
      if (tx >= topExitX && tx <= topExitX + topExitW)
      {
        KeyHit hit = {KEY_EXIT, 0};
        return hit;
      }
    }

    KeyRows rows = {};
    loadRows(rows);

    for (int r = 0; r < 4; r++)
    {
      int len = rows.len[r];
      int rowWidth = len * keyW + (len - 1) * gap;
      int x0 = (screenW - rowWidth) / 2;
      int y0 = rowsY + r * (keyH + gap);
      if (ty < y0 || ty > y0 + keyH || tx < x0 || tx > x0 + rowWidth)
        continue;
      int col = (tx - x0) / (keyW + gap);
      if (col < 0 || col >= len)
        continue;
      KeyHit hit = {KEY_CHAR, rows.row[r][col]};
      return hit;
    }

    int ctrlY = rowsY + 4 * (keyH + gap);
    int totalW = modeW + shiftW + spaceW + bkspW + okW + gap * 4;
    int x = (screenW - totalW) / 2;
    int modeX = x;
    int shiftX = modeX + modeW + gap;
    int spaceX = shiftX + shiftW + gap;
    int bkspX = spaceX + spaceW + gap;
    int okX = bkspX + bkspW + gap;

    if (ty >= ctrlY && ty <= ctrlY + keyH)
    {
      if (tx >= modeX && tx <= modeX + modeW)
      {
        KeyHit hit = {KEY_MODE, 0};
        return hit;
      }
      if (tx >= shiftX && tx <= shiftX + shiftW)
      {
        KeyHit hit = {KEY_SHIFT, 0};
        return hit;
      }
      if (tx >= spaceX && tx <= spaceX + spaceW)
      {
        KeyHit hit = {KEY_SPACE, 0};
        return hit;
      }
      if (tx >= bkspX && tx <= bkspX + bkspW)
      {
        KeyHit hit = {KEY_DEL, 0};
        return hit;
      }
      if (tx >= okX && tx <= okX + okW)
      {
        KeyHit hit = {KEY_OK, 0};
        return hit;
      }
    }

    KeyHit hit = {KEY_NONE, 0};
    return hit;
  };

  auto drawKeyboard = [&](bool showPreview) {
    if (liveData->params.spriteInit)
    {
      spr.fillSprite(TFT_BLACK);
    }
    else
    {
      tft.fillScreen(TFT_BLACK);
    }

    auto drawRect = [&](int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bg, uint16_t fg) {
      if (liveData->params.spriteInit)
      {
        spr.fillRoundRect(x, y, w, h, 4, bg);
        spr.drawRoundRect(x, y, w, h, 4, fg);
      }
      else
      {
        tft.fillRoundRect(x, y, w, h, 4, bg);
        tft.drawRoundRect(x, y, w, h, 4, fg);
      }
    };

    auto drawText = [&](const char *text, int16_t x, int16_t y, bool bigFont) {
      if (liveData->params.spriteInit)
      {
        sprSetFont(bigFont ? fontRobotoThin24 : fontFont2);
        spr.setTextColor(TFT_WHITE);
        spr.setTextDatum(MC_DATUM);
        sprDrawString(text, x, y);
      }
      else
      {
        tft.setFont(bigFont ? fontRobotoThin24 : fontFont2);
        tft.setTextColor(TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(text, x, y);
      }
    };

    auto drawLinePrim = [&](int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t col) {
      if (liveData->params.spriteInit)
      {
        spr.drawLine(x0, y0, x1, y1, col);
      }
      else
      {
        tft.drawLine(x0, y0, x1, y1, col);
      }
    };

    auto drawEnterIcon = [&](int16_t x, int16_t y, int16_t w, int16_t h, uint16_t col) {
      // Simple bent "enter" arrow.
      int16_t cx = x + w / 2 + 7;
      int16_t top = y + 6;
      int16_t mid = y + h / 2 + 1;
      drawLinePrim(cx, top, cx, mid, col);
      drawLinePrim(cx, mid, x + 10, mid, col);
      drawLinePrim(x + 10, mid, x + 14, mid - 4, col);
      drawLinePrim(x + 10, mid, x + 14, mid + 4, col);
    };

    auto isActive = [&](KeyAction action, char ch) -> bool {
      return touchActive && activeAction == action && activeChar == ch;
    };

    drawRect(topExitX, topBtnY, topExitW, topBtnH, isActive(KEY_EXIT, 0) ? TFT_RED : 0x1082, isActive(KEY_EXIT, 0) ? TFT_CYAN : 0x39E7);
    drawText("EXIT", topExitX + topExitW / 2, topBtnY + topBtnH / 2 - 1, false);

    if (liveData->params.spriteInit)
    {
      // Keep title style consistent with keyboard text style.
      sprSetFont(fontFont2);
      spr.setTextColor(TFT_WHITE);
      spr.setTextDatum(TL_DATUM);
      sprDrawString(title, titleX, titleY);
    }
    else
    {
      tft.setFont(fontFont2);
      tft.setTextColor(TFT_WHITE);
      tft.setTextDatum(TL_DATUM);
      tft.drawString(title, titleX, titleY);
    }

    String display = value;
    if (mask)
    {
      display = "";
      for (size_t i = 0; i < value.length(); i++)
        display += "*";
    }

    drawRect(6, inputY, screenW - 12, inputH, 0x0841, TFT_CYAN);
    if (liveData->params.spriteInit)
    {
      sprSetFont(fontRobotoThin24);
      spr.setTextColor(TFT_WHITE);
      spr.setTextDatum(TL_DATUM);
      sprDrawString(display.c_str(), 10, inputY + 7);
    }
    else
    {
      tft.setFont(fontRobotoThin24);
      tft.setTextColor(TFT_WHITE);
      tft.setTextDatum(TL_DATUM);
      tft.drawString(display.c_str(), 10, inputY + 6);
    }

    KeyRows rows = {};
    loadRows(rows);

    for (int r = 0; r < 4; r++)
    {
      int len = rows.len[r];
      int rowWidth = len * keyW + (len - 1) * gap;
      int x0 = (screenW - rowWidth) / 2;
      int y0 = rowsY + r * (keyH + gap);
      for (int c = 0; c < len; c++)
      {
        char key[2] = {rows.row[r][c], 0};
        int x = x0 + c * (keyW + gap);
        bool active = isActive(KEY_CHAR, rows.row[r][c]);
        drawRect(x, y0, keyW, keyH, active ? 0x001B : 0x1082, active ? TFT_CYAN : 0x39E7);
        drawText(key, x + keyW / 2, y0 + keyH / 2 - 2, true);
      }
    }

    int ctrlY = rowsY + 4 * (keyH + gap);
    int totalW = modeW + shiftW + spaceW + bkspW + okW + gap * 4;
    int x = (screenW - totalW) / 2;
    drawRect(x, ctrlY, modeW, keyH, isActive(KEY_MODE, 0) ? 0x001B : 0x1082, isActive(KEY_MODE, 0) ? TFT_CYAN : 0x39E7);
    drawText(numericMode ? "abc" : "123", x + modeW / 2, ctrlY + keyH / 2 - 2, false);
    x += modeW + gap;
    drawRect(x, ctrlY, shiftW, keyH, isActive(KEY_SHIFT, 0) ? 0x001B : (shift ? TFT_BLUE : 0x1082),
             isActive(KEY_SHIFT, 0) ? TFT_CYAN : 0x39E7);
    drawText("SHIFT", x + shiftW / 2, ctrlY + keyH / 2 - 2, false);
    x += shiftW + gap;
    drawRect(x, ctrlY, spaceW, keyH, isActive(KEY_SPACE, 0) ? 0x001B : 0x1082, isActive(KEY_SPACE, 0) ? TFT_CYAN : 0x39E7);
    drawText("SPACE", x + spaceW / 2, ctrlY + keyH / 2 - 2, false);
    x += spaceW + gap;
    drawRect(x, ctrlY, bkspW, keyH, isActive(KEY_DEL, 0) ? 0x001B : 0x1082, isActive(KEY_DEL, 0) ? TFT_CYAN : 0x39E7);
    drawText("DEL", x + bkspW / 2, ctrlY + keyH / 2 - 2, false);
    x += bkspW + gap;
    drawRect(x, ctrlY, okW, keyH, isActive(KEY_OK, 0) ? TFT_GREEN : TFT_DARKGREEN2, isActive(KEY_OK, 0) ? TFT_CYAN : 0x39E7);
    drawEnterIcon(x, ctrlY, okW, keyH, TFT_WHITE);

    if (showPreview && previewChar != 0)
    {
      const int16_t bubbleW = 144;
      const int16_t bubbleH = 68;
      const int16_t bubbleX = (screenW - bubbleW) / 2;
      const int16_t bubbleY = 4;
      char previewText[2] = {previewChar, 0};
      drawRect(bubbleX, bubbleY, bubbleW, bubbleH, 0x52AA, 0x7BEF);
      if (liveData->params.spriteInit)
      {
        sprSetFont(fontOrbitronLight32);
        spr.setTextColor(TFT_WHITE);
        spr.setTextDatum(MC_DATUM);
        sprDrawString(previewText, bubbleX + bubbleW / 2, bubbleY + bubbleH / 2 + 1);
      }
      else
      {
        tft.setFont(fontOrbitronLight32);
        tft.setTextColor(TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(previewText, bubbleX + bubbleW / 2, bubbleY + bubbleH / 2 + 1);
      }
    }

    if (liveData->params.spriteInit)
      spr.pushSprite(0, 0);
  };

  drawKeyboard(false);
  bool previousPressed = false;

  auto readTouchRaw = [&](int16_t &x, int16_t &y) -> bool {
#ifdef BOARD_M5STACK_CORE2
    if (M5.Touch.ispressed() && M5.Touch.points > 0 && M5.Touch.point[0].valid())
    {
      x = M5.Touch.point[0].x;
      y = M5.Touch.point[0].y;
      if (x < 0 || y < 0 || x >= screenW || y >= screenH)
        return false;
      return true;
    }
#endif
#ifdef BOARD_M5STACK_CORES3
    auto t = CoreS3.Touch.getDetail();
    if (t.isPressed())
    {
      x = t.x;
      y = t.y;
      if (x < 0 || y < 0 || x >= screenW || y >= screenH)
        return false;
      return true;
    }
#endif
    return false;
  };

  while (true)
  {
    boardLoop();
    uint32_t nowMs = millis();
    int16_t tx = 0, ty = 0;
    bool touched = readTouchRaw(tx, ty);

    if (touched)
    {
      KeyHit hit = hitTest(tx, ty);
      if (!touchActive || hit.action != activeAction || hit.ch != activeChar)
      {
        activeAction = hit.action;
        activeChar = hit.ch;
        needsRedraw = true;
      }
      touchActive = true;
      if (hit.action == KEY_CHAR && hit.ch != 0)
      {
        previewChar = hit.ch;
        previewUntil = nowMs + 500;
        needsRedraw = true;
      }
      else if (previewChar != 0)
      {
        previewChar = 0;
        needsRedraw = true;
      }
    }
    else if (touchActive && previousPressed)
    {
      if (activeAction == KEY_CHAR && activeChar != 0)
      {
        if (value.length() < maxLen)
        {
          value += activeChar;
          if (shift && activeChar >= 'A' && activeChar <= 'Z')
            shift = false;
        }
        previewChar = activeChar;
        previewUntil = nowMs + 500;
      }
      else if (activeAction == KEY_MODE)
      {
        numericMode = !numericMode;
        shift = false;
      }
      else if (activeAction == KEY_SHIFT)
      {
        shift = !shift;
      }
      else if (activeAction == KEY_SPACE)
      {
        if (value.length() < maxLen)
          value += " ";
      }
      else if (activeAction == KEY_DEL)
      {
        if (value.length() > 0)
          value.remove(value.length() - 1);
      }
      else if (activeAction == KEY_OK)
      {
        keyboardInputActive = false;
        suppressTouchInputFor();
        return true;
      }
      else if (activeAction == KEY_EXIT)
      {
        keyboardInputActive = false;
        suppressTouchInputFor();
        return false;
      }

      touchActive = false;
      activeAction = KEY_NONE;
      activeChar = 0;
      needsRedraw = true;
    }
    else if (previewChar != 0 && nowMs >= previewUntil)
    {
      previewChar = 0;
      needsRedraw = true;
    }

    if (needsRedraw)
    {
      drawKeyboard(previewChar != 0);
      needsRedraw = false;
    }

    previousPressed = touched;
    if (!touched)
      delay(5);
  }
}

void Board320_240::suppressTouchInputFor(uint16_t durationMs)
{
  suppressTouchInputUntilMs = millis() + durationMs;
}

bool Board320_240::isTouchInputSuppressed() const
{
  if (suppressTouchInputUntilMs == 0)
    return false;
  return static_cast<int32_t>(suppressTouchInputUntilMs - millis()) > 0;
}

bool Board320_240::isKeyboardInputActive() const
{
  return keyboardInputActive;
}

bool Board320_240::isMessageDialogVisible() const
{
  return messageDialogVisible;
}

bool Board320_240::dismissMessageDialog()
{
  if (!messageDialogVisible)
    return false;

  messageDialogVisible = false;
  menuTouchHoverIndex = -1;
  suppressTouchInputFor();

  if (liveData->menuVisible)
  {
    menuDragScrollActive = true;
    showMenu();
    menuDragScrollActive = false;
  }
  else
  {
    redrawScreen();
  }

  return true;
}

bool Board320_240::promptWifiPassword(const char *ssid, String &outPassword, bool isOpenNetwork)
{
  if (isOpenNetwork)
  {
    outPassword = "";
    return true;
  }
  String value = outPassword;
  String title = String("Passwd for ") + ssid;
  bool ok = promptKeyboard(title.c_str(), value, false, sizeof(liveData->settings.wifiPassword) - 1);
  if (ok)
    outPassword = value;
  return ok;
}

bool Board320_240::canStatusMessageVisible()
{
  if (liveData->settings.commType != COMM_TYPE_CAN_COMMU)
    return false;

  String status = commInterface->getConnectStatus();
  if (status == "")
    return false;

  if (dismissedCanStatusText != "" && status == dismissedCanStatusText)
    return false;

  // New status text arrived, clear previous dismissal.
  if (dismissedCanStatusText != "" && status != dismissedCanStatusText)
    dismissedCanStatusText = "";

  return true;
}

bool Board320_240::canStatusMessageHitTest(int16_t x, int16_t y)
{
  if (!canStatusMessageVisible() && !netStatusMessageVisible())
    return false;
  return (x >= 0 && x < 320 && y >= 185 && y < 235);
}

void Board320_240::dismissCanStatusMessage()
{
  if (liveData->settings.commType == COMM_TYPE_CAN_COMMU)
  {
    String status = commInterface->getConnectStatus();
    if (status != "")
      dismissedCanStatusText = status;
  }

  if (netStatusMessageVisible())
  {
    dismissedNetFailureTime = liveData->params.netLastFailureTime;
  }
}

bool Board320_240::netStatusMessageVisible() const
{
  if (liveData->params.netAvailable)
    return false;
  if (liveData->params.netLastFailureTime == 0)
    return false;
  if (liveData->params.currentTime < liveData->params.netLastFailureTime)
    return false;
  if ((liveData->params.currentTime - liveData->params.netLastFailureTime) > kNetFailureStaleResetSec)
    return false;
  if (dismissedNetFailureTime != 0 && dismissedNetFailureTime == liveData->params.netLastFailureTime)
    return false;
  return true;
}

bool Board320_240::isContributeKeyValid(const char *key) const
{
  if (key == nullptr)
    return false;

  String normalized = String(key);
  normalized.trim();
  if (normalized.length() < 12)
    return false;
  if (normalized == "empty" || normalized == "not_set")
    return false;

  for (uint16_t i = 0; i < normalized.length(); i++)
  {
    const char ch = normalized.charAt(i);
    if (ch <= ' ' || ch > '~')
      return false;
  }

  return true;
}

String Board320_240::ensureContributeKey()
{
  String key = String(liveData->settings.contributeToken);
  key.trim();
  if (isContributeKeyValid(key.c_str()))
  {
    return key;
  }

  const uint64_t efuse = ESP.getEfuseMac();
  const uint32_t rnd = esp_random();
  const uint32_t nowMs = millis();
  char generated[sizeof(liveData->settings.contributeToken)] = {0};
  snprintf(generated, sizeof(generated), "k%08lX%08lX%08lX", (uint32_t)(efuse & 0xFFFFFFFFULL), (uint32_t)rnd, nowMs);
  key = String(generated);
  key.toCharArray(liveData->settings.contributeToken, sizeof(liveData->settings.contributeToken));
  saveSettings();
  syslog->print("Generated contribute key: ");
  syslog->println(key);
  return key;
}

String Board320_240::getHardwareDeviceId() const
{
  const uint64_t efuse = (ESP.getEfuseMac() & 0xFFFFFFFFFFFFULL);
  const uint32_t uuidPart1 = static_cast<uint32_t>((efuse >> 16) & 0xFFFFFFFFULL);
  const uint16_t uuidPart2 = static_cast<uint16_t>(efuse & 0xFFFFU);
  const uint16_t uuidPart3 = static_cast<uint16_t>(((efuse >> 32) & 0x0FFFU) | 0x4000U); // UUID version 4 layout
  const uint16_t uuidPart4 = static_cast<uint16_t>(((efuse >> 20) & 0x3FFFU) | 0x8000U); // UUID variant 1 layout
#ifdef BOARD_M5STACK_CORES3
  const uint8_t boardTag = 0x03U;
#else
  const uint8_t boardTag = 0x02U;
#endif
  const uint64_t uuidPart5 = ((efuse ^ (static_cast<uint64_t>(boardTag) << 40)) & 0xFFFFFFFFFFFFULL);
  char deviceId[40] = {0};
  snprintf(deviceId, sizeof(deviceId), "%08lX-%04X-%04X-%04X-%012llX",
           uuidPart1, uuidPart2, uuidPart3, uuidPart4, uuidPart5);
  return String(deviceId);
}

String Board320_240::getPairDeviceId() const
{
  const String hardwareDeviceId = normalizeDeviceIdForApi(getHardwareDeviceId());
  if (hardwareDeviceId.length() == 0)
  {
    return "";
  }
#ifdef BOARD_M5STACK_CORES3
  return String("c3.") + hardwareDeviceId;
#else
  return String("c2.") + hardwareDeviceId;
#endif
}

bool Board320_240::requestPairingStart(String &outCode, uint32_t &outExpiresInSec)
{
  outCode = "";
  outExpiresInSec = 0;

  if (WiFi.status() != WL_CONNECTED)
  {
    return false;
  }

  const String pairDeviceId = getPairDeviceId();
  if (pairDeviceId.length() == 0)
  {
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(kPairHttpTimeoutMs);

  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(kPairHttpTimeoutMs);
  http.setTimeout(kPairHttpTimeoutMs);

  String url = String(PAIR_START_URL);
  url += (url.indexOf('?') == -1) ? "?" : "&";
  url += "id=" + pairDeviceId;

  if (!http.begin(client, url))
  {
    syslog->println("Pair start: begin failed");
    return false;
  }

  http.addHeader("User-Agent", String("evDash/") + String(APP_VERSION));
  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    syslog->print("Pair start HTTP code: ");
    syslog->println(httpCode);
    http.end();
    return false;
  }

  const String response = http.getString();
  http.end();

  StaticJsonDocument<512> jsonDoc;
  const DeserializationError jsonErr = deserializeJson(jsonDoc, response);
  if (jsonErr)
  {
    syslog->print("Pair start JSON parse error: ");
    syslog->println(jsonErr.c_str());
    return false;
  }

  const char *statusRaw = jsonDoc["status"];
  if (statusRaw == nullptr || strcmp(statusRaw, "ok") != 0)
  {
    syslog->println("Pair start: status not ok");
    return false;
  }

  const char *pairCodeRaw = jsonDoc["pairCode"];
  if (pairCodeRaw == nullptr)
  {
    syslog->println("Pair start: missing pairCode");
    return false;
  }

  String code = String(pairCodeRaw);
  code.trim();
  if (code.length() != 6)
  {
    syslog->println("Pair start: invalid pairCode");
    return false;
  }
  for (uint8_t i = 0; i < 6; i++)
  {
    const char ch = code.charAt(i);
    if (ch < '0' || ch > '9')
    {
      syslog->println("Pair start: non-numeric pairCode");
      return false;
    }
  }

  uint32_t expiresInSec = jsonDoc["expiresInSec"] | 0;
  if (expiresInSec < 30)
  {
    expiresInSec = 300;
  }

  outCode = code;
  outExpiresInSec = expiresInSec;
  return true;
}

uint8_t Board320_240::requestPairingStatus(const String &pairCode, String &outCarName)
{
  outCarName = "";

  if (WiFi.status() != WL_CONNECTED)
  {
    return 255;
  }
  if (pairCode.length() != 6)
  {
    return 255;
  }

  const String pairDeviceId = getPairDeviceId();
  if (pairDeviceId.length() == 0)
  {
    return 255;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(kPairHttpTimeoutMs);

  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(kPairHttpTimeoutMs);
  http.setTimeout(kPairHttpTimeoutMs);

  String url = String(PAIR_STATUS_URL);
  url += (url.indexOf('?') == -1) ? "?" : "&";
  url += "id=" + pairDeviceId;
  url += "&code=" + pairCode;

  if (!http.begin(client, url))
  {
    syslog->println("Pair status: begin failed");
    return 255;
  }

  http.addHeader("User-Agent", String("evDash/") + String(APP_VERSION));
  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    syslog->print("Pair status HTTP code: ");
    syslog->println(httpCode);
    http.end();
    return 255;
  }

  const String response = http.getString();
  http.end();

  StaticJsonDocument<512> jsonDoc;
  const DeserializationError jsonErr = deserializeJson(jsonDoc, response);
  if (jsonErr)
  {
    syslog->print("Pair status JSON parse error: ");
    syslog->println(jsonErr.c_str());
    return 255;
  }

  const char *statusRaw = jsonDoc["status"];
  if (statusRaw == nullptr || strcmp(statusRaw, "ok") != 0)
  {
    return 255;
  }

  const char *pairStatusRaw = jsonDoc["pairStatus"];
  if (pairStatusRaw == nullptr)
  {
    return 255;
  }

  String pairStatus = String(pairStatusRaw);
  pairStatus.toLowerCase();
  if (pairStatus == "pending")
  {
    return 1;
  }
  if (pairStatus == "paired")
  {
    const char *carNameRaw = jsonDoc["carName"];
    if (carNameRaw != nullptr)
    {
      outCarName = String(carNameRaw);
      outCarName.trim();
    }
    return 2;
  }
  if (pairStatus == "expired")
  {
    return 3;
  }
  if (pairStatus == "none")
  {
    return 0;
  }

  return 255;
}

void Board320_240::startEvdashPairing()
{
  if (WiFi.status() != WL_CONNECTED || liveData->settings.wifiEnabled != 1)
  {
    displayMessage("Pair with evdash.eu", "WiFi not connected");
    return;
  }

  String code;
  uint32_t expiresInSec = 0;
  if (!requestPairingStart(code, expiresInSec))
  {
    displayMessage("Pairing failed", "Try again");
    return;
  }

  strncpy(pairPendingCode, code.c_str(), sizeof(pairPendingCode) - 1);
  pairPendingCode[sizeof(pairPendingCode) - 1] = '\0';

  time_t nowTs = liveData->params.currentTime;
  if (nowTs <= 0)
  {
    nowTs = static_cast<time_t>(millis() / 1000U);
  }
  if (expiresInSec < 30)
  {
    expiresInSec = 300;
  }
  pairPendingExpiresAt = nowTs + static_cast<time_t>(expiresInSec);
  pairLastStatusPollMs = 0;
  pairLastKnownState = 1;

  String row3 = String("Pin/Code ") + code;
  displayMessage("Open evdash.eu", "Settings / cars -> pair", row3.c_str());
}

void Board320_240::pollEvdashPairingStatus()
{
  if (pairPendingCode[0] == '\0')
  {
    return;
  }

  time_t nowTs = liveData->params.currentTime;
  if (nowTs <= 0)
  {
    nowTs = static_cast<time_t>(millis() / 1000U);
  }

  if (pairPendingExpiresAt > 0 && nowTs > pairPendingExpiresAt)
  {
    pairPendingCode[0] = '\0';
    pairPendingExpiresAt = 0;
    if (pairLastKnownState != 3)
    {
      displayMessage("Pair code expired", "Generate new code");
    }
    pairLastKnownState = 3;
    return;
  }

  if (WiFi.status() != WL_CONNECTED || liveData->settings.wifiEnabled != 1)
  {
    return;
  }

  const uint32_t nowMs = millis();
  if (pairLastStatusPollMs != 0 &&
      static_cast<uint32_t>(nowMs - pairLastStatusPollMs) < kPairStatusPollIntervalMs)
  {
    return;
  }
  pairLastStatusPollMs = nowMs;

  String carName;
  const uint8_t state = requestPairingStatus(String(pairPendingCode), carName);
  if (state == 255)
  {
    return;
  }

  if (state == 2)
  {
    pairPendingCode[0] = '\0';
    pairPendingExpiresAt = 0;
    pairLastKnownState = 2;
    if (carName.length() == 0)
    {
      carName = "Device linked";
    }
    displayMessage("Paired with evdash.eu", carName.c_str());
    return;
  }

  if (state == 3)
  {
    pairPendingCode[0] = '\0';
    pairPendingExpiresAt = 0;
    if (pairLastKnownState != 3)
    {
      displayMessage("Pair code expired", "Generate new code");
    }
    pairLastKnownState = 3;
    return;
  }

  pairLastKnownState = state;
}

int Board320_240::compareVersionTags(const String &left, const String &right) const
{
  const auto parse = [](const String &input, int out[4]) -> bool {
    for (uint8_t i = 0; i < 4; i++)
    {
      out[i] = 0;
    }

    String normalized = input;
    normalized.trim();
    if (normalized.length() == 0)
    {
      return false;
    }
    if (normalized.charAt(0) == 'v' || normalized.charAt(0) == 'V')
    {
      normalized.remove(0, 1);
    }
    if (normalized.length() == 0)
    {
      return false;
    }

    uint8_t partIndex = 0;
    int value = -1;
    for (uint16_t i = 0; i < normalized.length(); i++)
    {
      const char ch = normalized.charAt(i);
      if (ch >= '0' && ch <= '9')
      {
        if (value < 0)
        {
          value = 0;
        }
        value = (value * 10) + (ch - '0');
      }
      else if (ch == '.')
      {
        if (value < 0 || partIndex >= 4)
        {
          return false;
        }
        out[partIndex++] = value;
        value = -1;
        if (partIndex == 4)
        {
          return true;
        }
      }
      else
      {
        break;
      }
    }

    if (value >= 0 && partIndex < 4)
    {
      out[partIndex++] = value;
    }

    return partIndex >= 3;
  };

  int leftParts[4] = {0, 0, 0, 0};
  int rightParts[4] = {0, 0, 0, 0};
  if (!parse(left, leftParts) || !parse(right, rightParts))
  {
    return 0;
  }

  for (uint8_t i = 0; i < 4; i++)
  {
    if (leftParts[i] < rightParts[i])
    {
      return -1;
    }
    if (leftParts[i] > rightParts[i])
    {
      return 1;
    }
  }

  return 0;
}

void Board320_240::checkFirmwareVersionOnServer()
{
  const uint32_t nowMs = millis();
  if (lastFirmwareVersionCheckMs != 0 &&
      static_cast<uint32_t>(nowMs - lastFirmwareVersionCheckMs) < kFirmwareVersionCheckCooldownMs)
  {
    return;
  }
  lastFirmwareVersionCheckMs = nowMs;

  if (WiFi.status() != WL_CONNECTED)
  {
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(kFirmwareVersionHttpTimeoutMs);

  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(kFirmwareVersionHttpTimeoutMs);
  http.setTimeout(kFirmwareVersionHttpTimeoutMs);

  String firmwareCheckUrl = String(FW_VERSION_CHECK_URL);
  const String hardwareDeviceId = normalizeDeviceIdForApi(getHardwareDeviceId());
  String firmwareCheckId = "";
  if (hardwareDeviceId.length() > 0)
  {
#ifdef BOARD_M5STACK_CORES3
    firmwareCheckId = String("c3.") + hardwareDeviceId;
#else
    firmwareCheckId = String("c2.") + hardwareDeviceId;
#endif
  }

  String appVersionForApi = String(APP_VERSION);
  appVersionForApi.trim();
  if (appVersionForApi.startsWith("v") || appVersionForApi.startsWith("V"))
  {
    appVersionForApi.remove(0, 1);
  }

  if (firmwareCheckId.length() > 0)
  {
    firmwareCheckUrl += (firmwareCheckUrl.indexOf('?') == -1) ? "?" : "&";
    firmwareCheckUrl += "id=" + firmwareCheckId;
  }
  if (appVersionForApi.length() > 0)
  {
    firmwareCheckUrl += (firmwareCheckUrl.indexOf('?') == -1) ? "?" : "&";
    firmwareCheckUrl += "v=" + appVersionForApi;
  }

  if (!http.begin(client, firmwareCheckUrl))
  {
    syslog->println("Firmware check: begin failed");
    return;
  }

  http.addHeader("User-Agent", String("evDash/") + String(APP_VERSION));
  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    syslog->print("Firmware check HTTP code: ");
    syslog->println(httpCode);
    http.end();
    return;
  }

  const String response = http.getString();
  http.end();

  StaticJsonDocument<384> jsonDoc;
  const DeserializationError jsonErr = deserializeJson(jsonDoc, response);
  if (jsonErr)
  {
    syslog->print("Firmware check JSON parse error: ");
    syslog->println(jsonErr.c_str());
    return;
  }

  const char *latestRaw = jsonDoc["version"];
  if (latestRaw == nullptr || latestRaw[0] == '\0')
  {
    syslog->println("Firmware check: missing version field");
    return;
  }

  String latestVersion = String(latestRaw);
  latestVersion.trim();
  if (latestVersion.length() == 0)
  {
    syslog->println("Firmware check: empty version field");
    return;
  }
  if (latestVersion.charAt(0) != 'v' && latestVersion.charAt(0) != 'V')
  {
    latestVersion = String("v") + latestVersion;
  }
  else if (latestVersion.charAt(0) == 'V')
  {
    latestVersion.setCharAt(0, 'v');
  }

  String webFlasherUrl = String(WEBFLASHER_URL);
  const char *webFlasherRaw = jsonDoc["webflasher"];
  if ((webFlasherRaw == nullptr || webFlasherRaw[0] == '\0') && jsonDoc["url"].is<const char *>())
  {
    webFlasherRaw = jsonDoc["url"];
  }
  if (webFlasherRaw != nullptr && webFlasherRaw[0] != '\0')
  {
    webFlasherUrl = String(webFlasherRaw);
    webFlasherUrl.trim();
    if (webFlasherUrl.startsWith("https://"))
    {
      webFlasherUrl.remove(0, 8);
    }
    else if (webFlasherUrl.startsWith("http://"))
    {
      webFlasherUrl.remove(0, 7);
    }
    if (webFlasherUrl.startsWith("www."))
    {
      webFlasherUrl.remove(0, 4);
    }
  }

  String currentVersion = String(APP_VERSION);
  currentVersion.trim();
  if (currentVersion.length() == 0)
  {
    syslog->println("Firmware check: invalid APP_VERSION");
    return;
  }
  if (currentVersion.charAt(0) == 'V')
  {
    currentVersion.setCharAt(0, 'v');
  }

  const int cmp = compareVersionTags(latestVersion, currentVersion);
  if (cmp <= 0)
  {
    syslog->print("Firmware check: up to date (");
    syslog->print(currentVersion);
    syslog->print(" / ");
    syslog->print(latestVersion);
    syslog->println(")");
    return;
  }

  if (latestVersion == lastNotifiedFirmwareVersion)
  {
    syslog->print("Firmware check: already notified ");
    syslog->println(latestVersion);
    return;
  }

  lastNotifiedFirmwareVersion = latestVersion;
  String line2 = String("available ") + latestVersion;
  displayMessage("New version", line2.c_str());
  delay(1800);
  displayMessage("Update here", webFlasherUrl.c_str());
  delay(1800);
}

void Board320_240::addWifiTransferredBytes(size_t bytes)
{
  if (bytes == 0)
  {
    return;
  }
  if (bytes > (UINT32_MAX - wifiTransferredBytes))
  {
    wifiTransferredBytes = UINT32_MAX;
    return;
  }
  wifiTransferredBytes += static_cast<uint32_t>(bytes);
}

void Board320_240::updateNetAvailability(bool success)
{
  if (success)
  {
    liveData->params.netAvailable = true;
    liveData->params.netLastFailureTime = 0;
    liveData->params.netFailureStartTime = 0;
    liveData->params.netFailureCount = 0;
    dismissedNetFailureTime = 0;
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
 * sends data to ABRP if interval elapsed, contributes data if enabled.
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
    dismissedNetFailureTime = 0;
  }

  // Avoid stale "Net unavailable" state when no internet uploader is effectively active.
  const auto remoteApiConfigured = [this]() -> bool {
    if (liveData->settings.remoteUploadIntervalSec == 0)
    {
      return false;
    }
    const char *url = liveData->settings.remoteApiUrl;
    if (url == nullptr || url[0] == '\0')
    {
      return false;
    }
    if (strcmp(url, "not_set") == 0)
    {
      return false;
    }
    return strstr(url, "http") != nullptr;
  }();

  const auto abrpConfigured = [this]() -> bool {
    if (liveData->settings.remoteUploadAbrpIntervalSec == 0)
    {
      return false;
    }
    const char *token = liveData->settings.abrpApiToken;
    if (token == nullptr || token[0] == '\0')
    {
      return false;
    }
    if (strcmp(token, "empty") == 0 || strcmp(token, "not_set") == 0)
    {
      return false;
    }
    return true;
  }();

  const bool contributeConfigured = (liveData->settings.contributeData == 1);
  const bool internetTasksActive = remoteApiConfigured || abrpConfigured || contributeConfigured;

  if (wifiReady && !internetTasksActive)
  {
    liveData->params.netAvailable = true;
    liveData->params.netLastFailureTime = 0;
    liveData->params.netFailureStartTime = 0;
    liveData->params.netFailureCount = 0;
    dismissedNetFailureTime = 0;
  }
  else if (wifiReady && liveData->params.netAvailable == false &&
           liveData->params.netLastFailureTime != 0 &&
           liveData->params.currentTime >= liveData->params.netLastFailureTime &&
           (liveData->params.currentTime - liveData->params.netLastFailureTime) > kNetFailureStaleResetSec)
  {
    // Last error is too old, clear status and wait for the next real send attempt.
    liveData->params.netAvailable = true;
    liveData->params.netLastFailureTime = 0;
    liveData->params.netFailureStartTime = 0;
    liveData->params.netFailureCount = 0;
    dismissedNetFailureTime = 0;
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
  if (netReady && remoteApiConfigured &&
      liveData->params.currentTime - liveData->params.lastRemoteApiSent > liveData->settings.remoteUploadIntervalSec)
  {
    liveData->params.lastRemoteApiSent = liveData->params.currentTime;
    syslog->info(DEBUG_COMM, "Remote send tick");
    int64_t startTime = esp_timer_get_time();
    netSendData(false);
    int64_t endTime = esp_timer_get_time();
    lastNetSendDurationMs = static_cast<uint32_t>((endTime - startTime) / 1000);
  }

  // Upload to ABRP
  if (netReady && abrpConfigured &&
      liveData->params.currentTime - liveData->params.lastAbrpSent > liveData->settings.remoteUploadAbrpIntervalSec)
  {
    syslog->info(DEBUG_COMM, "ABRP send tick");
    int64_t startTime = esp_timer_get_time();
    netSendData(true);
    int64_t endTime = esp_timer_get_time();
    lastNetSendDurationMs = static_cast<uint32_t>((endTime - startTime) / 1000);
  }

  // Contribute data
  const uint32_t nowMs = millis();
  if (liveData->settings.contributeData == 0)
  {
    liveData->params.contributeStatus = CONTRIBUTE_NONE;
    contributeStatusSinceMs = 0;
    nextContributeCycleAtMs = 0;
  }
  if (nextContributeCycleAtMs == 0)
  {
    nextContributeCycleAtMs = nowMs;
  }
  if (netReady && liveData->settings.contributeData == 1 &&
      liveData->params.contributeStatus == CONTRIBUTE_NONE &&
      static_cast<int32_t>(nowMs - nextContributeCycleAtMs) >= 0)
  {
    liveData->params.lastContributeSent = liveData->params.currentTime;
    liveData->params.contributeStatus = CONTRIBUTE_WAITING;
    contributeStatusSinceMs = nowMs;
  }
  bool allowContributeWaitFallback = false;
  if (liveData->settings.commType == COMM_TYPE_CAN_COMMU && commInterface != nullptr)
  {
    const String canStatus = commInterface->getConnectStatus();
    allowContributeWaitFallback = (canStatus.indexOf("No MCP2515") != -1);
  }
  if (netReady && liveData->settings.contributeData == 1 &&
      liveData->params.contributeStatus == CONTRIBUTE_WAITING &&
      liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V2 &&
      allowContributeWaitFallback &&
      contributeStatusSinceMs != 0 &&
      (millis() - contributeStatusSinceMs) >= kContributeWaitFallbackMs)
  {
    syslog->println("contributeStatus ... waiting timeout fallback to ready");
    liveData->params.contributeStatus = CONTRIBUTE_READY_TO_SEND;
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
  int rc = 0;
  const bool netDebug = liveData->settings.debugLevel >= DEBUG_GSM;
  const bool wifiReady = (liveData->settings.wifiEnabled == 1 && WiFi.status() == WL_CONNECTED);
  const String contributeKey = ensureContributeKey();
  const String hardwareDeviceId = normalizeDeviceIdForApi(getHardwareDeviceId());

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
    jsonData["deviceKey"] = contributeKey;
    jsonData["deviceId"] = hardwareDeviceId;
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
        addWifiTransferredBytes(strlen(payload));
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

    size_t payloadLength = serializeJson(jsonData, gAbrpPayloadBuffer, sizeof(gAbrpPayloadBuffer));
    if (payloadLength == 0)
    {
      syslog->println("Failed to serialize ABRP payload");
      return false;
    }

    if (liveData->settings.abrpSdcardLog != 0 && liveData->settings.remoteUploadAbrpIntervalSec > 0)
    {
      queueAbrpSdLog(gAbrpPayloadBuffer, payloadLength, liveData->params.currentTime, liveData->params.operationTimeSec, liveData->params.currTimeSyncWithGps);
    }

    encodeQuotes(gAbrpEncodedPayloadBuffer, sizeof(gAbrpEncodedPayloadBuffer), gAbrpPayloadBuffer);

    int dtaLength = snprintf(gAbrpFormBuffer, sizeof(gAbrpFormBuffer), "api_key=%s&token=%s&tlm=%s", ABRP_API_KEY, liveData->settings.abrpApiToken, gAbrpEncodedPayloadBuffer);
    if (dtaLength < 0 || static_cast<size_t>(dtaLength) >= sizeof(gAbrpFormBuffer))
    {
      syslog->println("ABRP payload too large, skipping send");
      return false;
    }

    if (netDebug)
    {
      syslog->print("Sending data: ");
      syslog->println(gAbrpFormBuffer); // dta is total string sent to ABRP API including api-key and user-token (could be sensitive data to log)
    }
    else
    {
      syslog->println("Sending data to ABRP");
    }

    // Code for sending https data to ABRP api server
    rc = 0;
    if (liveData->settings.remoteUploadModuleType == REMOTE_UPLOAD_WIFI && liveData->settings.wifiEnabled == 1)
    {
      // Track ABRP attempt time only when payload is valid and we're about to do the actual HTTP request.
      liveData->params.lastAbrpSent = liveData->params.currentTime;

      WiFiClientSecure client;
      HTTPClient http;

      client.setInsecure();
      http.begin(client, "https://api.iternio.com/1/tlm/send");
      http.setConnectTimeout(kAbrpHttpsConnectTimeoutMs);
      http.setTimeout(kAbrpHttpsIoTimeoutMs);
      http.setReuse(false);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      const size_t bodyLength = static_cast<size_t>(dtaLength);
      addWifiTransferredBytes(bodyLength);
      rc = http.POST((uint8_t *)gAbrpFormBuffer, bodyLength);

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
  if (payload == nullptr || length == 0)
  {
    return;
  }
  if (liveData->params.stopCommandQueue)
  {
    return;
  }
  if (!liveData->params.sdcardInit || !liveData->params.sdcardRecording)
  {
    return;
  }

  struct tm now;
  time_t logTime = currentTime;
  localtime_r(&logTime, &now);

  if (operationTimeSec > 0 && strlen(liveData->params.sdcardAbrpFilename) == 0)
  {
    sprintf(liveData->params.sdcardAbrpFilename, "/%llu.abrp.json", operationTimeSec / 60);
  }
  if (timeSyncWithGps && strlen(liveData->params.sdcardAbrpFilename) < 20)
  {
    strftime(liveData->params.sdcardAbrpFilename, sizeof(liveData->params.sdcardAbrpFilename), "/%y%m%d%H%M.abrp.json", &now);
  }

  if (strlen(liveData->params.sdcardAbrpFilename) == 0)
  {
    return;
  }

  File file = SD.open(liveData->params.sdcardAbrpFilename, FILE_APPEND);
  if (!file)
  {
    syslog->println("Failed to open ABRP file for appending");
    file = SD.open(liveData->params.sdcardAbrpFilename, FILE_WRITE);
  }
  if (!file)
  {
    syslog->println("Failed to create ABRP file");
    return;
  }

  const size_t writeLen = file.write((const uint8_t *)payload, length);
  if (writeLen != length)
  {
    syslog->println("ABRP SD write truncated");
  }
  file.print(",\n");
  file.close();
}

/**
 * Contributes usage data if enabled in settings.
 * Sends data via WiFi to https://api.evdash.eu/v1/contribute.
 * Data includes vehicle info, location, temps, voltages, etc.
 * Receives a contribute token if successful.
 * Only called for M5Stack Core2 boards.
 **/
bool Board320_240::netContributeData()
{
  int rc = 0;

// Only for core2
#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
  // Contribute data (api.evdash.eu/v1/contribute) to project author (nick.n17@gmail.com)
  if (liveData->settings.wifiEnabled == 1 && WiFi.status() == WL_CONNECTED &&
      liveData->params.contributeStatus == CONTRIBUTE_READY_TO_SEND)
  {
    syslog->println("Contribute data...");
    const char *contributeHost = "api.evdash.eu";
    const char *contributeUrl = "https://api.evdash.eu/v1/contribute";
    const char *contributePath = "/v1/contribute";
    const String contributeKey = ensureContributeKey();
    const String hardwareDeviceId = normalizeDeviceIdForApi(getHardwareDeviceId());
    String payloadForPost;
    auto scheduleNextContributeCycle = [&]() {
      liveData->params.contributeStatus = CONTRIBUTE_NONE;
      contributeStatusSinceMs = 0;
      nextContributeCycleAtMs = millis() + kContributeCycleIntervalMs;
    };
    const bool useJsonV2 = (liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V2);
    if (useJsonV2)
    {
      if (!buildContributePayloadV2(payloadForPost, false))
      {
        syslog->println("Failed to build contribute v2 payload");
        scheduleNextContributeCycle();
        updateNetAvailability(false);
        return false;
      }
      if (isContributeV2SnapshotEffectivelyEmpty(liveData))
      {
        syslog->println("Contribute v2 empty snapshot, skipping send");
        scheduleNextContributeCycle();
        return false;
      }
    }
    else
    {
      if (liveData->contributeDataJson.length() == 0)
      {
        liveData->contributeDataJson = "{";
      }
      if (liveData->contributeDataJson.charAt(liveData->contributeDataJson.length() - 1) != '}')
      {
        liveData->contributeDataJson += "\"key\": \"" + contributeKey + "\", ";
        liveData->contributeDataJson += "\"deviceId\": \"" + hardwareDeviceId + "\", ";
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

        liveData->contributeDataJson += "\"register\": 1, ";
        liveData->contributeDataJson += "\"token\": \"" + contributeKey + "\"";
        liveData->contributeDataJson += "}";
      }
      payloadForPost = liveData->contributeDataJson;
    }

    if (payloadForPost.length() < 2 || payloadForPost.charAt(0) != '{' || payloadForPost.charAt(payloadForPost.length() - 1) != '}')
    {
      syslog->println("Contribute payload invalid, skipping send");
      scheduleNextContributeCycle();
      updateNetAvailability(false);
      return false;
    }
    syslog->print("Contribute payload bytes: ");
    syslog->println(payloadForPost.length());
    syslog->print("Heap intFree/intLargest/psram: ");
    syslog->println(String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + " / " +
                    String(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + " / " +
                    String(ESP.getFreePsram()));

    String responsePayload = "";
    int lastTlsErrCode = 0;
    String lastTlsErrText = "";
    auto postContributePayload = [&](String &outResponse, uint8_t attemptNo) -> int {
      const uint32_t startedMs = millis();
      lastTlsErrCode = 0;
      lastTlsErrText = "";
      WiFiClientSecure client;
      HTTPClient http;
      client.setInsecure();
      client.setHandshakeTimeout((kContributeHttpsConnectTimeoutMs + 999) / 1000);
      client.setTimeout((kContributeHttpsIoTimeoutMs + 999) / 1000);
      const bool beginOk = http.begin(client, contributeUrl);
      if (!beginOk)
      {
        syslog->print("Contribute POST attempt ");
        syslog->print(attemptNo);
        syslog->println(": http.begin failed");
        outResponse = "";
        return -1;
      }
      http.setConnectTimeout(kContributeHttpsConnectTimeoutMs);
      http.setTimeout(kContributeHttpsIoTimeoutMs);
      http.useHTTP10(true);
      http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Connection", "close");
      addWifiTransferredBytes(payloadForPost.length());
      const int postRc = http.POST((uint8_t *)payloadForPost.c_str(), payloadForPost.length());
      const uint32_t elapsedMs = millis() - startedMs;
      syslog->print("Contribute POST attempt ");
      syslog->print(attemptNo);
      syslog->print(" rc=");
      syslog->print(postRc);
      syslog->print(" (");
      syslog->print(elapsedMs);
      syslog->println("ms)");
      outResponse = "";
      if (postRc > 0)
      {
        outResponse = http.getString();
      }
      else
      {
        char tlsErrBuf[160] = {0};
        lastTlsErrCode = client.lastError(tlsErrBuf, sizeof(tlsErrBuf));
        lastTlsErrText = String(tlsErrBuf);
        syslog->print("Contribute TLS lastError: ");
        syslog->print(lastTlsErrCode);
        syslog->print(" ");
        syslog->println(tlsErrBuf);
        syslog->print("Heap intFree/intLargest/psram: ");
        syslog->println(String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + " / " +
                        String(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + " / " +
                        String(ESP.getFreePsram()));
      }
      http.end();
      client.stop();
      return postRc;
    };

    auto postContributePayloadRawTls = [&](String &outResponse, const IPAddress &ip, uint8_t attemptNo) -> int {
      const uint32_t startedMs = millis();
      outResponse = "";
      lastTlsErrCode = 0;
      lastTlsErrText = "";

      WiFiClientSecure client;
      client.setInsecure();
      client.setHandshakeTimeout((kContributeHttpsConnectTimeoutMs + 999) / 1000);
      client.setTimeout((kContributeHttpsIoTimeoutMs + 999) / 1000);

      if (!client.connect(ip, 443, kContributeHttpsConnectTimeoutMs))
      {
        char tlsErrBuf[160] = {0};
        lastTlsErrCode = client.lastError(tlsErrBuf, sizeof(tlsErrBuf));
        lastTlsErrText = String(tlsErrBuf);
        const uint32_t elapsedMs = millis() - startedMs;
        syslog->print("Contribute RAW TLS attempt ");
        syslog->print(attemptNo);
        syslog->print(" connect failed (");
        syslog->print(elapsedMs);
        syslog->println("ms)");
        syslog->print("Contribute TLS lastError: ");
        syslog->print(lastTlsErrCode);
        syslog->print(" ");
        syslog->println(tlsErrBuf);
        client.stop();
        return -1;
      }

      String headers = String("POST ") + contributePath + " HTTP/1.0\r\n" +
                       "Host: " + contributeHost + "\r\n" +
                       "Content-Type: application/json\r\n" +
                       "Connection: close\r\n" +
                       "Content-Length: " + String(payloadForPost.length()) + "\r\n\r\n";

      addWifiTransferredBytes(headers.length() + payloadForPost.length());
      client.print(headers);
      const size_t written = client.write((const uint8_t *)payloadForPost.c_str(), payloadForPost.length());
      if (written != payloadForPost.length())
      {
        syslog->print("Contribute RAW TLS payload short write: ");
        syslog->print(written);
        syslog->print("/");
        syslog->println(payloadForPost.length());
      }

      String rawResponse = "";
      const uint32_t readTimeoutMs = kContributeHttpsIoTimeoutMs;
      uint32_t lastDataMs = millis();
      while ((millis() - lastDataMs) < readTimeoutMs)
      {
        while (client.available())
        {
          const char nextChar = static_cast<char>(client.read());
          if (rawResponse.length() < kContributeResponseBufferCap)
          {
            rawResponse += nextChar;
          }
          lastDataMs = millis();
        }
        if (!client.connected())
        {
          break;
        }
        delay(2);
      }

      int statusCode = -1;
      const int lineEnd = rawResponse.indexOf("\r\n");
      if (lineEnd > 0)
      {
        const String statusLine = rawResponse.substring(0, lineEnd);
        const int sp1 = statusLine.indexOf(' ');
        if (sp1 > 0)
        {
          const int sp2 = statusLine.indexOf(' ', sp1 + 1);
          if (sp2 > sp1)
          {
            statusCode = statusLine.substring(sp1 + 1, sp2).toInt();
          }
          else
          {
            statusCode = statusLine.substring(sp1 + 1).toInt();
          }
        }
      }

      const int bodyPos = rawResponse.indexOf("\r\n\r\n");
      if (bodyPos >= 0)
      {
        outResponse = rawResponse.substring(bodyPos + 4);
      }
      else
      {
        outResponse = rawResponse;
      }

      const uint32_t elapsedMs = millis() - startedMs;
      syslog->print("Contribute RAW TLS attempt ");
      syslog->print(attemptNo);
      syslog->print(" rc=");
      syslog->print(statusCode);
      syslog->print(" (");
      syslog->print(elapsedMs);
      syslog->println("ms)");

      client.stop();
      return statusCode;
    };

    auto postContributePayloadHttp = [&](String &outResponse, uint8_t attemptNo) -> int {
      const uint32_t startedMs = millis();
      outResponse = "";

      WiFiClient client;
      client.setTimeout((kContributeHttpReadTimeoutMs + 999) / 1000);

      if (!client.connect(contributeHost, 80, kContributeHttpConnectTimeoutMs))
      {
        const uint32_t elapsedMs = millis() - startedMs;
        syslog->print("Contribute HTTP fallback attempt ");
        syslog->print(attemptNo);
        syslog->print(" connect failed (");
        syslog->print(elapsedMs);
        syslog->println("ms)");
        client.stop();
        return -1;
      }

      String headers = String("POST ") + contributePath + " HTTP/1.0\r\n" +
                       "Host: " + contributeHost + "\r\n" +
                       "Content-Type: application/json\r\n" +
                       "Connection: close\r\n" +
                       "Content-Length: " + String(payloadForPost.length()) + "\r\n\r\n";

      addWifiTransferredBytes(headers.length() + payloadForPost.length());
      client.print(headers);
      const size_t written = client.write((const uint8_t *)payloadForPost.c_str(), payloadForPost.length());
      if (written != payloadForPost.length())
      {
        syslog->print("Contribute HTTP fallback payload short write: ");
        syslog->print(written);
        syslog->print("/");
        syslog->println(payloadForPost.length());
      }

      String rawResponse = "";
      const uint32_t readTimeoutMs = kContributeHttpReadTimeoutMs;
      uint32_t lastDataMs = millis();
      while ((millis() - lastDataMs) < readTimeoutMs)
      {
        while (client.available())
        {
          const char nextChar = static_cast<char>(client.read());
          if (rawResponse.length() < kContributeResponseBufferCap)
          {
            rawResponse += nextChar;
          }
          lastDataMs = millis();
        }
        if (!client.connected())
        {
          break;
        }
        delay(2);
      }

      int statusCode = -1;
      const int lineEnd = rawResponse.indexOf("\r\n");
      if (lineEnd > 0)
      {
        const String statusLine = rawResponse.substring(0, lineEnd);
        const int sp1 = statusLine.indexOf(' ');
        if (sp1 > 0)
        {
          const int sp2 = statusLine.indexOf(' ', sp1 + 1);
          if (sp2 > sp1)
          {
            statusCode = statusLine.substring(sp1 + 1, sp2).toInt();
          }
          else
          {
            statusCode = statusLine.substring(sp1 + 1).toInt();
          }
        }
      }

      const int bodyPos = rawResponse.indexOf("\r\n\r\n");
      if (bodyPos >= 0)
      {
        outResponse = rawResponse.substring(bodyPos + 4);
      }
      else
      {
        outResponse = rawResponse;
      }

      const uint32_t elapsedMs = millis() - startedMs;
      syslog->print("Contribute HTTP fallback attempt ");
      syslog->print(attemptNo);
      syslog->print(" rc=");
      syslog->print(statusCode);
      syslog->print(" (");
      syslog->print(elapsedMs);
      syslog->println("ms)");

      client.stop();
      return statusCode;
    };

    rc = postContributePayload(responsePayload, 1);
    bool tlsMemIssue = (lastTlsErrCode == -16 || lastTlsErrText.indexOf("Memory allocation failed") != -1);
    IPAddress resolvedHost;
    int dnsRc = 0;
    if (rc < 0)
    {
      dnsRc = WiFi.hostByName(contributeHost, resolvedHost);
      syslog->print("Contribute DNS ");
      syslog->print(contributeHost);
      syslog->print(": ");
      if (dnsRc == 1)
      {
        syslog->println(resolvedHost.toString());
      }
      else
      {
        syslog->println("resolve_failed");
      }
      syslog->print("WiFi RSSI/ch/BSSID: ");
      syslog->println(String(WiFi.RSSI()) + " / " + String(WiFi.channel()) + " / " + WiFi.BSSIDstr());
      if (dnsRc == 1 && kContributeEnableTcpProbe)
      {
        WiFiClient tcpProbe;
        const uint32_t tcpStartedMs = millis();
        const int tcpRc = tcpProbe.connect(resolvedHost, 443, 2000);
        const uint32_t tcpElapsedMs = millis() - tcpStartedMs;
        syslog->print("Contribute TCP probe ");
        syslog->print(resolvedHost.toString());
        syslog->print(":443 rc=");
        syslog->print(tcpRc);
        syslog->print(" (");
        syslog->print(tcpElapsedMs);
        syslog->println("ms)");
        tcpProbe.stop();
      }
      if (!tlsMemIssue)
      {
        if (kContributeRetryOnceOnFail)
        {
          syslog->println("Retry contribute POST once...");
          delay(250);
          rc = postContributePayload(responsePayload, 2);
          tlsMemIssue = (lastTlsErrCode == -16 || lastTlsErrText.indexOf("Memory allocation failed") != -1);
        }
        else
        {
          syslog->println("Contribute HTTPS retry disabled (stability mode)");
        }
      }
      else
      {
        syslog->println("Contribute HTTPS retry skipped (TLS memory issue on attempt 1)");
      }
    }

    if (kContributeRawTlsFallbackOnTlsMem && rc < 0 && tlsMemIssue && dnsRc == 1)
    {
      syslog->println("Contribute TLS memory workaround: raw TLS POST to resolved IP...");
      rc = postContributePayloadRawTls(responsePayload, resolvedHost, 3);
      tlsMemIssue = (lastTlsErrCode == -16 || lastTlsErrText.indexOf("Memory allocation failed") != -1);
    }

    if (kContributeHttpFallbackOnTlsMem && rc < 0 && tlsMemIssue)
    {
      syslog->println("Contribute TLS memory workaround: plain HTTP fallback...");
      rc = postContributePayloadHttp(responsePayload, 4);
      if (rc > 0)
      {
        tlsMemIssue = false;
      }
    }

    if (rc < 0 && tlsMemIssue)
    {
      syslog->println("Contribute HTTPS TLS memory issue detected in low-memory mode.");
    }

    if (rc == HTTP_CODE_OK)
    {
      bool responseAccepted = true;
      syslog->println("HTTP Response (" + String(contributeHost) + "): " + responsePayload);

      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, responsePayload);
      if (!error)
      {
        const char *status = doc["status"];
        if (status != nullptr && strcmp(status, "ok") != 0)
        {
          responseAccepted = false;
          syslog->print("Contribute rejected by server: ");
          syslog->println(status);
        }

        const char *token = doc["token"];
        if (token != nullptr && strlen(token) > 10 &&
            strcmp(liveData->settings.contributeToken, token) != 0)
        {
          syslog->print("Assigned token: ");
          syslog->println(token);
          strncpy(liveData->settings.contributeToken, token, sizeof(liveData->settings.contributeToken) - 1);
          liveData->settings.contributeToken[sizeof(liveData->settings.contributeToken) - 1] = '\0';
          saveSettings();
        }
      }
      else
      {
        // Keep upload as successful even when payload is non-JSON due proxy/WAF text.
        syslog->print("Contribute response parse error: ");
        syslog->println(error.c_str());
      }

      if (responseAccepted)
      {
        scheduleNextContributeCycle();
        if (!useJsonV2)
        {
          liveData->contributeDataJson = "{"; // begin json
        }
        liveData->params.lastSuccessNetSendTime = liveData->params.currentTime;
        updateNetAvailability(true);
      }
      else
      {
        scheduleNextContributeCycle();
        updateNetAvailability(false);
      }
    }
    else
    {
      // Failed...
      if (rc > 0)
      {
        syslog->print("HTTP POST status: ");
        syslog->println(rc);
        if (responsePayload.length() > 0)
        {
          String responsePreview = responsePayload;
          responsePreview.replace('\r', ' ');
          responsePreview.replace('\n', ' ');
          if (responsePreview.length() > 180)
          {
            responsePreview = responsePreview.substring(0, 180) + "...";
          }
          syslog->println("HTTP Response preview (" + String(contributeHost) + "): " + responsePreview);
        }
      }
      else
      {
        syslog->print("HTTP POST error: ");
        syslog->println(rc);
        syslog->print("HTTP POST error text: ");
        syslog->println(HTTPClient::errorToString(rc).c_str());
      }
      syslog->print("WiFi status/IP/GW/DNS: ");
      syslog->println(String(WiFi.status()) + " / " +
                      WiFi.localIP().toString() + " / " +
                      WiFi.gatewayIP().toString() + " / " +
                      WiFi.dnsIP(0).toString());
      scheduleNextContributeCycle();
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
  else if (liveData->settings.gpsModuleType == GPS_MODULE_TYPE_GPS_V21_GNSS)
  {
    pushBaudCandidate(115200);
    pushBaudCandidate(38400);
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
    displayMessage("Error", "WiFi not enabled");
    return;
  }

  File dir = SD.open("/");
  String fileName;
  WiFiClientSecure client;
  HTTPClient http;
  String uploadedStr;
  int rc = 0;
  uint32_t part, uploaded, cntLogs, cntUploaded;
  bool errorUploadFile;
  constexpr size_t kUploadChunkSize = 1024;
  uint8_t buff[kUploadChunkSize];

  displayMessage("Upload logs to server", "No files found");
  cntLogs = cntUploaded = 0;
  String activeLogFilename = String(liveData->params.sdcardFilename);
  if (activeLogFilename.length() > 0 && activeLogFilename.charAt(0) != '/')
  {
    activeLogFilename = "/" + activeLogFilename;
  }

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
      const bool isJsonLog = fileName.endsWith(".json");
      const bool isActiveLog = (activeLogFilename.length() > 0 && fileName == activeLogFilename);
      if (isJsonLog && !isActiveLog)
      {
        cntLogs++;
        displayMessage(fileName.c_str(), "Uploading...");
        size_t sz = 0;
        while ((sz = entry.read(buff, sizeof(buff))) > 0)
        {
          errorUploadFile = false;
          syslog->println("Part:" + String(part) + "\tSize:" + String(sz));
          uploadedStr = "Uploading... " + String(uploaded / 1024) + " / " + String(entry.size() / 1024) + "kB";
          displayMessage(fileName.c_str(), uploadedStr.c_str());

          client.setInsecure();
          const String contributeKey = ensureContributeKey();
          const String hardwareDeviceId = normalizeDeviceIdForApi(getHardwareDeviceId());
          const String registerApiKey = String(liveData->settings.remoteApiKey);
          http.begin(client, "https://api.evdash.eu/v1/contribute/upload?key=" + contributeKey +
                                 "&deviceId=" + hardwareDeviceId +
                                 "&apiKey=" + registerApiKey +
                                 "&register=1" +
                                 "&filename=" + String(entry.name()) + "&part=" + String(part));
          http.setConnectTimeout(1000);
          http.setTimeout(2500);
          addWifiTransferredBytes(sz);
          rc = http.POST(buff, sz);
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
}
