#include "upload_manager.h"
#include "debug_utils.h"
#include <ESP8266WiFi.h>
#include <vector>
#include "compression.h"
#include "buffer.h"
#include "cloud_decode_utils.h"
#include "initiate_compression.h"
#include "encryption.h"
#include "cloudClient.h"
#include "security_layer.h"
#include "update_config.h"
<<<<<<< HEAD
#include "request_sim.h"
#include "protocol_adapter.h"
#include "inverterSIM_utils.h"
#include "frame_queue.h"
=======
#include "power_estimator.h"

>>>>>>> 48d2d0f5ec16b506dc6f234a2839defb3ef763d1

// ⚙️ Local namespace variables
namespace {
    UploadManager::UploadTarget target;
    unsigned long lastUploadTime = 0;
    const unsigned long uploadInterval = 30000; // every 30 seconds

    CloudClient cloud;
}

// 🌐 UploadManager namespace implementation
namespace UploadManager {

    // ✅ Initialize uploader (no API key needed)
    void begin(const String& url, const String& urlConfig, const String& urlCommand) {
        target.endpoint = url;
        target.fetchConfigEndpoint = urlConfig;
        target.fetchCommandEndpoint = urlCommand;
        DEBUG_PRINTF("[UploadManager] Initialized with endpoint: %s\n", url.c_str());
    }

    // 📤 Upload binary data to cloud
    bool uploadtoCloud(const std::vector<uint8_t>& data) {
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN("[UploadManager] ❌ Wi-Fi not connected. Upload skipped.");
            return false;
        }

        if (data.empty()) {
            DEBUG_PRINTLN("[UploadManager] ⚠️ No data to upload.");
            return false;
        }

        WiFiClient client;  // ✅ plain (non-secure) client
        HTTPClient http;

        if (!http.begin(client, target.endpoint)) {
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

    // 🔁 Periodic upload + cloud command handling
    void handle() {
        unsigned long now = millis();
        if (now - lastUploadTime < uploadInterval) return;
        lastUploadTime = now;

        DEBUG_PRINTLN("[UploadManager] ⏫ Upload check triggered.");

        // ☁️ Upload compressed + encrypted payload
           auto compressed = initiateCompression();
        // std::vector<uint8_t> encrypted = encryptBuffer(compressed);
        // std::vector<uint8_t> decrypted = decryptBuffer(encrypted);
            // 2️⃣ Encrypt (returns a new vector)
            const uint8_t key = 0x5A;
            // auto encrypted = encryptBuffer(compressed, key);
           // ===== Power Estimator: measure encryption time =====
            unsigned long __t0 = micros();
            vector<uint8_t> encrypted = encryptBuffer(compressed);
            unsigned long __t1 = micros();
            pe_addCpuMs((__t1 - __t0) / 1000UL);

            // ===== Power Estimator: measure decryption time =====
            unsigned long __t2 = micros();
            vector<uint8_t> decrypted = decryptBuffer(encrypted);
            unsigned long __t3 = micros();
            pe_addCpuMs((__t3 - __t2) / 1000UL);

        Serial.println("Encrypted:");
        for (auto b : encrypted) Serial.printf("%02X", b);
        Serial.println();

            Serial.println("Decrypted:");
            for (auto b : decrypted) { Serial.printf("%02X", b); }
            Serial.println();
            
            // Upload compressed+encrypted data
            bool ok = UploadManager::uploadtoCloud(encrypted);

            if (ok) {
                DEBUG_PRINTLN("[UploadManager] ✅ Upload successful → clearing buffer");
                Buffer::clear();
            } else {
                DEBUG_PRINTLN("[UploadManager] ❌ Upload failed → buffer NOT cleared");
            }


        UploadManager::uploadtoCloud(encrypted);

        // 🔹 Fetch configuration and command data
        String config_response = cloud.fetch(target.fetchConfigEndpoint.c_str());
        String command_response = cloud.fetch(target.fetchCommandEndpoint.c_str());

        // ================================================================
        // 🔧 CONFIGURATION HANDLING
        // ================================================================
        if (config_response.length() > 0) {
            DEBUG_PRINTLN("[UploadManager] ✅ Received config JSON:");
            DEBUG_PRINTLN(config_response);

            String status = cloud.getValue(config_response, "status");
            if (status == "success") {
                UpdateConfig::updateFromCloud(config_response);
            }
        }

        // ================================================================
        // ⚙️ COMMAND HANDLING
        // ================================================================
        if (command_response.length() > 0) {
            DEBUG_PRINTLN("[UploadManager] ✅ Received JSON command_response:");
            DEBUG_PRINTLN(command_response);

            // Extract only the "commands" section (avoid parsing other JSON)
            int commandsStart = command_response.indexOf("\"commands\"");
            if (commandsStart == -1) {
                DEBUG_PRINTLN("[UploadManager] ⚠️ No 'commands' found in response.");
                return;
            }

            int arrayStart = command_response.indexOf("[", commandsStart);
            int arrayEnd   = command_response.indexOf("]", arrayStart);
            if (arrayStart == -1 || arrayEnd == -1) {
                DEBUG_PRINTLN("[UploadManager] ⚠️ Invalid command array.");
                return;
            }

            String commandsBlock = command_response.substring(arrayStart, arrayEnd);
            int start = 0;

            // 🔁 Parse all commands in the array
            while (true) {
                start = commandsBlock.indexOf("{", start);
                if (start == -1) break;

                int end = commandsBlock.indexOf("}", start);
                if (end == -1) break;

                String cmdObj = commandsBlock.substring(start, end + 1);
                start = end + 1;

                // Extract key values
                String action    = cloud.getValue(cmdObj, "action");
                String targetReg = cloud.getValue(cmdObj, "target_register");
                String value     = cloud.getValue(cmdObj, "value");

                if (action.isEmpty() || targetReg.isEmpty() || value.isEmpty()) {
                    DEBUG_PRINTLN("[UploadManager] ⚠️ Skipping incomplete command.");
                    continue;
                }

                DEBUG_PRINTF("[UploadManager] 📩 Parsed command: Action=%s Target=%s Value=%s\n",
                             action.c_str(), targetReg.c_str(), value.c_str());

                // 🧱 Create a temporary RequestSIM object for this command
                RequestSIM cloudRequestSim;
                cloudRequestSim.clear();

                int regIndex = strtol(targetReg.c_str(), nullptr, 0);
                uint16_t data = (uint16_t)value.toInt();

                if (action.equalsIgnoreCase("write_register")) {
                    cloudRequestSim.write[regIndex] = true;
                    cloudRequestSim.writeData[regIndex] = data;
                } else if (action.equalsIgnoreCase("read_register")) {
                    cloudRequestSim.read[regIndex] = true;
                } else {
                    DEBUG_PRINTF("[UploadManager] ⚠️ Unknown action: %s\n", action.c_str());
                    continue;
                }

                // 🔹 Decode into Modbus frames
                const auto& frames = ProtocolAdapter::decodeRequestStruct(cloudRequestSim);
            }

            // ================================================================
            // 📬 Send ACK after processing all commands
            // ================================================================
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
        }
    }

} // namespace UploadManager
