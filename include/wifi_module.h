/**
 * @file wifi_module.h
 * @brief Header for WiFi functionality for configuration and OTA updates
 *
 * This module provides functions for managing WiFi connections, setting up the
 * configuration portal, handling web server endpoints, and interacting with
 * stored WiFi credentials.
 */

#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include "common.h"
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

/** @brief Log tag for WiFi module messages */
static const char *WIFI_LOG = "::WIFI_MODULE::";
/** @brief Maximum number of connection attempts before timeout */
#define WIFI_CONNECTION_ATTEMPTS 30
/** @brief Port for the web server */
#define WEB_SERVER_PORT 80 // Standard HTTP port

//==============================================================================
// FORWARD DECLARATIONS
//==============================================================================

/** @brief Forward declaration for OTA update state */
enum class OTAState;

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief States of the WiFi connection
 */
enum class WiFiState {
  CONFIG_MODE, /**< In configuration portal mode */
  CONNECTED,   /**< Connected to a WiFi network */
  ERROR,       /**< Error state */
  UNKNOWN      /**< Unknown state */
};

//==============================================================================
// EXTERNAL VARIABLES
//==============================================================================

/** @brief Web server instance for configuration portal */
extern WebServer webServer;
/** @brief Preferences instance for storing WiFi credentials */
extern Preferences preferences;
/** @brief Current state of the WiFi connection */
extern WiFiState wifiState;
/** @brief Message describing current WiFi status */
extern String wifiMessage;

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the WiFi manager
 *
 * Attempts to connect to saved network or starts the configuration portal.
 *
 * @return true if initialization successful, false otherwise
 */
bool initWiFiManager();

/**
 * @brief Handle web server client requests
 *
 * Should be called regularly in the main loop to process incoming requests.
 */
void handleWiFiManager();

/**
 * @brief Stop the WiFi manager, clear settings, and restart the device
 */
void stopWiFiManager();

/**
 * @brief Disconnect from the current WiFi network
 */
void disconnectWiFiManager();
/**
 * @brief Clean shutdown of WiFi services when transitioning away from
 * UPDATE_MODE
 *
 * Gracefully stops web server and AP mode without affecting saved credentials.
 * This function should be called by system module during mode transitions.
 */
void cleanupWiFiServices();

/**
 * @brief Prepare WiFi for ESP-NOW mode (STA mode, disconnected)
 * 
 * Sets up WiFi in station mode without connection, suitable for ESP-NOW.
 * Should be called when transitioning to ESP_MODE.
 * 
 * @return true if setup successful, false otherwise
 */
bool prepareForESPMode();

/**
 * @brief Prepare WiFi for Update mode (AP + web server)
 * 
 * Sets up access point and web server for configuration.
 * Should be called when transitioning to UPDATE_MODE.
 * 
 * @return true if setup successful, false otherwise
 */
bool prepareForUpdateMode();

/**
 * @brief Check if WiFi module is ready for ESP-NOW operations
 * 
 * @return true if in correct mode for ESP-NOW, false otherwise
 */
bool isReadyForESPNow();

/**
 * @brief Check if WiFi module is in configuration mode
 * 
 * @return true if in AP mode with web server running, false otherwise
 */
bool isInConfigMode();

#endif /* WIFI_MODULE_H */