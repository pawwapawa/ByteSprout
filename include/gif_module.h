/**
 * @file gif_module.h
 * @brief Header for GIF animation playback functionality
 *
 * This module provides functions for loading, initializing, and playing
 * animated GIF files from the filesystem. It handles memory allocation,
 * frame buffering, and rendering to the display.
 *
 * @note DEPENDENCY: This module requires the AnimatedGIF library version 2.1.1.
 * Newer versions (like 2.2.0+) introduce breaking changes and are not
 * compatible. Library source: https://github.com/bitbank2/AnimatedGIF
 */

#ifndef GIF_MODULE_H
#define GIF_MODULE_H

#include "common.h"
#include "effects_module.h"
#include <AnimatedGIF.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

/** @brief Log tag for GIF module messages */
static const char *GIF_LOG = "::GIF_MODULE::";

/**
 * @brief GIF Properties
 *
 * GIFs are sized to 128x128 pixels with 16FPS playback for optimal file size
 */
#define GIF_HEIGHT 128
#define GIF_WIDTH 128
/** @brief Microseconds between frames for 16FPS playback */
#define FRAME_DELAY_MICROSECONDS (1000000 / 16) // 16FPS

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Context structure for GIF playback
 *
 * Stores the frame buffer and positional information for rendering
 */
struct GIFContext {
  uint8_t *sharedFrameBuffer; /**< Buffer for frame data */
  int offsetX;                /**< X offset for centered display */
  int offsetY;                /**< Y offset for centered display */
};

//==============================================================================
// INITIALIZATION AND CLEANUP FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the GIF player
 *
 * Sets up the filesystem, allocates frame buffer memory, and initializes
 * the AnimatedGIF library.
 *
 * @return true if initialization was successful
 */
bool initializeGIFPlayer(void);

/**
 * @brief Stop GIF playback and free resources
 *
 * Releases the frame buffer memory and closes the GIF file.
 */
void stopGifPlayback(void);

/**
 * @brief Check if the GIF player is initialized
 *
 * @return true if the GIF player is initialized
 */
bool gifPlayerInitialized();

//==============================================================================
// DIAGNOSTIC FUNCTIONS
//==============================================================================

/**
 * @brief Check and log memory status
 *
 * Reports available heap and PSRAM memory, with warnings if levels are low.
 */
void checkMemoryStatus(void);

//==============================================================================
// PLAYBACK FUNCTIONS
//==============================================================================

/**
 * @brief Load a GIF file for playback
 *
 * Opens a GIF file, sets up rendering parameters, and prepares it for playback.
 *
 * @param filename Path to the GIF file
 * @return true if GIF was loaded successfully
 */
bool loadGIF(const char *filename);

/**
 * @brief Play a single frame of the current GIF
 *
 * @param bSync Whether to synchronize with the GIF timing
 * @param delayMilliseconds Pointer to store delay until next frame
 * @return Status code (0 = success, 1 = finished, negative = error)
 */
int playGIFFrame(bool bSync, int *delayMilliseconds);

#endif /* GIF_MODULE_H */