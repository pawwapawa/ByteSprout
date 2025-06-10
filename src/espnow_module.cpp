/**
 * @file espnow_module.cpp
 * @brief Implementation of ESP-NOW wireless communication functionality
 *
 * This module provides functions for device-to-device communication using
 * ESP-NOW, including device discovery, pairing, message exchange, and state
 * management. It handles animations tied to different conversation types.
 */

#include "espnow_module.h"
#include "common.h"
#include "emotes_module.h"
#include "motion_module.h"
#include "system_module.h"

//==============================================================================
// GLOBAL CONSTANTS AND VARIABLES
//==============================================================================

/** @brief Maximum consecutive communication failures before reset */
const int MAX_FAILURES = 4;
/** @brief Maximum discovery broadcast attempts before reset */
const int MAX_BROADCAST_ATTEMPTS = 30;
/** @brief Currently active animation path */
static const char *currentAnimationPath = nullptr;

/** @brief Mapping of conversation types to animation paths */
static const ConversationConfig CONVERSATIONS[] = {
    {ConversationType::HELLO, COMS_HELLO_EMOTE},
    {ConversationType::QUESTION_01, COMS_TALK_01_EMOTE},
    {ConversationType::QUESTION_02, COMS_TALK_02_EMOTE},
    {ConversationType::QUESTION_03, COMS_TALK_03_EMOTE},
    {ConversationType::AGREE, COMS_AGREED_EMOTE},
    {ConversationType::DISAGREE, COMS_DISAGREE_EMOTE},
    {ConversationType::YELL, COMS_YELL_EMOTE},
    {ConversationType::LAUGH, COMS_LAUGH_EMOTE},
    {ConversationType::WINK, COMS_WINK_EMOTE},
    {ConversationType::ZONE, COMS_ZONED_EMOTE},
    {ConversationType::SHOCK, COMS_SHOCK_EMOTE}};

//------------------------------------------------------------------------------
// Communication States
//------------------------------------------------------------------------------
/** @brief MAC address of currently paired peer device */
uint8_t peerMac[6] = {0};
/** @brief MAC address of last known peer for reconnection attempts */
uint8_t lastKnownPeerMac[6] = {0};
/** @brief Flag indicating if a last known peer exists */
bool hasLastKnownPeer = false;
/** @brief Flag indicating if ESP-NOW was toggled */
bool espNowToggled = false;

//------------------------------------------------------------------------------
// Status Variables
//------------------------------------------------------------------------------
/** @brief Current communication status (discovery/paired) */
ComStatus currentStatus = ComStatus::DISCOVERY;
/** @brief Current communication state (processing/waiting/none) */
ComState currentComState = ComState::NONE;
/** @brief Current device role in paired communication */
DeviceRole currentRole = DeviceRole::UNKNOWN;
/** @brief Current ESP-NOW activation state */
ESPNowState currentESPNowState = ESPNowState::OFF;

//------------------------------------------------------------------------------
// Communication Timers and Counters
//------------------------------------------------------------------------------
/** @brief Count of consecutive communication failures */
int consecutiveFailures = 0;
/** @brief Count of broadcast attempts in discovery mode */
int broadcastAttempts = 0;
/** @brief Timestamp of last broadcast attempt */
unsigned long lastBroadcastTime = 0;
/** @brief Timestamp of last status check */
unsigned long lastStatusTime = 0;
/** @brief Timestamp of last message sent */
unsigned long lastMessageTime = 0;
/** @brief Timestamp of last ESP-NOW toggle */
unsigned long lastToggleTime = 0;

//==============================================================================
// FORWARD DECLARATIONS
//==============================================================================

/**
 * @brief ESP-NOW send callback function
 * @param mac MAC address of the recipient
 * @param status Send status (success/failure)
 */
static void Send_data_cb(const uint8_t *mac, esp_now_send_status_t status);

/**
 * @brief ESP-NOW receive callback function
 * @param mac MAC address of the sender
 * @param data Pointer to received data
 * @param len Length of received data
 */
static void Receive_data_cb(const uint8_t *mac, const uint8_t *data, int len);

/**
 * @brief Handle device pairing process
 * @param mac MAC address of device to pair with
 */
static void handlePairing(const uint8_t *mac);

/**
 * @brief Handle connection loss and reset communication
 */
static void handleConnectionLost(void);

/**
 * @brief Send a data message to paired device
 * @param text Message text content
 * @param type Conversation type for animation selection
 * @return true if message was sent successfully
 */
static bool sendDataMessage(const char *text, ConversationType type);

/**
 * @brief Send a discovery broadcast message
 * @return true if message was sent successfully
 */
static bool sendDiscoveryMessage(void);

//==============================================================================
// ANIMATION PATH MANAGEMENT
//==============================================================================

/**
 * @brief Get animation path for a specific conversation type
 * @param type Conversation type to find animation for
 * @return Path to the animation file or nullptr if not found
 */
const char *getAnimationPath(ConversationType type) {
  for (const auto &conv : CONVERSATIONS) {
    if (conv.type == type)
      return conv.gifPath;
  }
  return nullptr;
}

/**
 * @brief Get the currently active animation path
 * @return Current animation path or nullptr if none
 */
const char *getCurrentAnimationPath() { return currentAnimationPath; }

/**
 * @brief Reset the animation path and communication state
 */
void resetAnimationPath() {
  currentAnimationPath = nullptr;
  currentComState = ComState::NONE;
}

//==============================================================================
// ESP-NOW CORE FUNCTIONS
//==============================================================================

/**
 * @brief Format MAC address as string
 * @param mac MAC address bytes
 * @param output String buffer to store formatted address
 */
static void formatMacAddress(const uint8_t *mac, char *output) {
  snprintf(output, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

/**
 * @brief Set up a peer for ESP-NOW communication
 * @param mac MAC address of the peer to set up
 * @return true if peer was set up successfully
 */
static bool setupPeer(const uint8_t *mac) {
  // First check if peer already exists
  if (esp_now_is_peer_exist(mac)) {
    return true;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  return esp_now_add_peer(&peerInfo) == ESP_OK;
}

/**
 * @brief Initialize ESP-NOW communication
 * @return true if initialization successful
 */
bool initializeESPNOW() {
  // Make sure we're in station mode for ESP-NOW
  if (WiFi.getMode() != WIFI_STA) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
  }

  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(ESPNOW_LOG, "ESP-NOW init failed!");
    return false;
  }

  esp_now_register_send_cb(Send_data_cb);
  esp_now_register_recv_cb(Receive_data_cb);
  // ESP_LOGI log removed
  return true;
}

/**
 * @brief Callback function to receive data from ESP-NOW protocol
 */
static void Receive_data_cb(const uint8_t *mac, const uint8_t *data, int len) {
  // Validate message size before processing
  if (len != sizeof(Message)) {
    ESP_LOGE(ESPNOW_LOG, "Invalid message size: received %d bytes, expected %d",
             len, sizeof(Message));
    return;
  }
  // Cast received data and check if App signature matches to accept data
  Message *msg = (Message *)data;
  if (msg->signature != APP_SIGNATURE) {
    ESP_LOGE(ESPNOW_LOG, "App signature mismatch: received v%d, expected v%d",
             msg->signature, APP_SIGNATURE);
    return;
  }

  // Handle discovery mode separately
  if (currentStatus == ComStatus::DISCOVERY) {
    handlePairing(mac);
  }
  // Update animation path based on message type
  currentAnimationPath = getAnimationPath(msg->type);
  // Response animations, Ensure special animation types get priority
  if (msg->type == ConversationType::SHOCK ||
      msg->type == ConversationType::ZONE) {
    currentComState = ComState::PROCESSING;
  } else if (currentAnimationPath != nullptr) {
    currentComState = ComState::PROCESSING;
  }
}

/**
 * @brief Callback function to send data from ESP-NOW protocol
 */
static void Send_data_cb(const uint8_t *mac, esp_now_send_status_t status) {
  char macStr[18];
  formatMacAddress(mac, macStr);
  if (status == ESP_NOW_SEND_SUCCESS) {
    consecutiveFailures = 0;
  } else {
    ESP_LOGE(ESPNOW_LOG, "::Delivery failed to:: %s", macStr);
    if (++consecutiveFailures >= MAX_FAILURES) {
      // If too many fail attempts disconnect and restart ESPNOW communications
      handleConnectionLost();
    }
  }
}

//==============================================================================
// CONNECTION MANAGEMENT
//==============================================================================

/**
 * @brief Check if device is paired with another device
 * @return true if paired and ESP-NOW is active
 */
bool isPaired() {
  ESPNowState espnowState = getCurrentESPNowState();
  return espnowState == ESPNowState::ON && currentStatus == ComStatus::PAIRED;
}

/**
 * @brief Start discovery mode to find other devices
 * @return true if discovery mode started successfully
 */
bool startDiscovery() {
  if (getCurrentESPNowState() == ESPNowState::OFF) {
    // ESP_LOGI log removed
    return false;
  }

  uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  if (!setupPeer(broadcastAddr)) {
    ESP_LOGE(ESPNOW_LOG, "ESPNOW Discovery setup failed!");
    return false;
  }

  // ESP_LOGI log removed
  return true;
}

/**
 * @brief Handle lost connection to paired device
 */
static void handleConnectionLost() {
  // Store current peer MAC before resetting if we're paired
  if (isPaired()) {
    memcpy(lastKnownPeerMac, peerMac, 6);
    hasLastKnownPeer = true;
  }

  currentStatus = ComStatus::DISCOVERY;
  currentRole = DeviceRole::UNKNOWN;
  ESP_LOGW(ESPNOW_LOG, "Connection reset - returning to discovery mode");

  if (isPaired()) {
    esp_now_del_peer(peerMac);
  }

  memset(peerMac, 0, 6);

  resetAnimationPath();
  consecutiveFailures = 0;
  broadcastAttempts = 0;
  lastBroadcastTime = 0;
  lastMessageTime = 0;

  if (!startDiscovery()) {
    ESP_LOGE(ESPNOW_LOG, "Failed to return to discovery mode");
    return;
  }
}

/**
 * @brief Handle pairing with another device
 * @param mac MAC address of device to pair with
 */
static void handlePairing(const uint8_t *mac) {
  // First, check if we're already paired with this MAC
  if (currentStatus == ComStatus::PAIRED && memcmp(peerMac, mac, 6) == 0) {
    ESP_LOGW(ESPNOW_LOG, "Already paired with this device");
    return;
  }

  // Clear last known peer data if pairing with a different device
  if (hasLastKnownPeer && memcmp(lastKnownPeerMac, mac, 6) != 0) {
    hasLastKnownPeer = false;
    memset(lastKnownPeerMac, 0, 6);
  }

  memcpy(peerMac, mac, 6);
  if (!setupPeer(mac)) {
    ESP_LOGE(ESPNOW_LOG, "Failed to add peer!");
    return;
  }
  // Update status to PAIRED
  currentStatus = ComStatus::PAIRED;

  // Get our MAC address
  uint8_t myMac[6];
  WiFi.macAddress(myMac);
  // Compare full MAC addresses - larger becomes INITIATOR
  if (memcmp(myMac, mac, 6) > 0) {
    currentRole = DeviceRole::INITIATOR;
  } else {
    currentRole = DeviceRole::RESPONDER;
  }

  char macStr[18];
  formatMacAddress(mac, macStr);
  // ESP_LOGI log removed
}

//==============================================================================
// MESSAGE HANDLING
//==============================================================================

/**
 * @brief Send a discovery broadcast message
 * @return true if message was sent successfully
 */
static bool sendDiscoveryMessage() {
  if (getCurrentESPNowState() == ESPNowState::OFF ||
      currentStatus != ComStatus::DISCOVERY) {
    return false;
  }

  unsigned long currentTime = millis();
  unsigned long previousMessageTime = lastMessageTime;

  // Try direct reconnection on first attempt if we have a known peer
  if (hasLastKnownPeer && broadcastAttempts == 0) {
    char macStr[18];
    formatMacAddress(lastKnownPeerMac, macStr);
    // Use existing setupPeer function to establish connection
    if (setupPeer(lastKnownPeerMac)) {
      // Use existing sendDataMessage for the reconnection attempt
      if (sendDataMessage("RECONNECT", ConversationType::HELLO)) {
        delay(1000);

        if (currentStatus == ComStatus::PAIRED) {
          // ESP_LOGI log removed
          broadcastAttempts = 0;
          return true;
        }
      }
    }
  }

  if (currentTime - lastMessageTime < ComsInterval::DISCOVERY_INTERVAL)
    return false;

  if (currentTime - lastBroadcastTime < ComsInterval::DISCOVERY_INTERVAL) {
    return false;
  }

  lastMessageTime = currentTime;
  lastBroadcastTime = currentTime;
  broadcastAttempts++;

  Message msg = {};
  msg.signature = APP_SIGNATURE;
  WiFi.macAddress(msg.mac);
  strcpy(msg.text, "SEARCHING PEERS");
  msg.type = ConversationType::HELLO;

  uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // ESP_LOGI log removed

  // If we've reached max attempts with no response, reinitialize
  if (broadcastAttempts >= MAX_BROADCAST_ATTEMPTS) {
    hasLastKnownPeer = false;
    memset(lastKnownPeerMac, 0, 6);
    handleConnectionLost();
    broadcastAttempts = 0;
  }

  return esp_now_send(broadcastAddr, (uint8_t *)&msg, sizeof(Message)) ==
         ESP_OK;
}

/**
 * @brief Send a data message to the paired device
 * @param text Message text content
 * @param type Conversation type for animation selection
 * @return true if message was sent successfully
 */
static bool sendDataMessage(const char *text,
                            ConversationType type = ConversationType::HELLO) {
  if (!isPaired())
    return false;

  unsigned long currentTime = millis();
  unsigned long previousMessageTime = lastMessageTime;

  if (currentTime - lastMessageTime < ComsInterval::MESSAGE_INTERVAL)
    return false;

  lastMessageTime = currentTime;

  Message msg = {};
  msg.signature = APP_SIGNATURE;
  WiFi.macAddress(msg.mac);
  strncpy(msg.text, text, sizeof(msg.text) - 1);
  msg.text[sizeof(msg.text) - 1] = '\0';
  msg.type = type;

  currentAnimationPath = getAnimationPath(msg.type);
  currentComState = ComState::PROCESSING;
  return esp_now_send(peerMac, (uint8_t *)&msg, sizeof(Message)) == ESP_OK;
}

/**
 * @brief Handle sequential conversation between paired devices
 */
static void handleSequentialConversation() {
  static unsigned long lastConversationTime = 0;
  static int sequenceIndex = 0;
  static bool orientationTriggered = false;

  if (!isPaired() ||
      millis() - lastConversationTime < ComsInterval::MESSAGE_INTERVAL) {
    return;
  }

  // Check for orientation changes first
  if (motionOriented()) {
    if (!orientationTriggered) {
      if (sendDataMessage("ORIENTATION_CHANGE", ConversationType::SHOCK)) {
        orientationTriggered = true;
        lastConversationTime = millis();
      }
    } else {
      if (sendDataMessage("ORIENTATION_ZONED", ConversationType::ZONE)) {
        orientationTriggered = false;
        lastConversationTime = millis();
      }
    }
    return;
  }

  // Reset orientation trigger if no orientation detected
  orientationTriggered = false;
  // Normal sequential conversation logic
  if (millis() - lastConversationTime > ComsInterval::MESSAGE_INTERVAL) {
    const size_t sequenceLength =
        sizeof(CONVERSATIONS) / sizeof(CONVERSATIONS[0]);

    bool activeSender =
        (currentRole == DeviceRole::INITIATOR && sequenceIndex % 2 == 0) ||
        (currentRole == DeviceRole::RESPONDER && sequenceIndex % 2 == 1);

    currentComState = ComState::WAITING;

    if (activeSender) {
      delay(random(100, 500));
      if (sendDataMessage("CONVERSE", CONVERSATIONS[sequenceIndex].type)) {
        lastConversationTime = millis();
      }
    }

    sequenceIndex = (sequenceIndex + 1) % sequenceLength;
  }
}

//==============================================================================
// COMMUNICATION MANAGEMENT
//==============================================================================

/**
 * @brief Main communication handling function
 */
void handleCommunication() {
  static unsigned long lastAttempt = 0;
  // Don't process anything if we're disconnected
  if (getCurrentESPNowState() == ESPNowState::OFF)
    return;

  if (millis() - lastAttempt > ComsInterval::STATUS_INTERVAL) {
    if (currentStatus == ComStatus::DISCOVERY) {
      sendDiscoveryMessage();
    } else if (currentStatus == ComStatus::PAIRED) {
      handleSequentialConversation();
    }
    lastAttempt = millis();
  }
}

/**
 * @brief Attempt to discover other devices
 */
void attemptDiscovery() {
  if (!isPaired()) {
    sendDiscoveryMessage();
  }
}

/**
 * @brief Force disconnect from current peer
 */
void forceDisconnect() {
  if (isPaired() || currentStatus == ComStatus::DISCOVERY) {
    handleConnectionLost();
  }
}

/**
 * @brief Shut down ESP-NOW communication
 */
void shutdownCommunication() {
  // ESP_LOGI log removed
  // First disconnect from any peers
  forceDisconnect();
  // Deinitialize ESP-NOW
  esp_now_deinit();
  // Disconnect Wi-Fi
  if (getCurrentMode() == SystemMode::ESP_MODE) {
    WiFi.disconnect(true); // true = disable station mode
  }
  currentESPNowState = ESPNowState::OFF;
  currentStatus = ComStatus::DISCOVERY;
  ESP_LOGW(ESPNOW_LOG, "ESPNOW is turned off");
}

/**
 * @brief Restart ESP-NOW communication
 * @return true if restart successful
 */
bool restartCommunication() {
  // Only initialize if necessary
  if (currentESPNowState == ESPNowState::OFF) {
    initializeESPNOW();
  }
  // Update state before starting discovery
  currentESPNowState = ESPNowState::ON;
  currentStatus = ComStatus::DISCOVERY;

  if (!startDiscovery()) {
    currentESPNowState = ESPNowState::OFF;
    shutdownCommunication();
    ESP_LOGE(ESPNOW_LOG, "Failed to restart discovery mode!");
    return false;
  }

  // ESP_LOGI log removed
  return true;
}

/**
 * @brief Toggle ESP-NOW on/off
 * @return true if toggle successful
 */
bool toggleESPNow() {
  unsigned long currentTime = millis();
  // Debounce prevents accidental toggles
  if (currentTime - lastToggleTime < ComsInterval::TOGGLE_DEBOUNCE) {
    return false;
  }

  lastToggleTime = currentTime;

  if (currentESPNowState == ESPNowState::ON) {
    // ESP_LOGI log removed
    shutdownCommunication();
    espNowToggled = true;
    return espNowToggled;
  }

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(ESPNOW_LOG, "ESP-NOW init failed!");
    espNowToggled = false;
    return espNowToggled;
  }

  // ESP_LOGI log removed
  espNowToggled = true;
  return restartCommunication();
}

/**
 * @brief Get current ESP-NOW state
 * @return Current ESP-NOW state (ON/OFF)
 */
ESPNowState getCurrentESPNowState() { return currentESPNowState; }

/**
 * @brief Check if ESP-NOW was toggled
 * @return true if ESP-NOW was toggled
 */
bool espNowToggledState() { return espNowToggled; }

/**
 * @brief Get current communication state
 * @return Current communication state (PROCESSING/WAITING/NONE)
 */
ComState getCurrentComState() { return currentComState; }

/**
 * @brief Reset ESP-NOW toggle state
 */
void resetEspNowToggleState() { espNowToggled = false; }