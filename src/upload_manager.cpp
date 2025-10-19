#include "upload_manager.h"
#include "debug_utils.h"
#include <ESP8266WiFi.h>
#include "compression.h"
#include <vector>
#include "buffer.h"
#include "cloud_decode_utils.h"
#include "initiate_compression.h"
#include "encryption.h"

namespace {
    UploadManager::UploadTarget target;
    unsigned long lastUploadTime = 0;
    const unsigned long uploadInterval = 30000; // every 30 seconds
}

namespace UploadManager {

    // ✅ Initialize uploader (no API key needed)
    void begin(const String& url) {
        target.endpoint = url;
        DEBUG_PRINTF("[UploadManager] Initialized with endpoint: %s\n", url.c_str());
    }

    bool uploadtoCloud(const std::vector<uint8_t>& data) {
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN("[UploadManager] ❌ Wi-Fi not connected. Upload skipped.");
            return false;
        }
    
        if (data.empty()) {
            DEBUG_PRINTLN("[UploadManager] ⚠️ No data to upload.");
            return false;
        }
    
        WiFiClient client;  // ✅ use plain (non-secure) client
        HTTPClient http;
    
        if (!http.begin(client, target.endpoint)) {  // ✅ new API: pass client + URL
            DEBUG_PRINTLN("[UploadManager] ❌ HTTP connection failed.");
            return false;
        }
    
        http.addHeader("Content-Type", "application/octet-stream");
    
        int httpCode = http.POST((uint8_t*)data.data(), data.size());
    
        if (httpCode > 0) {
            DEBUG_PRINTF("[UploadManager] 🌐 Upload response: %d\n", httpCode);
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_ACCEPTED) {
                DEBUG_PRINTLN("[UploadManager] ✅ Upload successful.");
                http.end();
                return true;
            } else {
                DEBUG_PRINTF("[UploadManager] ⚠️ Unexpected code: %d\n", httpCode);
            }
        } else {
            DEBUG_PRINTF("[UploadManager] ⚠️ Upload failed: %s\n",
                         http.errorToString(httpCode).c_str());
        }
    
        http.end();
        return false;
    }
    
    void handle() {
        unsigned long now = millis();
        if (now - lastUploadTime >= uploadInterval) {
            lastUploadTime = now;

            DEBUG_PRINTLN("[UploadManager] ⏫ Upload check triggered.");
            // ☁️ Upload compressed payload
            auto compressed = initiateCompression();

            // 2️⃣ Encrypt (returns a new vector)
            const uint8_t key = 0x5A;
            auto encrypted = encryptBuffer(compressed, key);
            UploadManager::uploadtoCloud(encrypted);
        }
    }
}// namespace UploadManager 
