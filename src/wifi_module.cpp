/**
 * @file wifi_module.cpp
 * @brief Implementation of WiFi functionality for configuration and OTA updates
 *
 * This module provides functions for managing WiFi connections, setting up the
 * configuration portal, handling web server endpoints, and interacting with
 * stored WiFi credentials.
 */

#include "wifi_module.h"
#include "common.h"
#include "flash_module.h"
#include "ota_module.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================
/** @brief Wifi AP Credentials */
const char *WIFI_AP_SSID = "BYTE90_Setup";
const char *WIFI_AP_PASSWORD = "00000000";
/** @brief Web server instance for configuration portal */
WebServer webServer(WEB_SERVER_PORT);
/** @brief Preferences instance for storing WiFi credentials */
Preferences preferences;
/** @brief Current state of the WiFi connection */
WiFiState wifiState = WiFiState::CONFIG_MODE;
/** @brief Local IP address for the access point */
IPAddress AP_LOCAL_IP(192, 168, 4, 1);
IPAddress AP_GATEWAY(192, 168, 4, 1);
IPAddress AP_NETWORK_MASK(255, 255, 255, 0);

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Get string representation of current WiFi state
 *
 * @return String representation of the current WiFi state
 */
const char *getWiFiStateString() {
  switch (wifiState) {
  case WiFiState::CONFIG_MODE:
    return "CONFIG_MODE";
  case WiFiState::CONNECTED:
    return "CONNECTED";
  case WiFiState::ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

/**
 * @brief Convert RSSI value to a human-readable signal strength description
 *
 * @param rssi RSSI value in dBm
 * @return String describing the signal strength
 */
String getSignalStrength(int rssi) {
  if (rssi == 0) {
    return "Not connected";
  } else if (rssi > -50) {
    return "Great signal"; // -50 dBm or higher is excellent
  } else if (rssi > -70) {
    return "Good signal"; // -70 to -50 dBm is good for most applications
  } else {
    return "Poor signal"; // Below -70 dBm is considered poor
  }
}

/**
 * @brief Create a JSON response with WiFi status information
 *
 * @param success Whether the operation was successful
 * @param ssid SSID of the connected network
 * @param rssi RSSI value of the connection
 * @param message Status message
 * @param connected Whether the device is currently connected
 * @param networks JSON string with available networks (for scan results)
 * @return JSON string with status information
 */
String createJsonResponse(bool success, String ssid, int rssi, String message,
                          bool connected = false, String networks = "[]") {
  return "{\"success\":" + String(success ? "true" : "false") +
         ",\"status\":\"" + String(getWiFiStateString()) + "\"" +
         ",\"ssid\":\"" + ssid + "\"" + ",\"rssi\":" + String(rssi) +
         ",\"signal_strength\":\"" + getSignalStrength(rssi) + "\"" +
         ",\"message\":\"" + message + "\"" +
         ",\"connected\":" + String(connected ? "true" : "false") +
         ",\"networks\":" + networks + "}";
}

//==============================================================================
// WIFI CONNECTION MANAGEMENT
//==============================================================================

/**
 * @brief Attempt to connect to a WiFi network
 *
 * @param ssid SSID of the network to connect to
 * @param password Password for the network
 * @return true if connection successful, false otherwise
 */
bool attemptWiFiConnection(const String &ssid, const String &password) {
   // Store current mode to restore if connection fails
  wifi_mode_t currentMode = WiFi.getMode();

  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECTION_ATTEMPTS) {
    delay(500);
    attempts++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(currentMode);
    ESP_LOGW(WIFI_LOG, "Failed to connect to %s, restoring previous WiFi mode", ssid.c_str());
    return false;
  }

  ESP_LOGI(WIFI_LOG, "Successfully connected to %s", ssid.c_str());
  return true;
}

/**
 * @brief Connect to the previously saved WiFi network
 *
 * @return true if connection successful, false otherwise
 */
bool connectToSavedWiFi() {
  // NOTE: this is causing a bug where when connected to wifi, AP mode is gone
  preferences.begin("wifi", false);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  return (ssid.length() > 0) ? attemptWiFiConnection(ssid, password) : false;
}

/**
 * @brief Save WiFi credentials to persistent storage
 *
 * @param ssid SSID to save
 * @param password Password to save
 * @return true if credentials saved successfully, false otherwise
 */
bool saveWiFiCredentials(const String &ssid, const String &password) {
  if (ssid.isEmpty() || password.isEmpty()) {
    return false;
  }
  preferences.begin("wifi", false);
  bool success = preferences.putString("ssid", ssid) > 0 &&
                 preferences.putString("password", password) > 0;
  preferences.end();
  return success;
}

/**
 * @brief Disconnect from the current WiFi network
 */
void disconnectWiFiManager() { WiFi.disconnect(true); }

/**
 * @brief Stop the WiFi manager, clear settings, and restart the device
 */
void stopWiFiManager() {
  if (wifiState == WiFiState::CONFIG_MODE) {
    webServer.stop();
    WiFi.softAPdisconnect(true);
  }

  // Disconnect and clear settings
  WiFi.disconnect(true);
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();

  WiFi.mode(WIFI_MODE_NULL);
  delay(100);
  ESP.restart();
}

//==============================================================================
// WEB SERVER ENDPOINT SETUP
//==============================================================================

/**
 * @brief Set up the root endpoint for the configuration portal
 */
void setupRootEndpoint() {
  webServer.on("/", HTTP_GET, []() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
      wifiState = WiFiState::ERROR;
      String message = "Failed to load configuration page";
      webServer.send(500, "text/plain", message);
      return;
    }
    webServer.streamFile(file, "text/html");
    file.close();
  });

  webServer.on("/styles.css", HTTP_GET, []() {
    File file = LittleFS.open("/styles.css", "r");
    if (!file) {
      wifiState = WiFiState::ERROR;
      String message = "Failed to load CSS styles";
      webServer.send(500, "text/plain", message);
      return;
    }
    webServer.streamFile(file, "text/css");
    file.close();
  });

  webServer.on("/script.js", HTTP_GET, []() {
    File file = LittleFS.open("/script.js", "r");
    if (!file) {
      wifiState = WiFiState::ERROR;
      String message = "Failed to load javascript";
      webServer.send(500, "text/plain", message);
      return;
    }
    webServer.streamFile(file, "application/javascript");
    file.close();
  });
}

/**
 * @brief Set up the network scan endpoint
 */
void setupScanEndpoint() {
  webServer.on("/scan", HTTP_GET, []() {
    wifiState = WiFiState::CONFIG_MODE;
    String json = "{";
    int networkCount = WiFi.scanNetworks(false);
    bool scanSuccess = networkCount >= 0;

    String message = scanSuccess ? "Network scan complete, found " +
                                       String(networkCount) + " networks."
                                 : "Failed to scan networks, please try again.";

    String networks = "[";
    if (scanSuccess) {
      for (int i = 0; i < networkCount; i++) {
        if (i > 0)
          networks += ",";
        int rssi = WiFi.RSSI(i);
        networks += "{\"ssid\":\"" + WiFi.SSID(i) + "\"," +
                    "\"rssi\":" + String(rssi) + "," +
                    "\"signal_strength\":\"" + getSignalStrength(rssi) + "\"}";
      }
    }
    networks += "]";

    String jsonResponse = createJsonResponse(
        scanSuccess, "", 0, message, (WiFi.status() == WL_CONNECTED), networks);
    // Send the response
    webServer.send(200, "application/json", jsonResponse);
    WiFi.scanDelete();
  });
}

/**
 * @brief Set up the connection status endpoint
 */
void setupConnectionStatusEndpoint() {
  webServer.on("/status", HTTP_GET, []() {
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    wifiState = isConnected ? WiFiState::CONNECTED : WiFiState::CONFIG_MODE;

    String currentSSID = isConnected ? WiFi.SSID() : "";
    int rssi = isConnected ? WiFi.RSSI() : 0;
    String message = isConnected
                         ? "Connected to " + currentSSID +
                               ", you can proceed to uploading firmware."
                         : "Currently not connected to a network.";

    String jsonResponse = createJsonResponse(isConnected, currentSSID, rssi,
                                             message, isConnected);

    webServer.send(200, "application/json", jsonResponse);
  });
}

/**
 * @brief Set up the connect endpoint for connecting to a network
 */
void setupConnectEndpoint() {
  webServer.on("/connect", HTTP_POST, []() {
    if (!webServer.hasArg("ssid") || !webServer.hasArg("password")) {
      String message = "Missing SSID or password";
      String jsonResponse = createJsonResponse(false, "", 0, message);
      webServer.send(400, "application/json", jsonResponse);
      return;
    }

    String ssid = webServer.arg("ssid");
    String password = webServer.arg("password");
    bool isConnected = attemptWiFiConnection(ssid, password);
    wifiState = isConnected ? WiFiState::CONNECTED : WiFiState::CONFIG_MODE;
    // NOTE: saveWiFiCredentials logic related to conntect to saved WiFi bug
    //  if (isConnected) {
    //    saveWiFiCredentials(ssid, password);
    //  }

    String message = isConnected
                         ? "Successfully connected to " + ssid
                         : "Failed to connect to " + ssid +
                               ". Please verify the password and try again.";

    String jsonResponse = createJsonResponse(
        isConnected, ssid, isConnected ? WiFi.RSSI() : 0, message, isConnected);
    webServer.send(200, "application/json", jsonResponse);
  });
}

/**
 * @brief Set up the disconnect endpoint
 */
void setupDisconnectEndpoint() {
  webServer.on("/disconnect", HTTP_POST, []() {
    wifiState = WiFiState::CONFIG_MODE;
    String ssid = WiFi.SSID();
    int rssi = WiFi.RSSI();
    String message = "Disconnecting from your Wi-Fi network.";
    String jsonResponse = createJsonResponse(true, ssid, rssi, message, false);

    webServer.send(200, "application/json", jsonResponse);
    delay(2000);
    WiFi.disconnect(false);
  });
}

/**
 * @brief Set up the restart endpoint
 */
void setupRestartEndpoint() {
  webServer.on("/restart", HTTP_POST, []() {
    wifiState = WiFiState::CONFIG_MODE;
    String ssid = WiFi.SSID();
    int rssi = WiFi.RSSI();
    String message =
        "Disconnecting and clearing settings, your device will restart.";
    String jsonResponse = createJsonResponse(true, ssid, rssi, message, false);

    webServer.send(200, "application/json", jsonResponse);
    delay(2000);
    stopWiFiManager();
  });
}

//  /**
//   * @brief Set up the not found handler
//   * NOTE: Redirects causes issues with Windows 11 and the AP, Windows 11 will
//   refuse to connect and just disconnect
//   * from the AP, this is because Windows 11 sees redirects as a security
//   issue when checking for internet access.
//   */
//  void setupNotFoundHandler() {
//    webServer.onNotFound([]() {
//      IPAddress apIP = WiFi.softAPIP();
//      webServer.sendHeader("Location", String("http://") + apIP.toString(),
//      true); webServer.send(302, "text/plain", "Redirecting to setup page.");
//    });
//  }

/**
 * @brief Set up all web server endpoints
 */
void setupWebEndpoints() {
  setupRootEndpoint();
  setupScanEndpoint();
  setupConnectionStatusEndpoint();
  setupConnectEndpoint();
  setupDisconnectEndpoint();
  setupRestartEndpoint();
  // setupNotFoundHandler();
  webServer.begin();
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Start the WiFi configuration portal
 *
 * Sets up the access point and web server for configuration.
 */
void startWiFiConfigPortal() {
  wifiState = WiFiState::CONFIG_MODE;
    // Check if AP is already running to avoid redundant AP setup
  bool apEnabled = (WiFi.getMode() == WIFI_MODE_AP && WiFi.softAPIP() != IPAddress(0, 0, 0, 0));
  
  if (!apEnabled) {
 
    WiFi.mode(WIFI_MODE_AP);
    delay(1000);
    // Configure AP with explicit DHCP range
    WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY, AP_NETWORK_MASK);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, 1, 0, 4);
    // Additional delay specifically for Windows DHCP issues
    delay(1000);
    // Wait for the AP to be ready
    unsigned long startTime = millis();
    while (WiFi.softAPIP() == IPAddress(0, 0, 0, 0) &&
           millis() - startTime < 5000) {
      yield();
    }
  }

  setupWebEndpoints();
  setupOTAEndpoints();
}

/**
 * @brief Initialize the WiFi manager
 *
 * Attempts to connect to saved network or starts the configuration portal.
 *
 * @return true if initialization successful, false otherwise
 */
bool initWiFiManager() {
  wifiState = WiFiState::CONFIG_MODE;

  if (!getFSStatus()) {
    wifiState = WiFiState::ERROR;
    ESP_LOGE(WIFI_LOG, "Failed to initialize file system");
    return false;
  }

  if (!initOTA()) {
    wifiState = WiFiState::ERROR;
    ESP_LOGE(WIFI_LOG, "Failed to initialize OTA");
    return false;
  }

  startWiFiConfigPortal();
  // Note: current bug, when connected to saved WiFi, AP mode is not trigger
  // because portal is never configed after a system mode toggle
  //  if (!connectToSavedWiFi()) {
  //    startWiFiConfigPortal();
  //  }
  return true;
}

/**
 * @brief Handle web server client requests
 *
 * Should be called regularly in the main loop to process incoming requests.
 */
void handleWiFiManager() {
  webServer.handleClient();

  // Log connected clients and AP IP periodically (every 30 seconds)
  static unsigned long lastClientCheck = 0;
  if (millis() - lastClientCheck > 30000) {
    int connectedClients = WiFi.softAPgetStationNum();
    IPAddress apIP = WiFi.softAPIP();

    // ESP_LOGI(WIFI_LOG, "AP IP: %s, Connected clients: %d",
    //          apIP.toString().c_str(), connectedClients);

    lastClientCheck = millis();
  }
}

/**
 * @brief Clean shutdown of WiFi services when transitioning away from UPDATE_MODE
 * 
 * Gracefully stops web server and AP mode without affecting saved credentials.
 * This function should be called by system module during mode transitions.
 */
void cleanupWiFiServices() {
  if (wifiState == WiFiState::CONFIG_MODE || wifiState == WiFiState::CONNECTED) {
    ESP_LOGI(WIFI_LOG, "Cleaning up WiFi services for mode transition");
    
    // Stop web server if running
    webServer.stop();
    
    // Disconnect AP mode but don't clear settings
    WiFi.softAPdisconnect(true);
    
    // Update state to indicate we're no longer in config mode
    wifiState = WiFiState::UNKNOWN; // Temporary state during transition
  }
}

//==============================================================================
// MODE MANAGEMENT FUNCTIONS (for system module coordination)
//==============================================================================

/**
 * @brief Prepare WiFi for ESP-NOW mode (STA mode, disconnected)
 * 
 * Sets up WiFi in station mode without connection, suitable for ESP-NOW.
 * Should be called when transitioning to ESP_MODE.
 * 
 * @return true if setup successful, false otherwise
 */
bool prepareForESPMode() {
  ESP_LOGI(WIFI_LOG, "Preparing WiFi for ESP-NOW mode");
  
  // Clean shutdown of any existing services
  if (wifiState == WiFiState::CONFIG_MODE) {
    webServer.stop();
    WiFi.softAPdisconnect(true);
  }
  
  // Set to STA mode for ESP-NOW compatibility
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  
  // Update state
  wifiState = WiFiState::UNKNOWN; // No in config mode, but ready for ESP-NOW
  
  return true;
}

/**
 * @brief Prepare WiFi for Update mode (AP + web server)
 * 
 * Sets up access point and web server for configuration.
 * Should be called when transitioning to UPDATE_MODE.
 * 
 * @return true if setup successful, false otherwise
 */
bool prepareForUpdateMode() {
  ESP_LOGI(WIFI_LOG, "Preparing WiFi for Update mode");
  return initWiFiManager();
}

/**
 * @brief Check if WiFi module is ready for ESP-NOW operations
 * 
 * @return true if in correct mode for ESP-NOW, false otherwise
 */
bool isReadyForESPNow() {
  return (WiFi.getMode() == WIFI_MODE_STA && WiFi.status() != WL_CONNECTED);
}

/**
 * @brief Check if WiFi module is in configuration mode
 * 
 * @return true if in AP mode with web server running, false otherwise
 */
bool isInConfigMode() {
  return (wifiState == WiFiState::CONFIG_MODE && 
          (WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA));
}