/**
 * @file ota_module.h
 * @brief Header for Over-The-Air update functionality
 * 
 * This module provides functions for handling firmware and filesystem updates
 * over WiFi, including status tracking, file uploads, and update application.
 */

 #ifndef OTA_MODULE_H
 #define OTA_MODULE_H
 
 #include "common.h"
 #include "wifi_module.h"
 #include "flash_module.h"
 #include <WebServer.h>
 #include <Update.h>
 #include <esp_ota_ops.h>
 
 //==============================================================================
 // CONSTANTS & DEFINITIONS
 //==============================================================================
 
 /** @brief Current firmware version number */
 #define FIRMWARE_VERSION 1
 /** @brief Filename for firmware updates */
 #define FIRMWARE_BIN "byte90.bin"
 /** @brief Filename for filesystem updates */
 #define FILESYSTEM_BIN "byte90animations.bin"
 
 /** @brief Log tag for OTA module messages */
 static const char* OTA_LOG = "::OTA_MODULE::";
 
 //==============================================================================
 // TYPE DEFINITIONS
 //==============================================================================
 
 /**
  * @brief States of the OTA update process
  */
 enum class OTAState {
   IDLE,       /**< No update in progress */
   UPLOADING,  /**< File is being uploaded */
   UPDATING,   /**< Update is being applied */
   SUCCESS,    /**< Update completed successfully */
   ERROR,      /**< Update failed */
   UNKNOWN     /**< Unknown state */
 };
 
 //==============================================================================
 // EXTERNAL VARIABLES
 //==============================================================================
 
 /** @brief Web server instance for handling HTTP requests */
 extern WebServer webServer;
 /** @brief Current state of the OTA update process */
 extern OTAState otaState;
 /** @brief Current status message for OTA updates */
 extern String otaMessage;
 
 //==============================================================================
 // PUBLIC API FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Initialize the OTA system
  * 
  * Sets up OTA partitions and checks for their availability.
  * 
  * @return true if initialization was successful
  */
 bool initOTA();
 
 /**
  * @brief Set up HTTP endpoints for OTA updates
  * 
  * Creates web server routes for handling update uploads and status queries.
  */
 void setupOTAEndpoints();
 
 /**
  * @brief Handle the file upload process for OTA updates
  * 
  * This function handles different stages of file upload: start, write, end, and abort.
  */
 void handleFileUpload();
 
 #endif /* OTA_MODULE_H */