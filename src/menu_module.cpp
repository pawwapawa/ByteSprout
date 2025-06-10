/**
 * @file menu_module.cpp
 * @brief Implementation of single-button hierarchical menu system functionality
 * 
 * This module implements a complete menu navigation system using a single push
 * button the system uses different button press patterns to navigate through menus and integrates with
 * existing system modules.
 * 
 * Integration with existing system:
 * - effects_module: For visual effect control and cycling
 * - system_module: For mode transitions
 * - espnow_module: For communication control
 * - motion_module: For deep sleep functionality
 * - display_module: For menu UI rendering
 */

#include "menu_module.h"
#include "display_module.h"
#include "effects_module.h"
#include "espnow_module.h"
#include "gif_module.h"
#include "motion_module.h"
#include "system_module.h"

// External display object from display_module
extern Adafruit_SSD1351 oled;

//==============================================================================
// MODULE STATE VARIABLES
//==============================================================================

//------------------------------------------------------------------------------
// Menu Navigation State
//------------------------------------------------------------------------------
static MenuState currentMenuState = NORMAL_OPERATION;      /**< Current position in menu hierarchy */
static TopLevelMenu selectedTopMenu = EFFECTS_OPTION;      /**< Currently selected top-level menu option */
static EffectType currentEffect = EFFECT_NONE;             /**< Currently active effect */
static EffectType selectedEffect = EFFECT_NONE;            /**< Currently selected effect in menu */
static unsigned long lastMenuActivity = 0;                 /**< Last time menu was interacted with for timeout */

//------------------------------------------------------------------------------
// Button State Management
//------------------------------------------------------------------------------
static volatile ButtonState buttonState = ButtonState::IDLE;       /**< Current button state machine position */
static volatile ButtonEvent buttonEvent = ButtonEvent::NONE;       /**< Detected button event type */
static volatile unsigned long buttonPressStartTime = 0;            /**< Time when button press started */
static volatile unsigned long lastReleaseTime = 0;                 /**< Time when button was last released */
static volatile bool buttonEventReady = false;                     /**< Whether button event is ready for processing */
static volatile bool buttonHandled = true;                         /**< Whether current button event has been handled */
static volatile bool longPressHandled = false;                     /**< Whether long press has been processed */

//------------------------------------------------------------------------------
// Button Debouncing
//------------------------------------------------------------------------------
static volatile unsigned long lastDebounceTime = 0;        /**< Last debounce timestamp */
static volatile bool lastButtonState = HIGH;               /**< Previous stable button state */

//------------------------------------------------------------------------------
// Callback Function Pointers
//------------------------------------------------------------------------------
static EffectChangeCallback onEffectChange = nullptr;              /**< Callback for effect changes */
static GlitchToggleCallback onGlitchToggle = nullptr;              /**< Callback for glitch toggle */
static ESPNowToggleCallback onESPNowToggle = nullptr;              /**< Callback for ESP-NOW toggle */
static UpdateModeToggleCallback onUpdateModeToggle = nullptr;      /**< Callback for update mode toggle */
static DeepSleepCallback onEnterDeepSleep = nullptr;               /**< Callback for deep sleep request */

//------------------------------------------------------------------------------
// Submenu Selection State
//------------------------------------------------------------------------------
static int selectedGlitchItem = 0;         /**< Selected item in glitch submenu */
static int selectedESPNowItem = 0;         /**< Selected item in ESP-NOW submenu */
static int selectedUpdateItem = 0;         /**< Selected item in update mode submenu */

//==============================================================================
// INTERNAL FUNCTION DECLARATIONS
//==============================================================================

static void IRAM_ATTR handleButtonInterrupt();
static void handleSingleClick();
static void handleDoubleClick();
static void handleVeryLongPress();
static void handleMenuTimeout();
static void exitToNormalOperation();
static void processButtonEvents();
static void drawMenuHeader(const char *title);
static void drawMenuItem(int index, const char *text, bool selected);

//==============================================================================
// BUTTON INTERRUPT HANDLING
//==============================================================================

/**
 * @brief Interrupt service routine for button state changes
 * 
 * Handles button press and release detection with debouncing and state machine
 * management for single click, double click, and long press recognition.
 * Replaces handleButtonInterrupt() from button_module with enhanced functionality.
 */
static void IRAM_ATTR handleButtonInterrupt() {
  bool reading = digitalRead(MENU_BUTTON_PIN);
  unsigned long currentTime = millis();

  if ((currentTime - lastDebounceTime) <= MENU_DEBOUNCE_TIME)
    return;

  // If the button state has changed and is stable
  if (reading != lastButtonState) {
    lastDebounceTime = currentTime;
    lastButtonState = reading;

    // Now process the stable button state
    switch (buttonState) {
    case ButtonState::IDLE:
      if (reading == LOW) { // Button pressed
        buttonState = ButtonState::PRESSED;
        buttonPressStartTime = currentTime;
        buttonHandled = false;
        longPressHandled = false;
      }
      break;

    case ButtonState::PRESSED:
      if (reading == HIGH) { // Button released
        lastReleaseTime = currentTime;

        // If this could be part of a double click (first press was short)
        if ((currentTime - buttonPressStartTime) < MENU_LONG_PRESS_TIME &&
            !longPressHandled) {
          buttonState = ButtonState::POTENTIAL_DOUBLE;
        } else {
          // Press was too long or already handled as long press
          buttonState = ButtonState::RELEASED;
          // Only register as a click if it wasn't already handled as a long press
          if (!longPressHandled) {
            buttonEvent = ButtonEvent::CLICK;
            buttonEventReady = true;
          }
        }
      }
      break;

    case ButtonState::POTENTIAL_DOUBLE:
      if (reading == LOW) { // Second press detected
        // If second press is within double click time window
        if ((currentTime - lastReleaseTime) <= MENU_DOUBLE_CLICK_TIME) {
          // Valid double click detected
          buttonState = ButtonState::PRESSED;
          buttonEvent = ButtonEvent::DOUBLE_CLICK;
          buttonEventReady = true;
          buttonHandled = false;
        } else {
          // Too much time elapsed between clicks, treat as new press
          buttonState = ButtonState::PRESSED;
          buttonPressStartTime = currentTime;
          buttonHandled = false;
          longPressHandled = false;
        }
      }
      break;

    case ButtonState::RELEASED:
      buttonState = ButtonState::IDLE;
      break;
    }
  }
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

void menu_init() {
  pinMode(MENU_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MENU_BUTTON_PIN), handleButtonInterrupt,
                  CHANGE);

  // Initialize effect state from effects_module using improved integration
  EffectCycleState currentState = getCurrentEffectState();
  currentEffect = static_cast<EffectType>(getEffectTypeFromState(currentState));
  selectedEffect = currentEffect;

  ESP_LOGI(MENU_LOG, "Menu system initialized on pin A3");
  menu_updateDisplay();
}

void menu_update() {
  unsigned long currentTime = millis();
  // Handle long press detection (replaces button_module logic)
  if (buttonState == ButtonState::PRESSED && !longPressHandled) {
    if ((currentTime - buttonPressStartTime) >= MENU_LONG_PRESS_TIME) {
      buttonEvent = ButtonEvent::LONG_PRESS;
      buttonEventReady = true;
      longPressHandled = true;
    }
  }

  // Handle double click timeout
  if (buttonState == ButtonState::POTENTIAL_DOUBLE) {
    if ((currentTime - lastReleaseTime) > MENU_DOUBLE_CLICK_TIME) {
      buttonEvent = ButtonEvent::CLICK;
      buttonEventReady = true;
      buttonState = ButtonState::IDLE;
    }
  }

  // Process button events
  processButtonEvents();

  // Reset event flags
  if (buttonEventReady && buttonHandled) {
    buttonEvent = ButtonEvent::NONE;
    buttonEventReady = false;
  }

  // Handle menu timeout
  handleMenuTimeout();
}

void menu_resetStates() {
  buttonState = ButtonState::IDLE;
  buttonEvent = ButtonEvent::NONE;
  buttonEventReady = false;
  buttonHandled = true;
  longPressHandled = false;
  lastDebounceTime = millis();

  // Reset menu state
  currentMenuState = NORMAL_OPERATION;
  selectedTopMenu = EFFECTS_OPTION;
}

//==============================================================================
// BUTTON EVENT PROCESSING
//==============================================================================

/**
 * @brief Process detected button events and execute appropriate actions
 * 
 * Dispatches button events to appropriate handlers based on event type.
 * Replaces processButtonEvents() from button_module with menu integration.
 */
static void processButtonEvents() {
  if (buttonEventReady && !buttonHandled) {
    switch (buttonEvent) {
    case ButtonEvent::CLICK:
      handleSingleClick();
      break;

    case ButtonEvent::DOUBLE_CLICK:
      handleDoubleClick();
      break;

    case ButtonEvent::LONG_PRESS:
      handleVeryLongPress();
      break;

    default:
      break;
    }

    buttonHandled = true;
  }
}

/**
 * @brief Handle single click actions based on current menu state
 * 
 * Processes single click events for menu navigation, cycling through
 * options within the current menu level.
 */
static void handleSingleClick() {
  lastMenuActivity = millis();

  switch (currentMenuState) {
  case NORMAL_OPERATION:
    stopGifPlayback();
    currentMenuState = MENU_SELECTION;
    selectedTopMenu = EFFECTS_OPTION;
    break;

  case MENU_SELECTION:
    // Cycle through top-level menu options including Exit
    selectedTopMenu = (TopLevelMenu)((selectedTopMenu + 1) % 5);
    break;

  case EFFECTS_MENU:
    {
      // Cycle through effects (using dynamic count from effects module) + GO BACK
      int totalEffects = getEffectCount();
      selectedEffect = (EffectType)((selectedEffect + 1) % (totalEffects + 1));
      break;
    }

  case GLITCH_MENU:
    // Cycle through: ENABLE/DISABLE -> GO BACK
    selectedGlitchItem = (selectedGlitchItem + 1) % 2;
    break;

  case ESP_NOW_MENU:
    // Cycle through: ENABLE/DISABLE -> GO BACK
    selectedESPNowItem = (selectedESPNowItem + 1) % 2;
    break;

  case UPDATE_MENU:
    // Cycle through: ENABLE/DISABLE -> GO BACK
    selectedUpdateItem = (selectedUpdateItem + 1) % 2;
    break;
  }

  menu_updateDisplay();
}

/**
 * @brief Handle double click actions based on current menu state
 * 
 * Processes double click events for menu selection and action execution,
 * entering submenus or applying selected options.
 */
static void handleDoubleClick() {
  ESP_LOGI(MENU_LOG, "Double Click");
  lastMenuActivity = millis(); // Reset timeout on any interaction

  switch (currentMenuState) {
  case NORMAL_OPERATION:
    // Double click in normal operation also enters menu - stop any playing GIFs
    stopGifPlayback();
    currentMenuState = MENU_SELECTION;
    selectedTopMenu = EFFECTS_OPTION;
    break;

  case MENU_SELECTION:
    // Enter the selected menu or exit
    switch (selectedTopMenu) {
    case EFFECTS_OPTION:
      currentMenuState = EFFECTS_MENU;
      selectedEffect = currentEffect; // Start with current effect selected
      break;
    case GLITCH_OPTION:
      currentMenuState = GLITCH_MENU;
      selectedGlitchItem = 0; // Start with toggle option
      break;
    case ESP_NOW_OPTION:
      currentMenuState = ESP_NOW_MENU;
      selectedESPNowItem = 0; // Start with toggle option
      break;
    case UPDATE_OPTION:
      currentMenuState = UPDATE_MENU;
      selectedUpdateItem = 0; // Start with toggle option
      break;
    case EXIT_OPTION:
      exitToNormalOperation();
      break;
    }
    break;

  case EFFECTS_MENU:
    {
      int totalEffects = getEffectCount();
      if (selectedEffect == totalEffects) { // GO BACK position
        // Go back to main menu
        currentMenuState = MENU_SELECTION;
        selectedEffect = currentEffect; // Reset to current effect
      } else {
        // Apply selected effect and exit menu entirely
        currentEffect = selectedEffect;
        exitToNormalOperation();
        
        menu_applyEffect(currentEffect);
        if (onEffectChange) {
          onEffectChange(currentEffect);
        }
      }
      break;
    }

  case GLITCH_MENU:
    switch (selectedGlitchItem) {
      case 0: // Toggle action
        if (onGlitchToggle) {
          bool currentStatus = menu_getGlitchStatus();
          onGlitchToggle(!currentStatus);
        } else {
          // Use effects_module toggle function directly
          toggleCRTGlitches();
        }
        exitToNormalOperation();
        break;
      case 1: // GO BACK
        currentMenuState = MENU_SELECTION;
        selectedGlitchItem = 0; // Reset to toggle option
        break;
    }
    break;

  case ESP_NOW_MENU:
    switch (selectedESPNowItem) {
      case 0: // Toggle action
        if (onESPNowToggle) {
          bool currentStatus = menu_getESPNowStatus();
          onESPNowToggle(!currentStatus);
        } else {
          toggleESPNow();
        }
        exitToNormalOperation();
        break;
      case 1: // GO BACK
        currentMenuState = MENU_SELECTION;
        selectedESPNowItem = 0; // Reset to toggle option
        break;
    }
    break;

  case UPDATE_MENU:
    {
      switch (selectedUpdateItem) {
        case 0: // Toggle action
          {
            bool wasInUpdateMode = menu_getUpdateModeStatus();
            
            if (onUpdateModeToggle) {
              onUpdateModeToggle(!wasInUpdateMode);
            } else {
              toggleSystemMode();
            }
            
            bool nowInUpdateMode = menu_getUpdateModeStatus();
            
            // If we disabled update mode, clear display and exit normally
            if (wasInUpdateMode && !nowInUpdateMode) {
              exitToNormalOperation(); // This will clear display since update mode is now off
            } else {
              // If we enabled update mode, exit without clearing display
              currentMenuState = NORMAL_OPERATION;
            }
            
          }
          break;
        case 1: // GO BACK
          currentMenuState = MENU_SELECTION;
          selectedUpdateItem = 0; // Reset to toggle option
          break;
      }
      break;
    }
  }

  menu_updateDisplay();
}

/**
 * @brief Handle very long press for deep sleep request
 * 
 * Processes long press events to initiate deep sleep mode through
 * motion_module integration or registered callback.
 */
static void handleVeryLongPress() {
  // Call callback if set
  if (onEnterDeepSleep) {
    onEnterDeepSleep();
  } else {
    // Fallback to motion module deep sleep
    handleDeepSleep();
  }
}

//==============================================================================
// MENU STATE MANAGEMENT
//==============================================================================

/**
 * @brief Handle menu timeout and auto-exit to normal operation
 * 
 * Monitors menu inactivity and automatically returns to normal operation
 * after the configured timeout period to prevent menu from staying open.
 */
static void handleMenuTimeout() {
  // Auto-exit to normal operation if no menu activity for 30 seconds
  if (currentMenuState != NORMAL_OPERATION &&
      (millis() - lastMenuActivity) > MENU_TIMEOUT) {
    exitToNormalOperation();
  }
}

/**
 * @brief Exit to normal operation state
 * 
 * Cleans up menu state and returns to normal operation mode,
 * restoring appropriate display content based on system mode.
 */
static void exitToNormalOperation() {
  currentMenuState = NORMAL_OPERATION;
  // Always clear the menu display first
  clearDisplay();
  // If we're in update mode, restore the update mode display
  if (menu_getUpdateModeStatus()) {
    // Use the existing system module function to restore update mode display
    updateDisplayForMode(SystemMode::UPDATE_MODE);
  }

  menu_updateDisplay();
}

//==============================================================================
// SYSTEM INTEGRATION FUNCTIONS
//==============================================================================

void menu_applyEffect(EffectType effect) {
  // Use improved effects_module integration with direct state setting
  EffectCycleState targetState = getStateFromEffectType(static_cast<int>(effect));
  
  // Set the effect state directly (much cleaner than cycling)
  if (setEffectStateDirect(targetState)) {
    ESP_LOGI(MENU_LOG, "Applied: %s", menu_getEffectName(effect).c_str());
  } else {
    ESP_LOGW(MENU_LOG, "Failed to apply: %s", menu_getEffectName(effect).c_str());
    // Fallback to cycling method
    while (getCurrentEffectState() != targetState) {
      cycleVisualEffects();
      
      // Safety check to prevent infinite loop
      static int cycleCount = 0;
      cycleCount++;
      if (cycleCount > getEffectCount()) {
        ESP_LOGW(MENU_LOG, "Could not reach target effect state, stopping cycle");
        cycleCount = 0;
        break;
      }
    }
  }
}

bool menu_getGlitchStatus() {
  return areCRTGlitchesEnabled();
}

bool menu_getESPNowStatus() {
  return getCurrentESPNowState() == ESPNowState::ON;
}

bool menu_getUpdateModeStatus() {
  return getCurrentMode() == SystemMode::UPDATE_MODE;
}

//==============================================================================
// DISPLAY RENDERING FUNCTIONS
//==============================================================================

void menu_updateDisplay() {
  if (currentMenuState == NORMAL_OPERATION)
    return;

  clearDisplay();
  oled.setFont(); // Use default small font for menus
  oled.setTextSize(MENU_TEXT_SIZE);
  oled.setTextColor(COLOR_YELLOW);

  switch (currentMenuState) {
  case MENU_SELECTION:
    drawMenuHeader(MENU_LABEL_MAIN_MENU);
    for (int i = 0; i < 5; i++) { // 5 options total
      TopLevelMenu menuItem = (TopLevelMenu)i;
      drawMenuItem(i, menu_getTopMenuName(menuItem).c_str(),
                   i == selectedTopMenu);
    }
    break;

  case EFFECTS_MENU: {
    drawMenuHeader(MENU_LABEL_EFFECTS);

    // Calculate how many items can fit on screen
    int headerHeight = 28;
    int availableHeight = DISPLAY_HEIGHT - headerHeight - MENU_PADDING;

    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    oled.getTextBounds("Ag", 0, 0, &x1, &y1, &textWidth, &textHeight);
    int itemHeight = textHeight + 4;
    int itemSpacing = itemHeight + MENU_ITEM_Y_OFFSET;

    int maxVisibleItems = availableHeight / itemSpacing;
    int totalEffects = getEffectCount(); // Dynamic count from effects module
    int totalItems = totalEffects + 1; // +1 for "GO BACK"

    // Show effects first, then "GO BACK" at the end
    int effectsToShow =
        min(totalEffects, maxVisibleItems - 1); // Reserve space for "GO BACK"

    // Calculate range for effects
    int startIdx = max(0, min(selectedEffect - effectsToShow / 2,
                              totalEffects - effectsToShow));
    int endIdx = min(totalEffects - 1, startIdx + effectsToShow - 1);

    // Draw effects
    for (int i = startIdx; i <= endIdx; i++) {
      EffectType effect = (EffectType)i;
      int displayIndex = i - startIdx;
      bool isHighlighted = (i == selectedEffect);
      drawMenuItem(displayIndex, menu_getEffectName(effect).c_str(),
                   isHighlighted);
    }

    // Draw "GO BACK" option
    int backIndex = endIdx - startIdx + 1;
    bool backSelected = (selectedEffect == totalEffects);
    drawMenuItem(backIndex, MENU_LABEL_GO_BACK, backSelected);

    break;
  }

  case GLITCH_MENU: {
    drawMenuHeader(MENU_LABEL_GLITCH);
    // Show current state reversed to signal the immediate action
    String glitchStatus = menu_getGlitchStatus() ? LABEL_DISABLE : LABEL_ENABLE;
    drawMenuItem(0, glitchStatus.c_str(), selectedGlitchItem == 0);
    drawMenuItem(1, MENU_LABEL_GO_BACK, selectedGlitchItem == 1);
    break;
  }

  case ESP_NOW_MENU: {
    drawMenuHeader(MENU_LABEL_ESP_NOW);
    // Show current state reversed to signal the immediate action
    String espStatus = menu_getESPNowStatus() ? LABEL_DISABLE : LABEL_ENABLE;
    drawMenuItem(0, espStatus.c_str(), selectedESPNowItem == 0);
    drawMenuItem(1, MENU_LABEL_GO_BACK, selectedESPNowItem == 1);
    break;
  }

  case UPDATE_MENU: {
    drawMenuHeader(MENU_LABEL_UPDATE);
    String updateStatus = menu_getUpdateModeStatus() ? LABEL_DISABLE : LABEL_ENABLE;
    drawMenuItem(0, updateStatus.c_str(), selectedUpdateItem == 0);
    drawMenuItem(1, MENU_LABEL_GO_BACK, selectedUpdateItem == 1);
    break;
  }
  }
}

/**
 * @brief Draw menu header with title and separator line
 * 
 * Renders the header section of the menu with title text and
 * decorative separator line for visual organization.
 * 
 * @param title Header text to display
 */
static void drawMenuHeader(const char *title) {
  oled.setCursor(MENU_PADDING + MENU_ITEM_X_OFFSET, MENU_PADDING);
  oled.println(title);

  int16_t x1, y1;
  uint16_t textWidth, textHeight;
  oled.getTextBounds(title, 0, 0, &x1, &y1, &textWidth, &textHeight);

  // Draw separator line below the header text
  int separatorY = MENU_PADDING + textHeight + MENU_ITEM_Y_OFFSET;
  oled.drawLine(MENU_PADDING, separatorY, DISPLAY_WIDTH - MENU_PADDING,
                separatorY, COLOR_YELLOW);
}

/**
 * @brief Draw a single menu item with selection highlighting
 * 
 * Renders a menu item with appropriate positioning, text, and
 * visual highlighting for the currently selected item.
 * 
 * @param index Item index (0-based) for vertical positioning
 * @param text Item text to display
 * @param selected Whether this item is currently selected
 */
static void drawMenuItem(int index, const char *text, bool selected) {
  // Get text dimensions once for both height and width calculations
  int16_t x1, y1;
  uint16_t textWidth, textHeight;
  oled.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

  int itemHeight = textHeight + 4; // Add 4 pixels padding (2 top + 2 bottom)
  int y = 28 + (index * (itemHeight + MENU_ITEM_Y_OFFSET));
  int x = MENU_PADDING + MENU_ITEM_X_OFFSET;

  // Highlight selected item with filled rectangle
  if (selected) {
    int highlightWidth = textWidth + 6;
    oled.fillRect(x - MENU_ITEM_X_OFFSET, y - MENU_ITEM_Y_OFFSET,
                  highlightWidth, itemHeight, COLOR_YELLOW);
    oled.setTextColor(COLOR_BLACK); // Black text on white background
  } else {
    oled.setTextColor(COLOR_YELLOW); // White text on black background
  }

  oled.setCursor(x, y);
  oled.println(text);
}

//==============================================================================
// STATE ACCESSOR FUNCTIONS
//==============================================================================

MenuState menu_getCurrentState() { 
  return currentMenuState; 
}

EffectType menu_getCurrentEffect() { 
  return currentEffect; 
}

bool menu_isActive() { 
  return currentMenuState != NORMAL_OPERATION; 
}

//==============================================================================
// STATE MODIFIER FUNCTIONS
//==============================================================================

void menu_setCurrentEffect(EffectType effect) {
  currentEffect = effect;
  selectedEffect = effect;
  menu_applyEffect(effect);
  menu_updateDisplay();
}

//==============================================================================
// CALLBACK REGISTRATION FUNCTIONS
//==============================================================================

void menu_setEffectChangeCallback(EffectChangeCallback callback) {
  onEffectChange = callback;
  ESP_LOGD(MENU_LOG, "Effect change callback registered");
}

void menu_setGlitchToggleCallback(GlitchToggleCallback callback) {
  onGlitchToggle = callback;
  ESP_LOGD(MENU_LOG, "Glitch toggle callback registered");
}

void menu_setESPNowToggleCallback(ESPNowToggleCallback callback) {
  onESPNowToggle = callback;
  ESP_LOGD(MENU_LOG, "ESP-NOW toggle callback registered");
}

void menu_setUpdateModeToggleCallback(UpdateModeToggleCallback callback) {
  onUpdateModeToggle = callback;
  ESP_LOGD(MENU_LOG, "Update mode toggle callback registered");
}

void menu_setDeepSleepCallback(DeepSleepCallback callback) {
  onEnterDeepSleep = callback;
  ESP_LOGD(MENU_LOG, "Deep sleep callback registered");
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

String menu_getEffectName(EffectType effect) {
  // Use effects module function for consistency
  EffectCycleState state = getStateFromEffectType(static_cast<int>(effect));
  return String(getEffectStateName(state));
}

String menu_getTopMenuName(TopLevelMenu menu) {
  switch (menu) {
  case EFFECTS_OPTION:
    return MENU_LABEL_EFFECTS;
  case GLITCH_OPTION:
    return MENU_LABEL_GLITCH;
  case ESP_NOW_OPTION:
    return MENU_LABEL_ESP_NOW;
  case UPDATE_OPTION:
    return MENU_LABEL_UPDATE;
  case EXIT_OPTION:
    return MENU_LABEL_EXIT;
  default:
    return "UNKNOWN";
  }
}

