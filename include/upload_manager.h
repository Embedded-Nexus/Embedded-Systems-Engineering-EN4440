#ifndef UPLOAD_MANAGER_H
#define UPLOAD_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "WiFiClientSecure.h"
#include "ESP8266HTTPClient.h"

namespace UploadManager {

    struct UploadTarget {
        String endpoint;
        String fetchConfigEndpoint;
        String fetchCommandEndpoint;       
    };

    // Initialize uploader
    void begin(const String& url, const String& urlConfig, const String& urlCommand);

    // Initialize firmware updater (called from main to configure FW updates per upload cycle)
    void initializeFirmwareUpdater(const String& firmwareEndpoint, const String& firmwareVersion);

    // Upload compressed data
    bool uploadtoCloud(const std::vector<uint8_t>& data);

    // Periodic handler (triggered every uploadInterval)
    void handle();

} // namespace UploadManager

#endif
