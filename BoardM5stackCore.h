
// Setup for m5stack core
#define USER_SETUP_LOADED 1
#define SPI_FREQUENCY  27000000
#define SPI_TOUCH_FREQUENCY  2500000

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
#define SPEAKER_PIN 25
#define INVERT_DISPLAY

#define BUTTON_LEFT 37
#define BUTTON_MIDDLE 38
#define BUTTON_RIGHT 39

#define SD_CS    4
#define SD_MOSI  23
#define SD_MISO  19
#define SD_SCLK  18

#endif // BOARD_M5STACK_CORE
