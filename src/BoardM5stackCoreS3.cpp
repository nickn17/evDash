#ifdef BOARD_M5STACK_CORES3

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardM5stackCoreS3.h"
#include <time.h>
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
void BoardM5stackCoreS3::initBoard()
{
  pinButtonLeft = BUTTON_LEFT;
  pinButtonRight = BUTTON_RIGHT;
  pinButtonMiddle = BUTTON_MIDDLE;

  // Core instead M5 & AXP begin
  //////////
  auto cfg = M5.config();
  CoreS3.begin(cfg);
  CoreS3.Power.setChargeVoltage(3350);
  // true - Power from bus; false - Power from USB
  CoreS3.Power.setExtOutput((liveData->settings.boardPowerMode == 1));
  CoreS3.Power.setLed(0);
  // CoreS3.Power.SetLDOVoltage(2, 3300);
  // CoreS3.Power.SetLDOVoltage(3, 2000);
  // CoreS3.Power.SetLDOEnable(2, true);
  // CoreS3.Power.set SetDCDC3(false);
  CoreS3.Speaker.end();
  CoreS3.Touch.begin(&CoreS3.Display);
  CoreS3.Rtc.begin();
  CoreS3.Imu.begin(); // Gyro

  Board320_240::initBoard();
}

/**
 * After setup
 */
void BoardM5stackCoreS3::afterSetup()
{
  Board320_240::afterSetup();

  syslog->println(" START -> BoardM5stackCoreS3::afterSetup ");

  // Touch screen zone
  // uint16_t events = (false) ? E_ALL : (E_ALL - E_MOVE); // Show all events, or everything but E_MOVE? Controlled with A button.

  /*M5.background.delHandlers();
  M5.background.tapTime = 50;
  M5.background.dbltapTime = 300;
  M5.background.longPressTime = 700;
  M5.background.repeatDelay = 250;
  M5.background.repeatInterval = 250;
  M5.background.addHandler(eventDisplay, events);
  M5.Buttons.addHandler(eventDisplay, events);*/
}

/**
 * Button pressed and touchscreen support
 */
bool BoardM5stackCoreS3::isButtonPressed(int button)
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
/*void BoardM5stackCoreS3::eventDisplay(Event &e)
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
                   e.distance(), e.duration);
}*/

/**
 * Enter sleep mode
 */
void BoardM5stackCoreS3::enterSleepMode(int secs)
{
  if (secs > 0)
  {
    syslog->println("Going to sleep for " + String(secs) + " seconds!");
    syslog->flush();
    delay(100);
    CoreS3.Power.deepSleep(secs * 1000000ULL);
  }
  else
  {
    syslog->println("Shutting down...");
    syslog->flush();
    delay(100);
    CoreS3.Power.powerOff();
  }
}

static m5::touch_state_t prev_state;

/**
 * Board loop
 */
void BoardM5stackCoreS3::boardLoop()
{
  M5.update();

  auto t = CoreS3.Touch.getDetail();
  if (t.wasClicked())
  {
    lastTouchX = t.x;
    lastTouchY = t.y;
    touchPressed = true;
  }
  if (prev_state != t.state)
  {
    prev_state = t.state;
    static constexpr const char *state_name[16] = {
        "none", "touch", "touch_end", "touch_begin",
        "___", "hold", "hold_end", "hold_begin",
        "___", "flick", "flick_end", "flick_begin",
        "___", "drag", "drag_end", "drag_begin"};
    syslog->println(state_name[t.state]);
  }

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
void BoardM5stackCoreS3::mainLoop()
{
  Board320_240::mainLoop();
}

/**
 * Skip adapter scan
 */
bool BoardM5stackCoreS3::skipAdapterScan()
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
void BoardM5stackCoreS3::setTime(String timestamp)
{
  struct tm tm;
  tm.tm_year = timestamp.substring(0, 4).toInt() - 1900;
  tm.tm_mon = timestamp.substring(5, 7).toInt() - 1;
  tm.tm_mday = timestamp.substring(8, 10).toInt();
  tm.tm_hour = timestamp.substring(11, 13).toInt();
  tm.tm_min = timestamp.substring(14, 16).toInt();
  tm.tm_sec = timestamp.substring(17, 19).toInt();
  time_t t = mktime(&tm);
  CoreS3.Rtc.setDateTime(gmtime(&t));

  BoardInterface::setTime(timestamp);
}

/**
 * Sync NTP time
 */
void BoardM5stackCoreS3::ntpSync()
{
  syslog->println("Syncing NTP time.");

  char ntpServer[] = "de.pool.ntp.org";
  configTime(liveData->settings.timezone * 3600, liveData->settings.daylightSaving * 3600, ntpServer);
  liveData->params.ntpTimeSet = true;

  showTime();
}

#endif // BOARD_M5STACK_CORES3