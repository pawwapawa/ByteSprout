/**
 * @file motion_module.cpp
 * @brief Implementation of motion detection and device orientation
 * functionality
 *
 * This module provides functions for detecting and handling different types of
 * motion events including taps, shakes, orientation changes, and inactivity. It
 * manages power states based on device motion and controls display brightness.
 */

#include "common.h"
#include "motion_module.h"
#include "adxl_module.h"
#include "menu_module.h"
#include "display_module.h"
#include "emotes_module.h"
#include "espnow_module.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

/** @brief Storage for current motion states */
static bool g_motionStates[static_cast<size_t>(
    MotionStateType::MOTION_STATE_COUNT)] = {false};

//------------------------------------------------------------------------------
// Sensor Thresholds
//------------------------------------------------------------------------------
/** @brief Threshold for shake detection (m/s²) */
const float SHAKE_THRESHOLD = 8.0;
/** @brief Threshold for inactivity detection (m/s²) */
const float INACTIVITY_THRESHOLD = 1.5;
/** @brief Threshold for tilt detection (m/s²) */
const float TILT_THRESHOLD = 9.0;
/** @brief Threshold for half tilt detection (m/s²) */
const float HALF_TILT_THRESHOLD = 4.2;  // About half of full tilt
/** @brief Threshold for flip detection (m/s²) */
const float FLIP_THRESHOLD = -8;

//------------------------------------------------------------------------------
// Timing Constants
//------------------------------------------------------------------------------
/** @brief Time before entering deep sleep after countdown (ms) */
const unsigned long ENTER_DEEP_SLEEP_TIMER = 20000;
/** @brief Time of inactivity before deep sleep countdown starts (ms) */
const unsigned long INACTIVITY_TIMEOUT = timeToMillis(1, 30);
/** @brief Time of inactivity before dimming display (ms), triggers display dimming for inactivity */
const unsigned long DISPLAY_TIMEOUT = timeToMillis(0, 30);
/** @brief Time of inactivity before entering idle mode (ms), triggers SLEEP animations for idle */
const unsigned long IDLE_TIMEOUT = timeToMillis(1, 00);

//------------------------------------------------------------------------------
// Runtime Variables
//------------------------------------------------------------------------------
/** @brief Timestamp when inactivity started */
unsigned long INACTIVITY_TIME = 0;
/** @brief Timestamp for display timeout tracking */
unsigned long DISPLAY_TIME = 0;
/** @brief Timestamp for idle timeout tracking */
unsigned long IDLE_TIME = 0;

//==============================================================================
// MOTION STATE MANAGEMENT FUNCTIONS
//==============================================================================

/**
 * @brief Set a specific motion state
 *
 * @param state The motion state to set
 * @param value The boolean value to set (true/false)
 */
void setMotionState(MotionStateType state, bool value) {
  g_motionStates[static_cast<size_t>(state)] = value;
  // Clear interrupts after state update
  clearInterrupts();
}

/**
 * @brief Check if a specific motion state is active
 *
 * @param state The motion state to check
 * @return true if the state is active, false otherwise
 */
bool checkMotionState(MotionStateType state) {
  return g_motionStates[static_cast<size_t>(state)];
}

/**
 * @brief Check if any states in a given list are active
 *
 * @param states Array of motion states to check
 * @param count Number of states in the array
 * @return true if any state in the array is active
 */
static bool checkAnyMotionStates(const MotionStateType *states, int count) {
  for (int i = 0; i < count; i++) {
    if (g_motionStates[static_cast<size_t>(states[i])]) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Reset all motion states to inactive
 */
void resetMotionState() {
  // ESP_LOGI log removed
  for (size_t i = 0;
       i < static_cast<size_t>(MotionStateType::MOTION_STATE_COUNT); i++) {
    g_motionStates[i] = false;
  }
}

//==============================================================================
// MOTION STATE ACCESSOR FUNCTIONS
//==============================================================================

/**
 * @brief Check if device was tapped
 * @return true if tapped, false otherwise
 */
bool motionTapped() { return checkMotionState(MotionStateType::TAPPED); }

/**
 * @brief Check if device was double-tapped
 * @return true if double-tapped, false otherwise
 */
bool motionDoubleTapped() {
  return checkMotionState(MotionStateType::DOUBLE_TAPPED);
}

/**
 * @brief Check if device is upside down
 * @return true if upside down, false otherwise
 */
bool motionUpsideDown() {
  return checkMotionState(MotionStateType::UPSIDE_DOWN);
}

/**
 * @brief Check if device is tilted left
 * @return true if tilted left, false otherwise
 */
bool motionTiltedLeft() {
  return checkMotionState(MotionStateType::TILTED_LEFT);
}

/**
 * @brief Check if device is tilted right
 * @return true if tilted right, false otherwise
 */
bool motionTiltedRight() {
  return checkMotionState(MotionStateType::TILTED_RIGHT);
}

/**
 * @brief Check if device is half tilted left
 * @return true if half tilted left, false otherwise
 */
bool motionHalfTiltedLeft() {
  return checkMotionState(MotionStateType::HALF_TILTED_LEFT);
}

/**
 * @brief Check if device is half tilted right
 * @return true if half tilted right, false otherwise
 */
bool motionHalfTiltedRight() {
  return checkMotionState(MotionStateType::HALF_TILTED_RIGHT);
}

/**
 * @brief Check if device has been interacted with (tapped, double-tapped, or
 * shaking)
 * @return true if interacted with, false otherwise
 */
bool motionInteracted() {
  static const MotionStateType interactedStates[] = {
      MotionStateType::SHAKING, MotionStateType::TAPPED,
      MotionStateType::DOUBLE_TAPPED, MotionStateType::SUDDEN_ACCELERATION};
  return checkAnyMotionStates(interactedStates, 4);
}

/**
 * @brief Check if device is in a non-standard orientation
 * @return true if tilted left/right or upside down, false otherwise
 */
bool motionOriented() {
  static const MotionStateType orientedStates[] = {
      MotionStateType::TILTED_LEFT, MotionStateType::TILTED_RIGHT,
      MotionStateType::HALF_TILTED_LEFT, MotionStateType::HALF_TILTED_RIGHT,
      MotionStateType::UPSIDE_DOWN};
  return checkAnyMotionStates(orientedStates, 5);
}

/**
 * @brief Check if device is in sleep mode
 * @return true if in sleep mode, false otherwise
 */
bool motionSleep() { return checkMotionState(MotionStateType::SLEEP); }

/**
 * @brief Check if device is in deep sleep mode
 * @return true if in deep sleep mode, false otherwise
 */
bool motionDeepSleep() { return checkMotionState(MotionStateType::DEEP_SLEEP); }

/**
 * @brief Check if device is being shaken
 * @return true if being shaken, false otherwise
 */
bool motionShaking() { return checkMotionState(MotionStateType::SHAKING); }

//==============================================================================
// DEVICE MODE AND SLEEP FUNCTIONS
//==============================================================================

/**
 * @brief Display the appropriate static image based on device mode
 */
static void checkDeviceModes() {
// This code is determined at runtime
// Use runtime variables set by the preprocessor
#if DEVICE_MODE == MAC_MODE
  deviceMode = "MAC_MODE";
  displayStaticImage(MAC_STATIC, 128, 128);
#elif DEVICE_MODE == PC_MODE
  deviceMode = "PC_MODE";
  displayStaticImage(PC_STATIC, 128, 128);
#else
  deviceMode = "BYTE_MODE";
  displayStaticImage(BYTE_STATIC, 128, 128);
#endif
}

/**
 * @brief Handle entry into deep sleep mode
 *
 * Dims the display, shows the appropriate mode image, and enters deep sleep.
 */
void handleDeepSleep() {
  // Change depending on the device
  setDisplayBrightness(DISPLAY_BRIGHTNESS_DIM);
  // Display mode static image
  checkDeviceModes();
  enterDeepSleep();
}

//==============================================================================
// MOTION DETECTION FUNCTIONS
//==============================================================================

/**
 * @brief Detect tap and double-tap events using the accelerometer
 */
void detectTapping() {
  // Detect tapping interactions and set proper states to relay GIF animations
  if (digitalRead(INTERRUPT_PIN_D1) != HIGH)
    return;

  uint8_t intSource = readRegister(ADXL345_REG_INT_SOURCE);
  uint8_t tapStatus = readRegister(ADXL345_REG_ACT_TAP_STATUS);

  // Handle Z-axis taps 
  if (tapStatus & ADXL345_TAP_SOURCE_Z) {
    if (intSource & ADXL345_INT_SOURCE_DOUBLETAP) {
      return;
    }
    
    if (intSource & ADXL345_INT_SOURCE_SINGLETAP) {
      return;
    }
  }

  // You can handle specific interaction a tap interaction for Y or X axis
  if (tapStatus & ADXL345_TAP_SOURCE_Y) {
    if (intSource & ADXL345_INT_SOURCE_DOUBLETAP) {
      setMotionState(MotionStateType::DOUBLE_TAPPED, true);
      return;
    }
  }

  if (tapStatus & ADXL345_TAP_SOURCE_X) {
    if (intSource & ADXL345_INT_SOURCE_DOUBLETAP) {
      setMotionState(MotionStateType::DOUBLE_TAPPED, true);
      return;
    }

    if (intSource & ADXL345_INT_SOURCE_SINGLETAP) {
      setMotionState(MotionStateType::TAPPED, true);
      return;
    }
  }

  // Handle taps on any axis
  if (intSource & ADXL345_INT_SOURCE_DOUBLETAP) {
    setMotionState(MotionStateType::DOUBLE_TAPPED, true);
    return;
  }

  if (intSource & ADXL345_INT_SOURCE_SINGLETAP) {
    setMotionState(MotionStateType::TAPPED, true);
  }
}

/**
 * @brief Detect sudden acceleration using the accelerometer
 *
 * This function analyzes accelerometer data to detect rapid changes in movement
 * that exceed a specified threshold within a short time window.
 *
 * @param samples Number of samples to analyze
 * @return true if sudden acceleration detected, false otherwise
 */
bool detectSuddenAcceleration(uint8_t samples) {
  // Minimum samples needed for detection
  if (samples < 2)
    return false;

  // Constants
  const float ACCELERATION_THRESHOLD = 6.0; // m/s² - adjust as needed
  const float ACCELERATION_CHANGE_THRESHOLD =
      4.0; // m/s² change between readings

  // Prevent false detection if Tapping was a recent interaction
  static unsigned long accelLockoutTime = 0;
  const unsigned long ACCEL_LOCKOUT_PERIOD = 600; // ms

  if (checkMotionState(MotionStateType::DOUBLE_TAPPED) ||
      checkMotionState(MotionStateType::TAPPED) ||
      checkMotionState(MotionStateType::SHAKING)) {
    accelLockoutTime = millis();
    return false;
  }

  if (millis() - accelLockoutTime < ACCEL_LOCKOUT_PERIOD) {
    return false;
  }

  // Track previous magnitude to detect changes
  static float prevMagnitude = 0;
  float currentMagnitude = 0;

  // Get current acceleration magnitude
  sensors_event_t event = getSensorData();
  currentMagnitude = calculateCombinedMagnitude(
      event.acceleration.x, event.acceleration.y, event.acceleration.z);

  // Calculate change in magnitude
  float magnitudeChange = abs(currentMagnitude - prevMagnitude);

  // Log raw acceleration values for debugging
  // ESP_LOGI(MOTION_LOG, "Accel XYZ: (%.2f, %.2f, %.2f) Mag: %.2f, Change:
  // %.2f",
  //   event.acceleration.x, event.acceleration.y, event.acceleration.z,
  //   currentMagnitude, magnitudeChange);
  // Store current magnitude for next comparison
  prevMagnitude = currentMagnitude;

  // Check if both thresholds are exceeded
  if (currentMagnitude >= ACCELERATION_THRESHOLD &&
      magnitudeChange >= ACCELERATION_CHANGE_THRESHOLD) {
    ESP_LOGI(MOTION_LOG,
             "Sudden acceleration detected! Magnitude: %.2f, Change: %.2f",
             currentMagnitude, magnitudeChange);
    setMotionState(MotionStateType::SUDDEN_ACCELERATION, true);
    return true;
  }

  // Reset motion state if no sudden acceleration
  setMotionState(MotionStateType::SUDDEN_ACCELERATION, false);
  return false;
}

/**
 * @brief Detect shaking motion using the accelerometer
 *
 * @param samples Number of samples to analyze
 */
void detectShakes(uint8_t samples) {
  // Prevent false detection if Tapping was a recent interaction
  static unsigned long tapLockoutTime = 0; // Renamed to avoid shadowing
  const unsigned long TAP_LOCKOUT_PERIOD = 500;

  if (checkMotionState(MotionStateType::TAPPED) ||
      checkMotionState(MotionStateType::DOUBLE_TAPPED)) {
    tapLockoutTime = millis();
    return;
  }

  if (millis() - tapLockoutTime < TAP_LOCKOUT_PERIOD) {
    return;
  }

  float totalMagnitude = 0;
  for (int i = 0; i < samples; i++) {
    sensors_event_t event = getSensorData();
    totalMagnitude += calculateCombinedMagnitude(
        event.acceleration.x, event.acceleration.y, event.acceleration.z);
  }

  float avgMagnitude = totalMagnitude / samples;
  if (avgMagnitude >= SHAKE_THRESHOLD) {
    setMotionState(MotionStateType::SHAKING, true);
  }
}

/**
 * @brief Detect device orientation changes
 *
 * @param samples Number of samples to analyze
 */
void detectOrientation(uint8_t samples) {
  if (samples == 0)
    return;

  float avgX = 0, avgY = 0, avgZ = 0;

  // Take multiple samples to reduce noise
  for (int i = 0; i < samples; i++) {
    sensors_event_t event = getSensorData();
    avgX += event.acceleration.x;
    avgY += event.acceleration.y;
    avgZ += event.acceleration.z;
  }

  avgX /= samples;
  avgY /= samples;
  avgZ /= samples;

  // ESP_LOGI(ADXL_LOG, "Orientation averages - X: %.2f, Y: %.2f, Z: %.2f", avgX, avgY, avgZ); 
  // Reset all orientation states first
  setMotionState(MotionStateType::UPSIDE_DOWN, false);
  setMotionState(MotionStateType::TILTED_LEFT, false);
  setMotionState(MotionStateType::TILTED_RIGHT, false);
  setMotionState(MotionStateType::HALF_TILTED_LEFT, false);
  setMotionState(MotionStateType::HALF_TILTED_RIGHT, false);

  // Check orientation relative to normal Z-axis resting position
  // NOTE: in earlier PCB design, ADXL345 was mounted upside down, so the
  // FLIP_THRESHOLD was inverted
  if (avgZ <= FLIP_THRESHOLD) {
    setMotionState(MotionStateType::UPSIDE_DOWN, true);
  } else if (avgY >= TILT_THRESHOLD) {
    setMotionState(MotionStateType::TILTED_RIGHT, true);
  } else if (avgY <= -TILT_THRESHOLD) {
    setMotionState(MotionStateType::TILTED_LEFT, true);
  } else if (avgY >= HALF_TILT_THRESHOLD && avgY < TILT_THRESHOLD) {
    setMotionState(MotionStateType::HALF_TILTED_RIGHT, true);
    // ESP_LOGI(MOTION_LOG, "HALF_TILTED_RIGHT detected: Y=%.2f (threshold range=%.2f to %.2f)", 
    //   avgY, HALF_TILT_THRESHOLD, TILT_THRESHOLD);
  } else if (avgY <= -HALF_TILT_THRESHOLD && avgY > -TILT_THRESHOLD) {
    setMotionState(MotionStateType::HALF_TILTED_LEFT, true);
    // ESP_LOGI(MOTION_LOG, "HALF_TILTED_LEFT detected: Y=%.2f (threshold range=%.2f to %.2f)", 
    //   avgY, -HALF_TILT_THRESHOLD, -TILT_THRESHOLD);
  }
}

/**
 * @brief Detect lack of movement over time
 *
 * We manually handle Activity and Inactivity because ADXL native detection
 * does not work well with Deep Sleep. This gives us more control on how to
 * handle activity detection.
 *
 * @param samples Number of samples to analyze
 * @return true if device should enter deep sleep
 */
bool detectInactivity(uint8_t samples) {
  if (samples == 0)
    return false;

  float totalMagnitude = 0;
  for (int i = 0; i < samples; i++) {
    sensors_event_t event = getSensorData();
    totalMagnitude += calculateCombinedMagnitude(
        event.acceleration.x, event.acceleration.y, event.acceleration.z);
  }

  float avgMagnitude = totalMagnitude / samples;
  if (avgMagnitude < INACTIVITY_THRESHOLD) {
    if (INACTIVITY_TIME == 0) {
      INACTIVITY_TIME = millis();
    } else if (millis() - INACTIVITY_TIME >= INACTIVITY_TIMEOUT) {
      setMotionState(MotionStateType::DEEP_SLEEP, true);
      return true;
    }
  } else {
    INACTIVITY_TIME = 0;
    // Reset deep sleep state when we start polling again
    setMotionState(MotionStateType::DEEP_SLEEP, false);
  }
  return false;
}

/**
 * @brief Automatically adjust display brightness based on activity
 *
 * @param samples Number of samples to analyze
 */
static void autoDimDisplay(uint8_t samples) {
  if (samples == 0)
    return;

  float totalMagnitude = 0;

  for (int i = 0; i < samples; i++) {
    sensors_event_t event = getSensorData();
    totalMagnitude += calculateCombinedMagnitude(
        event.acceleration.x, event.acceleration.y, event.acceleration.z);
  }

  float avgMagnitude = totalMagnitude / samples;
  // Check for activity to wake display
  if (avgMagnitude > INACTIVITY_THRESHOLD && debounce(DISPLAY_TIME, 200)) {
    setDisplayBrightness(DISPLAY_BRIGHTNESS_FULL);
    DISPLAY_TIME = millis();
    setMotionState(MotionStateType::SLEEP, false);
    return;
  }
  // Check for display timeout
  if (millis() - DISPLAY_TIME >= DISPLAY_TIMEOUT) {
    // Display timeout is set to dim display if inactivity is detected for an
    // amount of time. Sleep state and Deep sleep are two different states, this
    // is more of idling inactivity. Only transition to sleep if we're not
    // already sleeping
    if (!checkMotionState(MotionStateType::SLEEP) &&
        setTimeout(IDLE_TIME, IDLE_TIMEOUT)) {
      setDisplayBrightness(DISPLAY_BRIGHTNESS_LOW);
      setMotionState(MotionStateType::SLEEP, true);
    }
  }
}

/**
 * @brief Monitor and handle sleep state transitions
 *
 * @param samples Number of samples to analyze
 */
static void monitorSleep(uint8_t samples) {
  static unsigned long lastInactivityTime = 0;

  if (detectInactivity(samples)) {
    if (lastInactivityTime == 0) {
      lastInactivityTime = millis();
    }

    if (setTimeout(lastInactivityTime, ENTER_DEEP_SLEEP_TIMER)) {
      // ESP_LOGI log removed
      handleDeepSleep();
    }
  } else {
    if (lastInactivityTime != 0) {
      lastInactivityTime = 0;
    }
  }
}

/**
 * @brief Check if device detected sudden acceleration
 * @return true if sudden acceleration detected, false otherwise
 */
bool motionSuddenAcceleration() {
  return checkMotionState(MotionStateType::SUDDEN_ACCELERATION);
}

//==============================================================================
// MAIN POLLING FUNCTION
//==============================================================================

/**
 * @brief Main function to poll accelerometer data and process all motion events
 *
 * NOTE: ADXL Data polling needs to be handled within the playGIF loop because
 * it is a blocking loop. We need to constantly poll ADXL data to handle
 * interactions.
 */
void ADXLDataPolling() {
  // Poll button events first
  // handleClickEvents();
  menu_update();

  if (!isSensorEnabled())
    return;
  // Read FIFO status once for all detections
  uint8_t samplesAvailable = getFifoSampleData();
  if (samplesAvailable == 0)
    return;
  // Add orientation detection to the processing pipeline
  // Process in order of priority
  detectShakes(samplesAvailable);
  if (!checkMotionState(MotionStateType::SHAKING)) {
    detectTapping();
    detectInactivity(samplesAvailable);
  }
  // IMPORTANT: Order of interaction is important here, tap detection gets
  // interrupted if not the first check priority
  // Shaking detection is the highest priority, so it should be checked first
  // Tapping detection is next, but it should not interrupt shaking detection
  // Acceleration and Orientation detection should be checked last
  // This is to prevent false positives when the device is in motion
  detectSuddenAcceleration(samplesAvailable);
  detectOrientation(samplesAvailable);
  monitorSleep(samplesAvailable);
  autoDimDisplay(samplesAvailable);
}