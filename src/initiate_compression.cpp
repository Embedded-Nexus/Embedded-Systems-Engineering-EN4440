#include "initiate_compression.h"
#include "debug_utils.h"
#include "compression.h"
#include "buffer.h"
#include "upload_manager.h"
#include "cloud_decode_utils.h"
#include <Arduino.h>
#include <vector>

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
    for (auto v : rawValues) Serial.printf("%u ", v);
    Serial.println();

    // 🔹 Compress and benchmark
    auto result = Compression::TimeSeriesCompressor::benchmark(rawValues, REGISTER_COUNT + 6);
    std::vector<uint8_t> compressed = Compression::TimeSeriesCompressor::compress(rawValues, REGISTER_COUNT + 6);

    Serial.printf("[DEBUG] Compressed bytes:\n");
    for (auto b : compressed) Serial.printf("%02X ", b);
    Serial.println();

    // 🔹 Verify decompression and decoding
    auto decompressed = Compression::TimeSeriesCompressor::decompress(compressed, REGISTER_COUNT + 6);
    auto decoded = decodeDecompressedData(decompressed, REGISTER_COUNT);
    printDecodedSnapshots(decoded);

    Serial.println("[DEBUG] Decompressed values:");
    for (auto v : decompressed) Serial.printf("%u ", v);
    Serial.println();

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

    // ✅ Clear buffer after successful processing
    Buffer::clear();
    DEBUG_PRINTLN("[Buffer] 🧹 Main buffer cleared after compression.\n");

    return compressed;
}
