/**
 * @file emotes_module.h
 * @brief Header for animated and static emote definitions
 * 
 * This module provides definitions for all GIF animations and static images
 * used for the device's emotional expression system. It includes different
 * categories of emotes for various device modes and states.
 */

 #ifndef EMOTES_MODULE_H
 #define EMOTES_MODULE_H
 
 #include "common.h"
 
 //==============================================================================
 // ANIMATED EMOTES (GIF FILES)
 //==============================================================================
 
 //------------------------------------------------------------------------------
 // Common Emotes
 //------------------------------------------------------------------------------
 /** @brief GIF Animation Emotes for ALL variations MAC, PC, and BYTE-90 default
  * Animations are optimized GIFs for the ESP32's SPIFFS filesystem
  * GIFs are 128x128 pixels and 16FPS
  */
 #define ANGRY_EMOTE "/gifs/your_gif_here.gif"
 #define CRASH01_EMOTE "/gifs/your_gif_here.gif"
 #define CRASH02_EMOTE "/gifs/your_gif_here.gif"
 #define CRASH03_EMOTE "/gifs/your_gif_here.gif"
 #define CRY_EMOTE "/gifs/your_gif_here.gif"
 #define DIZZY_EMOTE "/gifs/your_gif_here.gif"
 #define DOUBTFUL_EMOTE "/gifs/your_gif_here.gif"
 #define IDLE_EMOTE "/gifs/your_gif_here.gif"
 #define LOOK_DOWN_EMOTE "/gifs/your_gif_here.gif"
 #define LOOK_LEFT_RIGHT_EMOTE "/gifs/your_gif_here.gif"
 #define LOOK_UP_EMOTE "/gifs/your_gif_here.gif"
 #define REST_EMOTE "/gifs/your_gif_here.gif"
 #define SCAN_EMOTE "/gifs/your_gif_here.gif"
 #define SHOCK_EMOTE "/gifs/your_gif_here.gif"
 #define SLEEP01_EMOTE "/gifs/your_gif_here.gif"
 #define SLEEP02_EMOTE "/gifs/your_gif_here.gif"
 #define SLEEP03_EMOTE "/gifs/your_gif_here.gif"
 #define STARTUP_EMOTE "/gifs/your_gif_here.gif"
 #define TALK_EMOTE "/gifs/your_gif_here.gif"
 #define TAP_EMOTE "/gifs/your_gif_here.gif"
 #define WINK_EMOTE "/gifs/your_gif_here.gif"
 #define ZONED_EMOTE "/gifs/your_gif_here.gif"
 #define PIXEL_EMOTE "/gifs/your_gif_here.gif"
 #define GLEE_EMOTE "/gifs/your_gif_here.gif"
 #define BLINK_EMOTE "/gifs/your_gif_here.gif"
 #define EXCITED_EMOTE "/gifs/your_gif_here.gif"
 #define UWU_EMOTE "/gifs/your_gif_here.gif"
 #define HEARTS_EMOTE "/gifs/your_gif_here.gif"
 #define STARTLED_EMOTE "/gifs/your_gif_here.gif"

 //------------------------------------------------------------------------------
 // BYTE-90 Specific Emotes
 //------------------------------------------------------------------------------
 /** @brief Special Emotes for BYTE-90 only */
 #define WHISTLE_EMOTE "/gifs/your_gif_here.gif"
 #define MISCHIEF_EMOTE "/gifs/your_gif_here.gif"
 #define HUMSUP_EMOTE "/gifs/your_gif_here.gif"
 #define WINK_02_EMOTE "/gifs/your_gif_here.gif"

 //------------------------------------------------------------------------------
 // ESPNOW Communication Emotes
 //------------------------------------------------------------------------------
 /** @brief Special Emotes for ESPNOW communication mode */
 #define COMS_AGREED_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_CONNECT_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_DISCONNECT_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_DISAGREE_EMOTE "/gifs/coms_disagree.gif"
 #define COMS_HELLO_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_LAUGH_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_SHOCK_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_TALK_01_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_TALK_02_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_TALK_03_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_WINK_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_YELL_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_ZONED_EMOTE "/gifs/your_gif_here.gif"
 #define COMS_IDLE_EMOTE "/gifs/your_gif_here.gif"

 //==============================================================================
 // STATIC IMAGES AND EXTERNAL DECLARATIONS
 //==============================================================================
 
 /** @brief External declaration for device mode */
 extern const char* deviceMode;
 
 /** 
  * @brief Static images are converted to C arrays using the RGB565 image converter
  * RGB565 image converter: https://marlinfw.org/tools/rgb565/converter.html
  */
 /** @brief BYTE Static crash image */
 extern const uint16_t BYTE_CRASH_STATIC[];
 /** @brief Static crash image */
 extern const uint16_t CRASH_STATIC[];
 /** @brief BYTE mode static image */
 extern const uint16_t BYTE_STATIC[];
 /** @brief MAC mode static image */
 extern const uint16_t MAC_STATIC[];
 /** @brief PC mode static image */
 extern const uint16_t PC_STATIC[];
 /** @brief Updater mode static image */
 extern const uint16_t UPDATER_STATIC[];
  /** @brief Startup static image */
 extern const uint16_t STARTUP_STATIC[];
 
 #endif /* EMOTES_MODULE_H */