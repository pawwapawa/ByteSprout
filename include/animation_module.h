/**
 * @file animation_module.h
 * @brief Header for animation playback and management
 * 
 * This module provides functions for managing and playing GIF animations,
 * handling animation sequences, responding to device states like orientation
 * changes or sleep modes, and coordinating interactions between animations
 * and other device systems.
 */

 #ifndef ANIMATION_MODULE_H
 #define ANIMATION_MODULE_H
 
 #include "common.h"
 #include "gif_module.h"
 
 //==============================================================================
 // CONSTANTS & DEFINITIONS
 //==============================================================================
 
 /** @brief Log tag for Animation module messages */
 static const char* ANIM_LOG = "::ANIMATION_MODULE::";
 
 //==============================================================================
 // TYPE DEFINITIONS
 //==============================================================================
 
 /**
  * @brief States for the animation sequence state machine
  */
 enum class SequenceState {
   REST_START,      /**< Starting rest state before animation cycle */
   ANIMATION_CYCLE, /**< Active animation cycle state */
   REST_END         /**< Ending rest state after animation cycle */
 };
 
 /**
  * @brief States for the sleep animation sequence
  */
 enum class SleepState {
   NONE,           /**< Not in sleep state */
   ENTERING_SLEEP, /**< Transitioning into sleep */
   SLEEPING,       /**< Currently sleeping */
   EXITING_SLEEP   /**< Waking up from sleep */
 };
 
 /**
  * @brief States for the crash animation sequence
  */
 enum class CrashState {
   NONE,           /**< Not in crash state */
   ENTERING_CRASH, /**< Initial impact */
   CRASHED,        /**< Currently in crashed state */
   RECOVERING      /**< Recovering from crash */
 };
 
 /**
  * @brief Structure to hold animation sequence state information
  */
 struct AnimationSequence {
   /** @brief Current state in the animation sequence */
   SequenceState currentState = SequenceState::REST_START;
   /** @brief Timestamp when the current state started */
   unsigned long stateStartTime = 0;
   /** @brief Whether currently in idle mode or active mode */
   bool isIdleMode = true;
   /** @brief Delay between state transitions (ms) */
   const unsigned long STATE_DELAY = 3000;
   /** @brief Delay for idle animation (ms) */
   const unsigned long IDLE_DELAY = 20000;
 };
 
 //==============================================================================
 // PUBLIC API FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Initialize the animation module
  * 
  * Sets up initial state for animation sequences and state tracking.
  */
 void initializeAnimationModule(void);
 
 /**
  * @brief Play a GIF animation with interaction detection
  * 
  * This is a blocking function that plays a GIF file while monitoring
  * for interactions that might interrupt the animation.
  * 
  * @param filename Path to the GIF file to play
  * @return true if playback completed successfully
  */
 bool playGIF(const char* filename);
 
 /**
  * @brief Main function to handle emote playback
  * 
  * Coordinates the playback of animations based on device state,
  * ESP-NOW communication, and normal animation sequences.
  * Called in the main loop when in ESP mode.
  */
 void playEmotes(void);
 
 /**
  * @brief Play the boot animation
  * 
  * Shows the startup animation when the device boots.
  */
 void playBootAnimation(void);
 
 #endif /* ANIMATION_MODULE_H */