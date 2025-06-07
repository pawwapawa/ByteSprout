/**
 * @file common.cpp
 * @brief Implementation of common utility functions
 * 
 * This file contains the implementation of utility functions
 * that are used across multiple modules in the application.
 */

 #include "common.h"

 //==============================================================================
 // UTILITY FUNCTIONS IMPLEMENTATION
 //==============================================================================
 
 /**
  * @brief Timer utility function to check if a specified time has elapsed
  * 
  * @param setTime Reference to the timestamp variable to check and update
  * @param delayTime Time period in milliseconds to check against
  * @return true if the specified time has elapsed, false otherwise
  */
 bool setTimeout(unsigned long &setTime, unsigned long delayTime) {
   unsigned long currentTime = millis();
   if (currentTime - setTime >= delayTime) {
     setTime = currentTime;
     return true;
   }
   return false;
 }
 
 /**
  * @brief Debounce utility function for handling input debouncing
  * 
  * @param lastTime Reference to the timestamp variable to check and update
  * @param delay Debounce time period in milliseconds
  * @return true if the debounce period has elapsed, false otherwise
  */
 bool debounce(unsigned long &lastTime, unsigned long delay) {
   unsigned long currentTime = millis();
   if (currentTime - lastTime >= delay) {
     lastTime = currentTime;
     return true;
   }
   return false;
 }
 
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
 unsigned long timeToMillis(int hours, int minutes) {
   // Convert hours and minutes to milliseconds
   unsigned long totalMinutes = (hours * 60) + minutes;
   return totalMinutes * 60 * 1000; // Convert to milliseconds
 }