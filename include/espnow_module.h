/**
 * @file espnow_module.h
 * @brief Header for ESP-NOW wireless communication functionality
 * 
 * This module provides functions and types for device-to-device communication
 * using ESP-NOW, including device discovery, pairing, message exchange, and 
 * state management.
 */

 #ifndef ESPNOW_MODULE_H
 #define ESPNOW_MODULE_H
 
 #include "common.h"
 #include <esp_now.h>
 #include <WiFi.h>
 
 //==============================================================================
 // FORWARD DECLARATIONS
 //==============================================================================
 
 /** @brief Forward declaration for motion state */
 class MotionState;
 
 //==============================================================================
 // CONSTANTS & DEFINITIONS
 //==============================================================================
 
 /** @brief Log tag for ESP-NOW module messages */
 static const char* ESPNOW_LOG = "::ESPNOW_MODULE::";
 
 /** @brief Byte90 unique application signature for ESP-NOW messages */
 #define APP_SIGNATURE 0xCAFE2025
 
 //==============================================================================
 // TYPE DEFINITIONS
 //==============================================================================
 
 /**
  * @brief ESP-NOW activation state
  */
 enum class ESPNowState {
     ON,     /**< ESP-NOW is active */
     OFF     /**< ESP-NOW is inactive */
 };
 
 /**
  * @brief Role of the device in a paired connection
  * 
  * Devices with larger MAC addresses are auto assigned as initiators
  */
 enum class DeviceRole {
     UNKNOWN,    /**< Role not yet determined */
     INITIATOR,  /**< Conversation initiator (starts exchanges) */
     RESPONDER   /**< Conversation responder */
 };
 
 /**
  * @brief Communication processing state
  */
 enum class ComState {
     NONE,       /**< No active communication */
     WAITING,    /**< Waiting for response */
     PROCESSING  /**< Processing received message */
 };
 
 /**
  * @brief Communication pairing status
  */
 enum class ComStatus {
     DISCOVERY,  /**< Searching for devices */
     PAIRED      /**< Paired with another device */
 };
 
 /**
  * @brief Types of conversation messages with associated animations
  */
 enum class ConversationType {
     HELLO,      /**< Greeting animation */
     QUESTION_01,/**< First question type */
     QUESTION_02,/**< Second question type */
     QUESTION_03,/**< Third question type */
     AGREE,      /**< Agreement animation */
     DISAGREE,   /**< Disagreement animation */
     YELL,       /**< Yelling animation */
     LAUGH,      /**< Laughing animation */
     WINK,       /**< Winking animation */
     ZONE,       /**< Zoned out animation */
     SHOCK       /**< Shocked animation */
 };
 
 /**
  * @brief Configuration mapping conversation types to animation paths
  */
 struct ConversationConfig {
     ConversationType type;  /**< Type of conversation */
     const char* gifPath;    /**< Path to associated GIF animation */
 };
 
 /**
  * @brief Message structure for ESP-NOW communication
  */
 struct Message {
     uint32_t signature;         /**< Application signature for validation */
     uint8_t mac[6];             /**< MAC address of sender */
     char text[32];              /**< Message text content */
     ConversationType type;      /**< Conversation type for animation */
 };
 
 /**
  * @brief Timing constants for communication operations
  */
 struct ComsInterval {
     /** @brief Interval between status checks (ms) */
     static const unsigned long STATUS_INTERVAL = 6000;
     /** @brief Interval between messages (ms) */
     static const unsigned long MESSAGE_INTERVAL = 4000;
     /** @brief Interval between discovery broadcasts (ms) */
     static const unsigned long DISCOVERY_INTERVAL = 1000;
     /** @brief Debounce time for ESP-NOW toggle (ms) */
     static const unsigned long TOGGLE_DEBOUNCE = 5000;
 };
 
 //==============================================================================
 // PUBLIC API FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Initialize ESP-NOW communication
  * @return true if initialization successful
  */
 bool initializeESPNOW();
 
 /**
  * @brief Start discovery mode to find other devices
  * @return true if discovery mode started successfully
  */
 bool startDiscovery();
 
 /**
  * @brief Check if device is paired with another device
  * @return true if paired and ESP-NOW is active
  */
 bool isPaired();
 
 /**
  * @brief Restart ESP-NOW communication
  * @return true if restart successful
  */
 bool restartCommunication();
 
 /**
  * @brief Shut down ESP-NOW communication
  */
 void shutdownCommunication();
 
 /**
  * @brief Toggle ESP-NOW on/off
  * @return true if toggle successful
  */
 bool toggleESPNow();
 
 /**
  * @brief Main communication handling function
  */
 void handleCommunication();
 
 /**
  * @brief Force disconnect from current peer
  */
 void forceDisconnect();
 
 /**
  * @brief Reset the animation path and communication state
  */
 void resetAnimationPath();
 
 /**
  * @brief Get the currently active animation path
  * @return Current animation path or nullptr if none
  */
 const char* getCurrentAnimationPath();
 
 /**
  * @brief Get current ESP-NOW state
  * @return Current ESP-NOW state (ON/OFF)
  */
 ESPNowState getCurrentESPNowState();
 
 /**
  * @brief Get current communication state
  * @return Current communication state (PROCESSING/WAITING/NONE)
  */
 ComState getCurrentComState();
 
 /**
  * @brief Check if ESP-NOW was toggled
  * @return true if ESP-NOW was toggled
  */
 bool espNowToggledState();
 
 /**
  * @brief Reset ESP-NOW toggle state
  */
 void resetEspNowToggleState();
 
 #endif /* ESPNOW_MODULE_H */