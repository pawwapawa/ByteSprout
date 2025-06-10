/**
 * @file system_module.h
 * @brief Header for system mode management functionality
 * 
 * This module provides functions for handling different system operation modes,
 * including ESP-NOW communication mode and OTA update mode. It manages
 * transitions between modes and updates the display accordingly.
 */

 #ifndef SYSTEM_MODULE_H
 #define SYSTEM_MODULE_H
 
 //==============================================================================
 // CONSTANTS & DEFINITIONS
 //==============================================================================
 
 /** @brief Log tag for System module messages */
 static const char* SYSTEM_LOG = "::SYSTEM_MODULE::";
 
 //==============================================================================
 // TYPE DEFINITIONS
 //==============================================================================
 
 /**
  * @brief System operation modes
  */
 enum class SystemMode {
   ESP_MODE,     /**< Default mode - ESP-NOW communication */
   UPDATE_MODE   /**< WiFi configuration and OTA update mode */
 };
 
 //==============================================================================
 // PUBLIC API FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Get the current system operation mode
  * 
  * @return Current SystemMode value
  */
 SystemMode getCurrentMode();
 
 /**
  * @brief Transition the system to a new operation mode
  * 
  * Handles the complete transition process: cleanup, initialization,
  * and display update.
  * 
  * @param targetMode The mode to transition to
  * @return true if transition successful, false otherwise
  */
 bool transitionToMode(SystemMode targetMode);
 
 /**
  * @brief Toggle between ESP-NOW and Update modes
  * 
  * Switches from current mode to the alternative mode.
  */
 void toggleSystemMode();
 
 /**
  * @brief Update the display based on the new system mode
  * 
  * @param newMode The mode to update display for
  */
 void updateDisplayForMode(SystemMode newMode);
 
 #endif /* SYSTEM_MODULE_H */