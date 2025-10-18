#include "polling_manager.h"
#include "compression.h"
#include "buffer.h"
#include "request_config.h"
#include "protocol_adapter.h"
#include "inverter_comm.h"
#include "debug_utils.h"

namespace {
    unsigned long lastPollTime = 0;
    unsigned long pollInterval = 5000;
}

namespace PollingManager {

    void begin(unsigned long intervalMs) {
        pollInterval = intervalMs;
        lastPollTime = millis();
        DEBUG_PRINTF("[PollingManager] Initialized (interval = %lu ms)\n", pollInterval);
    }

    void handle() {
        unsigned long now = millis();

        if (now - lastPollTime >= pollInterval) {
            lastPollTime = now;
            DEBUG_PRINTLN("\n================ POLLING CYCLE START =================");

            // Step 1️⃣: Build request from config
            RequestSIM req = RequestConfig::buildRequestConfig();

            // Step 2️⃣: Generate Modbus frames
            const auto& frames = ProtocolAdapter::decodeRequestStruct(req);

            // Step 3️⃣: Simulate send & receive from inverter
            InverterSim::processFrameQueue(frames);

            // Step 4️⃣: Append filtered data to main buffer
            Buffer::appendFromTemporary(req);

            // Step 5️⃣: Print all logged data
            const auto& history = Buffer::getAll();

            DEBUG_PRINTLN("\n=== MAIN BUFFER (Filtered + Timestamped Data) ===");
            for (const auto& entry : history) {
                const auto& r = entry.reg;
                DEBUG_PRINTF("[%s]  R%-2d %-30s = %.2f %s (raw=%d)\n",
                             entry.timestamp.c_str(),
                             r.index, r.name.c_str(),
                             r.scaledValue, r.unit.c_str(), r.rawValue);
            }
            DEBUG_PRINTLN("==================================================");

        // Step 6️⃣: Delta16 Compression & Flush
        const auto& currentBuffer = Buffer::getAll();

        if (!currentBuffer.empty()) {
            std::vector<uint16_t> rawValues;
            rawValues.reserve(currentBuffer.size());

            for (const auto& entry : currentBuffer) {
                rawValues.push_back((uint16_t)entry.reg.rawValue);
            }

            // Original size
            size_t originalSize = rawValues.size() * sizeof(uint16_t);

            // Compress
            String compressed = Compression::compressDelta16(rawValues);
            size_t compressedSize = compressed.length();

            float ratio = (originalSize > 0)
                            ? (100.0f * (originalSize - compressedSize) / originalSize)
                            : 0.0f;

            DEBUG_PRINTF("[PollingManager] 🗜️ Delta16 Compression:\n");
            DEBUG_PRINTF("  Original size   : %u bytes\n", originalSize);
            DEBUG_PRINTF("  Compressed size : %u bytes\n", compressedSize);
            DEBUG_PRINTF("  Reduction       : %.2f%%\n", ratio);

            if (compressedSize < originalSize)
                DEBUG_PRINTLN("  ✅ Compression effective.");
            else
                DEBUG_PRINTLN("  ⚠️ No compression benefit.");

            Buffer::clear();
            DEBUG_PRINTLN("[PollingManager] 🧹 Buffer flushed after compression.\n");
        }



        }
    }

}  // namespace PollingManager
