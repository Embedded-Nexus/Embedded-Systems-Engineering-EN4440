#ifndef UPLOAD_MANAGER_H
#define UPLOAD_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "WiFiClientSecure.h"
#include "ESP8266HTTPClient.h"

namespace UploadManager {

    struct UploadTarget {
        String endpoint;
    };

    // Initialize uploader
    void begin(const String& url);

    // Upload compressed data
    bool uploadtoCloud(const std::vector<uint8_t>& data);

    // Periodic handler (triggered every uploadInterval)
    void handle();

} // namespace UploadManager

#endif
