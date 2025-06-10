/**
 * @file system_module.cpp
 * @brief Implementation of system mode management functionality
 * 
 * This module provides functions for handling different system operation modes,
 * including ESP-NOW communication mode and OTA update mode. It manages
 * transitions between modes and updates the display accordingly.
 */

 #include "system_module.h"
 #include "common.h"
 #include "display_module.h"
 #include "emotes_module.h"
 #include "espnow_module.h"
 #include "gif_module.h"
 #include "wifi_module.h"
 #include "menu_module.h"
 
 //==============================================================================
 // GLOBAL VARIABLES
 //==============================================================================
 
 /** @brief Current system operation mode */
 SystemMode currentMode = SystemMode::ESP_MODE;
 
 //==============================================================================
 // MODE MANAGEMENT FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Get the current system operation mode
  * 
  * @return Current SystemMode value
  */
 SystemMode getCurrentMode() { return currentMode; }
 
 /**
  * @brief Check if the system is already in the target mode
  * 
  * @param targetMode The mode to check against current mode
  * @return true if already in target mode, false otherwise
  */
 static bool isAlreadyInTargetMode(SystemMode targetMode) {
   return currentMode == targetMode;
 }
 
 /**
  * @brief Clean up resources from the current mode before transition
  * 
  * Handles shutting down ESP-NOW or WiFi services as needed when
  * transitioning between modes.
  * 
  * @return true if cleanup successful, false otherwise
  */
 static bool cleanupCurrentMode() {
   if (currentMode == SystemMode::ESP_MODE) {
     // When switching from ESP mode to WiFi mode
     if (getCurrentESPNowState() == ESPNowState::ON) {
       // ESP_LOGI log removed
       if (!toggleESPNow()) {
         ESP_LOGE(SYSTEM_LOG, "Failed to disable ESP-NOW communications");
         return false;
       }
     }
   } else {
  // When switching from WiFi mode to ESP mode
    // Use WiFi module's cleanup function instead of direct WiFi manipulation
    cleanupWiFiServices();
    ESP_LOGI(SYSTEM_LOG, "WiFi services cleaned up for mode transition");
   }
   return true;
 }
 
 /**
  * @brief Initialize the target mode after transition
  * 
  * Sets up necessary services for the new mode.
  * 
  * @param targetMode The mode to initialize
  * @return true if initialization successful, false otherwise
  */
 static bool initializeTargetMode(SystemMode targetMode) {
   bool success = false;
 
   if (targetMode == SystemMode::ESP_MODE) {
   // Ensure we're in STA mode for ESP-NOW - but let WiFi module clean up first
    // Only set mode if we're not already in a compatible state
    success = prepareForESPMode();
    if (!success) {
      ESP_LOGE(SYSTEM_LOG, "Failed to prepare WiFi for ESP-NOW mode");
    }
   } else {
     // Initialize WiFi Manager (sets up AP mode)
    success = prepareForUpdateMode();
     if (!success) {
       ESP_LOGE(SYSTEM_LOG, "Failed to initialize WiFi Manager in AP mode");
     }
   }
   return success;
 }
 
 /**
  * @brief Update the display based on the new system mode
  * 
  * @param newMode The mode to update display for
  */
 void updateDisplayForMode(SystemMode newMode) {
  // Importantant to stop GIF playback before newMode is detected
  // this prevents a timing issue for any GIF currently in progress
   stopGifPlayback();
   if (newMode == SystemMode::UPDATE_MODE) {
     clearDisplay();
     delay(200);
     displayStaticImage(UPDATER_STATIC, 128, 128);
   }
 }
 
 //==============================================================================
 // PUBLIC API FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Transition the system to a new operation mode
  * 
  * Handles the complete transition process: cleanup, initialization,
  * and display update.
  * 
  * @param targetMode The mode to transition to
  * @return true if transition successful, false otherwise
  */
 bool transitionToMode(SystemMode targetMode) {
   // Skip if already in target mode
   if (isAlreadyInTargetMode(targetMode))
     return true;
  
    menu_resetStates();
 
   if (!cleanupCurrentMode())
     return false;
 
   bool success = initializeTargetMode(targetMode);
 
   if (success) {
     // Update the global state
     currentMode = targetMode;
     updateDisplayForMode(targetMode);
   }
 
   return success;
 }
 
 /**
  * @brief Toggle between ESP-NOW and Update modes
  * 
  * Switches from current mode to the alternative mode.
  */
 void toggleSystemMode() {
   SystemMode targetMode = (currentMode == SystemMode::ESP_MODE)
                               ? SystemMode::UPDATE_MODE
                               : SystemMode::ESP_MODE;
   if (!transitionToMode(targetMode)) {
     ESP_LOGE("SystemMode", "Failed to toggle system mode");
     // Handle failure - could try to revert to previous state if needed
   }
 }