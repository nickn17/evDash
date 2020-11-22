////////////////////////////////////////////////////////////
// SELECT HARDWARE !!!!
////////////////////////////////////////////////////////////

#define BOARD_TTGO_T4
//#define BOARD_M5STACK_CORE

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

#define USER_SETUP_LOADED 1
#define SPI_FREQUENCY  27000000
#define SPI_TOUCH_FREQUENCY  2500000

////////////////////////////////////////////////////////////
// BOARD LILYGO TTGO T4 v1.3
/////////////////////////////////////////////////////////////
#ifdef BOARD_TTGO_T4
#define ILI9341_DRIVER
#define TFT_MISO 12
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   27
#define TFT_DC   26
#define TFT_RST   5
#define TFT_BL 4
#define USE_HSPI_PORT
#define SPI_READ_FREQUENCY  6000000 // 6 MHz is the maximum SPI read speed for the ST7789V
#define BUTTON_LEFT 38
#define BUTTON_MIDDLE 37
#define BUTTON_RIGHT 39
#endif // BOARD_TTGO_T4

/////////////////////////////////////////////////////////////
// BOARD M5STACK CORE IOT (M5-K001)
/////////////////////////////////////////////////////////////
#ifdef BOARD_M5STACK_CORE
#define USER_SETUP_LOADED 1
#define ILI9341_DRIVER
#define M5STACK
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   14  // Chip select control pin
#define TFT_DC   27  // Data Command control pin
#define TFT_RST  33  // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL   32  // LED back-light
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  5000000
#define BUTTON_LEFT 37
#define BUTTON_MIDDLE 38
#define BUTTON_RIGHT 39
#define SPEAKER 25
#define INVERT_DISPLAY
#endif // BOARD_M5STACK_CORE

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

// TFT COMMON
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
#define SMOOTH_FONT
#define GFXFF 1  // TFT FOnts

// TFT COLORS FOR TTGO
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xD69A      /* 211, 211, 211 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFE19      /* 255, 192, 203 */ //Lighter pink, was 0xFC9F      
#define TFT_BROWN       0x9A60      /* 150,  75,   0 */
#define TFT_GOLD        0xFEA0      /* 255, 215,   0 */
#define TFT_SILVER      0xC618      /* 192, 192, 192 */
#define TFT_SKYBLUE     0x867D      /* 135, 206, 235 */
#define TFT_VIOLET      0x915C      /* 180,  46, 226 */
#define TFT_DEFAULT_BK     0x0000    // 0x38E0
#define TFT_TEMP        0x0000    // NAVY
#define TFT_GREENYELLOW 0xB7E0
#define TFT_DARKRED     0x3800      /* 128,   0,   0 */
#define TFT_DARKGREEN2  0x01E0      /* 128,   0,   0 */
