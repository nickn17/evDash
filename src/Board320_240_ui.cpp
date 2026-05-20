/**
 * Board 320x240 UI primitives
 *
 * @author     petrovic@next176.sk
 * @copyright  2004-2026, NEXT176 s.r.o.
 * @refactor   extracted from Board320_240.cpp
 */
#include "Board320_240.h"
#include "LogSerial.h"

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
