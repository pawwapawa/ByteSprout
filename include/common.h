/**
 * @file common.h
 * @brief Common definitions and utility functions
 * 
 * This header provides common includes, constants, and utility functions
 * that are used across multiple modules in the application.
 */

 #ifndef COMMON_H
 #define COMMON_H
 
 //==============================================================================
 // INCLUDES
 //==============================================================================
 
 #include <Arduino.h>
 #include <ESP_log.h>
 #include <SPI.h>
 #include <Wire.h>
 
 //------------------------------------------------------------------------------
 // Device Mode Definitions
 //------------------------------------------------------------------------------
 /** @brief Current operational mode of the device */
 #define DEVICE_MODE BYTE_MODE
 /** @brief MAC address operation mode */
 #define MAC_MODE 1
 /** @brief PC communication mode */
 #define PC_MODE 2
 /** @brief Byte/raw data mode */
 #define BYTE_MODE 3
 
 //==============================================================================
 // UTILITY FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Timer utility function to check if a specified time has elapsed
  * 
  * This function checks if the specified delay time has passed since the last
  * set time. If the time has elapsed, it updates the set time to the current time.
  * 
  * @param setTime Reference to the timestamp variable to check and update
  * @param delayTime Time period in milliseconds to check against
  * @return true if the specified time has elapsed, false otherwise
  */
 bool setTimeout(unsigned long &setTime, unsigned long delayTime);
 
 /**
  * @brief Debounce utility function for handling input debouncing
  * 
  * Similar to setTimeout, but specifically named for the debouncing use case.
  * Used to prevent multiple triggers from inputs (like buttons) due to signal bounce.
  * 
  * @param lastTime Reference to the timestamp variable to check and update
  * @param delay Debounce time period in milliseconds
  * @return true if the debounce period has elapsed, false otherwise
  */
 bool debounce(unsigned long &lastTime, unsigned long delay);
 
 /**
  * @brief Convert hours and minutes to milliseconds
  * 
  * Utility function to convert a time specified in hours and minutes
  * to its equivalent in milliseconds, useful for timing operations.
  * 
  * @param hours Number of hours
  * @param minutes Number of minutes
  * @return Total time in milliseconds
  */
 unsigned long timeToMillis(int hours, int minutes);
 
 #endif /* COMMON_H */