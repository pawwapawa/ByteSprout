/**
 * @file display_module.h
 * @brief Header for OLED display functionality
 * 
 * This module provides functions for initializing and controlling the 
 * SSD1351 OLED display, including rendering text, images, and controlling
 * display parameters like brightness.
 */

#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <Fonts/FreeSansBold9pt7b.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================
/** @brief External reference to OLED display object */
extern Adafruit_SSD1351 oled;
// Log tag
static const char *DISPLAY_LOG = "::DISPLAY_MODULE::";
// Display is using a Waveshare 1.5" RGB OLED display
// https://www.waveshare.com/wiki/1.5inch_RGB_OLED_Module
// DISPLAY PINs CORRESPONDENCE to ESP32
// GND -> GND (0V) // Common
// VCC -> 5V or 3.3V // Better to power with 5V if display PCB supports it
// DIN -> MOSI // SPI data
// SCK -> SCLK // SPI clock
// DC -> DC // Distinguish between a command or its data
// RST -> RST // Hardware reset, can connect to MCU RST pin as well
// CS -> CS // Chip select, Set to -1 if for manually use with multiple displays
/////////////////////////////////////////////////////////////////////////////////
// Display brightness control is done by adjusting the current output
// The display has a master contrast current control register that can be set to
// 16 levels The display brightness can be adjusted by reducing the output
// current The display brightness can be set to 16 levels from 6.25% to 100% of
// maximum brightness
/*
 * Master Contrast Current Control Values (Command 0xC7)
 * Adjusts the brightness of the display by reducing the output current.
 *
 * Hex Value (A[3:0]) | Reduction Factor | Brightness Level (% of Maximum)
 * ------------------------------------------------------------
 * 0x00               | 1/16            | 6.25%
 * 0x01               | 2/16            | 12.5%
 * 0x02               | 3/16            | 18.75%
 * 0x03               | 4/16            | 25%
 * 0x04               | 5/16            | 31.25%
 * 0x05               | 6/16            | 37.5%
 * 0x06               | 7/16            | 43.75%
 * 0x07               | 8/16            | 50%
 * 0x08               | 9/16            | 56.25%
 * 0x09               | 10/16           | 62.5%
 * 0x0A               | 11/16           | 68.75%
 * 0x0B               | 12/16           | 75%
 * 0x0C               | 13/16           | 81.25%
 * 0x0D               | 14/16           | 87.5%
 * 0x0E               | 15/16           | 93.75%
 * 0x0F               | 16/16 (no reduction) | 100% (Maximum Brightness)
 *
 * To set the brightness, use the sendCommand() function with:
 * Command: 0xC7
 * Data: A single byte representing the desired brightness level (0x00 to 0x0F)
 */

//------------------------------------------------------------------------------
// Pin Configuration
//------------------------------------------------------------------------------
// Pin configuration for SEEED XIAO ESP32-S3
#define MOSI_PIN_D10 D10 // GPIO 9
#define SCLK_PIN_D8 D8   // GPIO 7
#define CS_PIN_D7 D7     // GPIO 44
#define DC_PIN_D6 D6     // GPIO 43
#define RST_PIN_D0 D0    // GPIO 1

//------------------------------------------------------------------------------
// Display Parameters
//------------------------------------------------------------------------------
// Predefined brightness levels
#define DISPLAY_BRIGHTNESS_DIM 0x00
#define DISPLAY_BRIGHTNESS_LOW 0x02
#define DISPLAY_BRIGHTNESS_MEDIUM 0x05
#define DISPLAY_BRIGHTNESS_HIGH 0x07
#define DISPLAY_BRIGHTNESS_FULL 0x0F
// Display Frequency 20MHz
#define DISPLAY_FREQUENCY 20000000
// Display dimensions
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128
// static icon sizes
#define ERROR_ICON_SIZE 128
// Color definitions
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xffff
#define COLOR_YELLOW 0xFFE0

//------------------------------------------------------------------------------
// DOS Animation Constants
//------------------------------------------------------------------------------
// DOS Colors
#define DOS_YELLOW 0xFFE0     // Yellow terminal color
#define DOS_AMBER 0xFBE0      // Amber terminal color
#define DOS_WHITE 0xFFFF      // White text
#define DOS_BLACK 0x0000      // Black background

// Animation timing
#define TYPE_DELAY_FAST 25    // Fast typing delay (ms)
#define TYPE_DELAY_NORMAL 40  // Normal typing delay (ms)
#define TYPE_DELAY_SLOW 60    // Slow typing delay (ms)
#define LINE_DELAY 150        // Delay between lines (ms)
#define PAUSE_SHORT 300       // Short pause (ms)
#define PAUSE_LONG 500        // Long pause (ms)
#define CURSOR_BLINK_MS 400   // Cursor blink interval (ms)
#define CURSOR_BLINK_COUNT 3  // Number of cursor blinks before typing

//==============================================================================
// LOW-LEVEL DISPLAY FUNCTIONS
//==============================================================================

/**
 * @brief Draw a bitmap image on the display
 * 
 * @param x X-coordinate for the top-left corner
 * @param y Y-coordinate for the top-left corner
 * @param bitmap Pointer to the bitmap image data in RGB565 format
 * @param w Width of the bitmap in pixels
 * @param h Height of the bitmap in pixels
 */
void drawBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);

/**
 * @brief Set the active display window for drawing operations
 * 
 * @param x X-coordinate for the top-left corner
 * @param y Y-coordinate for the top-left corner
 * @param w Width of the window in pixels
 * @param h Height of the window in pixels
 */
void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/**
 * @brief Begin a batch write operation to the display
 */
void startWrite();

/**
 * @brief End a batch write operation to the display
 */
void endWrite();

/**
 * @brief Write a batch of pixels to the display
 * 
 * @param pixels Pointer to pixel data in RGB565 format
 * @param len Number of pixels to write
 */
void writePixels(uint16_t *pixels, uint32_t len);

//==============================================================================
// HIGH-LEVEL DISPLAY FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the OLED display
 * 
 * @return true if initialization was successful, false otherwise
 */
bool initializeOLED();

/**
 * @brief Display a message centered on the screen
 * 
 * @param message The text message to display
 */
void displayBootMessage(const char *message);

/**
 * @brief Run the complete DOS startup animation
 * 
 * Displays a retro DOS-style boot sequence with typewriter effect,
 * blinking cursor, and authentic command-line messages leading up
 * to "ALXV LABS" splash screen.
 */
void displayDOSStartupAnimation();

/**
 * @brief Display a static image centered on the screen
 * 
 * @param imageData Pointer to the image data in RGB565 format
 * @param imageWidth Width of the image in pixels
 * @param imageHeight Height of the image in pixels
 */
void displayStaticImage(const uint16_t *imageData, uint16_t imageWidth, uint16_t imageHeight);

/**
 * @brief Set the display brightness level
 * 
 * @param contrastLevel Brightness level (0-15, with 15 being brightest)
 */
void setDisplayBrightness(uint8_t contrastLevel);

/**
 * @brief Turn the display on or off
 * 
 * @param displayOn true to turn display on, false to turn it off
 */
void toggleDisplay(bool displayOn);

/**
 * @brief Clear the display by filling it with black
 */
void clearDisplay(void);

#endif /* DISPLAY_MODULE_H */