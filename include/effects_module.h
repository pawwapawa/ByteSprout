/**
 * @file effects_module.h
 * @brief Header for visual effects and retro display aesthetics functionality
 * 
 * This module provides visual effects for display content including white pixel
 * tinting, CRT scanline effects, Bayer dithering, and horizontal jitter glitches. 
 * Effects can be applied individually or combined for authentic retro display aesthetics.
 * 
 * Key features:
 * - White pixel tinting (green/yellow terminal colors)
 * - CRT scanline simulation (static and animated)
 * - Bayer dithering patterns for retro computer graphics
 * - Horizontal glitch effects simulating CRT sync issues
 * - Effect cycling system for easy user control
 * - Direct effect state setting for menu integration
 * - Real-time processing optimized for embedded systems
 */

#ifndef EFFECTS_MODULE_H
#define EFFECTS_MODULE_H

//==============================================================================
// INCLUDES AND DEPENDENCIES
//==============================================================================

#include "common.h"
#include <stdint.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

/** @brief Log tag for effects module messages */
static const char *EFFECTS_LOG = "::EFFECTS_MODULE::";

//------------------------------------------------------------------------------
// Timing and Control Definitions
//------------------------------------------------------------------------------
/** @brief Debounce time for effect operations in milliseconds */
#define EFFECT_DEBOUNCE_TIME 300

//------------------------------------------------------------------------------
// Color Definitions
//------------------------------------------------------------------------------
/** @brief No tint applied - transparent */
#define TINT_NONE 0x0000   
/** @brief Yellow tint color in RGB565 format */
#define TINT_YELLOW 0xFFE0 
/** @brief Green tint color in RGB565 format */
#define TINT_GREEN 0x3FE0

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief CRT scanline effect types
 */
typedef enum {
  SCANLINE_NONE,        /**< No scanline effects applied */
  SCANLINE_CLASSIC,     /**< Static horizontal scanlines */
  SCANLINE_ANIMATED,    /**< Moving scanlines that travel down screen */
  SCANLINE_CURVE        /**< Curved brightness like real CRT phosphor decay */
} ScanlineMode;

/**
 * @brief CRT glitch effect intensity levels
 */
typedef enum {
  GLITCH_NONE,          /**< No glitch effects applied */
  GLITCH_LIGHT,         /**< Light horizontal jitter */
  GLITCH_MEDIUM,        /**< Medium horizontal jitter */
  GLITCH_HEAVY          /**< Heavy horizontal jitter */
} GlitchMode;

/**
 * @brief Bayer dithering pattern types
 */
typedef enum {
  DITHER_NONE,          /**< No dithering applied */
  DITHER_2X2,           /**< 2x2 Bayer dithering pattern */
  DITHER_4X4,           /**< 4x4 Bayer dithering pattern */
  DITHER_8X8            /**< 8x8 Bayer dithering pattern */
} DitherMode;

/**
 * @brief Visual effect states for cycling system
 */
typedef enum {
  EFFECT_STATE_NONE = 0,        /**< No effects applied */
  EFFECT_STATE_SCANLINE,        /**< CRT scanlines only */
  EFFECT_STATE_DITHER,          /**< Bayer dithering only */
  EFFECT_STATE_GREEN_TINT,      /**< Green terminal tint with scanlines */
  EFFECT_STATE_YELLOW_TINT,     /**< Yellow terminal tint with scanlines */
  EFFECT_STATE_DITHER_GREEN,    /**< Dithering combined with green tint */
  EFFECT_STATE_DITHER_YELLOW,   /**< Dithering combined with yellow tint */
  EFFECT_STATE_COUNT            /**< Total number of effect states */
} EffectCycleState;

//==============================================================================
// MODULE INITIALIZATION
//==============================================================================

/**
 * @brief Initialize the effects module
 * 
 * Sets up the effects system, initializes random seed for glitch effects,
 * and resets all effects to disabled state.
 */
void initializeEffectsModule(void);

/**
 * @brief Initialize effect cycling system
 * 
 * Sets up the effect cycling system with default glitch settings
 * and initializes timing for menu integration.
 */
void initializeEffectCycling(void);

//==============================================================================
// WHITE TINTING EFFECTS
//==============================================================================

/**
 * @brief Set white replacement tint
 * 
 * Applies color tinting to bright pixels, typically used for terminal-style
 * color schemes like green-on-black or amber monitors.
 * 
 * @param tintColor RGB565 color value (use TINT_* constants)
 * @param intensity Replacement intensity (0.0-1.0)
 * @param whiteThreshold Brightness threshold for white detection (0.6-0.9)
 */
void setWhiteTint(uint16_t tintColor, float intensity, float whiteThreshold = 0.7f);

/**
 * @brief Disable white tinting
 * 
 * Turns off all white pixel tinting effects.
 */
void disableWhiteTint(void);

/**
 * @brief Get current white tint settings
 * 
 * Retrieves the current tinting configuration for inspection or saving.
 * 
 * @param tintColor Pointer to store current tint color (can be NULL)
 * @param intensity Pointer to store current intensity (can be NULL)
 * @param threshold Pointer to store current threshold (can be NULL)
 * @return true if white tinting is currently enabled
 */
bool getWhiteTintSettings(uint16_t *tintColor, float *intensity, float *threshold);

//==============================================================================
// CRT SCANLINE EFFECTS
//==============================================================================

/**
 * @brief Set scanline effect
 * 
 * Applies CRT-style horizontal scanlines to simulate vintage monitor appearance.
 * Supports both static and animated scanline modes.
 * 
 * @param mode Type of scanline effect (SCANLINE_CLASSIC, SCANLINE_ANIMATED, etc.)
 * @param intensity Strength of the scanline effect (0.3-0.5 recommended)
 * @param speed Lines per second for animated modes (1.0-5.0)
 */
void setScanlineEffect(ScanlineMode mode, float intensity, float speed = 2.0f);

/**
 * @brief Disable scanline effects
 * 
 * Turns off all CRT scanline effects and stops animations.
 */
void disableScanlineEffect(void);

/**
 * @brief Get current scanline settings
 * 
 * Retrieves the current scanline configuration for inspection or saving.
 * 
 * @param mode Pointer to store current scanline mode (can be NULL)
 * @param intensity Pointer to store current intensity (can be NULL)
 * @param speed Pointer to store current animation speed (can be NULL)
 * @return true if scanline effects are currently enabled
 */
bool getScanlineSettings(ScanlineMode *mode, float *intensity, float *speed);

//==============================================================================
// BAYER DITHERING EFFECTS
//==============================================================================

/**
 * @brief Set bayer dithering effect
 * 
 * Applies Bayer dithering patterns to simulate retro computer graphics
 * with reduced color palettes and pixelated appearance.
 * 
 * @param mode Type of dithering pattern (DITHER_2X2, DITHER_4X4, DITHER_8X8)
 * @param intensity Strength of the dithering effect (0.1-1.0)
 * @param quantization Color quantization levels (2-16, lower = more retro)
 */
void setBayerDithering(DitherMode mode, float intensity, int quantization = 4);

/**
 * @brief Disable bayer dithering effects
 * 
 * Turns off all dithering effects and restores full color depth.
 */
void disableBayerDithering(void);

/**
 * @brief Get current dithering settings
 * 
 * Retrieves the current dithering configuration for inspection or saving.
 * 
 * @param mode Pointer to store current dither mode (can be NULL)
 * @param intensity Pointer to store current intensity (can be NULL)
 * @param quantization Pointer to store current quantization (can be NULL)
 * @return true if dithering effects are currently enabled
 */
bool getDitherSettings(DitherMode *mode, float *intensity, int *quantization);

//==============================================================================
// CRT GLITCH EFFECTS
//==============================================================================

/**
 * @brief Enable CRT glitch effects
 * 
 * Applies random horizontal jitter to simulate CRT sync issues and
 * interference for authentic retro display glitches.
 * 
 * @param mode Intensity level of horizontal jitter (GLITCH_LIGHT recommended)
 * @param probability Base chance of glitch per scanline (0.01-0.05)
 */
void enableCRTGlitches(GlitchMode mode, float probability = 0.03f);

/**
 * @brief Disable CRT glitch effects
 * 
 * Turns off all glitch effects and restores stable display output.
 */
void disableCRTGlitches(void);

/**
 * @brief Get current glitch settings
 * 
 * Retrieves the current glitch configuration for inspection or saving.
 * 
 * @param mode Pointer to store current glitch mode (can be NULL)
 * @param probability Pointer to store current probability (can be NULL)
 * @return true if glitch effects are currently enabled
 */
bool getGlitchSettings(GlitchMode *mode, float *probability);

//==============================================================================
// COMBINED EFFECTS
//==============================================================================

/**
 * @brief Set white tint with CRT scanline effect
 * 
 * Convenience function to apply both white tinting and scanlines simultaneously
 * for complete retro terminal aesthetics.
 * 
 * @param tintColor RGB565 color value (use TINT_* constants)
 * @param intensity Base tint replacement intensity (0.0-1.0)
 * @param whiteThreshold Brightness threshold for white detection (0.6-0.9)
 * @param scanlineMode Type of scanline effect
 * @param scanlineIntensity Strength of scanline darkening effect (0.3-0.5)
 * @param speed Animation speed in lines per second for animated modes
 */
void setWhiteTintWithScanlines(uint16_t tintColor, float intensity, float whiteThreshold, 
                               ScanlineMode scanlineMode, float scanlineIntensity, float speed = 2.0f);

/**
 * @brief Disable all visual effects
 * 
 * Turns off all effects including tinting, scanlines, dithering, and glitches.
 * Does not affect the effect cycling system state.
 */
void disableAllEffects(void);

//==============================================================================
// EFFECT CYCLING SYSTEM
//==============================================================================

/**
 * @brief Cycle through visual effects
 * 
 * Advances to the next effect in the cycling sequence with debouncing.
 * Preserves glitch settings independently of visual effects.
 */
void cycleVisualEffects(void);

/**
 * @brief Toggle CRT glitch effects on/off
 * 
 * Toggles glitch effects independently of other visual effects
 * with debouncing to prevent rapid switching.
 */
void toggleCRTGlitches(void);

/**
 * @brief Get the current effect cycle state
 * 
 * Returns the current position in the effect cycling sequence
 * for menu system integration and state persistence.
 * 
 * @return Current EffectCycleState value
 */
EffectCycleState getCurrentEffectState(void);

/**
 * @brief Check if CRT glitches are currently enabled
 * 
 * Returns the current glitch enable state for menu system
 * integration and status display.
 * 
 * @return true if glitch effects are currently enabled
 */
bool areCRTGlitchesEnabled(void);

//==============================================================================
// MENU INTEGRATION FUNCTIONS
//==============================================================================

/**
 * @brief Set effect state directly without cycling
 * 
 * Directly sets the effect state for efficient menu system integration,
 * bypassing the cycling mechanism for immediate effect changes.
 * 
 * @param targetState The desired effect state
 * @return true if state was set successfully
 */
bool setEffectStateDirect(EffectCycleState targetState);

/**
 * @brief Convert EffectCycleState to a standardized effect ID
 * 
 * Provides a clean mapping between internal effect states and
 * menu system effect type enumeration for consistent integration.
 * 
 * @param state The effect cycle state
 * @return Standardized effect ID (0-based)
 */
int getEffectTypeFromState(EffectCycleState state);

/**
 * @brief Convert standardized effect ID to EffectCycleState
 * 
 * Reverse mapping from menu system effect types to internal
 * effect states for seamless menu integration.
 * 
 * @param effectType Effect ID (0-based)
 * @return Corresponding EffectCycleState
 */
EffectCycleState getStateFromEffectType(int effectType);

/**
 * @brief Get the total number of available effects
 * 
 * Returns the count of available effects for dynamic menu
 * generation and bounds checking.
 * 
 * @return Number of effects (excluding COUNT sentinel)
 */
int getEffectCount(void);

/**
 * @brief Get human-readable name for an effect state
 * 
 * Provides consistent effect naming for menu display and
 * debugging output across the entire system.
 * 
 * @param state The effect state
 * @return Pointer to string containing effect name
 */
const char* getEffectStateName(EffectCycleState state);

/**
 * @brief Check if an effect state is valid
 * 
 * Validates effect state values for bounds checking and
 * error prevention in menu and API usage.
 * 
 * @param state The effect state to validate
 * @return true if state is valid
 */
bool isValidEffectState(EffectCycleState state);

//==============================================================================
// PIXEL PROCESSING FUNCTIONS
//==============================================================================

/**
 * @brief Apply all enabled effects to a scanline
 * 
 * Main rendering function that applies all currently enabled effects
 * to a horizontal line of pixels in the correct order for optimal
 * visual quality and performance.
 * 
 * @param pixels Array of RGB565 pixels for current scanline
 * @param width Number of pixels in the scanline
 * @param row Current row number (Y coordinate)
 */
void applyEffectsToScanline(uint16_t *pixels, int width, int row);

//==============================================================================
// LOW-LEVEL PIXEL FUNCTIONS
//==============================================================================

/**
 * @brief Apply selective color tint based on pixel brightness
 * 
 * Applies color tinting only to pixels above a brightness threshold,
 * creating terminal-style color schemes while preserving darker pixels.
 * 
 * @param pixel Original RGB565 pixel
 * @param tintColor Tint color in RGB565 format
 * @param intensity Maximum tint intensity (0.0-1.0)
 * @param brightnessThreshold Minimum brightness to apply tint (0.0-1.0)
 * @return Tinted RGB565 pixel
 */
uint16_t applySelectiveColorTint(uint16_t pixel, uint16_t tintColor, float intensity, float brightnessThreshold);

/**
 * @brief Replace white pixels with tint color
 * 
 * Replaces pixels that are close to white with the specified tint color,
 * useful for creating monochrome terminal aesthetics.
 * 
 * @param pixel Original RGB565 pixel
 * @param tintColor Tint color in RGB565 format
 * @param intensity Replacement intensity (0.0-1.0)
 * @param threshold How close to white a pixel needs to be (0.6-0.9)
 * @return Tinted RGB565 pixel
 */
uint16_t replaceWhitePixels(uint16_t pixel, uint16_t tintColor, float intensity, float threshold);

#endif /* EFFECTS_MODULE_H */