#ifdef BOARD_M5STACK_CORE2

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardM5stackCore2.h"

// Defines gestures
Gesture swipeRight("swipe right", 160, DIR_RIGHT, 30, true);
Gesture swipeDown("swipe down", 120, DIR_DOWN, 30, true);
Gesture swipeLeft("swipe left", 160, DIR_LEFT, 30, true);
Gesture swipeUp("swipe up", 120, DIR_UP, 30, true);

int16_t lastTouchX, lastTouchY;

/**
  Init board
*/
void BoardM5stackCore2::initBoard()
{

  invertDisplay = true;
  pinButtonLeft = BUTTON_LEFT;
  pinButtonRight = BUTTON_RIGHT;
  pinButtonMiddle = BUTTON_MIDDLE;

  Wire.begin(32, 33);
  Wire1.begin(21, 22);
  Wire1.setClock(400000);

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
  M5.Axp.SetBusPowerMode(1); // 1 - Power from bus; 0 - Power from USB
  M5.Axp.SetLDOVoltage(2, 3300);
  M5.Axp.SetLDOVoltage(3, 2000);
  M5.Axp.SetLDOEnable(2, true);
  M5.Axp.SetDCDC3(false);
  M5.Axp.SetLed(false);

  M5.Rtc.begin();
  delay(100);

  Board320_240::initBoard();
}

void BoardM5stackCore2::afterSetup()
{

  Board320_240::afterSetup();

  M5.background.addHandler(eventDisplay, E_ALL /* - E_MOVE*/);
  M5.BtnA.addHandler(eventDisplay, E_ALL /* - E_MOVE*/);
  M5.BtnB.addHandler(eventDisplay, E_ALL /* - E_MOVE*/);
  M5.BtnC.addHandler(eventDisplay, E_ALL /* - E_MOVE*/);
}

void BoardM5stackCore2::wakeupBoard()
{
  M5.Axp.SetLcdVoltage(2500);
  M5.Axp.SetLCDRSet(0);
  delay(100);
  M5.Axp.SetLCDRSet(1);
  M5.Touch.begin();
}

void BoardM5stackCore2::Write1Byte(uint8_t Addr, uint8_t Data)
{
  Wire1.beginTransmission(0x34);
  Wire1.write(Addr);
  Wire1.write(Data);
  Wire1.endTransmission();
}

uint8_t BoardM5stackCore2::Read8bit(uint8_t Addr)
{
  Wire1.beginTransmission(0x34);
  Wire1.write(Addr);
  Wire1.endTransmission();
  Wire1.requestFrom(0x34, 1);
  return Wire1.read();
}

bool BoardM5stackCore2::isButtonPressed(int button)
{

  switch (button)
  {
  case BUTTON_LEFT:
    return M5.BtnA.pressedFor(200, 500);
    break;
  case BUTTON_MIDDLE:
    return M5.BtnB.pressedFor(200, 500);
    break;
  case BUTTON_RIGHT:
    return M5.BtnC.pressedFor(200, 500);
    break;
  default:
    return false;
    break;
  }
}

void BoardM5stackCore2::eventDisplay(Event &e)
{

  lastTouchX = e.to.x;
  lastTouchY = e.to.y;

  /*syslog->printf("%-12s finger%d  %-18s (%3d, %3d) --> (%3d, %3d)   ",
                 e.typeName(), e.finger, e.objName(), e.from.x, e.from.y,
                 e.to.x, e.to.y);
  syslog->printf("( dir %d deg, dist %d, %d ms )\n", e.direction(),
                 e.distance(), e.duration);
                 */
}

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

void BoardM5stackCore2::mainLoop()
{

  Board320_240::mainLoop();
  M5.update();

  // Touch
  if (M5.background.pressedFor(200, 500))
  {
    liveData->params.lastButtonPushedTime = liveData->params.currentTime; // prevent screen sleep mode

    if (!liveData->menuVisible)
    {
      liveData->params.lastButtonPushedTime = liveData->params.currentTime;
      tft.setRotation(liveData->settings.displayRotation);
      if (lastTouchY > 64 && lastTouchY < 220)
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
          showMenu();
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
 *
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
