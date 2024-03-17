#ifdef BOARD_M5STACK_CORE2 

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardM5stackCore2.h"
// #include "I2C_MPU6886.h"

// Touch screen
int16_t lastTouchX, lastTouchY;
uint32_t lastTouchTime = 0;
uint32_t lastTouchProcessedTime = 0;
bool touchPressed = false;
bool btnAPressed = false;
bool btnBPressed = false;
bool btnCPressed = false;

// I2C_MPU6886 imu(I2C_MPU6886_DEFAULT_ADDRESS, Wire1);

/*float accX = 0.0F;  // Define variables for storing inertial sensor data
float accY = 0.0F;
float accZ = 0.0F;
float gyroX = 0.0F;
float gyroY = 0.0F;
float gyroZ = 0.0F;
float pitch = 0.0F;
float roll  = 0.0F;
float yaw   = 0.0F;
float temp = 0.0F;
*/

/**
 * Init board
 */
void BoardM5stackCore2::initBoard()
{
  invertDisplay = true;
  pinButtonLeft = BUTTON_LEFT;
  pinButtonRight = BUTTON_RIGHT;
  pinButtonMiddle = BUTTON_MIDDLE;

  // Core instead M5 & AXP begin
  //////////
  Wire.begin(32, 33);     // I2C enable
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

  M5.Axp.SetESPVoltage(3350);
  M5.Axp.SetBusPowerMode((liveData->settings.boardPowerMode == 0 ? 0 : 1)); // 1 - Power from bus; 0 - Power from USB
  M5.Axp.SetLDOVoltage(2, 3300);
  M5.Axp.SetLDOVoltage(3, 2000);
  M5.Axp.SetLDOEnable(2, true);
  M5.Axp.SetDCDC3(false);
  M5.Axp.SetLed(false);
  M5.Axp.SetSpkEnable(false);

  M5.Touch.begin();
  M5.Rtc.begin();
  // M5.IMU.Init(); // Gyro
  // delay(100);

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
 * Wakeup board
 */
void BoardM5stackCore2::wakeupBoard()
{
  M5.Axp.SetLcdVoltage(2500);
  M5.Axp.SetLCDRSet(0);
  delay(100);
  M5.Axp.SetLCDRSet(1);
  M5.Touch.begin();
}

/**
 * Write to Wire1
 */
void BoardM5stackCore2::Write1Byte(uint8_t Addr, uint8_t Data)
{
  Wire1.beginTransmission(0x34);
  Wire1.write(Addr);
  Wire1.write(Data);
  Wire1.endTransmission();
}

/**
 * Read from Wire1
 */
uint8_t BoardM5stackCore2::Read8bit(uint8_t Addr)
{
  Wire1.beginTransmission(0x34);
  Wire1.write(Addr);
  Wire1.endTransmission();
  Wire1.requestFrom(0x34, 1);
  return Wire1.read();
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

  /*    M5.IMU.getGyroData(&gyroX, &gyroY, &gyroZ);
      M5.IMU.getAccelData(
          &accX, &accY,
          &accZ);
      M5.IMU.getAhrsData(
          &pitch, &roll,
          &yaw);
      M5.IMU.getTempData(&temp);  // Stores the inertial sensor temperature to
      if (gyroX != 0.0 || gyroY != 0.0 || gyroZ != 0.0) {
      syslog->printf("gyroX,  gyroY, gyroZ\n");
      syslog->printf("%6.2f %6.2f%6.2f o/s\n", gyroX, gyroY, gyroZ);
      }
      if (accX != 0.0 || accY != 0.0 || accZ != 0.0) {
      syslog->printf("accX,   accY,  accZ\n");
      syslog->printf("%5.2f  %5.2f  %5.2f G\n", accX, accY, accZ);
      }
      if (pitch != 0.0 || roll != 0.0 || yaw != -8.5) {
      syslog->printf("pitch,  roll,  yaw\n");
      syslog->printf("%5.2f  %5.2f  %5.2f deg\n", pitch, roll, yaw);
      }
      syslog->flush();
      */
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