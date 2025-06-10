/**
 * @file menu_module.h
 * @brief Header for single-button hierarchical menu system functionality
 * 
 * This module provides a complete menu navigation system using a single push button
 * with different press patterns (single click, double click, long press). Features
 * include effect selection, glitch control, ESP-NOW toggle, update mode, and deep 
 * sleep functionality with automatic timeout and visual display support.
 * 
 * This module replaces the button_module and integrates with the existing system
 * architecture including system_module, effects_module, motion_module, and display_module.
 * 
 * Navigation Logic:
 * - Normal Operation: Any button press enters menu
 * - Single Click: Navigate/cycle through options
 * - Double Click: Enter menu or apply selection
 * - Hold 3s: Deep sleep from anywhere
 * - Auto-timeout: Return to normal operation after 30s inactivity
 * 
 * Key Features:
 * - Dynamic effect counting from effects_module
 * - Independent glitch effect control
 * - Clean integration with effects_module functions
 * - Callback system for custom integration
 * - Automatic menu sizing and display
 */

#ifndef MENU_MODULE_H
#define MENU_MODULE_H

#include "common.h"
#include <Adafruit_GFX.h>

// Forward declarations for system integration
class Adafruit_SSD1351;

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

/** @brief Log tag for menu module messages */
static const char* MENU_LOG = "::MENU_MODULE::";

//------------------------------------------------------------------------------
// Pin and Timing Definitions
//------------------------------------------------------------------------------
/** @brief Button input pin (A3 on Seeedstudio XIAO board) */
#define MENU_BUTTON_PIN A3
/** @brief Time threshold in milliseconds to detect a long press */
#define MENU_LONG_PRESS_TIME 3000
/** @brief Maximum time in milliseconds between clicks to detect a double click */
#define MENU_DOUBLE_CLICK_TIME 300
/** @brief Debounce time in milliseconds to filter button noise */
#define MENU_DEBOUNCE_TIME 50
/** @brief Menu auto-timeout in milliseconds (30 seconds) */
#define MENU_TIMEOUT 30000

//------------------------------------------------------------------------------
// Display Layout Definitions
//------------------------------------------------------------------------------
/** @brief Text size for menu items */
#define MENU_TEXT_SIZE 1
/** @brief Vertical offset between menu items in pixels */
#define MENU_ITEM_Y_OFFSET 3
/** @brief Horizontal offset for menu item text in pixels */
#define MENU_ITEM_X_OFFSET 3
/** @brief Padding around menu content in pixels */
#define MENU_PADDING 6

//------------------------------------------------------------------------------
// Menu Header Labels
//------------------------------------------------------------------------------
/** @brief Device name label */
#define MENU_LABEL_BYTE_90 "BYTE-90"
/** @brief Main menu header text */
#define MENU_LABEL_MAIN_MENU "SETTINGS"
/** @brief Effects submenu header text */
#define MENU_LABEL_EFFECTS "RETRO EFFECTS"
/** @brief Glitch submenu header text */
#define MENU_LABEL_GLITCH "RETRO GLITCH"
/** @brief ESP-NOW submenu header text */
#define MENU_LABEL_ESP_NOW "BYTE-90 PAIRING"
/** @brief Update mode submenu header text */
#define MENU_LABEL_UPDATE "UPDATE MODE"
/** @brief Go back navigation text */
#define MENU_LABEL_GO_BACK "GO BACK"
/** @brief Exit menu option label */
#define MENU_LABEL_EXIT "EXIT"

//------------------------------------------------------------------------------
// Effect Display Labels
//------------------------------------------------------------------------------
/** @brief No effects label */
#define EFFECT_LABEL_NONE "NONE"
/** @brief Scanlines effect label */
#define EFFECT_LABEL_SCANLINES "SCANLINES"
/** @brief Dithering effect label */
#define EFFECT_LABEL_DITHER "DITHERING"
/** @brief Green tint effect label */
#define EFFECT_LABEL_GREEN_TINT "RETRO GREEN"
/** @brief Yellow tint effect label */
#define EFFECT_LABEL_YELLOW_TINT "CLASSIC YELLOW"
/** @brief Dithering with green tint effect label */
#define EFFECT_LABEL_DITHER_GREEN "GREEN DITHER"
/** @brief Dithering with yellow tint effect label */
#define EFFECT_LABEL_DITHER_YELLOW "YELLOW DITHER"
/** @brief Unknown effect fallback label */
#define EFFECT_LABEL_UNKNOWN "UNKNOWN"

//------------------------------------------------------------------------------
// UI Labels
//------------------------------------------------------------------------------
/** @brief Feature toggle label enable */
#define LABEL_ENABLE "ENABLE"
/** @brief Feature toggle label disable */
#define LABEL_DISABLE "DISABLE"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Menu navigation states
 */
enum MenuState {
  NORMAL_OPERATION,     /**< Running current effect, no menu visible */
  MENU_SELECTION,       /**< Top level - selecting which menu to enter */
  EFFECTS_MENU,         /**< Inside effects selection submenu */
  GLITCH_MENU,          /**< Inside glitch control submenu */
  ESP_NOW_MENU,         /**< Inside ESP-NOW toggle submenu */
  UPDATE_MENU           /**< Inside update mode submenu */
};

/**
 * @brief Top-level menu options
 */
enum TopLevelMenu {
  EFFECTS_OPTION,       /**< Effects selection menu option */
  GLITCH_OPTION,        /**< Glitch control menu option */
  ESP_NOW_OPTION,       /**< ESP-NOW toggle menu option */
  UPDATE_OPTION,        /**< Update mode menu option */
  EXIT_OPTION           /**< Exit to normal operation option */
};

/**
 * @brief Available effect types (mapped to effects_module states)
 */
enum EffectType {
  EFFECT_NONE = 0,      /**< No effects applied (EFFECT_STATE_NONE) */
  EFFECT_SCANLINES,     /**< CRT scanlines only (EFFECT_STATE_SCANLINE) */
  EFFECT_DITHER,        /**< Bayer dithering only (EFFECT_STATE_DITHER) */
  EFFECT_GREEN_TINT,    /**< Green terminal tint with scanlines (EFFECT_STATE_GREEN_TINT) */
  EFFECT_YELLOW_TINT,   /**< Yellow terminal tint with scanlines (EFFECT_STATE_YELLOW_TINT) */
  EFFECT_DITHER_GREEN,  /**< Dithering combined with green tint (EFFECT_STATE_DITHER_GREEN) */
  EFFECT_DITHER_YELLOW  /**< Dithering combined with yellow tint (EFFECT_STATE_DITHER_YELLOW) */
};

/**
 * @brief Button state machine states
 */
enum class ButtonState {
  IDLE,                 /**< Button is not pressed */
  PRESSED,              /**< Button is currently pressed down */
  RELEASED,             /**< Button was just released */
  POTENTIAL_DOUBLE      /**< Button was released but may be part of a double click */
};

/**
 * @brief Types of button events that can be detected
 */
enum class ButtonEvent {
  NONE,                 /**< No button event detected */
  CLICK,                /**< Single click detected */
  DOUBLE_CLICK,         /**< Double click detected */
  LONG_PRESS            /**< Long press detected */
};

//==============================================================================
// CALLBACK FUNCTION TYPES
//==============================================================================

/**
 * @brief Callback function type for effect changes
 * @param newEffect The newly selected effect type
 */
typedef void (*EffectChangeCallback)(EffectType newEffect);

/**
 * @brief Callback function type for glitch toggle
 * @param enabled True if glitch effects should be enabled, false if disabled
 */
typedef void (*GlitchToggleCallback)(bool enabled);

/**
 * @brief Callback function type for ESP-NOW toggle
 * @param enabled True if ESP-NOW should be enabled, false if disabled
 */
typedef void (*ESPNowToggleCallback)(bool enabled);

/**
 * @brief Callback function type for update mode toggle
 * @param enabled True if update mode should be enabled, false if disabled
 */
typedef void (*UpdateModeToggleCallback)(bool enabled);

/**
 * @brief Callback function type for deep sleep request
 */
typedef void (*DeepSleepCallback)();

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the menu system with display integration
 * 
 * Sets up button pin, interrupt handling, and synchronizes with current
 * effects_module state. Replaces initializeButton() from button_module
 * with enhanced menu functionality and display integration.
 */
void menu_init();

/**
 * @brief Update menu system - call this in main loop
 * 
 * Handles button debouncing, click detection, menu timeouts, and state transitions.
 * Replaces handleClickEvents() from button_module with comprehensive menu
 * navigation and timing management.
 */
void menu_update();

/**
 * @brief Reset all menu and button states
 * 
 * Forces reset of all menu states, button events, and timing variables.
 * Replaces resetButtonStates() from button_module with additional
 * menu state cleanup functionality.
 */
void menu_resetStates();

/**
 * @brief Get current menu state
 * 
 * Returns the current position in the menu navigation hierarchy
 * for state monitoring and integration purposes.
 * 
 * @return Current MenuState value
 */
MenuState menu_getCurrentState();

/**
 * @brief Get currently active effect
 * 
 * Returns the effect type currently applied to the display system
 * for status monitoring and persistence.
 * 
 * @return Current EffectType value
 */
EffectType menu_getCurrentEffect();

/**
 * @brief Check if menu is currently active
 * 
 * Determines whether the menu system is currently displayed and
 * accepting input, or if the system is in normal operation mode.
 * 
 * @return True if menu is active, false if in normal operation
 */
bool menu_isActive();

/**
 * @brief Set current effect programmatically
 * 
 * Directly sets the active effect using improved effects_module integration
 * for direct state setting without user navigation.
 * 
 * @param effect Effect type to set as current
 */
void menu_setCurrentEffect(EffectType effect);

/**
 * @brief Register callback for effect changes
 * 
 * Sets up callback function to be called whenever the user selects
 * a new effect through the menu system.
 * 
 * @param callback Function to call when effect changes
 */
void menu_setEffectChangeCallback(EffectChangeCallback callback);

/**
 * @brief Register callback for glitch toggle
 * 
 * Sets up callback function to be called whenever the user toggles
 * glitch effects on or off through the menu system.
 * 
 * @param callback Function to call when glitch effects are toggled
 */
void menu_setGlitchToggleCallback(GlitchToggleCallback callback);

/**
 * @brief Register callback for ESP-NOW toggle
 * 
 * Sets up callback function to be called whenever the user toggles
 * ESP-NOW communication on or off through the menu system.
 * 
 * @param callback Function to call when ESP-NOW is toggled
 */
void menu_setESPNowToggleCallback(ESPNowToggleCallback callback);

/**
 * @brief Register callback for update mode toggle
 * 
 * Sets up callback function to be called whenever the user toggles
 * update mode on or off through the menu system.
 * 
 * @param callback Function to call when update mode is toggled
 */
void menu_setUpdateModeToggleCallback(UpdateModeToggleCallback callback);

/**
 * @brief Register callback for deep sleep request
 * 
 * Sets up callback function to be called when the user requests
 * deep sleep through long press detection.
 * 
 * @param callback Function to call before entering deep sleep
 */
void menu_setDeepSleepCallback(DeepSleepCallback callback);

/**
 * @brief Update display with current menu state
 * 
 * Renders the current menu interface using Adafruit_SSD1351 display
 * integration. Automatically adapts to available effects from effects_module
 * and handles dynamic menu sizing and scrolling.
 */
void menu_updateDisplay();

/**
 * @brief Print current state to ESP_LOG for debugging
 * 
 * Outputs comprehensive state information including current menu position,
 * active effects, system status, and all relevant state variables for
 * debugging and monitoring purposes.
 */
void menu_printCurrentState();

/**
 * @brief Get human-readable effect name
 * 
 * Retrieves consistent effect naming from effects_module for display
 * purposes and debugging output.
 * 
 * @param effect Effect type to get name for
 * @return String containing effect name
 */
String menu_getEffectName(EffectType effect);

/**
 * @brief Get human-readable top menu name
 * 
 * Converts top-level menu enumeration values to display-ready
 * text strings for menu rendering.
 * 
 * @param menu Top-level menu option to get name for
 * @return String containing menu name
 */
String menu_getTopMenuName(TopLevelMenu menu);

/**
 * @brief Get human-readable menu state name
 * 
 * Converts menu state enumeration values to descriptive text
 * for debugging and status display purposes.
 * 
 * @param state Menu state to get name for
 * @return String containing state name
 */
String menu_getMenuStateName(MenuState state);

/**
 * @brief Apply selected effect using improved effects_module integration
 * 
 * Efficiently applies the specified effect using setEffectStateDirect()
 * for immediate state changes without cycling through other effects.
 * 
 * @param effect Effect type to apply
 */
void menu_applyEffect(EffectType effect);

/**
 * @brief Get current glitch status from effects_module
 * 
 * Queries the effects_module for current glitch effect enable state
 * for menu display and status purposes.
 * 
 * @return True if glitch effects are currently enabled
 */
bool menu_getGlitchStatus();

/**
 * @brief Get current ESP-NOW status from espnow_module
 * 
 * Queries the espnow_module for current communication enable state
 * for menu display and status purposes.
 * 
 * @return True if ESP-NOW is currently enabled
 */
bool menu_getESPNowStatus();

/**
 * @brief Get current update mode status from system_module
 * 
 * Queries the system_module for current update mode state
 * for menu display and status purposes.
 * 
 * @return True if system is currently in update mode
 */
bool menu_getUpdateModeStatus();

#endif // MENU_MODULE_H