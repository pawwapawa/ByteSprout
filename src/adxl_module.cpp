/**
 * @file adxl_module.cpp
 * @brief Implementation of the ADXL345 accelerometer module functionality
 *
 * This module provides functions for initializing, configuring, and interacting
 * with the ADXL345 accelerometer. It handles sensor initialization, register
 * operations, interrupt management, and deep sleep functionality.
 */

#include "adxl_module.h"
#include "common.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

/** @brief ADXL345 accelerometer instance with I2C address 12345 */
static Adafruit_ADXL345_Unified adxl = Adafruit_ADXL345_Unified(12345);
/** @brief Storage for sensor event data */
static sensors_event_t event;
/** @brief Flag indicating if ADXL345 is properly initialized and enabled */
static bool ADXL345Enabled = false;

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Calculates scaled threshold value for G-force
 * @param gforce G-force value to scale
 * @return Scaled 8-bit threshold value (0-255)
 */
static uint8_t calcGforce(float gforce) {
  uint8_t threshold =
      min((uint8_t)(gforce * 1000 / FORCE_SCALE_FACTOR), (uint8_t)255);
  return threshold;
};

/**
 * @brief Calculates scaled duration value for tap detection
 * @param durationMs Duration in milliseconds
 * @return Scaled 8-bit duration value (0-255)
 */
static uint8_t calcDuration(float durationMs) {
  uint8_t duration =
      min((uint8_t)(durationMs / DURATION_SCALE_FACTOR), (uint8_t)255);
  return duration;
};

/**
 * @brief Calculates scaled latency value for tap detection
 * @param latencyMs Latency in milliseconds
 * @return Scaled 8-bit latency value (0-255)
 */
static uint8_t calcLatency(float latencyMs) {
  uint8_t duration =
      min((uint8_t)(latencyMs / LATENCY_SCALE_FACTOR), (uint8_t)255);
  return duration;
};

/**
 * @brief Attempts to initialize the ADXL345 sensor with multiple retries
 * @param attempts Maximum number of initialization attempts (default: 3)
 * @return true if initialization successful, false otherwise
 */
static bool retrySensorInit(uint8_t attempts = 3) {
  uint8_t retriesLeft = attempts;
  // Initialize I2C on the specified pins
  Wire.begin(SDA_PIN_D4, SCL_PIN_D5);

  while (retriesLeft--) {
    // Check I2C communication
    Wire.beginTransmission(ADXL345_DEFAULT_ADDRESS);
    byte error = Wire.endTransmission();
    if (error != 0) {
      ESP_LOGE(ADXL_LOG,
               "I2C communication failed. Error on pins SDA=%d, SCL=%d, error code=%d, %d retries left",
               SDA_PIN_D4, SCL_PIN_D5, error, retriesLeft);
      
      // Try the alternative address in case the wrong address is being used
      Wire.beginTransmission(error == 2 ? (ADXL345_DEFAULT_ADDRESS == 0x1D ? 0x53 : 0x1D) : ADXL345_DEFAULT_ADDRESS);
      error = Wire.endTransmission();
      if (error == 0) {
        ESP_LOGW(ADXL_LOG, "Device found at alternative address, check your wiring configuration");
      }
      if (retriesLeft > 0) {
        delay(500);
        continue; // Skip to next retry attempt
      } else {
        return false;
      }
    }

    // Configure interrupt pin
    pinMode(INTERRUPT_PIN_D1, INPUT);

    // Try to initialize the ADXL345 sensor
    if (adxl.begin()) {
      // Verify the sensor ID if possible
      sensors_event_t event;
      adxl.getEvent(&event);
      // Log the initial readings to verify sensor is active
      ESP_LOGI(ADXL_LOG, "Initial sensor readings - X: %.2f, Y: %.2f, Z: %.2f m/s^2", 
        event.acceleration.x, event.acceleration.y, event.acceleration.z);
      uint8_t intSource = adxl.readRegister(ADXL345_REG_INT_SOURCE);
      ESP_LOGI(ADXL_LOG, "Cleared interrupt flags: 0x%02X", intSource);

      ESP_LOGI(ADXL_LOG, "ADXL345 sensor initialized successfully");
      return true;
    }

    ESP_LOGW(ADXL_LOG, "ADXL345 begin() failed, %d retries left", retriesLeft);
    delay(500);
  }

  ESP_LOGE(ADXL_LOG, "ADXL345 initialization failed after %d attempts",
           attempts);
  return false;
}

/**
 * @brief Writes a value to a specified register on the ADXL345
 * @param reg Register address
 * @param value Value to write
 */
static void writeRegister(uint8_t reg, uint8_t value) {
  if (!ADXL345Enabled)
    return;
  adxl.writeRegister(reg, value);
}

/**
 * @brief Configures ESP deep sleep mode with ADXL345 as wake-up source
 *
 * Sets up the ESP to wake from deep sleep when the ADXL345 generates an
 * interrupt. Configures necessary power domains and GPIO pins for interrupt
 * detection.
 */
static void configureESPDeepSleep() {
  // Check wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    // ESP_LOGI log removed
  }

  clearInterrupts();
  // Configure power domains
  // ADXL requires RTC peripherals to be powered on during deep sleep to detect
  // interrupts
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  // NOTE: S3 requires esp_sleep_enabled_ext0_wakeup, ext1 gets triggers for
  // unknown reasons C6 does not support ext0, commented out line is for C6
  // support.
  esp_sleep_enable_ext0_wakeup((gpio_num_t)INTERRUPT_PIN_D1, HIGH);
  // esp_sleep_enable_ext1_wakeup(INT_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Checks if the ADXL345 sensor is enabled
 * @return true if sensor is enabled, false otherwise
 */
bool isSensorEnabled() { return ADXL345Enabled; }

/**
 * @brief Retrieves current sensor event data
 * @return sensors_event_t structure containing acceleration data
 */
sensors_event_t getSensorData() {
  adxl.getEvent(&event);
  return event;
}

/**
 * @brief Reads a value from a specified register on the ADXL345
 * @param reg Register address to read
 * @return Register value or 0 if sensor is disabled
 */
uint8_t readRegister(uint8_t reg) {
  if (!ADXL345Enabled) {
    ESP_LOGW(ADXL_LOG,
             "WARNING: Attempted to read register while sensor disabled");
    return 0;
  }
  return adxl.readRegister(reg);
}

/**
 * @brief Clears all pending interrupts by reading the interrupt source register
 */
void clearInterrupts() {
  if (!ADXL345Enabled)
    return;
  uint8_t interruptSource = adxl.readRegister(ADXL345_REG_INT_SOURCE);
  adxl.readRegister(ADXL345_REG_INT_SOURCE);
}

/**
 * @brief Calculates the combined magnitude of acceleration across all axes
 *
 * Applies smoothing, removes gravity component, and focuses on dynamic
 * acceleration.
 *
 * @param accelX X-axis acceleration in m/s²
 * @param accelY Y-axis acceleration in m/s²
 * @param accelZ Z-axis acceleration in m/s²
 * @return Smoothed, gravity-compensated acceleration magnitude as integer
 */
int calculateCombinedMagnitude(float accelX, float accelY, float accelZ) {
  if (!ADXL345Enabled)
    return 0;
  static float smoothedMagnitude = 0;
  static const float SMOOTHING_FACTOR = 0.1;
  // Calculate the raw acceleration magnitude
  float rawAccelMagnitude = sqrt(sq(accelX) + sq(accelY) + sq(accelZ));
  // Subtract gravity to focus on dynamic acceleration
  float dynamicAccelMagnitude = abs(rawAccelMagnitude - SENSORS_GRAVITY_EARTH);
  smoothedMagnitude = (SMOOTHING_FACTOR * dynamicAccelMagnitude) +
                      ((1 - SMOOTHING_FACTOR) * smoothedMagnitude);

  return (int)round(smoothedMagnitude);
}

/**
 * @brief Puts the ESP into deep sleep mode
 *
 * Clears any pending interrupts and initiates deep sleep.
 * The device will wake up when the ADXL345 detects motion.
 */
void enterDeepSleep() {
  if (!ADXL345Enabled)
    return;

  // ESP_LOGI log removed
  clearInterrupts();
  delay(100);
  esp_deep_sleep_start();
}

/**
 * @brief Initializes and configures the ADXL345 accelerometer
 *
 * Sets up I2C communication, configures sensor parameters, tap detection
 * settings, interrupt mapping, and deep sleep support.
 *
 * @return true if initialization successful, false otherwise
 */
bool initializeADXL345() {

  if (!retrySensorInit()) {
    ESP_LOGE(ADXL_LOG, "ERROR: ADXL345 initialization failed");
    ADXL345Enabled = false;
    return false;
  }

  // Setup ADXL345 module
  ADXL345Enabled = true;
  adxl.setRange(ADXL345_RANGE_16_G);
  adxl.setDataRate(ADXL345_DATARATE_100_HZ);
  // Setup interuptions and tap/double tap sensitivity
  adxl.writeRegister(ADXL345_REG_INT_ENABLE, 0x00);
  adxl.writeRegister(ADXL345_REG_THRESH_TAP, calcGforce(14.0));
  adxl.writeRegister(ADXL345_REG_DUR, calcDuration(30.0));
  adxl.writeRegister(ADXL345_REG_LATENT, calcLatency(100.0));
  adxl.writeRegister(ADXL345_REG_WINDOW, calcLatency(250.0));
  // Enable X, Y, Z axes for tap detection
  adxl.writeRegister(ADXL345_REG_TAP_AXES, 0x0F);
  // Map interrupts - Set to 0x00 to map both single and double taps to INT1
  adxl.writeRegister(ADXL345_REG_INT_MAP, 0x00);
  // Enable only single and double tap interrupts (0x60 = 0b01100000)
  adxl.writeRegister(ADXL345_REG_INT_ENABLE, 0x60);
  // Add FIFO configuration after your other settings
  // Configure for stream mode with 16 samples
  adxl.writeRegister(ADXL345_REG_FIFO_CTL, 0x80 | 0x10);
  // Clear any existing interrupts by reading INT_SOURCE
  clearInterrupts();
  configureESPDeepSleep();

  // ESP_LOGI log removed
  return true;
}

/**
 * @brief Gets the number of samples available in the FIFO buffer
 * @return Number of samples (0-32) or 0 if sensor is disabled
 */
uint8_t getFifoSampleData() {
  if (!ADXL345Enabled)
    return 0;
  // Read the FIFO status register
  uint8_t fifoStatus = adxl.readRegister(ADXL345_REG_FIFO_STATUS);
  // Extract the number of samples available (lower 6 bits)
  uint8_t samplesAvailable = fifoStatus & 0x3F;
  return samplesAvailable;
}