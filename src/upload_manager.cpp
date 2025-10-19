#include "upload_manager.h"
#include "debug_utils.h"
#include <ESP8266WiFi.h>
#include "compression.h"
#include <vector>
#include "buffer.h"
#include "cloud_decode_utils.h"
#include "initiate_compression.h"

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

    bool uploadCompressed(const std::vector<uint8_t>& data) {
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
            initiateCompressionAndUpload();
            // const auto& currentBuffer = Buffer::getAll();

            // if (!currentBuffer.empty()) {
            //     std::vector<uint16_t> rawValues;
            //     //Flatteing the buffer
            //     for (const auto& snap : currentBuffer) {
            //         int year, month, day, hour, minute, second;
                
            //         // Parse "YYYY-MM-DD HH:MM:SS" into numeric components
            //         if (sscanf(snap.timestamp.c_str(), "%d-%d-%d %d:%d:%d",
            //                    &year, &month, &day, &hour, &minute, &second) == 6) {
            //             rawValues.push_back(year);
            //             rawValues.push_back(month);
            //             rawValues.push_back(day);
            //             rawValues.push_back(hour);
            //             rawValues.push_back(minute);
            //             rawValues.push_back(second);
            //         } else {
            //             // fallback if timestamp string is malformed
            //             rawValues.insert(rawValues.end(), {0, 0, 0, 0, 0, 0});
            //         }
                
            //         // Add register values (use 0xFFFF for unread ones)
            //         for (float v : snap.values) {
            //             rawValues.push_back((v < 0.0f) ? 0xFFFF : static_cast<uint16_t>(v));
            //         }
            //     }                

            //     DEBUG_PRINTF("[UploadManager] 📦 Flattened %d values for compression\n",
            //                 (int)rawValues.size());

            //     Serial.println("[DEBUG] RawValues (uint16_t) before compression:");
            //     for (auto v : rawValues) {
            //         Serial.printf("%u ", v);
            //     }
            //     Serial.println();

            //     auto result = Compression::TimeSeriesCompressor::benchmark(rawValues, REGISTER_COUNT+6);
            //     std::vector<uint8_t> compressed = Compression::TimeSeriesCompressor::compress(rawValues, REGISTER_COUNT+6);

            //     Serial.printf("[DEBUG] Compressed bytes:\n");
            //     for (auto b : compressed) Serial.printf("%02X ", b);
            //     Serial.println();
                
            //     auto decompressed = Compression::TimeSeriesCompressor::decompress(compressed, REGISTER_COUNT+6);
            //     auto decoded = decodeDecompressedData(decompressed, REGISTER_COUNT);
            //     printDecodedSnapshots(decoded);

            //     Serial.println("[DEBUG] Decompressed values:");
            //     for (auto v : decompressed) {
            //         Serial.printf("%u ", v);
            //     }
            //     Serial.println();

            //     float ratio = (result.origBytes > 0)
            //                     ? (100.0f * (result.origBytes - result.compBytes) / result.origBytes)
            //                     : 0.0f;

            //     DEBUG_PRINTLN("\n[UploadManager] 🗜️ Compression Summary:");
            //     DEBUG_PRINTF("  Method          : %s\n", result.mode.c_str());
            //     DEBUG_PRINTF("  Samples         : %u\n", result.samples);
            //     DEBUG_PRINTF("  Original Size   : %u bytes\n", result.origBytes);
            //     DEBUG_PRINTF("  Compressed Size : %u bytes\n", result.compBytes);
            //     DEBUG_PRINTF("  Reduction       : %.2f%%\n", ratio);
            //     DEBUG_PRINTF("  CPU Time        : %lu µs\n", result.tCompressUs);
            //     DEBUG_PRINTF("  Lossless Verify : %s\n", result.lossless ? "✅ YES" : "❌ NO");

            //     // ✅ Clear buffer after upload
            //     Buffer::clear();
            //     DEBUG_PRINTLN("[Buffer] 🧹 Main buffer cleared after compression.\n");

            //     // ☁️ Upload compressed payload
            //     uploadCompressed(compressed);
            // } else {
            //     DEBUG_PRINTLN("[UploadManager] ⚠️ No data in buffer to upload.");
            // }

        }
    }
}// namespace UploadManager 
