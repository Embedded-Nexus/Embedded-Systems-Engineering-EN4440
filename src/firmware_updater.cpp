#include "firmware_updater.h"
#include <ESP8266WiFi.h>

namespace FirmwareUpdater {

    // ============================================================
    // Static variables - DEFAULT FIRMWARE VERSION
    // ============================================================
    static String firmwareEndpoint = "";
    static String currentVersion = "1.0.0";  // ← DEFAULT VERSION (change this value)
    static bool isChecking = false;

    // ============================================================
    // Version comparison function
    // Compares two versions in "major.minor.patch" format
    // Returns: true if serverVersion > currentVersion
    // ============================================================
    bool isVersionNewer(const String& serverVersion, const String& deviceVersion) {
        int serverMajor = 0, serverMinor = 0, serverPatch = 0;
        int deviceMajor = 0, deviceMinor = 0, devicePatch = 0;

        // Parse server version (e.g., "1.2.3")
        if (sscanf(serverVersion.c_str(), "%d.%d.%d", &serverMajor, &serverMinor, &serverPatch) != 3) {
            DEBUG_PRINTF("[FirmwareUpdater] ⚠️ Invalid server version format: %s\n", serverVersion.c_str());
            return false;
        }

        // Parse device version (e.g., "1.0.0")
        if (sscanf(deviceVersion.c_str(), "%d.%d.%d", &deviceMajor, &deviceMinor, &devicePatch) != 3) {
            DEBUG_PRINTF("[FirmwareUpdater] ⚠️ Invalid device version format: %s\n", deviceVersion.c_str());
            return false;
        }

        // Compare versions: major.minor.patch
        DEBUG_PRINTF("[FirmwareUpdater] 📊 Version comparison:\n");
        DEBUG_PRINTF("[FirmwareUpdater]    Server:  %d.%d.%d\n", serverMajor, serverMinor, serverPatch);
        DEBUG_PRINTF("[FirmwareUpdater]    Device:  %d.%d.%d\n", deviceMajor, deviceMinor, devicePatch);

        if (serverMajor > deviceMajor) {
            DEBUG_PRINTLN("[FirmwareUpdater] ✅ Major version higher on server");
            return true;
        }
        if (serverMajor < deviceMajor) {
            DEBUG_PRINTLN("[FirmwareUpdater] ⚠️ Device has newer major version (downgrade prevented)");
            return false;
        }

        // Major versions are equal, check minor
        if (serverMinor > deviceMinor) {
            DEBUG_PRINTLN("[FirmwareUpdater] ✅ Minor version higher on server");
            return true;
        }
        if (serverMinor < deviceMinor) {
            DEBUG_PRINTLN("[FirmwareUpdater] ⚠️ Device has newer minor version");
            return false;
        }

        // Major and minor are equal, check patch
        if (serverPatch > devicePatch) {
            DEBUG_PRINTLN("[FirmwareUpdater] ✅ Patch version higher on server");
            return true;
        }
        if (serverPatch < devicePatch) {
            DEBUG_PRINTLN("[FirmwareUpdater] ⚠️ Device has newer patch version");
            return false;
        }

        // All versions are equal
        DEBUG_PRINTLN("[FirmwareUpdater] ℹ️ Versions are identical (no update needed)");
        return false;
    }

    // ============================================================
    // Fetch version from server
    // Checks the server endpoint and returns available version
    // Header format: "version_level" (e.g., "1.0.0_2")
    // Returns just the version part (e.g., "1.0.0")
    // Returns empty string if unable to fetch version
    // ============================================================
    String fetchServerVersion() {
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN("[FirmwareUpdater] ❌ Wi-Fi not connected, cannot fetch version");
            return "";
        }

        WiFiClient client;
        HTTPClient http;

        // Build version endpoint URL (append /version to firmware endpoint)
        String versionEndpoint = firmwareEndpoint;
        if (!versionEndpoint.endsWith("/")) {
            versionEndpoint += "/";
        }
        versionEndpoint += "version";

        DEBUG_PRINTF("[FirmwareUpdater] 🌐 Fetching version info from: %s\n", versionEndpoint.c_str());

        if (!http.begin(client, versionEndpoint)) {
            DEBUG_PRINTLN("[FirmwareUpdater] ❌ Failed to connect to version endpoint");
            return "";
        }

        // Make GET request to fetch version JSON
        int httpCode = http.GET();

        if (httpCode <= 0) {
            DEBUG_PRINTF("[FirmwareUpdater] ❌ HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
            http.end();
            return "";
        }

        // Check response code
        if (httpCode != 200) {
            DEBUG_PRINTF("[FirmwareUpdater] ⚠️ Server returned: %d\n", httpCode);
            http.end();
            return "";
        }

        // Parse JSON response to extract version
        String payload = http.getString();
        http.end();

        if (payload.isEmpty()) {
            DEBUG_PRINTLN("[FirmwareUpdater] ❌ Server returned empty response");
            return "";
        }

        DEBUG_PRINTF("[FirmwareUpdater] 📋 Server response: %s\n", payload.c_str());

        // Extract version from JSON response: {"version": "1.0.0", "level": 2, ...}
        // Simple JSON parsing - look for "version": "X.Y.Z"
        int versionIndex = payload.indexOf("\"version\"");
        if (versionIndex < 0) {
            DEBUG_PRINTLN("[FirmwareUpdater] ⚠️ No 'version' field in server response");
            return "";
        }

        // Find the actual version string value (skip to first quote after colon)
        int startQuote = payload.indexOf('"', versionIndex + 10);
        int endQuote = payload.indexOf('"', startQuote + 1);

        if (startQuote < 0 || endQuote < 0) {
            DEBUG_PRINTLN("[FirmwareUpdater] ⚠️ Could not parse version string from response");
            return "";
        }

        String serverVersion = payload.substring(startQuote + 1, endQuote);

        // Also extract update level if available
        int levelIndex = payload.indexOf("\"level\"");
        int updateLevel = 0;
        if (levelIndex >= 0) {
            int levelColon = payload.indexOf(':', levelIndex);
            int levelEnd = payload.indexOf(',', levelColon);
            if (levelEnd < 0) levelEnd = payload.indexOf('}', levelColon);
            String levelStr = payload.substring(levelColon + 1, levelEnd);
            levelStr.trim();
            updateLevel = levelStr.toInt();
        }

        DEBUG_PRINTF("[FirmwareUpdater] ✅ Server version: %s (update level: %d)\n", 
                    serverVersion.c_str(), updateLevel);

        return serverVersion;
    }

    void begin(const String& endpoint, const String& firmwareVersion) {
        firmwareEndpoint = endpoint;
        currentVersion = firmwareVersion;
        DEBUG_PRINTLN("\n");
        DEBUG_PRINTLN("╔════════════════════════════════════════════════════════════╗");
        DEBUG_PRINTLN("║         FIRMWARE UPDATER INITIALIZATION                    ║");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝");
        DEBUG_PRINTF("[FirmwareUpdater] 📌 Endpoint: %s\n", endpoint.c_str());
        DEBUG_PRINTF("[FirmwareUpdater] 📦 Current Version: %s\n", currentVersion.c_str());
        DEBUG_PRINTLN("[FirmwareUpdater] ✅ Firmware updater ready\n");
    }

    String getCurrentVersion() {
        return currentVersion;
    }

    void setCurrentVersion(const String& version) {
        currentVersion = version;
        DEBUG_PRINTF("[FirmwareUpdater] Version updated to: %s\n", version.c_str());
    }

    bool checkForUpdate() {
        if (isChecking) {
            DEBUG_PRINTLN("[FirmwareUpdater] ⚠️ Already checking for updates, skipping...");
            return false;
        }

        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN("[FirmwareUpdater] ❌ Wi-Fi not connected, skipping firmware check");
            return false;
        }

        if (firmwareEndpoint.isEmpty()) {
            DEBUG_PRINTLN("[FirmwareUpdater] ❌ Firmware endpoint not configured");
            return false;
        }

        isChecking = true;

        DEBUG_PRINTLN("\n");
        DEBUG_PRINTLN("╔════════════════════════════════════════════════════════════╗");
        DEBUG_PRINTLN("║           FIRMWARE VERSION CHECK                           ║");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝");
        DEBUG_PRINTF("[FirmwareUpdater] 🌐 Endpoint: %s\n", firmwareEndpoint.c_str());
        DEBUG_PRINTF("[FirmwareUpdater] 📦 Device Version: %s\n", currentVersion.c_str());

        // ============================================================
        // Step 1: Fetch and compare version numbers
        // ============================================================
        String serverVersion = fetchServerVersion();

        if (serverVersion.isEmpty()) {
            DEBUG_PRINTLN("[FirmwareUpdater] ❌ Failed to retrieve server version");
            DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝\n");
            isChecking = false;
            return false;
        }

        DEBUG_PRINTF("[FirmwareUpdater] ☁️  Server Version: %s\n", serverVersion.c_str());

        // Check if server version is newer
        if (!isVersionNewer(serverVersion, currentVersion)) {
            DEBUG_PRINTLN("╠════════════════════════════════════════════════════════════╣");
            DEBUG_PRINTLN("║ ℹ️  NO NEWER VERSION - Device is up-to-date                ║");
            DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝\n");
            isChecking = false;
            return false;
        }

        DEBUG_PRINTLN("╠════════════════════════════════════════════════════════════╣");
        DEBUG_PRINTF("║ ✅ UPDATE AVAILABLE: %s → %s           ║\n", currentVersion.c_str(), serverVersion.c_str());
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝");
        DEBUG_PRINTLN("[FirmwareUpdater] 📥 Downloading firmware from server...\n");

        WiFiClient client;

        // Configure httpUpdate to not reboot automatically (we'll handle it)
        ESPhttpUpdate.rebootOnUpdate(false);

        // Perform the update check and download
        t_httpUpdate_return ret = ESPhttpUpdate.update(client, firmwareEndpoint);

        switch (ret) {
            case HTTP_UPDATE_FAILED:
                DEBUG_PRINTF("[FirmwareUpdater] ❌ Firmware update failed! Error: %s\n", 
                            ESPhttpUpdate.getLastErrorString().c_str());
                DEBUG_PRINTF("[FirmwareUpdater] Error code: %d\n", ESPhttpUpdate.getLastError());
                isChecking = false;
                return false;

            case HTTP_UPDATE_NO_UPDATES:
                // This shouldn't happen since we already checked version
                DEBUG_PRINTLN("[FirmwareUpdater] ℹ️ Server returned no updates");
                isChecking = false;
                return false;

            case HTTP_UPDATE_OK:
                DEBUG_PRINTLN("\n");
                DEBUG_PRINTLN("╔════════════════════════════════════════════════════════════╗");
                DEBUG_PRINTLN("║         FIRMWARE UPDATE SUCCESSFUL                         ║");
                DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════╝");
                DEBUG_PRINTF("[FirmwareUpdater] ✅ Firmware flashed to chip successfully!\n");
                DEBUG_PRINTF("[FirmwareUpdater] 📊 Version Updated:\n");
                DEBUG_PRINTF("[FirmwareUpdater]    OLD: %s\n", currentVersion.c_str());
                DEBUG_PRINTF("[FirmwareUpdater]    NEW: %s\n", serverVersion.c_str());
                DEBUG_PRINTF("[FirmwareUpdater] 💾 New version installed on chip\n");
                
                // Update current version to new version
                currentVersion = serverVersion;
                
                DEBUG_PRINTLN("[FirmwareUpdater] 🔄 Rebooting device with new firmware...");
                DEBUG_PRINTLN("╔════════════════════════════════════════════════════════════╗\n");
                
                // Brief delay to flush logs
                delay(1000);
                
                // Force restart to boot the new firmware
                ESP.restart();
                
                // Code below will not execute after restart
                isChecking = false;
                return true;
        }

        isChecking = false;
        return false;
    }

    void handle() {
        // Called from UploadManager every upload cycle (every 30 seconds)
        // Check for firmware updates at the start of each upload cycle
        checkForUpdate();
    }

} // namespace FirmwareUpdater
