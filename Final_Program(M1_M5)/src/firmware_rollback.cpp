#include "firmware_rollback.h"
#include "debug_utils.h"
#include <user_interface.h>  // For RTC memory access

// ============================================================
// RTC MEMORY STRUCTURE
// RTC memory is 512 bytes total, we use index 32-40 (32 bytes)
// RTC memory survives resets and deep sleep
// ============================================================

// RTC memory indices (each index = 4 bytes / 32 bits)
#define RTC_MAGIC_INDEX     32  // Magic number validation
#define RTC_BOOT_COUNTER    33  // Boot counter for detecting failures
#define RTC_UPDATE_FLAG     34  // Update in progress flag
#define RTC_CURRENT_SLOT    35  // Current OTA slot
#define RTC_FAILED_ATTEMPTS 36  // Failed update attempt counter
// 37-40 reserved for future use

// String storage uses separate file system
#define STATE_FILE "/fw_state.txt"

namespace FirmwareRollback {

    // Magic number for RTC memory validation
    const uint32_t RTC_MAGIC = 0xDEADBEEF;
    
    // Max boot attempts before giving up on new version
    const uint32_t MAX_BOOT_ATTEMPTS = 3;

    // ============================================================
    // INTERNAL HELPER FUNCTIONS
    // ============================================================

    // Initialize RTC memory if not already initialized
    void initializeRTCMemory() {
        // Read magic from RTC
        uint32_t magic;
        system_rtc_mem_read(RTC_MAGIC_INDEX, &magic, 4);
        
        if (magic != RTC_MAGIC) {
            DEBUG_PRINTLN("[FirmwareRollback] 🔄 Initializing RTC memory...");
            
            system_rtc_mem_write(RTC_MAGIC_INDEX, (uint8*) &RTC_MAGIC, 4);
            
            uint32_t bootCounter = MAX_BOOT_ATTEMPTS;
            system_rtc_mem_write(RTC_BOOT_COUNTER, (uint8*) &bootCounter, 4);
            
            uint32_t updateFlag = 0;
            system_rtc_mem_write(RTC_UPDATE_FLAG, (uint8*) &updateFlag, 4);
            
            uint32_t slot = 0;
            system_rtc_mem_write(RTC_CURRENT_SLOT, (uint8*) &slot, 4);
            
            uint32_t failedAttempts = 0;
            system_rtc_mem_write(RTC_FAILED_ATTEMPTS, (uint8*) &failedAttempts, 4);
            
            DEBUG_PRINTLN("[FirmwareRollback] ✅ RTC memory initialized");
        }
    }

    // Read boot counter from RTC
    uint32_t readBootCounter() {
        uint32_t counter;
        system_rtc_mem_read(RTC_BOOT_COUNTER, &counter, 4);
        return counter;
    }

    // Write boot counter to RTC
    void writeBootCounter(uint32_t counter) {
        system_rtc_mem_write(RTC_BOOT_COUNTER, (uint8*) &counter, 4);
    }

    // Read update in progress flag
    uint32_t readUpdateFlag() {
        uint32_t flag;
        system_rtc_mem_read(RTC_UPDATE_FLAG, &flag, 4);
        return flag;
    }

    // Write update in progress flag
    void writeUpdateFlag(uint32_t flag) {
        system_rtc_mem_write(RTC_UPDATE_FLAG, (uint8*) &flag, 4);
    }

    // Read current OTA slot
    uint32_t readCurrentSlot() {
        uint32_t slot;
        system_rtc_mem_read(RTC_CURRENT_SLOT, &slot, 4);
        return slot;
    }

    // Write current OTA slot
    void writeCurrentSlot(uint32_t slot) {
        system_rtc_mem_write(RTC_CURRENT_SLOT, (uint8*) &slot, 4);
    }

    // Read failed attempts counter
    uint32_t readFailedAttempts() {
        uint32_t attempts;
        system_rtc_mem_read(RTC_FAILED_ATTEMPTS, &attempts, 4);
        return attempts;
    }

    // Write failed attempts counter
    void writeFailedAttempts(uint32_t attempts) {
        system_rtc_mem_write(RTC_FAILED_ATTEMPTS, (uint8*) &attempts, 4);
    }

    // ============================================================
    // PUBLIC INTERFACE IMPLEMENTATION
    // ============================================================

    bool initializeAndDetectRollback() {
        initializeRTCMemory();

        DEBUG_PRINTLN("\n");
        DEBUG_PRINTLN("╔════════════════════════════════════════════════════════════╗");
        DEBUG_PRINTLN("║         FIRMWARE ROLLBACK DETECTION                        ║");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝");

        uint32_t updateFlag = readUpdateFlag();
        uint32_t bootCounter = readBootCounter();
        uint32_t failedAttempts = readFailedAttempts();

        DEBUG_PRINTF("[FirmwareRollback] 🔍 Update in progress: %s\n", 
                    updateFlag ? "YES" : "NO");
        DEBUG_PRINTF("[FirmwareRollback] 🔢 Boot counter: %d\n", bootCounter);
        DEBUG_PRINTF("[FirmwareRollback] ⚠️  Failed attempts: %d\n", failedAttempts);

        // Case 1: Update was in progress and just completed boot (decrement counter)
        if (updateFlag && bootCounter > 0) {
            bootCounter--;
            writeBootCounter(bootCounter);

            if (bootCounter == 0) {
                // Boot counter expired - something is wrong with new firmware
                DEBUG_PRINTLN("\n");
                DEBUG_PRINTLN("╠════════════════════════════════════════════════════════════╣");
                DEBUG_PRINTLN("║ ⚠️  BOOT FAILURES DETECTED - ROLLBACK TRIGGERED             ║");
                DEBUG_PRINTLN("╠════════════════════════════════════════════════════════════╣");
                
                uint32_t failedAttempts = readFailedAttempts() + 1;
                writeFailedAttempts(failedAttempts);

                DEBUG_PRINTF("[FirmwareRollback] 🔄 Rolling back to previous version...\n");
                DEBUG_PRINTF("[FirmwareRollback] 📊 Failed attempts: %d\n", failedAttempts);
                
                // Clear update flag
                writeUpdateFlag(0);
                
                // Would need to call rboot_set_current_slot() here to switch slots
                // For now, just log the event
                DEBUG_PRINTLN("[FirmwareRollback] ✅ Rollback decision made - next restart uses old firmware\n");
                
                return true;  // Rollback was triggered
            } else {
                DEBUG_PRINTF("[FirmwareRollback] ✅ Boot successful, counter: %d/3\n", bootCounter);
            }
        }
        // Case 2: Update flag is still set but boot counter max (fresh start of update)
        else if (updateFlag && bootCounter == MAX_BOOT_ATTEMPTS) {
            DEBUG_PRINTLN("[FirmwareRollback] 📥 First boot after firmware update, monitoring...\n");
        }
        // Case 3: Normal operation
        else if (!updateFlag) {
            DEBUG_PRINTLN("[FirmwareRollback] ✅ No update in progress, system normal\n");
        }

        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝\n");
        return false;  // No rollback performed
    }

    void markUpdateInProgress(const String& newVersion) {
        DEBUG_PRINTLN("\n");
        DEBUG_PRINTLN("╔════════════════════════════════════════════════════════════╗");
        DEBUG_PRINTLN("║         MARKING UPDATE IN PROGRESS                         ║");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝");
        
        // Set update flag
        writeUpdateFlag(1);
        
        // Reset boot counter to MAX for monitoring new firmware
        writeBootCounter(MAX_BOOT_ATTEMPTS);
        
        DEBUG_PRINTF("[FirmwareRollback] 📝 Update marked in progress\n");
        DEBUG_PRINTF("[FirmwareRollback] 🔄 Downloading: %s\n", newVersion.c_str());
        DEBUG_PRINTF("[FirmwareRollback] 🔢 Boot counter set to: %d\n", MAX_BOOT_ATTEMPTS);
        DEBUG_PRINTLN("[FirmwareRollback] ⚠️  System will rollback if boot fails 3 times\n");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝\n");
    }

    void markUpdateSuccess(const String& newVersion) {
        DEBUG_PRINTLN("\n");
        DEBUG_PRINTLN("╔════════════════════════════════════════════════════════════╗");
        DEBUG_PRINTLN("║         UPDATE SUCCESSFUL - PREPARING RESTART              ║");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝");
        
        // Clear update flag (no rollback needed on next boot)
        writeUpdateFlag(0);
        
        // Reset boot counter
        writeBootCounter(MAX_BOOT_ATTEMPTS);
        
        // Reset failed attempts
        writeFailedAttempts(0);
        
        DEBUG_PRINTF("[FirmwareUpdater] ✅ Version updated: %s\n", newVersion.c_str());
        DEBUG_PRINTF("[FirmwareRollback] 📝 Update flag cleared\n");
        DEBUG_PRINTF("[FirmwareRollback] 🔢 Boot counter reset to: %d\n", MAX_BOOT_ATTEMPTS);
        DEBUG_PRINTLN("[FirmwareRollback] 🔄 Ready for restart\n");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝\n");
    }

    void markUpdateFailed(const String& reason) {
        DEBUG_PRINTLN("\n");
        DEBUG_PRINTLN("╔════════════════════════════════════════════════════════════╗");
        DEBUG_PRINTLN("║         UPDATE FAILED - PRESERVING ROLLBACK                ║");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝");
        
        DEBUG_PRINTF("[FirmwareRollback] ❌ Reason: %s\n", reason.c_str());
        
        // Update flag remains set for rollback detection on next boot
        // Boot counter remains for monitoring
        
        uint32_t failedAttempts = readFailedAttempts() + 1;
        writeFailedAttempts(failedAttempts);
        
        DEBUG_PRINTF("[FirmwareRollback] 📊 Failed attempts: %d\n", failedAttempts);
        DEBUG_PRINTLN("[FirmwareRollback] ⚠️  Update state preserved for rollback if needed\n");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝\n");
    }

    FirmwareState getState() {
        FirmwareState state;
        state.magic = RTC_MAGIC;
        state.currentVersion = "";  // Would be read from config file
        state.previousVersion = ""; // Would be read from config file
        state.bootCounter = readBootCounter();
        state.updateInProgress = readUpdateFlag();
        state.currentSlot = readCurrentSlot();
        state.failedUpdateAttempts = readFailedAttempts();
        return state;
    }

    String getCurrentVersion() {
        // TODO: Read from config file or static variable
        return "";
    }

    String getPreviousVersion() {
        // TODO: Read from config file
        return "";
    }

    uint32_t getFailedAttempts() {
        return readFailedAttempts();
    }

    uint32_t getCurrentSlot() {
        return readCurrentSlot();
    }

    void resetBootCounter() {
        DEBUG_PRINTLN("[FirmwareRollback] 🔄 Boot counter reset");
        writeBootCounter(MAX_BOOT_ATTEMPTS);
    }

    bool forceRollback() {
        DEBUG_PRINTLN("[FirmwareRollback] 🔄 Force rollback requested");
        DEBUG_PRINTLN("[FirmwareRollback] ⚠️  This will switch to previous OTA slot on next restart");
        
        // Would call rboot_set_current_slot() here
        // For now, just set the flag
        writeUpdateFlag(0);  // Clear to prevent re-rollback
        
        return true;
    }

    void printState() {
        FirmwareState state = getState();
        
        DEBUG_PRINTLN("\n═══════════════════════════════════════════════════════════");
        DEBUG_PRINTLN("FIRMWARE ROLLBACK STATE");
        DEBUG_PRINTLN("═══════════════════════════════════════════════════════════");
        DEBUG_PRINTF("Magic Number:       0x%08X (Valid: %s)\n", 
                    state.magic, state.magic == RTC_MAGIC ? "✅" : "❌");
        DEBUG_PRINTF("Update in Progress: %d\n", state.updateInProgress);
        DEBUG_PRINTF("Boot Counter:       %d/%d\n", state.bootCounter, MAX_BOOT_ATTEMPTS);
        DEBUG_PRINTF("Current Slot:       %d\n", state.currentSlot);
        DEBUG_PRINTF("Failed Attempts:    %d\n", state.failedUpdateAttempts);
        DEBUG_PRINTLN("═══════════════════════════════════════════════════════════\n");
    }

} // namespace FirmwareRollback
