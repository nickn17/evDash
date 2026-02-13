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
bool touchDragged = false;
bool touchDragGestureActive = false;
bool touchSwipeGestureActive = false;
int16_t touchDragDeltaY = 0;
int16_t touchDragDeltaYPrev = 0;
int16_t touchSwipeDeltaX = 0;
bool btnAPressed = false;
bool btnBPressed = false;
bool btnCPressed = false;

static constexpr int16_t MENU_DRAG_THRESHOLD_PX = 6;
static constexpr int16_t TOUCH_SWIPE_THRESHOLD_PX = 18;

// I2C_MPU6886 imu(I2C_MPU6886_DEFAULT_ADDRESS, Wire1);

float accX = 0.0F; // Inertial sensor data
float accY = 0.0F;
float accZ = 0.0F;
float gyroX = 0.0F;
float gyroY = 0.0F;
float gyroZ = 0.0F;

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
  // boardPowerMode: 1 = power from external bus (input), 0 = power from USB (output to external bus)
  const bool extPowerInputMode = (liveData->settings.boardPowerMode == 1);
  CoreS3.Power.setExtOutput(!extPowerInputMode);
  CoreS3.Power.setLed(0);
  syslog->print("CoreS3 bus power mode: ");
  syslog->println(extPowerInputMode ? "external input" : "USB output");
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
  if (isTouchInputSuppressed())
  {
    touchPressed = false;
    touchDragged = false;
    touchDragGestureActive = false;
    touchSwipeGestureActive = false;
    touchDragDeltaY = 0;
    touchDragDeltaYPrev = 0;
    touchSwipeDeltaX = 0;
    screenSwipePreviewActive = false;
  }

  if (isMessageDialogVisible())
  {
    if (touchPressed || btnAPressed || btnBPressed || btnCPressed)
    {
      touchPressed = false;
      touchDragged = false;
      touchDragGestureActive = false;
      touchSwipeGestureActive = false;
      touchDragDeltaY = 0;
      touchDragDeltaYPrev = 0;
      touchSwipeDeltaX = 0;
      screenSwipePreviewActive = false;
      btnAPressed = false;
      btnBPressed = false;
      btnCPressed = false;
      dismissMessageDialog();
    }
    return false;
  }

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
      liveData->continueWithCommandQueue();
      if (commInterface->isSuspended())
      {
        commInterface->resumeDevice();
      }
      touchPressed = false;
      touchSwipeGestureActive = false;
      touchSwipeDeltaX = 0;
      return true;
    }
  }

  // Touch screen
  if (touchPressed)
  {
    if (modalDialogActive)
    {
      touchPressed = false;
      return false;
    }

    bool menuDrag = touchDragged;
    int16_t menuDragDeltaY = touchDragDeltaY;
    touchDragged = false;
    touchDragDeltaY = 0;
    if (!menuDrag)
    {
      touchDragDeltaYPrev = 0;
    }

    syslog->println("Touch event");
    liveData->continueWithCommandQueue();
    if (commInterface->isSuspended())
    {
      commInterface->resumeDevice();
    }
    touchPressed = false;

    // Process action
    if (!liveData->menuVisible)
    {
      const uint8_t firstCarouselScreen = SCREEN_DASH;
      const uint8_t lastCarouselScreen = SCREEN_DEBUG;
      int16_t swipeCommitThresholdPx = int16_t((tft.width() * 30) / 100);
      if (swipeCommitThresholdPx < TOUCH_SWIPE_THRESHOLD_PX)
        swipeCommitThresholdPx = TOUCH_SWIPE_THRESHOLD_PX;
      tft.setRotation(liveData->settings.displayRotation);
      if (touchSwipeGestureActive)
      {
        if (abs(touchSwipeDeltaX) >= swipeCommitThresholdPx)
        {
          if (touchSwipeDeltaX > 0)
          {
            if (liveData->params.displayScreen > firstCarouselScreen &&
                liveData->params.displayScreen <= lastCarouselScreen)
              liveData->params.displayScreen--;
          }
          else
          {
            if (liveData->params.displayScreen >= firstCarouselScreen &&
                liveData->params.displayScreen < lastCarouselScreen)
              liveData->params.displayScreen++;
          }
        }
        touchSwipeGestureActive = false;
        screenSwipePreviewActive = false;
        touchSwipeDeltaX = 0;
        setBrightness();
        redrawScreen();
      }
      else
      {
        if (!menuDrag && canStatusMessageHitTest(lastTouchX, lastTouchY))
        {
          dismissCanStatusMessage();
          redrawScreen();
          return false;
        }
        bool cellSceneVisible = (liveData->params.displayScreen == SCREEN_CELLS || liveData->params.displayScreenAutoMode == SCREEN_CELLS);
        if (cellSceneVisible && lastTouchY < 64)
        {
          // Cell screen paging: top-left = previous page, top-right = next page.
          batteryCellsPageMove(lastTouchX >= 160);
        }
        else if (lastTouchY > 64 && lastTouchY < 150)
        {
          bool screenChanged = false;
          // lastTouchX < 120
          if (lastTouchX < 120)
          {
            if (liveData->params.displayScreen == 0)
              liveData->params.displayScreen = displayScreenCount - 1;
            else
              liveData->params.displayScreen--;
            screenChanged = true;
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
              liveData->params.displayScreen = 0;
            screenChanged = true;
          }
          if (screenChanged)
          {
            setBrightness();
            redrawScreen();
          }
        }
      }
    }
    else
    {
      bool inUpperLeftZone = (!menuDrag && lastTouchX < 64 && lastTouchY < 64);
      bool inPageUpZone = (!menuDrag && lastTouchX > 320 - 64 && lastTouchY < 64);
      bool inPageDownZone = (!menuDrag && lastTouchX > 320 - 64 && lastTouchY > 240 - 64);

      if (inUpperLeftZone)
      {
        menuTouchHoverIndex = -1;
        liveData->menuItemSelected = 0;
        menuItemClick();
      }
      else if (inPageUpZone)
      {
        menuTouchHoverIndex = -1;
        for (uint8_t i = 0; i < menuVisibleCount; i++)
          menuMove(false, false);
        showMenu();
      }
      else if (inPageDownZone)
      {
        menuTouchHoverIndex = -1;
        for (uint8_t i = 0; i < menuVisibleCount; i++)
          menuMove(true, false);
        showMenu();
      }
      else if (menuDrag) // Drag menu page (pixel scrolling)
      {
        int16_t delta = menuDragDeltaY - touchDragDeltaYPrev;
        touchDragDeltaYPrev = menuDragDeltaY;
        menuScrollByPixels(delta);
        menuTouchHoverIndex = -1; // no hover highlight during drag
        // Keep cursor near finger while dragging to avoid snap-back on release.
        uint16_t totalItems = menuItemsCountCurrent();
        if (totalItems > 0)
        {
          bool inUpperLeftZone = (lastTouchX < 64 && lastTouchY < 64);
          bool inPageUpZone = (lastTouchX > 320 - 64 && lastTouchY < 64);
          bool inPageDownZone = (lastTouchX > 320 - 64 && lastTouchY > 240 - 64);
          if (!inUpperLeftZone && !inPageUpZone && !inPageDownZone)
          {
            uint16_t hoverItem = menuItemFromTouchY(lastTouchY);
            if (hoverItem < totalItems)
            {
              liveData->menuItemSelected = hoverItem;
            }
          }
        }
        menuDragScrollActive = true;
        showMenu();
        menuDragScrollActive = false;
      }
      else // Click item
      {
        menuTouchHoverIndex = -1;
        liveData->menuItemSelected = menuItemFromTouchY(lastTouchY);
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

bool BoardM5stackCoreS3::getTouch(int16_t &x, int16_t &y)
{
  if (!touchPressed)
  {
    return false;
  }
  x = lastTouchX;
  y = lastTouchY;
  touchPressed = false;
  return true;
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
  if (isMessageDialogVisible())
  {
    const bool dismissRequested = t.wasClicked() || btnAPressed || btnBPressed || btnCPressed;
    touchPressed = false;
    touchDragged = false;
    touchDragGestureActive = false;
    touchSwipeGestureActive = false;
    touchDragDeltaY = 0;
    touchDragDeltaYPrev = 0;
    touchSwipeDeltaX = 0;
    screenSwipePreviewActive = false;
    menuTouchHoverIndex = -1;

    if (dismissRequested)
    {
      btnAPressed = false;
      btnBPressed = false;
      btnCPressed = false;
      dismissMessageDialog();
    }

    Board320_240::boardLoop();

    CoreS3.Imu.getGyroData(&gyroX, &gyroY, &gyroZ);
    CoreS3.Imu.getAccelData(&accX, &accY, &accZ);

    if (gyroX != 0.0 || gyroY != 0.0 || gyroZ != 0.0 || accX != 0.0 || accY != 0.0 || accZ != 0.0)
    {
      liveData->params.gyroSensorMotion = false;
      if (abs(gyroX) > 15.0 || abs(gyroY) > 15.0 || abs(gyroZ) > 15.0)
      {
        liveData->params.gyroSensorMotion = true;
      }
    }

    return;
  }

  if (liveData->menuVisible && t.isPressed())
  {
    bool inUpperLeftZone = (t.x < 64 && t.y < 64);
    bool inPageUpZone = (t.x > 320 - 64 && t.y < 64);
    bool inPageDownZone = (t.x > 320 - 64 && t.y > 240 - 64);
    bool inReservedZone = inUpperLeftZone || inPageUpZone || inPageDownZone;

    uint16_t totalItems = menuItemsCountCurrent();
    int16_t hoverIndex = (totalItems == 0 || inReservedZone) ? -1 : int16_t(menuItemFromTouchY(t.y));
    if (hoverIndex >= int16_t(totalItems))
      hoverIndex = -1;
    if (hoverIndex != menuTouchHoverIndex)
    {
      menuTouchHoverIndex = hoverIndex;
      menuDragScrollActive = true;
      showMenu();
      menuDragScrollActive = false;
    }
  }
  else if (liveData->menuVisible && menuTouchHoverIndex != -1 && !touchPressed)
  {
    menuTouchHoverIndex = -1;
    menuDragScrollActive = true;
    showMenu();
    menuDragScrollActive = false;
  }

  const bool touchMoveGesture =
      t.wasDragStart() || t.isDragging() || t.wasDragged() ||
      t.wasFlickStart() || t.isFlicking() || t.wasFlicked();

  if (t.wasClicked())
  {
    if (touchSwipeGestureActive)
    {
      // Release after horizontal swipe triggers screen switch handling.
      touchDragged = false;
      touchDragDeltaY = 0;
      touchDragDeltaYPrev = 0;
      touchPressed = true;
    }
    else if (touchDragGestureActive)
    {
      // Consume release after drag; do not turn it into item click.
      touchDragGestureActive = false;
      touchDragged = false;
      touchDragDeltaY = 0;
      touchDragDeltaYPrev = 0;
    }
    else
    {
      lastTouchX = t.x;
      lastTouchY = t.y;
      touchDragged = false;
      touchDragDeltaY = 0;
      touchDragDeltaYPrev = 0;
      touchPressed = true;
    }
  }
  else if (liveData->menuVisible && touchMoveGesture)
  {
    int16_t dragX = t.distanceX();
    int16_t dragY = t.distanceY();
    if (abs(dragY) >= MENU_DRAG_THRESHOLD_PX && abs(dragY) > abs(dragX))
    {
      lastTouchX = t.x;
      lastTouchY = t.y;
      touchDragged = true;
      touchDragGestureActive = true;
      touchDragDeltaY = dragY;
      touchPressed = true;
    }
  }
  else if (!liveData->menuVisible && touchMoveGesture)
  {
    int16_t dragX = t.distanceX();
    int16_t dragY = t.distanceY();
    bool horizontalSwipeDrag = (abs(dragX) > abs(dragY) &&
                                (touchSwipeGestureActive || abs(dragX) >= TOUCH_SWIPE_THRESHOLD_PX));
    if (horizontalSwipeDrag)
    {
      if (!touchSwipeGestureActive)
      {
        screenSwipePreviewActive = true;
      }
      touchSwipeGestureActive = true;
      if (abs(dragX) > abs(touchSwipeDeltaX))
      {
        touchSwipeDeltaX = dragX;
      }
      showScreenSwipePreview(dragX);
    }
  }
  const bool touchReleased = t.wasReleased();
  const bool touchIdle = (t.state == m5::touch_state_t::none);
  if (touchIdle || touchReleased)
  {
    screenSwipePreviewActive = false;
    touchDragGestureActive = false;
    touchDragDeltaYPrev = 0;
    if (touchSwipeGestureActive && !touchPressed && touchReleased)
    {
      touchPressed = true;
    }
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

  CoreS3.Imu.getGyroData(&gyroX, &gyroY, &gyroZ);
  CoreS3.Imu.getAccelData(&accX, &accY, &accZ);

  if (gyroX != 0.0 || gyroY != 0.0 || gyroZ != 0.0 || accX != 0.0 || accY != 0.0 || accZ != 0.0)
  {
    liveData->params.gyroSensorMotion = false;
    if (abs(gyroX) > 15.0 || abs(gyroY) > 15.0 || abs(gyroZ) > 15.0)
    {
      liveData->params.gyroSensorMotion = true;
    }
  }
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
