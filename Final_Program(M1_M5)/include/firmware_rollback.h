#ifndef FIRMWARE_ROLLBACK_H
#define FIRMWARE_ROLLBACK_H

#include <Arduino.h>

// ============================================================
// FIRMWARE ROLLBACK MECHANISM
// 
// Provides automatic rollback capability using RTC memory:
// - Tracks current and previous firmware versions
// - Implements boot counter for safety
// - Detects failed updates and reverts if needed
// - Manages OTA slot switching
// ============================================================

namespace FirmwareRollback {

    // Firmware state structure (stored in RTC memory - survives resets)
    struct FirmwareState {
        uint32_t magic;                 // Magic number for validation (0xDEADBEEF)
        String currentVersion;          // Currently running firmware version
        String previousVersion;         // Last known working firmware version
        uint32_t bootCounter;           // Counter for detecting boot failures
        bool updateInProgress;          // Flag: set before OTA, cleared after success
        uint32_t currentSlot;           // Current OTA slot (0 or 1)
        uint32_t failedUpdateAttempts;  // Count of failed update attempts
    };

    // ============================================================
    // BOOT-TIME INITIALIZATION & ROLLBACK DETECTION
    // ============================================================

    // Call this at the very start of setup() BEFORE any other initialization
    // Detects if previous update failed and triggers rollback if needed
    // Returns: true if rollback was performed, false if system is normal
    bool initializeAndDetectRollback();

    // ============================================================
    // OTA UPDATE LIFECYCLE MANAGEMENT
    // ============================================================

    // Call this BEFORE starting an OTA update
    // Marks update as in-progress to enable rollback detection on next boot
    // Pass the new version being downloaded
    void markUpdateInProgress(const String& newVersion);

    // Call this AFTER a successful OTA update (before restart)
    // Clears the in-progress flag and saves new version as current
    // Boot counter is reset to 3 for monitoring
    void markUpdateSuccess(const String& newVersion);

    // Call this if OTA update FAILS at any point
    // Clears in-progress flag but doesn't change versions (preserves rollback)
    void markUpdateFailed(const String& reason);

    // ============================================================
    // STATE QUERY & MANAGEMENT
    // ============================================================

    // Get current firmware state
    FirmwareState getState();

    // Get current version running
    String getCurrentVersion();

    // Get previous working version (for rollback reference)
    String getPreviousVersion();

    // Get number of failed update attempts
    uint32_t getFailedAttempts();

    // Get current OTA slot number (0 or 1)
    uint32_t getCurrentSlot();

    // Manually reset boot counter (for testing/recovery)
    void resetBootCounter();

    // Manually trigger a rollback to previous slot (emergency)
    // Returns: true if rollback was possible, false if already on last slot
    bool forceRollback();

    // ============================================================
    // LOGGING & DEBUG
    // ============================================================

    // Print current state to serial for debugging
    void printState();

} // namespace FirmwareRollback

#endif // FIRMWARE_ROLLBACK_H
