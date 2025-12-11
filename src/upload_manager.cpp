#include "upload_manager.h"
#include "debug_utils.h"
#include <ESP8266WiFi.h>
#include "compression.h"
#include <vector>
#include "buffer.h"
#include "cloud_decode_utils.h"
#include "initiate_compression.h"
#include "encryption.h"
#include "cloudClient.h"
#include "security_layer.h"
#include "update_config.h"

namespace {
    UploadManager::UploadTarget target;
    unsigned long lastUploadTime = 0;
    const unsigned long uploadInterval = 30000; // every 30 seconds

    CloudClient cloud;
}

namespace UploadManager {

    // ✅ Initialize uploader (no API key needed)
    void begin(const String& url, const String& urlConfig, const String& urlCommand) {
        target.endpoint = url;
        target.fetchConfigEndpoint = urlConfig;
        target.fetchCommandEndpoint = urlCommand;
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
            // auto encrypted = encryptBuffer(compressed, key);
            vector<uint8_t> encrypted = encryptBuffer(compressed);
            vector<uint8_t> decrypted = decryptBuffer(encrypted);

            Serial.println("Encrypted:");
            for (auto b : encrypted) { Serial.printf("%02X", b); }
            Serial.println();

            Serial.println("Decrypted:");
            for (auto b : decrypted) { Serial.printf("%02X", b); }
            Serial.println();

            // 🔐 Print uploading data before upload
            Serial.printf("[Upload] 📤 Uploading %d bytes of encrypted data:\n", encrypted.size());
            for (size_t i = 0; i < encrypted.size(); i++) {
                Serial.printf("%02X", encrypted[i]);
                if ((i + 1) % 16 == 0) Serial.println();
            }
            if (encrypted.size() % 16 != 0) Serial.println();

            UploadManager::uploadtoCloud(encrypted);

            // 🔹 Fetch configuration and command data
            String config_response = cloud.fetch(target.fetchConfigEndpoint.c_str());
            String command_response = cloud.fetch(target.fetchCommandEndpoint.c_str());
            

            if (config_response.length() > 0) {
                DEBUG_PRINTLN("[UploadManager] ✅ Received config JSON:");
                DEBUG_PRINTLN(config_response);
            
                String status = cloud.getValue(config_response, "status");
                if (status == "success") {
                    UpdateConfig::updateFromCloud(config_response);
            
                    // Optional: use the new interval value
                    unsigned long newInterval = UpdateConfig::getLastInterval();
                    DEBUG_PRINTF("[UploadManager] 🌐 Applied new interval: %lu ms\n", newInterval);
                }
            }
            
            
            ////////////////////////////////////////////////////////////////////////
            if (command_response.length() > 0) {
                DEBUG_PRINTLN("[UploadManager] ✅ Received JSON command_response:");
                DEBUG_PRINTLN(command_response);
            
                // 🔧 Extract first command object from {"commands":[{...}]}
                String firstCommand = "";
                int start = command_response.indexOf("\"commands\"");
                if (start != -1) {
                    start = command_response.indexOf("{", start);  // find the first '{' after "commands"
                    int end = command_response.indexOf("}", start);
                    if (start != -1 && end != -1) {
                        firstCommand = command_response.substring(start, end + 1);
                    }
                }
            
                if (firstCommand.length() > 0) {
                    DEBUG_PRINTLN("[UploadManager] 🧩 Extracted first command object:");
                    DEBUG_PRINTLN(firstCommand);
            
                    // ✅ Rename to avoid shadowing global target
                    String action = cloud.getValue(firstCommand, "action");
                    String targetReg = cloud.getValue(firstCommand, "target_register");
                    String value  = cloud.getValue(firstCommand, "value");
            
                    if (action.length() > 0 && targetReg.length() > 0 && value.length() > 0) {
                        DEBUG_PRINTF("[UploadManager] 📩 Action: %s | Target Register: %s | Value: %s\n",
                                     action.c_str(), targetReg.c_str(), value.c_str());
            
                        //Create ack json
                        time_t now = time(nullptr);
                        struct tm* t = localtime(&now);
                        char timeBuf[25];
                        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S", t);
                        
                        String ackPayload = "{";
                        ackPayload += "\"command_result\":{";
                        ackPayload += "\"result\":\"success\",";
                        ackPayload += "\"executed_at\":\"" + String(timeBuf) + "\"";
                        ackPayload += "}";
                        ackPayload += "}";
                                     
            
                        DEBUG_PRINTLN("[UploadManager] 🚀 Sending ACK to cloud...");
                        bool ackOk = cloud.postJSON(target.fetchCommandEndpoint.c_str(), ackPayload);
            
                        if (ackOk)
                            DEBUG_PRINTLN("[UploadManager] ✅ ACK sent successfully!");
                        else
                            DEBUG_PRINTLN("[UploadManager] ⚠️ Failed to send ACK.");
                    } else {
                        DEBUG_PRINTLN("[UploadManager] ⚠️ Incomplete command data, skipping ACK.");
                    }
                } else {
                    DEBUG_PRINTLN("[UploadManager] ⚠️ No 'commands' object found in JSON.");
                }
            }
        }
    }

} // namespace UploadManager
