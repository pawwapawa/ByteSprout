/**
 * @file flash_module.cpp
 * @brief Implementation of flash memory and filesystem operations
 * 
 * This module provides functions for initializing and interacting with the
 * LittleFS filesystem, including operations for checking file existence,
 * tracking storage statistics, and managing the filesystem state.
 */

 #include "common.h"
 #include "flash_module.h"
 
 //==============================================================================
 // GLOBAL VARIABLES
 //==============================================================================
 
 /** @brief Flag indicating if filesystem is initialized */
 bool FSInitialized = false;
 /** @brief Total bytes available in the filesystem */
 static size_t totalBytes = 0;
 /** @brief Bytes currently used in the filesystem */
 static size_t usedBytes = 0;
 
 //==============================================================================
 // INTERNAL FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Update filesystem statistics (total, used, free space)
  * 
  * Retrieves current filesystem statistics and updates internal tracking variables.
  * The function will log a warning if called when filesystem is not initialized.
  */
 StorageInfo updateFlashStats() {
  const float KB = 1024.0;
  const float MB = KB * 1024.0;
  StorageInfo info = {0, 0, 0, 0};  // Initialize all values to 0

  if (!FSInitialized) {
    ESP_LOGW(FLASH_LOG, "Cannot update stats: filesystem not initialized");
    return info;
  }

  // Get basic storage information directly
  info.totalSpaceMB = LittleFS.totalBytes() / MB;
  info.usedSpaceMB = LittleFS.usedBytes() / MB;
  info.freeSpaceMB = info.totalSpaceMB - info.usedSpaceMB;
  
  // Update global tracking variables if needed
  totalBytes = LittleFS.totalBytes();
  usedBytes = LittleFS.usedBytes();

  // Calculate percentage used for logging
  float percentUsed = (info.totalSpaceMB > 0) ? (info.usedSpaceMB * 100.0f / info.totalSpaceMB) : 0;

  // Count GIF files
  File root = LittleFS.open("/gifs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (strstr(file.name(), ".gif")) {
        info.gifCount++;
      }
      file = root.openNextFile();
    }
    root.close();
  }

  // Log the information
  ESP_LOGW(FLASH_LOG, "Storage Stats: %.2f%% used (%.2f/%.2f MB), %.2f MB free, %d GIFs", 
           percentUsed, info.usedSpaceMB, info.totalSpaceMB, info.freeSpaceMB, info.gifCount);

  return info;
}

bool checkFileStatus() {
  // Check for required directories
  const char *requiredDirs[] = {"/gifs"};
  bool dirMissing = false;
  
  for (const char *dir : requiredDirs) {
    if (!LittleFS.exists(dir) || !LittleFS.open(dir).isDirectory()) {
      ESP_LOGW(FLASH_LOG, "Warning: Required directory %s not found", dir);
      dirMissing = true;
    }
  }

  // Check for required files
  const char *requiredFiles[] = {"/index.html", "/styles.css", "/script.js"};
  bool fileMissing = false;
  
  for (const char *file : requiredFiles) {
    if (!fileExists(file)) {
      ESP_LOGW(FLASH_LOG, "Warning: Required file %s not found", file);
      fileMissing = true;
    }
  }

  // Check for essential GIF files
  const char *essentialGifs[] = {
    "/gifs/startup.gif", 
    "/gifs/rest.gif"
    // Add other critical GIFs here
  };
  
  bool gifMissing = false;
  for (const char *gif : essentialGifs) {
    if (!fileExists(gif)) {
      ESP_LOGW(FLASH_LOG, "Warning: Essential GIF %s not found", gif);
      gifMissing = true;
    }
  }
 
  if (dirMissing || fileMissing || gifMissing) {
    ESP_LOGW(FLASH_LOG, "Please ensure you have uploaded the complete data folder");
    return false;
  }
  return true;
}
 
 //==============================================================================
 // PUBLIC API FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Initialize the filesystem
  * 
  * Attempts to mount the LittleFS filesystem. If mounting fails and formatOnFail
  * is true, it will attempt to format the filesystem before retrying.
  * 
  * @param formatOnFail If true, format the filesystem if mounting fails
  * @return FSStatus indicating success or specific failure type
  */
 FSStatus initializeFS(bool formatOnFail) {
  if (FSInitialized) {
    return FSStatus::FS_SUCCESS;
  }
   
  // Try to mount the filesystem
  if (!LittleFS.begin(false)) {
    ESP_LOGE(FLASH_LOG, "Failed to mount LittleFS");
    // Format if requested and retry
    if (formatOnFail) {
      ESP_LOGW(FLASH_LOG, "Formatting filesystem...");
      if (!LittleFS.begin(true)) {
        ESP_LOGE(FLASH_LOG, "Failed to format and mount LittleFS");
        return FSStatus::FS_FORMAT_FAILED;
      }
    } else {
      return FSStatus::FS_MOUNT_FAILED;
    }
  }
 
  FSInitialized = true;
  updateFlashStats();
  if (!checkFileStatus()) {
    return FSStatus::FS_FILE_MISSING;
  }
  return FSStatus::FS_SUCCESS;
}
 
 /**
  * @brief Check if filesystem is initialized
  * 
  * @return true if filesystem is initialized, false otherwise
  */
 bool getFSStatus() {
   return FSInitialized;
 }
 
 /**
  * @brief Check if a file exists in the filesystem
  * 
  * @param path Path to the file to check
  * @return true if file exists, false otherwise or if filesystem not initialized
  */
 bool fileExists(const char* path) {
   if (!FSInitialized) {
     return false;
   }
   return LittleFS.exists(path);
 }
 
 /**
  * @brief Get total filesystem capacity
  * 
  * @return Total space in bytes
  */
 size_t getTotalSpace() {
   return totalBytes;
 }
 
 /**
  * @brief Get used filesystem space
  * 
  * @return Used space in bytes
  */
 size_t getUsedSpace() {
   return usedBytes;
 }
 
 /**
  * @brief Get free filesystem space
  * 
  * @return Free space in bytes
  */
 size_t getFreeSpace() {
   return totalBytes - usedBytes;
 }