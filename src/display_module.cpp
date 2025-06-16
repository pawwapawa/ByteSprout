/**
 * @file display_module.cpp
 * @brief Implementation of OLED display functionality
 *
 * This module provides functions for initializing and controlling the
 * SSD1351 OLED display, including rendering text, images, and controlling
 * display parameters like brightness.
 */
#include "display_module.h"
#include "emotes_module.h"
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <soc/soc.h>

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================
/** @brief OLED display instance using the SSD1351 driver */
Adafruit_SSD1351 oled = Adafruit_SSD1351(DISPLAY_WIDTH, DISPLAY_HEIGHT, &SPI,
                                         CS_PIN_D7, DC_PIN_D6, RST_PIN_D0);

// DOS Animation global variables - simplified tracking
static int16_t dos_x = 0; // Current X position for DOS text
static int16_t dos_y = 0; // Current Y position for DOS text (baseline)

//==============================================================================
// LOW-LEVEL DISPLAY FUNCTIONS
//==============================================================================
/**
 * @brief Get chip model name as string
 * @return String containing the chip model
 */
String getChipModel() { return "ESP32-S3"; }

/**
 * @brief Get CPU frequency in MHz
 * @return CPU frequency as integer
 */
uint32_t getCpuFrequencyMHz() { return getCpuFrequencyMhz(); }

/**
 * @brief Get flash size in MB
 * @return Flash size in MB
 */
uint32_t getFlashSizeMB() {
  uint32_t flash_size;
  if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
    return flash_size / (1024 * 1024);
  }
  return 0;
}

/**
 * @brief Get PSRAM size in MB
 * @return PSRAM size in MB, 0 if no PSRAM
 */
uint32_t getPSRAMSizeMB() {
  size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  if (psram_size > 0) {
    return psram_size / (1024 * 1024);
  }
  return 0;
}

/**
 * @brief Get display controller info
 * @return String with display info
 */
String getDisplayInfo() {
  // You could enhance this further by reading the display ID register
  // For now, return the known controller
  return "SSD1351";
}

/**
 * @brief Draw a bitmap image on the display
 *
 * @param x X-coordinate for the top-left corner
 * @param y Y-coordinate for the top-left corner
 * @param bitmap Pointer to the bitmap image data in RGB565 format
 * @param w Width of the bitmap in pixels
 * @param h Height of the bitmap in pixels
 */
void drawBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w,
                int16_t h) {
  oled.drawRGBBitmap(x, y, bitmap, w, h);
}

/**
 * @brief Set the active display window for drawing operations
 *
 * @param x X-coordinate for the top-left corner
 * @param y Y-coordinate for the top-left corner
 * @param w Width of the window in pixels
 * @param h Height of the window in pixels
 */
void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  oled.setAddrWindow(x, y, w, h);
}

/**
 * @brief Begin a batch write operation to the display
 */
void startWrite() { oled.startWrite(); }

/**
 * @brief End a batch write operation to the display
 */
void endWrite() { oled.endWrite(); }

/**
 * @brief Write a batch of pixels to the display
 *
 * @param pixels Pointer to pixel data in RGB565 format
 * @param len Number of pixels to write
 */
void writePixels(uint16_t *pixels, uint32_t len) {
  oled.writePixels(pixels, len);
}

//==============================================================================
// HIGH-LEVEL DISPLAY FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the OLED display
 *
 * Sets up the SPI interface, initializes the display, and configures
 * default display settings.
 *
 * @return true if initialization was successful, false otherwise
 */
bool initializeOLED() {
  SPI.begin(SCLK_PIN_D8, -1, MOSI_PIN_D10, CS_PIN_D7);
  // Set SPI frequency to 20MHz before initializing display
  SPI.setFrequency(DISPLAY_FREQUENCY);
  // Initialize the OLED display
  oled.begin();
  // Additional check to ensure the display is working
  if (oled.width() != DISPLAY_WIDTH || oled.height() != DISPLAY_HEIGHT) {
    ESP_LOGE(DISPLAY_LOG, "ERROR: Display failed to initialize.");
    return false;
  }
  oled.fillScreen(COLOR_BLACK);
  oled.setTextColor(COLOR_WHITE);
  setDisplayBrightness(DISPLAY_BRIGHTNESS_FULL);
  return true;
}

/**
 * @brief Set the display brightness level
 *
 * @param contrastLevel Brightness level (0-15, with 15 being brightest)
 */
void setDisplayBrightness(uint8_t contrastLevel) {
  // Ensure contrastLevel is within 4-bit range (0x00 to 0x0F)
  if (contrastLevel > DISPLAY_BRIGHTNESS_FULL)
    contrastLevel = DISPLAY_BRIGHTNESS_FULL;
  // Use the sendCommand function to send the contrast command
  uint8_t data = contrastLevel;
  oled.sendCommand(SSD1351_CMD_CONTRASTMASTER, &data, 1);
}

/**
 * @brief Turn the display on or off
 *
 * @param displayOn true to turn display on, false to turn it off
 */
void toggleDisplay(bool displayOn) {
  if (displayOn) {
    oled.sendCommand(SSD1351_CMD_DISPLAYON);
  } else {
    oled.sendCommand(SSD1351_CMD_DISPLAYOFF);
  }
}

/**
 * @brief Display a message centered on the screen
 *
 * @param message The text message to display
 */
void displayBootMessage(const char *message) {
  int16_t x1, y1;
  uint16_t textWidth, textHeight;
  oled.setFont(&FreeSansBold9pt7b);
  oled.setTextSize(1);
  oled.getTextBounds(message, 0, 0, &x1, &y1, &textWidth, &textHeight);
  // Calculate the center of the screen
  int16_t centeredX = (DISPLAY_WIDTH - textWidth) / 2;
  int16_t centeredY = (DISPLAY_HEIGHT + textHeight) / 2;

  oled.setCursor(centeredX, centeredY);
  oled.println(message);
}

/**
 * @brief Clear the display by filling it with black
 */
void clearDisplay() {
  // No native way to clear display we just fill it with Black
  oled.fillScreen(COLOR_BLACK);
}

/**
 * @brief Display a static image centered on the screen
 *
 * @param imageData Pointer to the image data in RGB565 format
 * @param imageWidth Width of the image in pixels
 * @param imageHeight Height of the image in pixels
 */
void displayStaticImage(const uint16_t *imageData, uint16_t imageWidth,
                        uint16_t imageHeight) {
  // Image needs to be converted to RGB565 color format to support the RGB OLED
  // display Calculate the centered position
  int16_t x = (DISPLAY_WIDTH - imageWidth) / 2;
  int16_t y = (DISPLAY_HEIGHT - imageHeight) / 2;
  // Ensure we don't try to draw outside the display bounds
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  // Draw the image
  oled.drawRGBBitmap(x, y, imageData, imageWidth, imageHeight);
}

//==============================================================================
// DOS STARTUP ANIMATION FUNCTIONS (Simplified Implementation)
//==============================================================================

/**
 * @brief Type text with typewriter effect at current cursor position
 *
 * @param text String to type
 * @param delay_ms Delay between characters in milliseconds
 * @param color Text color (default: DOS yellow)
 */
void dosType(const char *text, int delay_ms = TYPE_DELAY_NORMAL,
             uint16_t color = DOS_YELLOW) {
  oled.setTextColor(color);
  oled.setCursor(dos_x, dos_y);

  while (*text) {
    oled.print(*text);
    text++;
    dos_x += 6; // Default font character width is 6 pixels

    // Handle line wrapping if text goes beyond screen width
    if (dos_x >= DISPLAY_WIDTH - 6) {
      dos_x = 0;
      dos_y += 10; // Move to next line (10 pixel line height)
    }

    delay(delay_ms);
  }
}

/**
 * @brief Move cursor to the beginning of next line
 */
void dosNewLine() {
  dos_x = 0;
  dos_y += 10; // Move down one line (10 pixel spacing)
}

/**
 * @brief Display blinking cursor block at current position
 *
 * @param blinks Number of blinks to show (default: 3)
 */
void dosBlinkCursor(int blinks = CURSOR_BLINK_COUNT) {
  for (int i = 0; i < blinks; i++) {
    // Draw cursor block aligned with text baseline
    oled.fillRect(dos_x, dos_y, 6, 8, DOS_YELLOW);
    delay(CURSOR_BLINK_MS);

    // Clear cursor block
    oled.fillRect(dos_x, dos_y, 6, 8, DOS_BLACK);
    delay(CURSOR_BLINK_MS);
  }
}

/**
 * @brief Run the complete DOS startup animation
 *
 * This is the main public function that displays a retro DOS-style boot
 * sequence with typewriter effect, blinking cursor, and authentic
 * command-line messages leading up to "ALXV LABS" splash screen.
 */
void displayDOSStartupAnimation() {
  // Initialize display for DOS animation
  oled.fillScreen(DOS_BLACK);
  oled.setFont(); // Use default 5x7 font for authentic DOS look
  oled.setTextSize(1);
  dos_x = 0;
  dos_y = 8; // Start at first line baseline

  // Get hardware information
  String chipModel = getChipModel();
  uint32_t cpuFreq = getCpuFrequencyMHz();
  uint32_t flashSize = getFlashSizeMB();
  uint32_t psramSize = getPSRAMSizeMB();
  String displayModel = getDisplayInfo();

  // Boot sequence messages with actual hardware info
  dosType("ALXV LABS BIOS v1.0", TYPE_DELAY_FAST, DOS_WHITE);
  dosNewLine();
  delay(LINE_DELAY);
  dosBlinkCursor(2);

  dosType("Detecting Hardware...", TYPE_DELAY_NORMAL);
  dosNewLine();
  delay(PAUSE_SHORT);

  // Display info with actual controller
  String displayLine = "Display:" + displayModel + " [OK]";
  dosType(displayLine.c_str(), TYPE_DELAY_FAST);
  dosNewLine();

  // MCU info with actual chip model and revision
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  String mcuLine = "MCU:" + chipModel;
  mcuLine += "R" + String(flashSize);
  mcuLine += " [OK]";
  dosType(mcuLine.c_str(), TYPE_DELAY_FAST);
  dosNewLine();

  // CPU frequency (actual)
  String cpuLine = "CPU:" + String(cpuFreq) + "MHz [OK]";
  dosType(cpuLine.c_str(), TYPE_DELAY_FAST);
  dosNewLine();

  // Memory info with actual sizes
  String memoryLine;
  if (psramSize > 0) {
    memoryLine = "PSRAM:" + String(psramSize) + "MB [OK]";
  } else {
    memoryLine = "PSRAM: None [--]";
  }
  dosType(memoryLine.c_str(), TYPE_DELAY_NORMAL);
  dosNewLine();

  // Additional hardware info
  // String coresLine = "Cores:" + String(chip_info.cores) + " [OK]";
  // dosType(coresLine.c_str(), TYPE_DELAY_FAST);
  // dosNewLine();

  dosType("///////////////////", TYPE_DELAY_SLOW);
  dosNewLine();
  delay(PAUSE_LONG);

  String osVersion = "BYTE-90 OS v" + String(FIRMWARE_VERSION);
  dosType(osVersion.c_str(), TYPE_DELAY_NORMAL, DOS_AMBER);
  dosNewLine();
  delay(LINE_DELAY);

  dosType("C:\\> ", TYPE_DELAY_NORMAL, DOS_WHITE);
  dosType("run BYTE90.exe", TYPE_DELAY_NORMAL);
  dosNewLine();
  delay(PAUSE_SHORT);
  dosBlinkCursor();

  // Final splash screen with large font
  oled.fillScreen(DOS_BLACK);
  delay(200);

  // Display the startup static image (128x128 pixels, centered)
  displayStaticImage(STARTUP_STATIC, 128, 128);

  // Brief pause to show the final image
  delay(800);
}