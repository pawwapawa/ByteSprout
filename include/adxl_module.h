/**
 * @file adxl_module.h
 * @brief Header for ADXL345 accelerometer module
 * 
 * This module provides functionality for interacting with the ADXL345 
 * accelerometer, including initialization, configuration, and data reading.
 * It also handles interrupt-based deep sleep functionality.
 */

 #ifndef ADXL_MODULE_H
 #define ADXL_MODULE_H
 
 #include "common.h"
 #include <Adafruit_ADXL345_U.h>
 #include <Adafruit_Sensor.h>
 
 //==============================================================================
 // CONSTANTS & DEFINITIONS
 //==============================================================================
 
 /** @brief Log tag for ADXL module messages */
 static const char* ADXL_LOG = "::ADXL_MODULE::";
 
 //------------------------------------------------------------------------------
 // Pin Definitions
 //------------------------------------------------------------------------------
 /** @brief Interrupt pin bitmask for ESP wakeup */
 #define INT_PIN_BITMASK (1ULL << GPIO_NUM_1)
 /** @brief Interrupt pin for ADXL345 (D1 on Seeedstudio XIAO board) */
 #define INTERRUPT_PIN_D1 D1
 /** @brief I2C SDA pin (D4 on Seeedstudio XIAO board) */
 #define SDA_PIN_D4 D4
 /** @brief I2C SCL pin (D5 on Seeedstudio XIAO board) */
 #define SCL_PIN_D5 D5
 
 //------------------------------------------------------------------------------
 // Conversion Factors
 //------------------------------------------------------------------------------
 /** @brief Scale factor for converting G-force to register values */
 #define FORCE_SCALE_FACTOR 62.5
 /** @brief Scale factor for converting duration to register values */
 #define DURATION_SCALE_FACTOR 0.625
 /** @brief Scale factor for converting latency to register values */
 #define LATENCY_SCALE_FACTOR 1.25
 
 //------------------------------------------------------------------------------
 // Interrupt Source Bitmasks (ADXL345 datasheet)
 //------------------------------------------------------------------------------
 /** @brief Overrun interrupt bit */
 #define ADXL345_INT_SOURCE_OVERRUN 0x01
 /** @brief Watermark interrupt bit */
 #define ADXL345_INT_SOURCE_WATERMARK 0x02
 /** @brief Freefall interrupt bit */
 #define ADXL345_INT_SOURCE_FREEFALL 0x04
 /** @brief Inactivity interrupt bit */
 #define ADXL345_INT_SOURCE_INACTIVITY 0x08
 /** @brief Activity interrupt bit */
 #define ADXL345_INT_SOURCE_ACTIVITY 0x10
 /** @brief Double tap interrupt bit */
 #define ADXL345_INT_SOURCE_DOUBLETAP 0x20
 /** @brief Single tap interrupt bit */
 #define ADXL345_INT_SOURCE_SINGLETAP 0x40
 /** @brief Data ready interrupt bit */
 #define ADXL345_INT_SOURCE_DATAREADY 0x80
 /** @brief FIFO bypass mode setting */
 #define ADXL345_FIFO_BYPASS_MODE 0x00
 
 //------------------------------------------------------------------------------
 // Tap Axis Source Bitmasks
 //------------------------------------------------------------------------------
 /** @brief X-axis tap detection bit */
 #define ADXL345_TAP_SOURCE_X 0x04
 /** @brief Y-axis tap detection bit */
 #define ADXL345_TAP_SOURCE_Y 0x02
 /** @brief Z-axis tap detection bit */
 #define ADXL345_TAP_SOURCE_Z 0x01
 
 //==============================================================================
 // PUBLIC API FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Initializes and configures the ADXL345 accelerometer
  * @return true if initialization successful, false otherwise
  */
 bool initializeADXL345();
 
 /**
  * @brief Clears all pending interrupts by reading the interrupt source register
  */
 void clearInterrupts();
 
 /**
  * @brief Puts the ESP into deep sleep mode
  * 
  * The device will wake up when the ADXL345 detects motion.
  */
 void enterDeepSleep();
 
 /**
  * @brief Calculates the combined magnitude of acceleration across all axes
  * 
  * @param accelX X-axis acceleration in m/s²
  * @param accelY Y-axis acceleration in m/s²
  * @param accelZ Z-axis acceleration in m/s²
  * @return Smoothed, gravity-compensated acceleration magnitude as integer
  */
 int calculateCombinedMagnitude(float accelX, float accelY, float accelZ);
 
 /**
  * @brief Gets the number of samples available in the FIFO buffer
  * @return Number of samples (0-32) or 0 if sensor is disabled
  */
 uint8_t getFifoSampleData();
 
 /**
  * @brief Retrieves current sensor event data
  * @return sensors_event_t structure containing acceleration data
  */
 sensors_event_t getSensorData();
 
 /**
  * @brief Checks if the ADXL345 sensor is enabled
  * @return true if sensor is enabled, false otherwise
  */
 bool isSensorEnabled();
 
 /**
  * @brief Reads a value from a specified register on the ADXL345
  * @param reg Register address to read
  * @return Register value or 0 if sensor is disabled
  */
 uint8_t readRegister(uint8_t reg);
 
 #endif /* ADXL_MODULE_H */