/**
 * @file effects_module.cpp
 * @brief Implementation of visual effects and retro display aesthetics functionality
 * 
 * This module provides visual effects for display content including white pixel
 * tinting, CRT scanline effects, Bayer dithering, and horizontal jitter glitches.
 * Effects can be applied individually or combined for authentic retro display aesthetics.
 * 
 * Optimized for real-time processing on embedded systems with RGB565 pixel format.
 */

#include "effects_module.h"

//==============================================================================
// MODULE STATE VARIABLES
//==============================================================================

//------------------------------------------------------------------------------
// White Tinting State
//------------------------------------------------------------------------------
static uint16_t whiteTintColor = TINT_NONE;            /**< Current tint color in RGB565 format */
static float whiteTintIntensity = 0.0f;                /**< Tint replacement intensity (0.0-1.0) */
static float whiteThreshold = 0.7f;                    /**< Brightness threshold for white detection */
static bool whiteTintEnabled = false;                  /**< Whether white tinting is currently active */

//------------------------------------------------------------------------------
// Scanline Effect State
//------------------------------------------------------------------------------
static ScanlineMode currentScanlineMode = SCANLINE_NONE;   /**< Current scanline effect mode */
static float scanlineIntensity = 0.3f;                     /**< Strength of scanline darkening effect */
static float scanlineSpeed = 2.0f;                         /**< Animation speed in lines per second */
static unsigned long scanlineStartTime = 0;                /**< Animation start time for timing calculations */
static bool scanlineAnimationEnabled = false;              /**< Whether scanline animation is active */

//------------------------------------------------------------------------------
// Glitch Effect State
//------------------------------------------------------------------------------
static GlitchMode currentGlitchMode = GLITCH_NONE;     /**< Current glitch effect intensity level */
static float glitchProbability = 0.03f;                /**< Base chance of glitch per scanline */
static unsigned long glitchSeed = 0;                   /**< Random seed for glitch generation */

//------------------------------------------------------------------------------
// Effect Cycling State
//------------------------------------------------------------------------------
static EffectCycleState currentEffectState = EFFECT_STATE_NONE;    /**< Current position in effect cycle */
static unsigned long lastEffectCycleTime = 0;                      /**< Last time effect was cycled for debouncing */
static unsigned long lastGlitchToggleTime = 0;                     /**< Last time glitch was toggled for debouncing */

//------------------------------------------------------------------------------
// Saved Glitch Settings for Toggle Functionality
//------------------------------------------------------------------------------
static GlitchMode savedGlitchMode = GLITCH_HEAVY;      /**< Saved glitch mode for toggle restoration */
static float savedGlitchProbability = 0.08f;           /**< Saved glitch probability for toggle restoration */
static bool glitchesCurrentlyEnabled = false;          /**< Current glitch enable state */

//------------------------------------------------------------------------------
// Bayer Dithering State
//------------------------------------------------------------------------------
static DitherMode currentDitherMode = DITHER_NONE;     /**< Current dithering pattern type */
static float ditherIntensity = 0.5f;                   /**< Strength of dithering effect */
static int ditherQuantization = 4;                     /**< Color quantization levels for retro effect */

//==============================================================================
// CONSTANTS & LOOKUP TABLES
//==============================================================================

//------------------------------------------------------------------------------
// Bayer Dithering Matrices
//------------------------------------------------------------------------------
/** @brief 2x2 Bayer dithering matrix for simple patterns */
static const int bayer2x2[2][2] = {{0, 2}, {3, 1}};

/** @brief 4x4 Bayer dithering matrix for medium complexity patterns */
static const int bayer4x4[4][4] = {
    {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

/** @brief 8x8 Bayer dithering matrix for high complexity patterns */
static const int bayer8x8[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42},  {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38}, {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41},  {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37}, {63, 31, 55, 23, 61, 29, 53, 21}};

//------------------------------------------------------------------------------
// Effect Names for Menu Integration
//------------------------------------------------------------------------------
/** @brief Human-readable effect names for menu display and debugging */
static const char* EFFECT_NAMES[] = {
    "NONE",             /**< No effects applied */
    "SCANLINES",        /**< CRT scanlines only */
    "DITHERING",        /**< Bayer dithering only */
    "GREEN TINT",       /**< Green terminal tint with scanlines */
    "YELLOW TINT",      /**< Yellow terminal tint with scanlines */
    "DITHER+GREEN",     /**< Dithering combined with green tint */
    "DITHER+YELLOW"     /**< Dithering combined with yellow tint */
};

//==============================================================================
// INTERNAL HELPER FUNCTIONS
//==============================================================================

/**
 * @brief Fast random number generator for glitch effects
 * 
 * Uses linear congruential generator for fast pseudo-random numbers
 * suitable for real-time glitch effect generation.
 * 
 * @return Pseudo-random number between 0 and 32767
 */
static uint16_t fastRandom() {
  glitchSeed = glitchSeed * 1103515245 + 12345;
  return (glitchSeed >> 16) & 0x7FFF;
}

/**
 * @brief Get Bayer threshold value for given coordinates
 * 
 * Retrieves the normalized threshold value from the appropriate Bayer matrix
 * based on the current dithering mode and pixel coordinates.
 * 
 * @param x X coordinate of pixel
 * @param y Y coordinate of pixel
 * @return Normalized threshold value (0.0-1.0)
 */
static float getBayerThreshold(int x, int y) {
  int threshold = 0;
  int maxValue = 0;

  switch (currentDitherMode) {
  case DITHER_2X2:
    threshold = bayer2x2[y % 2][x % 2];
    maxValue = 3; // 2x2 - 1
    break;

  case DITHER_4X4:
    threshold = bayer4x4[y % 4][x % 4];
    maxValue = 15; // 4x4 - 1
    break;

  case DITHER_8X8:
    threshold = bayer8x8[y % 8][x % 8];
    maxValue = 63; // 8x8 - 1
    break;

  default:
    return 0.0f;
  }

  return (float)threshold / (float)maxValue;
}

/**
 * @brief Quantize a color component to reduce color depth
 * 
 * Reduces the number of available color levels to create retro-style
 * reduced color palettes characteristic of older computer systems.
 * 
 * @param value Original color component
 * @param maxValue Maximum value for this component
 * @param levels Number of quantization levels
 * @return Quantized color component
 */
static uint8_t quantizeColor(uint8_t value, uint8_t maxValue, int levels) {
  if (levels <= 1)
    return 0;
  if (levels >= maxValue)
    return value;

  float normalized = (float)value / (float)maxValue;
  int quantized = (int)(normalized * (levels - 1) + 0.5f);
  return (uint8_t)((quantized * maxValue) / (levels - 1));
}

/**
 * @brief Initialize scanline animation timing
 * 
 * Sets up timing for animated scanline effects by recording the
 * current time and enabling the animation system.
 */
static void initScanlineAnimation() {
  scanlineStartTime = millis();
  scanlineAnimationEnabled = true;
}

/**
 * @brief Calculate animated scanline offset based on time
 * 
 * Computes the current scanline offset for animated effects based on
 * elapsed time and configured animation speed.
 * 
 * @return Current scanline offset in pixels
 */
static int getAnimatedScanlineOffset() {
  if (!scanlineAnimationEnabled) {
    return 0;
  }

  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - scanlineStartTime;

  // Calculate pixels moved based on speed (lines per second)
  float pixelsPerMs = scanlineSpeed / 1000.0f;
  int totalOffset = (int)(elapsed * pixelsPerMs);

  // Wrap around every 2 pixels to create repeating pattern (matches CLASSIC)
  return totalOffset % 2;
}

/**
 * @brief Apply horizontal jitter to pixel array for glitch effects
 * 
 * Shifts pixels horizontally with wrapping to simulate CRT sync issues
 * and interference for authentic retro display glitch effects.
 * 
 * @param pixels Array of RGB565 pixels for current scanline
 * @param width Number of pixels in the scanline
 * @param intensity Maximum shift amount in pixels
 */
static void applyHorizontalJitter(uint16_t *pixels, int width, int intensity) {
  if (intensity <= 0)
    return;

  // Random shift amount (-intensity to +intensity)
  int shift = (fastRandom() % (intensity * 2 + 1)) - intensity;
  if (shift == 0)
    return;

  // Create temporary buffer for shifted pixels
  static uint16_t tempBuffer[128]; // Max display width

  // Copy pixels with wrapping
  for (int i = 0; i < width; i++) {
    int sourceIndex = i - shift;

    // Wrap around if needed
    while (sourceIndex < 0)
      sourceIndex += width;
    while (sourceIndex >= width)
      sourceIndex -= width;

    tempBuffer[i] = pixels[sourceIndex];
  }

  // Copy back to original array
  for (int i = 0; i < width; i++) {
    pixels[i] = tempBuffer[i];
  }
}

/**
 * @brief Apply current visual effect state configuration
 * 
 * Configures all visual effects based on the current effect cycle state,
 * disabling conflicting effects and applying the appropriate combination
 * of tinting, scanlines, and dithering.
 */
static void applyCurrentVisualEffects(void) {
  // Disable visual effects (but not glitches)
  disableWhiteTint();
  disableScanlineEffect();
  disableBayerDithering();

  switch (currentEffectState) {
  case EFFECT_STATE_NONE:
    ESP_LOGI(EFFECTS_LOG, "Visual Effects: None");
    break;

  case EFFECT_STATE_SCANLINE:
    setScanlineEffect(SCANLINE_CLASSIC, 0.5f, 2.0f);
    ESP_LOGI(EFFECTS_LOG, "Visual Effects: Scanlines");
    break;

  case EFFECT_STATE_DITHER:
    setBayerDithering(DITHER_8X8, 2.0f, 2);
    ESP_LOGI(EFFECTS_LOG, "Visual Effects: Bayer dithering");
    break;

  case EFFECT_STATE_GREEN_TINT:
    setWhiteTintWithScanlines(TINT_GREEN, 1.0f, 0.7f, SCANLINE_CLASSIC, 0.6f, 2.0f);
    ESP_LOGI(EFFECTS_LOG, "Visual Effects: Green tint + scanlines");
    break;

  case EFFECT_STATE_YELLOW_TINT:
    setWhiteTintWithScanlines(TINT_YELLOW, 1.0f, 0.7f, SCANLINE_CLASSIC, 0.6f, 2.0f);
    ESP_LOGI(EFFECTS_LOG, "Visual Effects: Yellow tint + scanlines");
    break;

  case EFFECT_STATE_DITHER_GREEN:
    setBayerDithering(DITHER_8X8, 2.0f, 2);
    setWhiteTint(TINT_GREEN, 1.0f, 0.7f);
    ESP_LOGI(EFFECTS_LOG, "Visual Effects: Green tint + dithering");
    break;

  case EFFECT_STATE_DITHER_YELLOW:
    setBayerDithering(DITHER_8X8, 2.0f, 2);
    setWhiteTint(TINT_YELLOW, 1.0f, 0.7f);
    ESP_LOGI(EFFECTS_LOG, "Visual Effects: Yellow tint + dithering");
    break;

  default:
    currentEffectState = EFFECT_STATE_NONE;
    break;
  }

  // Restore glitches if they should be enabled
  if (glitchesCurrentlyEnabled) {
    enableCRTGlitches(savedGlitchMode, savedGlitchProbability);
  }
}

//==============================================================================
// LOW-LEVEL PIXEL PROCESSING
//==============================================================================

/**
 * @brief Apply Bayer dithering to a single pixel
 * 
 * Applies the configured Bayer dithering pattern to a single pixel,
 * reducing color depth and creating retro computer graphics aesthetics.
 * 
 * @param pixel Original RGB565 pixel
 * @param x X coordinate of pixel
 * @param y Y coordinate of pixel
 * @param intensity Dithering effect strength (0.0-1.0)
 * @param quantization Number of color levels per component
 * @return Dithered RGB565 pixel
 */
static uint16_t applyBayerDithering(uint16_t pixel, int x, int y,
                                    float intensity, int quantization) {
  if (currentDitherMode == DITHER_NONE || intensity <= 0.0f) {
    return pixel;
  }

  // Extract RGB components from RGB565
  uint8_t r = (pixel >> 11) & 0x1F;
  uint8_t g = (pixel >> 5) & 0x3F;
  uint8_t b = pixel & 0x1F;

  // Get Bayer threshold for this pixel position
  float threshold = getBayerThreshold(x, y);

  // Apply dithering with intensity control
  float ditherOffset = (threshold - 0.5f) * intensity;

  // Normalize to 0-1 range for processing
  float r_norm = (float)r / 31.0f;
  float g_norm = (float)g / 63.0f;
  float b_norm = (float)b / 31.0f;

  // Apply dither offset
  r_norm += ditherOffset * (1.0f / 31.0f) * 8.0f;  // Scale for 5-bit
  g_norm += ditherOffset * (1.0f / 63.0f) * 16.0f; // Scale for 6-bit
  b_norm += ditherOffset * (1.0f / 31.0f) * 8.0f;  // Scale for 5-bit

  // Clamp to valid range
  r_norm = (r_norm > 1.0f) ? 1.0f : ((r_norm < 0.0f) ? 0.0f : r_norm);
  g_norm = (g_norm > 1.0f) ? 1.0f : ((g_norm < 0.0f) ? 0.0f : g_norm);
  b_norm = (b_norm > 1.0f) ? 1.0f : ((b_norm < 0.0f) ? 0.0f : b_norm);

  // Convert back to component values
  r = (uint8_t)(r_norm * 31.0f);
  g = (uint8_t)(g_norm * 63.0f);
  b = (uint8_t)(b_norm * 31.0f);

  // Apply color quantization for retro effect
  r = quantizeColor(r, 31, quantization);
  g = quantizeColor(g, 63, quantization * 2); // Green gets more levels
  b = quantizeColor(b, 31, quantization);

  // Reconstruct RGB565 pixel
  return (r << 11) | (g << 5) | b;
}

/**
 * @brief Apply animated CRT scanline effect to a pixel
 * 
 * Applies scanline darkening effects with optional animation to simulate
 * vintage CRT monitor appearance and phosphor characteristics.
 * 
 * @param pixel Original RGB565 pixel
 * @param row Current row (Y coordinate)
 * @param mode Scanline effect type
 * @param intensity Effect strength (0.0-1.0)
 * @return Modified pixel with scanline effect applied
 */
static uint16_t applyAnimatedScanlineEffect(uint16_t pixel, int row, ScanlineMode mode,
                                     float intensity) {
  if (mode == SCANLINE_NONE || intensity <= 0.0f) {
    return pixel;
  }

  float brightnessFactor = 1.0f;

  switch (mode) {
  case SCANLINE_CLASSIC:
    // Static every other line is darker (non-animated)
    if (row % 2 == 1) {
      brightnessFactor = 1.0f - intensity;
    }
    break;

  case SCANLINE_ANIMATED:
    // Moving scanlines that travel down the screen (matches CLASSIC spacing)
    {
      int animOffset = getAnimatedScanlineOffset();
      int effectiveRow = (row + animOffset) % 2;

      if (effectiveRow == 1) {
        // Dark scanline (every other line, just like CLASSIC)
        brightnessFactor = 1.0f - intensity;
      }
    }
    break;

  case SCANLINE_CURVE:
    // Animated curved brightness like real CRT phosphor decay
    {
      int animOffset = getAnimatedScanlineOffset();
      float curve = sin(((row + animOffset) % 6) * 3.14159f / 6.0f);
      brightnessFactor = 1.0f - (intensity * 0.4f * curve);
    }
    break;

  default:
    break;
  }

  if (brightnessFactor >= 1.0f) {
    return pixel;
  }

  // Apply brightness reduction
  uint8_t r = (pixel >> 11) & 0x1F;
  uint8_t g = (pixel >> 5) & 0x3F;
  uint8_t b = pixel & 0x1F;

  r = (uint8_t)(r * brightnessFactor);
  g = (uint8_t)(g * brightnessFactor);
  b = (uint8_t)(b * brightnessFactor);

  // Clamp values
  r = (r > 31) ? 31 : r;
  g = (g > 63) ? 63 : g;
  b = (b > 31) ? 31 : b;

  return (r << 11) | (g << 5) | b;
}

/**
 * @brief Apply CRT glitch effects to a scanline
 * 
 * Applies random horizontal jitter to entire scanlines based on the
 * configured glitch mode and probability settings.
 * 
 * @param pixels Array of RGB565 pixels for current scanline
 * @param width Number of pixels in the scanline
 * @param row Current row number (for additional randomness)
 */
static void applyCRTGlitches(uint16_t *pixels, int width, int row) {
  if (currentGlitchMode == GLITCH_NONE)
    return;

  // Update random seed with row info for variation
  glitchSeed ^= (row * 7919);

  // Check if this line should have glitches
  float random = (float)(fastRandom() % 1000) / 1000.0f;
  if (random > glitchProbability)
    return;

  // Determine jitter intensity based on mode
  int jitterIntensity = 0;

  switch (currentGlitchMode) {
  case GLITCH_LIGHT:
    jitterIntensity = 1;
    break;
  case GLITCH_MEDIUM:
    jitterIntensity = 2;
    break;
  case GLITCH_HEAVY:
    jitterIntensity = 3;
    break;
  default:
    return;
  }

  // Apply horizontal jitter
  applyHorizontalJitter(pixels, width, jitterIntensity);
}

//==============================================================================
// PUBLIC API FUNCTIONS - MODULE INITIALIZATION
//==============================================================================

void initializeEffectsModule(void) {
  // Initialize random seed
  glitchSeed = millis() * 1337;

  // Reset all effects to disabled state
  disableAllEffects();

  ESP_LOGI(EFFECTS_LOG, "Effects module initialized");
}

void initializeEffectCycling(void) {
  // Save current glitch settings
  savedGlitchMode = GLITCH_HEAVY;
  savedGlitchProbability = 0.08f;
  glitchesCurrentlyEnabled = false; // Glitches start disabled

  // Initialize timing
  lastEffectCycleTime = millis();
  lastGlitchToggleTime = millis();

  ESP_LOGI(EFFECTS_LOG, "Effect cycling initialized");
}

//==============================================================================
// PUBLIC API FUNCTIONS - WHITE TINTING EFFECTS
//==============================================================================

void setWhiteTint(uint16_t tintColor, float intensity, float threshold) {
  whiteTintColor = tintColor;
  whiteTintIntensity =
      (intensity > 1.0f) ? 1.0f : ((intensity < 0.0f) ? 0.0f : intensity);
  whiteThreshold =
      (threshold > 1.0f) ? 1.0f : ((threshold < 0.0f) ? 0.0f : threshold);
  whiteTintEnabled = (intensity > 0.0f);

  ESP_LOGI(EFFECTS_LOG,
           "White tint set: Color=0x%04X, Intensity=%.2f, Threshold=%.2f",
           tintColor, intensity, threshold);
}

void disableWhiteTint(void) {
  whiteTintEnabled = false;
  whiteTintIntensity = 0.0f;
  ESP_LOGI(EFFECTS_LOG, "White tinting disabled");
}

bool getWhiteTintSettings(uint16_t *tintColor, float *intensity,
                          float *threshold) {
  if (tintColor)
    *tintColor = whiteTintColor;
  if (intensity)
    *intensity = whiteTintIntensity;
  if (threshold)
    *threshold = whiteThreshold;
  return whiteTintEnabled;
}

//==============================================================================
// PUBLIC API FUNCTIONS - CRT SCANLINE EFFECTS
//==============================================================================

void setScanlineEffect(ScanlineMode mode, float intensity, float speed) {
  currentScanlineMode = mode;
  scanlineIntensity =
      (intensity > 1.0f) ? 1.0f : ((intensity < 0.0f) ? 0.0f : intensity);
  scanlineSpeed = (speed > 10.0f) ? 10.0f : ((speed < 0.1f) ? 0.1f : speed);

  // Initialize animation if using animated modes
  if (mode == SCANLINE_ANIMATED || mode == SCANLINE_CURVE) {
    initScanlineAnimation();
  } else {
    scanlineAnimationEnabled = false;
  }

  ESP_LOGI(EFFECTS_LOG,
           "Scanline effect set: Mode=%d, Intensity=%.2f, Speed=%.1f", mode,
           intensity, speed);
}

void disableScanlineEffect(void) {
  currentScanlineMode = SCANLINE_NONE;
  scanlineAnimationEnabled = false;
  ESP_LOGI(EFFECTS_LOG, "Scanline effects disabled");
}

bool getScanlineSettings(ScanlineMode *mode, float *intensity, float *speed) {
  if (mode)
    *mode = currentScanlineMode;
  if (intensity)
    *intensity = scanlineIntensity;
  if (speed)
    *speed = scanlineSpeed;
  return (currentScanlineMode != SCANLINE_NONE);
}

//==============================================================================
// PUBLIC API FUNCTIONS - BAYER DITHERING EFFECTS
//==============================================================================

void setBayerDithering(DitherMode mode, float intensity, int quantization) {
  currentDitherMode = mode;
  ditherIntensity =
      (intensity > 1.0f) ? 1.0f : ((intensity < 0.0f) ? 0.0f : intensity);
  ditherQuantization =
      (quantization > 16) ? 16 : ((quantization < 2) ? 2 : quantization);

  ESP_LOGI(EFFECTS_LOG,
           "Bayer dithering set: Mode=%d, Intensity=%.2f, Quantization=%d",
           mode, intensity, quantization);
}

void disableBayerDithering(void) {
  currentDitherMode = DITHER_NONE;
  ESP_LOGI(EFFECTS_LOG, "Bayer dithering disabled");
}

bool getDitherSettings(DitherMode *mode, float *intensity, int *quantization) {
  if (mode)
    *mode = currentDitherMode;
  if (intensity)
    *intensity = ditherIntensity;
  if (quantization)
    *quantization = ditherQuantization;
  return (currentDitherMode != DITHER_NONE);
}

//==============================================================================
// PUBLIC API FUNCTIONS - CRT GLITCH EFFECTS
//==============================================================================

void enableCRTGlitches(GlitchMode mode, float probability) {
  currentGlitchMode = mode;
  glitchProbability = (probability > 0.1f)
                          ? 0.1f
                          : ((probability < 0.001f) ? 0.001f : probability);

  // Re-initialize random seed
  glitchSeed = millis() * 1337;

  ESP_LOGI(EFFECTS_LOG, "CRT glitches enabled: Mode=%d, Probability=%.3f", mode,
           probability);
}

void disableCRTGlitches(void) {
  currentGlitchMode = GLITCH_NONE;
  ESP_LOGI(EFFECTS_LOG, "CRT glitches disabled");
}

bool getGlitchSettings(GlitchMode *mode, float *probability) {
  if (mode)
    *mode = currentGlitchMode;
  if (probability)
    *probability = glitchProbability;
  return (currentGlitchMode != GLITCH_NONE);
}

//==============================================================================
// PUBLIC API FUNCTIONS - COMBINED EFFECTS
//==============================================================================

void setWhiteTintWithScanlines(uint16_t tintColor, float intensity,
                               float threshold, ScanlineMode scanlineMode,
                               float scanlineIntensity, float speed) {
  // Set white tint
  setWhiteTint(tintColor, intensity, threshold);

  // Set scanline effect
  setScanlineEffect(scanlineMode, scanlineIntensity, speed);

  ESP_LOGI(
      EFFECTS_LOG,
      "Combined effects set: Tint=0x%04X(%.2f), Scanlines=%d(%.2f), Speed=%.1f",
      tintColor, intensity, scanlineMode, scanlineIntensity, speed);
}

void disableAllEffects(void) {
  disableWhiteTint();
  disableBayerDithering();
  disableScanlineEffect();
  disableCRTGlitches();
  ESP_LOGI(EFFECTS_LOG, "All effects disabled");
}

//==============================================================================
// PUBLIC API FUNCTIONS - EFFECT CYCLING SYSTEM
//==============================================================================

void cycleVisualEffects(void) {
  unsigned long currentTime = millis();

  // Apply debouncing
  if (currentTime - lastEffectCycleTime < EFFECT_DEBOUNCE_TIME) {
    return;
  }
  lastEffectCycleTime = currentTime;

  // Advance to next state
  int nextState = (static_cast<int>(currentEffectState) + 1) %
                  static_cast<int>(EFFECT_STATE_COUNT);
  currentEffectState = static_cast<EffectCycleState>(nextState);

  // Apply the new visual effects
  applyCurrentVisualEffects();
}

void toggleCRTGlitches(void) {
  unsigned long currentTime = millis();

  // Apply debouncing
  if (currentTime - lastGlitchToggleTime < EFFECT_DEBOUNCE_TIME) {
    return;
  }
  lastGlitchToggleTime = currentTime;

  // Toggle glitch state
  glitchesCurrentlyEnabled = !glitchesCurrentlyEnabled;

  if (glitchesCurrentlyEnabled) {
    enableCRTGlitches(savedGlitchMode, savedGlitchProbability);
    ESP_LOGI(EFFECTS_LOG, "CRT Glitches: ENABLED");
  } else {
    disableCRTGlitches();
    ESP_LOGI(EFFECTS_LOG, "CRT Glitches: DISABLED");
  }
}

EffectCycleState getCurrentEffectState(void) { 
  return currentEffectState; 
}

bool areCRTGlitchesEnabled(void) { 
  return glitchesCurrentlyEnabled; 
}

//==============================================================================
// PUBLIC API FUNCTIONS - MENU INTEGRATION
//==============================================================================

bool setEffectStateDirect(EffectCycleState targetState) {
  // Validate the target state
  if (!isValidEffectState(targetState)) {
    ESP_LOGW(EFFECTS_LOG, "Invalid effect state: %d", targetState);
    return false;
  }

  // Set the state directly without cycling
  currentEffectState = targetState;
  
  // Apply the effect
  applyCurrentVisualEffects();
  
  ESP_LOGI(EFFECTS_LOG, "Effect state set directly to: %s", getEffectStateName(targetState));
  return true;
}

int getEffectTypeFromState(EffectCycleState state) {
  // Direct mapping from EffectCycleState to standardized effect ID
  switch (state) {
    case EFFECT_STATE_NONE:         return 0;
    case EFFECT_STATE_SCANLINE:     return 1;
    case EFFECT_STATE_DITHER:       return 2;
    case EFFECT_STATE_GREEN_TINT:   return 3;
    case EFFECT_STATE_YELLOW_TINT:  return 4;
    case EFFECT_STATE_DITHER_GREEN: return 5;
    case EFFECT_STATE_DITHER_YELLOW: return 6;
    default:                        return 0;
  }
}

EffectCycleState getStateFromEffectType(int effectType) {
  // Direct mapping from standardized effect ID to EffectCycleState
  switch (effectType) {
    case 0: return EFFECT_STATE_NONE;
    case 1: return EFFECT_STATE_SCANLINE;
    case 2: return EFFECT_STATE_DITHER;
    case 3: return EFFECT_STATE_GREEN_TINT;
    case 4: return EFFECT_STATE_YELLOW_TINT;
    case 5: return EFFECT_STATE_DITHER_GREEN;
    case 6: return EFFECT_STATE_DITHER_YELLOW;
    default: return EFFECT_STATE_NONE;
  }
}

int getEffectCount(void) {
  return static_cast<int>(EFFECT_STATE_COUNT);
}

const char* getEffectStateName(EffectCycleState state) {
  int effectType = getEffectTypeFromState(state);
  
  if (effectType >= 0 && effectType < (sizeof(EFFECT_NAMES) / sizeof(EFFECT_NAMES[0]))) {
    return EFFECT_NAMES[effectType];
  }
  
  return "UNKNOWN";
}

bool isValidEffectState(EffectCycleState state) {
  return (state >= EFFECT_STATE_NONE && state < EFFECT_STATE_COUNT);
}

//==============================================================================
// PUBLIC API FUNCTIONS - PIXEL PROCESSING
//==============================================================================

void applyEffectsToScanline(uint16_t *pixels, int width, int row) {
  // Apply white tinting first
  if (whiteTintEnabled && whiteTintIntensity > 0.0f) {
    for (int i = 0; i < width; i++) {
      pixels[i] = applySelectiveColorTint(pixels[i], whiteTintColor,
                                          whiteTintIntensity, whiteThreshold);
    }
  }

  // Apply dithering second (before scanlines for authentic retro look)
  if (currentDitherMode != DITHER_NONE) {
    for (int i = 0; i < width; i++) {
      pixels[i] = applyBayerDithering(pixels[i], i, row, ditherIntensity,
                                      ditherQuantization);
    }
  }

  // Apply scanline effects third
  if (currentScanlineMode != SCANLINE_NONE) {
    for (int i = 0; i < width; i++) {
      pixels[i] = applyAnimatedScanlineEffect(
          pixels[i], row, currentScanlineMode, scanlineIntensity);
    }
  }

  // Apply glitch effects last
  if (currentGlitchMode != GLITCH_NONE) {
    applyCRTGlitches(pixels, width, row);
  }
}

//==============================================================================
// PUBLIC API FUNCTIONS - LOW-LEVEL PIXEL FUNCTIONS
//==============================================================================

uint16_t applySelectiveColorTint(uint16_t pixel, uint16_t tintColor,
                                 float intensity, float brightnessThreshold) {
  // Extract RGB components from original pixel (RGB565)
  uint8_t r = (pixel >> 11) & 0x1F;
  uint8_t g = (pixel >> 5) & 0x3F;
  uint8_t b = pixel & 0x1F;

  // Calculate brightness (perceived luminance)
  // Convert to 8-bit scale for calculation
  float r_norm = (r * 255.0f) / 31.0f;
  float g_norm = (g * 255.0f) / 63.0f;
  float b_norm = (b * 255.0f) / 31.0f;

  // Calculate perceived brightness using standard luminance formula
  float brightness =
      (0.299f * r_norm + 0.587f * g_norm + 0.114f * b_norm) / 255.0f;

  // Only apply tint if pixel is bright enough
  if (brightness < brightnessThreshold) {
    return pixel; // Return original pixel unchanged
  }

  // Calculate tint intensity based on brightness
  // Brighter pixels get more tint
  float actualIntensity = intensity * ((brightness - brightnessThreshold) /
                                       (1.0f - brightnessThreshold));

  // Extract RGB components from tint color
  uint8_t tint_r = (tintColor >> 11) & 0x1F;
  uint8_t tint_g = (tintColor >> 5) & 0x3F;
  uint8_t tint_b = tintColor & 0x1F;

  // Apply tint with blending
  r = (uint8_t)(r * (1.0f - actualIntensity) + tint_r * actualIntensity);
  g = (uint8_t)(g * (1.0f - actualIntensity) + tint_g * actualIntensity);
  b = (uint8_t)(b * (1.0f - actualIntensity) + tint_b * actualIntensity);

  // Clamp values to valid ranges
  r = (r > 31) ? 31 : r;
  g = (g > 63) ? 63 : g;
  b = (b > 31) ? 31 : b;

  // Reconstruct RGB565 pixel
  return (r << 11) | (g << 5) | b;
}

uint16_t replaceWhitePixels(uint16_t pixel, uint16_t tintColor, float intensity,
                            float threshold) {
  // Extract RGB components from original pixel (RGB565)
  uint8_t r = (pixel >> 11) & 0x1F;
  uint8_t g = (pixel >> 5) & 0x3F;
  uint8_t b = pixel & 0x1F;

  // Normalize to 0-1 range
  float r_norm = r / 31.0f;
  float g_norm = g / 63.0f;
  float b_norm = b / 31.0f;

  // Check if pixel is close to white (all components high and similar)
  float minComponent = (r_norm < g_norm) ? r_norm : g_norm;
  minComponent = (minComponent < b_norm) ? minComponent : b_norm;

  // Only apply tint if pixel is close to white
  if (minComponent < threshold) {
    return pixel; // Return original pixel unchanged
  }

  // Calculate how "white" this pixel is (0-1)
  float whiteness = minComponent;
  float actualIntensity = intensity * whiteness;

  // Extract RGB components from tint color
  uint8_t tint_r = (tintColor >> 11) & 0x1F;
  uint8_t tint_g = (tintColor >> 5) & 0x3F;
  uint8_t tint_b = tintColor & 0x1F;

  // Apply tint with blending
  r = (uint8_t)(r * (1.0f - actualIntensity) + tint_r * actualIntensity);
  g = (uint8_t)(g * (1.0f - actualIntensity) + tint_g * actualIntensity);
  b = (uint8_t)(b * (1.0f - actualIntensity) + tint_b * actualIntensity);

  // Clamp values to valid ranges
  r = (r > 31) ? 31 : r;
  g = (g > 63) ? 63 : g;
  b = (b > 31) ? 31 : b;

  // Reconstruct RGB565 pixel
  return (r << 11) | (g << 5) | b;
}