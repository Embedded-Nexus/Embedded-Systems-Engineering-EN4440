#include "initiate_compression.h"
#include "debug_utils.h"
#include "compression.h"
#include "buffer.h"
#include "upload_manager.h"
#include "cloud_decode_utils.h"
#include <Arduino.h>
#include <vector>
#include "power_estimator.h"

std::vector<uint8_t> initiateCompression() {
    const auto& currentBuffer = Buffer::getAll();

    if (currentBuffer.empty()) {
        DEBUG_PRINTLN("[UploadManager] ⚠️ No data in buffer to upload.");
        return {}; // return empty vector
    }

    std::vector<uint16_t> rawValues;

    // 🔹 Flatten the buffer
    for (const auto& snap : currentBuffer) {
        int year, month, day, hour, minute, second;

        if (sscanf(snap.timestamp.c_str(), "%d-%d-%d %d:%d:%d",
                   &year, &month, &day, &hour, &minute, &second) == 6) {
            rawValues.push_back(year);
            rawValues.push_back(month);
            rawValues.push_back(day);
            rawValues.push_back(hour);
            rawValues.push_back(minute);
            rawValues.push_back(second);
        } else {
            rawValues.insert(rawValues.end(), {0, 0, 0, 0, 0, 0});
        }

        for (float v : snap.values) {
            rawValues.push_back((v < 0.0f) ? 0xFFFF : static_cast<uint16_t>(v));
        }
    }

    DEBUG_PRINTF("[UploadManager] 📦 Flattened %d values for compression\n",
                 (int)rawValues.size());

    Serial.println("[DEBUG] RawValues (uint16_t) before compression:");
    size_t rawLimit = min(rawValues.size(), (size_t)50);  // Limit to 50 values
    for (size_t i = 0; i < rawLimit; i++) {
        Serial.printf("%u ", rawValues[i]);
        if ((i + 1) % 10 == 0) { Serial.print(" | "); yield(); }  // Separator every 10 values
    }
    if (rawValues.size() > 50) Serial.printf("... (%d more)", rawValues.size() - 50);
    Serial.println();
    Serial.flush();

    // 🔹 Compress and benchmark
    auto result = Compression::TimeSeriesCompressor::benchmark(rawValues, REGISTER_COUNT + 6);
    
    // ===== Power Estimator: add compression CPU time =====
    pe_addCpuMs(result.tCompressUs / 1000UL);


    std::vector<uint8_t> compressed = Compression::TimeSeriesCompressor::compress(rawValues, REGISTER_COUNT + 6);

    Serial.printf("[DEBUG] Compressed bytes:\n");
    size_t compLimit = min(compressed.size(), (size_t)64);
    for (size_t i = 0; i < compLimit; i++) {
        Serial.printf("%02X ", compressed[i]);
        if ((i + 1) % 16 == 0) { Serial.println(); yield(); }
    }
    if (compressed.size() > 64) Serial.printf("... (%d more bytes)", compressed.size() - 64);
    Serial.println();
    Serial.flush();

    // 🔹 Verify decompression and decoding
    auto decompressed = Compression::TimeSeriesCompressor::decompress(compressed, REGISTER_COUNT + 6);
    auto decoded = decodeDecompressedData(decompressed, REGISTER_COUNT);
    printDecodedSnapshots(decoded);

    Serial.println("[DEBUG] Decompressed values:");
    size_t decomLimit = min(decompressed.size(), (size_t)50);
    for (size_t i = 0; i < decomLimit; i++) {
        Serial.printf("%u ", decompressed[i]);
        if ((i + 1) % 10 == 0) { Serial.print(" | "); yield(); }
    }
    if (decompressed.size() > 50) Serial.printf("... (%d more)", decompressed.size() - 50);
    Serial.println();
    Serial.flush();

    float ratio = (result.origBytes > 0)
                    ? (100.0f * (result.origBytes - result.compBytes) / result.origBytes)
                    : 0.0f;

    DEBUG_PRINTLN("\n[UploadManager] 🗜️ Compression Summary:");
    DEBUG_PRINTF("  Method          : %s\n", result.mode.c_str());
    DEBUG_PRINTF("  Samples         : %u\n", result.samples);
    DEBUG_PRINTF("  Original Size   : %u bytes\n", result.origBytes);
    DEBUG_PRINTF("  Compressed Size : %u bytes\n", result.compBytes);
    DEBUG_PRINTF("  Reduction       : %.2f%%\n", ratio);
    DEBUG_PRINTF("  CPU Time        : %lu µs\n", result.tCompressUs);
    DEBUG_PRINTF("  Lossless Verify : %s\n", result.lossless ? "✅ YES" : "❌ NO");

    return compressed;
}
