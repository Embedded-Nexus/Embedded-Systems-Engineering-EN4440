#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "debug_utils.h"

// ============================================================
// FIRMWARE VERSION CONFIGURATION
// Change this value when building new firmware versions
// Format: "major.minor.patch" (e.g., "1.0.0", "1.2.3")
// ============================================================
#define FIRMWARE_VERSION "1.0.0"

namespace FirmwareUpdater {

    // Initialize firmware updater with endpoint and firmware version
    // Version should be in "major.minor.patch" format (e.g., "1.0.0", "1.2.3")
    void begin(const String& firmwareEndpoint, const String& firmwareVersion);

    // Check for firmware updates with version comparison
    // Fetches version from server and updates if newer
    // Returns true if update was performed (device will restart)
    // Returns false if no update needed or error occurred
    bool checkForUpdate();

    // Get current firmware version
    String getCurrentVersion();

    // Set current firmware version
    void setCurrentVersion(const String& version);

    // Handler to be called from UploadManager at start of each upload cycle
    // Checks firmware version and updates if newer version available
    void handle();

    // Version comparison: returns true if serverVersion > deviceVersion
    // Both versions should be in "major.minor.patch" format (e.g., "1.2.3")
    bool isVersionNewer(const String& serverVersion, const String& deviceVersion);

    // Fetch version from server (makes HTTP HEAD request)
    // Returns server version string or empty string if failed
    String fetchServerVersion();

} // namespace FirmwareUpdater

#endif // FIRMWARE_UPDATER_H
