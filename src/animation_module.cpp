/**
 * @file animation_module.cpp
 * @brief Implementation of animation playback and management
 *
 * This module provides functions for managing and playing GIF animations,
 * handling animation sequences, responding to device states like orientation
 * changes or sleep modes, and coordinating interactions between animations
 * and other device systems.
 */

#include "animation_module.h"
#include "effects_module.h"
#include "emotes_module.h"
#include "espnow_module.h"
#include "gif_module.h"
#include "motion_module.h"
#include "system_module.h"
#include "menu_module.h"

//==============================================================================
// GLOBAL VARIABLES AND CONSTANTS
//==============================================================================

/** @brief Current animation sequence state */
static AnimationSequence animSequence;
/** @brief Pointer to the current emote set */
const char **currentEmotes;
/** @brief Number of emotes in the current set */
size_t emoteCount;

/** @brief Helper macro to get array size */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

//------------------------------------------------------------------------------
// Crash State Variables
//------------------------------------------------------------------------------
/** @brief Current crash animation state */
static CrashState currentCrashState = CrashState::NONE;
/** @brief Flag indicating if device was in crashed state */
static bool wasCrashed = false;

//------------------------------------------------------------------------------
// Sleep State Variables
//------------------------------------------------------------------------------
/** @brief Current sleep animation state */
static SleepState currentSleepState = SleepState::NONE;
/** @brief Flag indicating if device was in sleep state */
static bool wasAsleep = false;

//------------------------------------------------------------------------------
// Timing Variables
//------------------------------------------------------------------------------
/** @brief Last time ESP-NOW communication was checked */
static unsigned long lastCheckComs = 0;
/** @brief Interval for checking ESP-NOW communication status (ms) */
const unsigned long COMS_CHECK_INTERVAL = 20000;
/** @brief Last time interactions were checked */
static unsigned long lastInteractionCheck = 0;
/** @brief Debounce time for interaction checks (ms) */
const unsigned long INTERACTION_CHECK_DEBOUNCE = 10;

//------------------------------------------------------------------------------
// Emote Collections
//------------------------------------------------------------------------------
#if DEVICE_MODE == MAC_MODE | DEVICE_MODE == PC_MODE
// File paths for emotes (replace the const arrays)
/** @brief Random emotes for MAC and PC modes */
const char *randomEmotes[] = {
    ZONED_EMOTE, DOUBTFUL_EMOTE, TALK_EMOTE, SCAN_EMOTE, ANGRY_EMOTE,
    CRY_EMOTE, PIXEL_EMOTE, GLEE_EMOTE, EXCITED_EMOTE, HEARTS_EMOTE, UWU_EMOTE,
};

/** @brief Resting emotes for MAC and PC modes */
const char *restingEmotes[] = {REST_EMOTE, IDLE_EMOTE, LOOK_DOWN_EMOTE,
                               LOOK_UP_EMOTE, LOOK_LEFT_RIGHT_EMOTE};
#else
// File paths for emotes (replace the const arrays)
/** @brief Random emotes for BYTE-90 mode */
const char *randomEmotes[] = {
    WINK_02_EMOTE, ZONED_EMOTE, DOUBTFUL_EMOTE, TALK_EMOTE, SCAN_EMOTE,
    ANGRY_EMOTE, CRY_EMOTE, PIXEL_EMOTE, EXCITED_EMOTE, HEARTS_EMOTE,
    UWU_EMOTE, WHISTLE_EMOTE, GLEE_EMOTE, MISCHIEF_EMOTE, HUMSUP_EMOTE,
};

/** @brief Resting emotes for BYTE-90 mode */
const char *restingEmotes[] = {
    REST_EMOTE,
    IDLE_EMOTE,
    LOOK_DOWN_EMOTE,
    LOOK_UP_EMOTE,
    LOOK_LEFT_RIGHT_EMOTE,
};
#endif

//==============================================================================
// ANIMATION PLAYBACK FUNCTIONS
//==============================================================================

/**
 * @brief Play a GIF animation with interaction detection
 *
 * This is a blocking function that plays a GIF file while monitoring
 * for interactions that might interrupt the animation. Critical data polling
 * is performed within this function to ensure timely updates.
 *
 * @param filename Path to the GIF file to play
 * @return true if playback completed successfully
 */
bool playGIF(const char *filename) {
  const unsigned long TIMEOUT_MS = 10000;
  unsigned long startTime = millis();
  unsigned long frameTime = micros();
  unsigned long lastCheck = 0;
  const unsigned long INTERACTION_CHECK_DEBOUNCE = 10;

  if (!loadGIF(filename)) {
    ESP_LOGE(GIF_LOG, "ERROR: Failed to load GIF File.");
    return false;
  }

  while (playGIFFrame(false, NULL)) {
    // Efficient frame timing
    unsigned long currentTime = micros();
    unsigned long elapsed = currentTime - frameTime;
    // Native GIF framerate is 16FPS, ensure that playback matches 16FPS
    if (elapsed < FRAME_DELAY_MICROSECONDS) {
      delayMicroseconds(FRAME_DELAY_MICROSECONDS - elapsed);
    }
    frameTime = micros();

    currentTime = millis();
    // This is our ADXL and interactions polling to break and trigger specific
    // animations
    if (currentTime - lastCheck >= INTERACTION_CHECK_DEBOUNCE) {
      ADXLDataPolling();

      if (menu_isActive()) {
        ESP_LOGI(GIF_LOG, "Menu active - stopping GIF playback");
        break;
      }

      if (getCurrentMode() == SystemMode::UPDATE_MODE) {
        break;
      }

      if (espNowToggledState()) {
        break;
      }
      // NOTE: The order of these checks is important
      if (motionInteracted()) {
        if (motionDoubleTapped()) {
          break;
        }
        if (motionTapped()) {
          break;
        }
        if (motionShaking()) {
          break;
        }
        if (motionSuddenAcceleration()) {
          break;
        }
      }

      if ((motionTiltedLeft() || motionTiltedRight() || motionUpsideDown()) && strcmp(filename, CRASH01_EMOTE) != 0 &&
          strcmp(filename, CRASH02_EMOTE) != 0 && strcmp(filename, SHOCK_EMOTE) != 0) {
        break;
      }

      lastCheck = currentTime;
    }

    // Timeout check
    if (currentTime - startTime > TIMEOUT_MS) {
      ESP_LOGE(GIF_LOG, "ERROR: GIF playback timeout");
      break;
    }
  }

  stopGifPlayback();
  return true;
}

/**
 * @brief Initialize the animation module
 *
 * Sets up initial state for animation sequences and state tracking.
 */
void initializeAnimationModule() {
  // Initialize animation module state
  currentCrashState = CrashState::NONE;
  wasCrashed = false;
  currentSleepState = SleepState::NONE;
  wasAsleep = false;
  lastCheckComs = 0;

  // Initialize animation sequence
  animSequence.currentState = SequenceState::REST_START;
  animSequence.stateStartTime = 0;
  animSequence.isIdleMode = true;
}

/**
 * @brief Play a random emote from a collection
 *
 * Selects and plays an emote from the provided collection, ensuring
 * that emotes don't repeat until all have been played.
 *
 * @param emotes Array of emote file paths
 * @param count Number of emotes in the array
 */
void randomizeEmotes(const char **emotes, size_t count) {
  if (!emotes || count == 0) {
    ESP_LOGE(ANIM_LOG, "ERROR: Invalid emote parameters");
    return;
  }

  static uint8_t *unplayedEmotes = NULL;
  static size_t arraySize = 0;
  static size_t remainingCount = 0;

  // Reallocate if array size changed
  if (arraySize != count) {
    if (unplayedEmotes)
      free(unplayedEmotes);
    unplayedEmotes = (uint8_t *)malloc(count * sizeof(uint8_t));
    arraySize = count;
    remainingCount = 0; // Force reset
  }

  // If no emotes left, reset the pool
  if (remainingCount == 0) {
    // Initially fill the unplayed emotes array with all indices
    for (size_t i = 0; i < count; i++) {
      unplayedEmotes[i] = i;
    }
    remainingCount = count;
  }
  // Randomly select from remaining unplayed emotes
  size_t randomPos = random(remainingCount);
  uint8_t selectedIndex = unplayedEmotes[randomPos];
  // Replace the selected emote with the last unplayed emote
  unplayedEmotes[randomPos] = unplayedEmotes[remainingCount - 1];
  remainingCount--;
  // Play the selected emote
  playGIF(emotes[selectedIndex]);
}

//==============================================================================
// STATE HANDLING FUNCTIONS
//==============================================================================

/**
 * @brief Handle crash animations based on device orientation
 *
 * Manages the three stages of crash animations: entering crash,
 * crashed state, and recovery from crash.
 *
 * @return true if crash animation was played
 */
bool checkCrashOrientation() {
  // Crash animation has 3 stages of animation, entering into a crash, crashing,
  // and exiting a crash

  if (motionTiltedLeft() || motionTiltedRight() || motionUpsideDown()) {
    // First time entering crash state
    if (currentCrashState == CrashState::NONE) {
      currentCrashState = CrashState::ENTERING_CRASH;
      playGIF(CRASH01_EMOTE); // Initial impact animation
      currentCrashState = CrashState::CRASHED;
      wasCrashed = true;
      return true;
    }
    // Continue being crashed
    else if (currentCrashState == CrashState::CRASHED) {
      playGIF(CRASH02_EMOTE); // Dizzy/crashed animation loop
      return true;
    }
  }
  // Recovering from crash
  else if (wasCrashed) {
    currentCrashState = CrashState::RECOVERING;
    playGIF(CRASH03_EMOTE); // Recovery animation
    currentCrashState = CrashState::NONE;
    wasCrashed = false;
    return true;
  }

  return false;
}

/**
 * @brief Handle sleep animations based on device state
 *
 * Manages the three stages of sleep animations: entering sleep,
 * sleeping state, and waking up.
 *
 * @return true if sleep animation was played
 */
bool handleSleepSequence() {
  // Sleep animation has 3 stages of animation, entering into a crash, crashing,
  // and exiting a crash
  if (motionSleep()) {
    // First time entering sleep
    if (currentSleepState == SleepState::NONE) {
      currentSleepState = SleepState::ENTERING_SLEEP;
      playGIF(SLEEP01_EMOTE); // Play entering sleep animation once
      currentSleepState = SleepState::SLEEPING;
      wasAsleep = true;
      return true;
    }
    // Continue sleeping
    else if (currentSleepState == SleepState::SLEEPING) {
      playGIF(SLEEP02_EMOTE); // Play sleeping animation repeatedly
      return true;
    }
  }
  // Waking up from sleep
  else if (wasAsleep) {
    currentSleepState = SleepState::EXITING_SLEEP;
    playGIF(SLEEP03_EMOTE); // Play waking animation once
    currentSleepState = SleepState::NONE;
    wasAsleep = false;
    return true;
  }

  return false;
}

/**
 * @brief Handle special device states and related animations
 *
 * Checks for and responds to special states like ESP-NOW toggling,
 * motion interactions, crash states, and sleep states.
 *
 * @return true if a special state was handled and animation played
 */
bool handleSpecialStates() {
  // If Inactivity timed out and deep sleep kicks in loop REST_EMOTE
  if (espNowToggledState()) {
    resetEspNowToggleState();
    if (getCurrentESPNowState() == ESPNowState::ON) {
      // ESP-NOW was just turned ON
      playGIF(COMS_CONNECT_EMOTE); // Play connection animation
    } else {
      // ESP-NOW was just turned OFF
      playGIF(COMS_DISCONNECT_EMOTE); // Play disconnection animation
    }
    return true;
  }

  if (motionDeepSleep()) {
    setMotionState(MotionStateType::DEEP_SLEEP, false);
    stopGifPlayback();
    return true;
  }

  // Check motion interrupts, NOTE: the order of these checks is important
  if (motionInteracted()) {
    if (motionShaking()) {
      setMotionState(MotionStateType::SHAKING, false);
      playGIF(DIZZY_EMOTE);
      return true;
    }
    if (motionDoubleTapped()) {
      setMotionState(MotionStateType::DOUBLE_TAPPED, false);
      playGIF(SHOCK_EMOTE);
      return true;
    }
    if (motionTapped()) {
      setMotionState(MotionStateType::TAPPED, false);
      playGIF(TAP_EMOTE);
      return true;
    }
    if (motionSuddenAcceleration()) {
      setMotionState(MotionStateType::SUDDEN_ACCELERATION, false);
      playGIF(STARTLED_EMOTE);
      return true;
    }
  }

  if ((motionHalfTiltedLeft() || motionHalfTiltedRight()) &&
      !motionTiltedLeft() && !motionTiltedRight() && !motionUpsideDown()) {
    playGIF(SHOCK_EMOTE);
    return true;
  }

  // Check for crash state - either actively crashed or recovering
  if (motionOriented() || wasCrashed) {
    if (checkCrashOrientation()) {
      return true;
    }
  }

  // Check sleep state
  if (motionSleep() || wasAsleep) {
    if (handleSleepSequence()) {
      return true;
    }
  }

  return false;
}

/**
 * @brief Handle normal animation sequence when no special states are active
 *
 * Manages the standard animation cycle between rest and active states.
 *
 * @param currentTime Current system time in milliseconds
 */
void handleAnimationSequence(unsigned long currentTime) {
  switch (animSequence.currentState) {
  case SequenceState::REST_START:
    playGIF(WINK_EMOTE);
    animSequence.stateStartTime = currentTime;
    animSequence.currentState = SequenceState::ANIMATION_CYCLE;
    break;

  case SequenceState::ANIMATION_CYCLE:
    if (currentTime - animSequence.stateStartTime >= animSequence.STATE_DELAY) {
      // Set and play appropriate emote collection
      if (animSequence.isIdleMode) {
        currentEmotes = restingEmotes;
        emoteCount = ARRAY_SIZE(restingEmotes);
      } else {
        currentEmotes = randomEmotes;
        emoteCount = ARRAY_SIZE(randomEmotes);
      }

      randomizeEmotes(currentEmotes, emoteCount);
      animSequence.isIdleMode = !animSequence.isIdleMode;

      // Move to end rest if cycle is complete
      if (animSequence.isIdleMode) {
        animSequence.currentState = SequenceState::REST_END;
        animSequence.stateStartTime = currentTime;
      }
    }
    break;

  case SequenceState::REST_END:
    playGIF(BLINK_EMOTE);
    if (currentTime - animSequence.stateStartTime >= animSequence.IDLE_DELAY) {
      animSequence.currentState = SequenceState::REST_START;
      animSequence.stateStartTime = currentTime;
    }
    break;
  }
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================
/**
 * @brief Main function to handle emote playback
 *
 * Coordinates the playback of animations based on device state,
 * ESP-NOW communication, and normal animation sequences.
 * Called in the main loop when in ESP mode.
 */
void playEmotes() {
  // Don't start any animations if we're in UPDATE_MODE
  if (getCurrentMode() == SystemMode::UPDATE_MODE) {
    return;
  }

    // ADD: Don't start animations if menu is active
  if (menu_isActive()) {
    return;
  }

  if (!gifPlayerInitialized()) {
    ESP_LOGE(ANIM_LOG, "ERROR: GIF player not initialized");
    return;
  }

  unsigned long currentTime = millis();

  // Handle special states first (motion interrupts and sleep)
  if (handleSpecialStates()) {
    return;
  }

  if (animSequence.currentState == SequenceState::ANIMATION_CYCLE) {
    if (currentTime - lastCheckComs >= COMS_CHECK_INTERVAL) {
      if (getCurrentESPNowState() == ESPNowState::ON && !isPaired()) {
        playGIF(COMS_CONNECT_EMOTE);
        lastCheckComs = currentTime;
      }
      // Uncomment this if you want to periodically check when ESP-NOW is turned off
      // else if (getCurrentESPNowState() == ESPNowState::OFF) {
      //   playGIF(COMS_DISCONNECT_EMOTE);
      //   lastCheckComs = currentTime;
      // }
    }
  }

  if (isPaired()) {
    switch (getCurrentComState()) {
    case ComState::PROCESSING:
      if (getCurrentAnimationPath() != nullptr) {
        playGIF(getCurrentAnimationPath());
      }
      break;
    case ComState::WAITING:
      playGIF(COMS_IDLE_EMOTE);
      break;
    }
  } else {
    resetAnimationPath();
    handleAnimationSequence(millis());
  }
}

/**
 * @brief Play the boot animation
 *
 * Shows the startup animation when the device boots.
 */
void playBootAnimation() {
  if (!gifPlayerInitialized()) {
    ESP_LOGE(ANIM_LOG, "ERROR: GIF player not initialized");
    return;
  }
  playGIF(STARTUP_EMOTE);
}