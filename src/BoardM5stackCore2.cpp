/**
 * Initialization and setup for M5Stack Core2 board.
 *
 * Initializes board hardware like display, buttons, RTC, power management.
 * Registers touch handlers.
 * Contains board specific implementations of interface methods.
 *
 *

BoardM5stackCore2.cpp:

This code initializes and configures the M5Stack Core2 board.
It starts by including required header files and defining pins for the left, right and middle buttons on the device. Some variables are declared to track the touch screen status and button presses. An I2C sensor interface is created but commented out.
- The initBoard() function is called on startup to configure the board. It initializes the I2C buses, sets power modes and voltages on the AXP192 PMIC chip, and initializes the touch screen and RTC. It calls initBoard() on the parent Board320_240 class to initialize the display.
- afterSetup() is called after setup to configure touch and button handlers. It attaches callback functions to handle touch and button events.
- isButtonPressed() checks if a touch or button press occurred. It updates the last button press time, processes the touch or button event, and returns true if a relevant event occurred that should be handled.
Overall, this initializes the specific hardware on the M5Stack Core2, configures power and inputs, handles touch and button events, and acts as the interface between the hardware and the rest of the application code.
 */
#ifdef BOARD_M5STACK_CORE2

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardM5stackCore2.h"

// GNSS Module with Barometric Pressure, IMU, Magnetometer Sensors (NEO-M9N, BMP280, BMI270, BMM150)
// https://github.com/m5stack/M5Module-GNSS/blob/main/examples/getSensorData/getSensorData.ino
/*#include <Adafruit_BMP280.h>

#define BIM270_SENSOR_ADDR 0x68
#define BMP280_SENSOR_ADDR 0x76

BMI270::BMI270 bmi270;
Adafruit_BMP280 bmp(&Wire);
*/

// Touch screen
int16_t lastTouchX, lastTouchY;
uint32_t lastTouchTime = 0;
uint32_t lastTouchProcessedTime = 0;
bool touchPressed = false;
bool btnAPressed = false;
bool btnBPressed = false;
bool btnCPressed = false;

float accX = 0.0F; // Define variables for storing inertial sensor data
float accY = 0.0F;
float accZ = 0.0F;
float gyroX = 0.0F;
float gyroY = 0.0F;
float gyroZ = 0.0F;
float pitch = 0.0F;
float roll = 0.0F;
float yaw = 0.0F;
float temp = 0.0F;

/**
 * Init board
 */
void BoardM5stackCore2::initBoard()
{
  pinButtonLeft = BUTTON_LEFT;
  pinButtonRight = BUTTON_RIGHT;
  pinButtonMiddle = BUTTON_MIDDLE;

  // Core instead M5 & AXP begin
  //////////
  /*Wire.begin(32, 33);     // I2C enable
  Wire1.begin(21, 22);    // AXP begin
  Wire1.setClock(400000); // AXP
  // AXP192 30H
  Write1Byte(0x30, (Read8bit(0x30) & 0x04) | 0X02);
  // AXP192 GPIO1:OD OUTPUT
  Write1Byte(0x92, Read8bit(0x92) & 0xf8);
  // AXP192 GPIO2:OD OUTPUT
  Write1Byte(0x93, Read8bit(0x93) & 0xf8);
  // AXP192 RTC CHG
  Write1Byte(0x35, (Read8bit(0x35) & 0x1c) | 0xa2);
  // AXP192 GPIO4
  Write1Byte(0X95, (Read8bit(0x95) & 0x72) | 0X84);
  Write1Byte(0X36, 0X4C);
  Write1Byte(0x82, 0xff);
*/
  // Set AXP192 to automatically power on when VBUS or ACIN is applied
  uint8_t data = /*M5.Axp.*/ Read8bit(0x31);
  data |= 0x06; // Set bits 1 and 2 to enable power-on by ACIN and VBUS rising edge
  /*M5.Axp.*/ Write1Byte(0x31, data);

  liveData->settings.boardPowerMode = 1;
  M5.begin(true, true, true, true, (liveData->settings.boardPowerMode == 1 ? kMBusModeInput : kMBusModeOutput), false);

  // Configure AXP192 PMIC
  // Set the LCD and ESP voltages if necessary
  M5.Axp.SetLcdVoltage(2500);
  M5.Axp.SetESPVoltage(3350);

  // Enable LDO2 and set voltages
  M5.Axp.SetLDO2(true);
  M5.Axp.SetLDOVoltage(2, 3300);
  M5.Axp.SetLDOVoltage(3, 2000);
  M5.Axp.SetLDOEnable(2, true);
  M5.Axp.SetDCDC3(true);

  // Set AXP192 to automatically power on when VBUS or ACIN is applied
  data = /*M5.Axp.*/ Read8bit(0x31);
  data |= 0x06; // Set bits 1 and 2 to enable power-on by ACIN and VBUS rising edge
  /*M5.Axp.*/ Write1Byte(0x31, data);

  // Initialize other components
  M5.Touch.begin();
  M5.Rtc.begin();
  M5.IMU.Init();
  M5.Axp.SetLed(false);

  // Wake up the device
  M5.Axp.SetLcdVoltage(2500);
  // M5.Axp.SetLCDRSet(0);
  // delay(100);
  // M5.Axp.SetLCDRSet(1);

  Board320_240::initBoard();
}

/**
 * After setup
 */
void BoardM5stackCore2::afterSetup()
{
  Board320_240::afterSetup();

  syslog->println(" START -> BoardM5stackCore2::afterSetup ");

  // Touch screen zone
  uint16_t events = (false) ? E_ALL : (E_ALL - E_MOVE); // Show all events, or everything but E_MOVE? Controlled with A button.

  M5.background.delHandlers();
  M5.background.tapTime = 50;
  M5.background.dbltapTime = 300;
  M5.background.longPressTime = 700;
  M5.background.repeatDelay = 250;
  M5.background.repeatInterval = 250;
  M5.background.addHandler(eventDisplay, events);
  M5.Buttons.addHandler(eventDisplay, events);
}

/**
 * Button pressed and touchscreen support
 */
bool BoardM5stackCore2::isButtonPressed(int button)
{
  // All events
  if (touchPressed || btnAPressed || btnBPressed || btnCPressed)
  {
    liveData->params.lastButtonPushedTime = liveData->params.currentTime; // prevent screen sleep mode
    lastTouchProcessedTime = lastTouchTime = millis();                    // restart timer

    // Prevent touch handler when display is waking up
    if (currentBrightness == 0 && liveData->params.displayScreen != SCREEN_BLANK)
    {
      setBrightness();
      redrawScreen();
      return true;
    }
  }

  // Touch screen
  if (touchPressed)
  {
    syslog->println("Touch event");
    liveData->continueWithCommandQueue();
    touchPressed = false;

    // Process action
    if (!liveData->menuVisible)
    {
      tft.setRotation(liveData->settings.displayRotation);
      if (lastTouchY > 64 && lastTouchY < 150)
      {
        // lastTouchX < 120
        if (lastTouchX < 120)
        {
          if (liveData->params.displayScreen == 0) // rotate screens
            liveData->params.displayScreen = displayScreenCount - 1;
          else
            liveData->params.displayScreen--;
          setBrightness(); // Turn off display on screen 0
          redrawScreen();
        }
        // lastTouchX >= 120 && lastTouchX <= 200
        if (lastTouchX >= 120 && lastTouchX <= 200)
        {
          showMenu();
        }
        // lastTouchX > 200
        if (lastTouchX > 200)
        {
          liveData->params.displayScreen++;
          if (liveData->params.displayScreen > displayScreenCount - 1)
            liveData->params.displayScreen = 0; // rotate screens
          setBrightness();                      // Turn off display on screen 0
          redrawScreen();
        }
      }
    }
    else
    {
      // Left top corner (up menu or exit menu)
      if (lastTouchX < 64 && lastTouchY < 64)
      {
        liveData->menuItemSelected = 0;
        menuItemClick();
      }
      else // Right top corner - page up
        if (lastTouchX > 320 - 64 && lastTouchY < 64)
        {
          for (uint8_t i = 0; i < menuVisibleCount; i++)
            menuMove(false, false);
          showMenu();
        }
        else // Right bottom corne - page down
          if (lastTouchX > 320 - 64 && lastTouchY > 240 - 64)
          {
            for (uint8_t i = 0; i < menuVisibleCount; i++)
              menuMove(true, false);
            showMenu();
          }
          else // Click item
          {
            liveData->menuItemSelected = liveData->menuItemOffset + uint16_t(lastTouchY / menuItemHeight);
            showMenu();
            menuItemClick();
          }
    }
  }

  // Bottom touch buttons
  switch (button)
  {
  case BUTTON_LEFT:
    if (btnAPressed)
    {
      btnAPressed = false;
      return true;
    }
    return false;
  case BUTTON_MIDDLE:
    if (btnBPressed)
    {
      btnBPressed = false;
      return true;
    }
    return false;
  case BUTTON_RIGHT:
    if (btnCPressed)
    {
      btnCPressed = false;
      return true;
    }
    return false;
  }

  return false;
}

/**
 * Touch screen handler
 */
void BoardM5stackCore2::eventDisplay(Event &e)
{
  if (e.type == E_TOUCH && (lastTouchX != e.to.x || lastTouchY != e.to.y))
  {
    // syslog->println("E_TOUCH PRESSED");
    lastTouchX = e.to.x;
    lastTouchY = e.to.y;
    lastTouchTime = millis();
  }

  //  Not take button as touch event only background events
  // strcmp(e.objName(), "background") == 0 &&
  if (e.type == E_RELEASE &&
      !touchPressed && !btnAPressed && !btnBPressed && !btnCPressed &&
      lastTouchX == e.to.x && lastTouchY == e.to.y && lastTouchTime != 0)
  {
    if (millis() - lastTouchTime > M5.background.tapTime &&
        millis() - lastTouchProcessedTime > M5.background.longPressTime)
    {
      if (lastTouchY >= 240)
      {
        if (strcmp(e.objName(), "BtnA") == 0)
          btnAPressed = true;
        else if (strcmp(e.objName(), "BtnB") == 0)
          btnBPressed = true;
        else if (strcmp(e.objName(), "BtnC") == 0)
          btnCPressed = true;
        else
          syslog->println("Unknown button");
      }
      else
      {
        touchPressed = true;
      }

      syslog->print("Touch pressed: ");
      syslog->println(e.objName());
    }
  }
  /*
    syslog->printf("%-12s finger%d  %-18s (%3d, %3d) --> (%3d, %3d)   ",
                   e.typeName(), e.finger, e.objName(), e.from.x, e.from.y,
                   e.to.x, e.to.y);
    syslog->printf("( dir %d deg, dist %d, %d ms )\n", e.direction(),
                   e.distance(), e.duration);*/
}

/**
 * Enter sleep mode
 */
void BoardM5stackCore2::enterSleepMode(int secs)
{
  if (secs > 0)
  {
    syslog->println("Going to sleep for " + String(secs) + " seconds!");
    syslog->flush();
    delay(100);
    M5.Axp.DeepSleep(secs * 1000000ULL);
  }
  else
  {
    syslog->println("Shutting down...");
    syslog->flush();
    delay(100);
    M5.Axp.PowerOff();
  }
}

/**
 * Board loop
 */
void BoardM5stackCore2::boardLoop()
{
  M5.update();
  Board320_240::boardLoop();

  M5.IMU.getGyroData(&gyroX, &gyroY, &gyroZ);
  M5.IMU.getAccelData(
      &accX, &accY,
      &accZ);
  M5.IMU.getAhrsData(
      &pitch, &roll,
      &yaw);
  M5.IMU.getTempData(&temp);

  if (gyroX != 0.0 || gyroY != 0.0 || gyroZ != 0.0 || accX != 0.0 || accY != 0.0 || accZ != 0.0 || pitch != 0.0 || roll != 0.0 || yaw != -8.5)
  {
    liveData->params.gyroSensorMotion = false;
    if (abs(gyroX) > 15.0 || abs(gyroY) > 15.0 || abs(gyroZ) > 15.0)
    {
      liveData->params.gyroSensorMotion = true;
    }
    // syslog->printf("gyroX,  gyroY, gyroZ accX,   accY,  accZpitch,  roll,  yaw\n");
    // syslog->printf("%6.2f %6.2f%6.2f o/s %5.2f  %5.2f  %5.2f G %5.2f  %5.2f  %5.2f deg\n", gyroX, gyroY, gyroZ, accX, accY, accZ, pitch, roll, yaw);
    // delay(250);
  }
  syslog->flush();
}

/**
 * Main loop
 */
void BoardM5stackCore2::mainLoop()
{
  Board320_240::mainLoop();
}

/**
 * Skip adapter scan
 */
bool BoardM5stackCore2::skipAdapterScan()
{
  bool pressed = false;

  M5.Lcd.clear(RED);
  for (uint16_t i = 0; i < 2000 * 10; i++)
  {
    M5.update();
    if (M5.BtnA.isPressed() == true || M5.BtnB.isPressed() == true || M5.BtnC.isPressed() == true ||
        btnAPressed || btnBPressed || btnCPressed)
    {
      pressed = true;
      break;
    };
  }

  M5.Lcd.clear(BLACK);

  return pressed;
}

/**
 * Set time
 */
void BoardM5stackCore2::setTime(String timestamp)
{
  RTC_TimeTypeDef RTCtime;
  RTC_DateTypeDef RTCdate;

  RTCdate.Year = timestamp.substring(0, 4).toInt();
  RTCdate.Month = timestamp.substring(5, 7).toInt();
  RTCdate.Date = timestamp.substring(8, 10).toInt();
  RTCtime.Hours = timestamp.substring(11, 13).toInt();
  RTCtime.Minutes = timestamp.substring(14, 16).toInt();
  RTCtime.Seconds = timestamp.substring(17, 19).toInt();

  M5.Rtc.SetTime(&RTCtime);
  M5.Rtc.SetDate(&RTCdate);

  BoardInterface::setTime(timestamp);
}

/**
 * Sync NTP time
 */
void BoardM5stackCore2::ntpSync()
{
  syslog->println("Syncing NTP time.");

  char ntpServer[] = "de.pool.ntp.org";
  configTime(liveData->settings.timezone * 3600, liveData->settings.daylightSaving * 3600, ntpServer);
  liveData->params.ntpTimeSet = true;

  showTime();
}

#endif // BOARD_M5STACK_CORE2