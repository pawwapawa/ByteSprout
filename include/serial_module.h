/**
 * @file serial_module.h
 * @brief Header for Web Serial API integration functionality
 * 
 * This module provides functions for handling firmware and filesystem updates
 * via USB serial connection using Web Serial API. Works alongside the existing
 * WiFi-based OTA system to provide an alternative update method with enhanced
 * data integrity and size validation.
 * 
 * Key features:
 * - High-speed firmware updates via USB serial connection
 * - Base64 data encoding with integrity validation
 * - Size verification and partition safety checks
 * - JSON-based command protocol compatible with Web Serial API
 * - Real-time progress reporting and error handling
 * - Support for both firmware and filesystem updates
 */

#ifndef SERIAL_MODULE_H
#define SERIAL_MODULE_H

#include "common.h"
#include <Update.h>
#include <esp_ota_ops.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

/** @brief Log tag for Serial module messages */
static const char* SERIAL_LOG = "::SERIAL_MODULE::";

/** @brief Serial communication baud rate - optimized for fast transfers */
#define SERIAL_BAUD_RATE 921600

/** @brief Maximum size for serial command buffer */
#define SERIAL_COMMAND_BUFFER_SIZE 4096

//==============================================================================
// PROTOCOL COMMANDS
//==============================================================================

// Command identifiers
#define CMD_GET_INFO "GET_INFO"           /**< Request device information */
#define CMD_GET_STATUS "GET_STATUS"       /**< Request current status */
#define CMD_START_UPDATE "START_UPDATE"   /**< Initialize update process */
#define CMD_SEND_CHUNK "SEND_CHUNK"       /**< Send firmware data chunk */
#define CMD_FINISH_UPDATE "FINISH_UPDATE" /**< Finalize update process */
#define CMD_ABORT_UPDATE "ABORT_UPDATE"   /**< Cancel ongoing update */
#define CMD_RESTART "RESTART"             /**< Restart device */
#define CMD_GET_LOGS "GET_LOGS"           /**< Get logging status */

// Response prefixes for protocol communication
#define RESP_OK "OK:"                     /**< Success response prefix */
#define RESP_ERROR "ERROR:"               /**< Error response prefix */
#define RESP_PROGRESS "PROGRESS:"         /**< Progress update prefix */

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Serial update states for tracking firmware update progress
 */
enum class SerialUpdateState {
  IDLE,           /**< No update in progress */
  RECEIVING,      /**< Receiving firmware data chunks */
  PROCESSING,     /**< Processing received data and finalizing */
  SUCCESS,        /**< Update completed successfully */
  ERROR          /**< Update failed with error */
};

/**
 * @brief Serial command structure for parsed commands
 */
struct SerialCommand {
  String command;     /**< Command name (e.g., "START_UPDATE") */
  String data;        /**< Command data/parameters */
};

/**
 * @brief Update progress information for tracking transfer status
 */
struct UpdateProgress {
  size_t totalSize;       /**< Total firmware size in bytes */
  size_t receivedSize;    /**< Bytes received so far */
  int percentage;         /**< Progress percentage (0-100) */
  String message;         /**< Current status message */
};

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the serial system
 * 
 * Sets up serial communication with optimized baud rate and initializes
 * command processing system. Configures buffers for optimal performance
 * during firmware transfers.
 * 
 * @return true if initialization successful, false otherwise
 */
bool initSerial();

/**
 * @brief Handle incoming serial commands
 * 
 * Should be called regularly in main loop to process incoming
 * serial commands and manage update state. Uses optimized batch
 * processing for maximum performance during firmware transfers.
 */
void handleSerialCommands();

/**
 * @brief Check if serial update is in progress
 * 
 * @return true if serial update is active, false if idle
 */
bool isSerialUpdateActive();

/**
 * @brief Get current serial update state
 * 
 * @return Current SerialUpdateState enumeration value
 */
SerialUpdateState getSerialUpdateState();

/**
 * @brief Abort current serial update
 * 
 * Safely cancels any ongoing update process, cleans up resources,
 * and resets state to IDLE. Safe to call even if no update is active.
 */
void abortSerialUpdate();

/**
 * @brief Enable/disable verbose logging over serial
 * 
 * Controls the amount of debug information sent over serial.
 * Verbose mode provides detailed operation logs but may impact
 * performance during high-speed data transfers.
 * 
 * @param enabled True to enable verbose logging, false to disable
 */
void setSerialVerbose(bool enabled);

/**
 * @brief Clean shutdown of serial interface when leaving UPDATE_MODE
 * 
 * Gracefully stops serial command processing, aborts any active updates,
 * and resets state. This function should be called by system module 
 * during mode transitions to ensure clean state management.
 */
void cleanupSerial();

#endif /* SERIAL_MODULE_H */