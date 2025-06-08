/**
 * @file flash_module.h
 * @brief Header for flash memory and filesystem operations
 * 
 * This module provides functions for initializing and interacting with the
 * LittleFS filesystem, including operations for checking file existence,
 * tracking storage statistics, and managing the filesystem state.
 */

 #ifndef FLASH_MODULE_H
 #define FLASH_MODULE_H
 
 #include "common.h"
 #include <LittleFS.h>
 
 //==============================================================================
 // CONSTANTS & DEFINITIONS
 //==============================================================================
 
 /** @brief Log tag for Flash module messages */
 static const char* FLASH_LOG = "::FLASH_MODULE::";
 
 //==============================================================================
 // TYPE DEFINITIONS
 //==============================================================================
 
 /**
  * @brief Status codes for filesystem operations
  */
 enum class FSStatus {
   FS_SUCCESS,       /**< Operation completed successfully */
   FS_MOUNT_FAILED,  /**< Failed to mount the filesystem */
   FS_FORMAT_FAILED, /**< Failed to format the filesystem */
   FS_FILE_MISSING   /**< Requested file does not exist */
 };

 struct StorageInfo {
  int gifCount;          // Number of GIF files
  float usedSpaceMB;     // Space used in MB
  float totalSpaceMB;    // Total space in MB
  float freeSpaceMB;     // Free space in MB
};
 
 //==============================================================================
 // FILESYSTEM INITIALIZATION AND STATUS
 //==============================================================================
 
 /**
  * @brief Initialize the filesystem
  * 
  * Attempts to mount the LittleFS filesystem. If mounting fails and formatOnFail
  * is true, it will attempt to format the filesystem before retrying.
  * 
  * @param formatOnFail If true, format the filesystem if mounting fails (default: false)
  * @return FSStatus indicating success or specific failure type
  */
 FSStatus initializeFS(bool formatOnFail = false);
 
 /**
  * @brief Check if filesystem is initialized
  * 
  * @return true if filesystem is initialized, false otherwise
  */
 bool getFSStatus();
 
 /**
  * @brief Check if a file exists in the filesystem
  * 
  * @param path Path to the file to check
  * @return true if file exists, false otherwise or if filesystem not initialized
  */
 bool fileExists(const char* path);
 
 //==============================================================================
 // FILESYSTEM STATISTICS
 //==============================================================================
 
 /**
  * @brief Update filesystem statistics (total, used, free space)
  * 
  * Retrieves current filesystem statistics and updates internal tracking variables.
  */
 StorageInfo updateFlashStats();
 
 /**
  * @brief Get total filesystem capacity
  * 
  * @return Total space in bytes
  */
 size_t getTotalSpace();
 
 /**
  * @brief Get used filesystem space
  * 
  * @return Used space in bytes
  */
 size_t getUsedSpace();
 
 /**
  * @brief Get free filesystem space
  * 
  * @return Free space in bytes
  */
 size_t getFreeSpace();
 
 #endif /* FLASH_MODULE_H */