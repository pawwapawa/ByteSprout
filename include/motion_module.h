/**
 * @file motion_module.h
 * @brief Header for motion detection and device orientation functionality
 * 
 * This module provides functions for detecting and handling different types of motion
 * events including taps, shakes, orientation changes, and inactivity. It manages
 * power states based on device motion and controls display brightness.
 */

 #ifndef MOTION_MODULE_H
 #define MOTION_MODULE_H
 
 #include "adxl_module.h"
 #include "common.h"
 
 //==============================================================================
 // CONSTANTS & DEFINITIONS
 //==============================================================================
 
 /** @brief Log tag for Motion module messages */
 static const char *MOTION_LOG = "::MOTION_MODULE::";
 
 //==============================================================================
 // TYPE DEFINITIONS
 //==============================================================================
 
 /**
  * @brief Types of motion states that can be detected and tracked
  */
 enum class MotionStateType {
   SHAKING = 0,       /**< Device is being shaken */
   TAPPED,            /**< Device received a single tap */
   DOUBLE_TAPPED,     /**< Device received a double tap */
   SLEEP,             /**< Device is in sleep mode (display dimmed) */
   DEEP_SLEEP,        /**< Device is in deep sleep mode (power saving) */
   UPSIDE_DOWN,       /**< Device is flipped upside down */
   TILTED_LEFT,       /**< Device is tilted to the left */
   TILTED_RIGHT,      /**< Device is tilted to the right */
   HALF_TILTED_LEFT,  /**< Device is tilted to the left at 45 degree */
   HALF_TILTED_RIGHT, /**< Device is tilted to the right at 45 degree */
   SUDDEN_ACCELERATION, /**< Device detected sudden acceleration */
   // Keep track of the total number of states
   MOTION_STATE_COUNT /**< Total count of motion states (for array sizing) */
 };
 
 //==============================================================================
 // MOTION DETECTION FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Main function to poll accelerometer data and process all motion events
  */
 void ADXLDataPolling(void);
 /**
 * @brief Check if device detected sudden acceleration
 * @return true if sudden acceleration detected, false otherwise
 */
 bool motionSuddenAcceleration();
 /**
  * @brief Detect shaking motion using the accelerometer
  * 
  * @param samples Number of samples to analyze
  */
 void detectShakes(uint8_t samples);
 
 /**
  * @brief Detect tap and double-tap events using the accelerometer
  */
 void detectTapping(void);
 
 /**
  * @brief Detect device orientation changes
  * 
  * @param samples Number of samples to analyze
  */
 void detectOrientation(uint8_t samples);
 
 /**
  * @brief Detect lack of movement over time
  * 
  * @param samples Number of samples to analyze
  * @return true if device should enter deep sleep
  */
 bool detectInactivity(uint8_t samples);
 
 /**
  * @brief Handle entry into deep sleep mode
  * 
  * Dims the display, shows the appropriate mode image, and enters deep sleep.
  */
 void handleDeepSleep(void);
 
 //==============================================================================
 // MOTION STATE MANAGEMENT FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Set a specific motion state
  * 
  * @param state The motion state to set
  * @param value The boolean value to set (true/false)
  */
 void setMotionState(MotionStateType state, bool value);
 
 /**
  * @brief Check if a specific motion state is active
  * 
  * @param state The motion state to check
  * @return true if the state is active, false otherwise
  */
 bool checkMotionState(MotionStateType state);
 
 /**
  * @brief Reset all motion states to inactive
  */
 void resetMotionState();
 
 //==============================================================================
 // MOTION STATE ACCESSOR FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Check if device was tapped
  * @return true if tapped, false otherwise
  */
 bool motionTapped();
 
 /**
  * @brief Check if device was double-tapped
  * @return true if double-tapped, false otherwise
  */
 bool motionDoubleTapped();
 
 /**
  * @brief Check if device is upside down
  * @return true if upside down, false otherwise
  */
 bool motionUpsideDown();
 
 /**
  * @brief Check if device is tilted left
  * @return true if tilted left, false otherwise
  */
 bool motionTiltedLeft();
 
 /**
  * @brief Check if device is tilted right
  * @return true if tilted right, false otherwise
  */
 bool motionTiltedRight();

 /**
 * @brief Check if device is half tilted left
 * @return true if half tilted left, false otherwise
 */
bool motionHalfTiltedLeft();

/**
 * @brief Check if device is half tilted right
 * @return true if half tilted right, false otherwise
 */
bool motionHalfTiltedRight();
 
 /**
  * @brief Check if device has been interacted with (tapped, double-tapped, or shaking)
  * @return true if interacted with, false otherwise
  */
 bool motionInteracted();
 
 /**
  * @brief Check if device is in a non-standard orientation
  * @return true if tilted left/right or upside down, false otherwise
  */
 bool motionOriented();
 
 /**
  * @brief Check if device is in sleep mode
  * @return true if in sleep mode, false otherwise
  */
 bool motionSleep();
 
 /**
  * @brief Check if device is in deep sleep mode
  * @return true if in deep sleep mode, false otherwise
  */
 bool motionDeepSleep();
 
 /**
  * @brief Check if device is being shaken
  * @return true if being shaken, false otherwise
  */
 bool motionShaking();
 
 #endif /* MOTION_MODULE_H */