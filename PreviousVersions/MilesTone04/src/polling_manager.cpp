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
    unsigned long lastCompressionTime = 0;
    const unsigned long compressionInterval = 15000; // compress every 15s
}

namespace PollingManager {

    void begin(unsigned long intervalMs) {
        pollInterval = intervalMs;
        lastPollTime = millis();
        lastCompressionTime = millis();
        DEBUG_PRINTF("[PollingManager] Initialized (interval = %lu ms)\n", pollInterval);
    }

    void handle() {
        unsigned long now = millis();

        // Step 1️⃣: Perform data polling every pollInterval
        if (now - lastPollTime >= pollInterval) {
            lastPollTime = now;
            DEBUG_PRINTLN("\n================ POLLING CYCLE START =================");

            // Build Modbus request
            RequestSIM req = RequestConfig::buildRequestConfig();
            const auto& frames = ProtocolAdapter::decodeRequestStruct(req);

            // Simulate sending & receiving
            InverterSim::processFrameQueue(frames);

            // Append filtered data to buffer
            Buffer::appendFromTemporary(req);

            // Display the buffer
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
        }

        // Step 2️⃣: Compress and flush every 15 seconds
        if (now - lastCompressionTime >= compressionInterval) {
            lastCompressionTime = now;

            const auto& currentBuffer = Buffer::getAll();

            if (!currentBuffer.empty()) {
                std::vector<uint16_t> rawValues;
                rawValues.reserve(currentBuffer.size());

                for (const auto& entry : currentBuffer) {
                    rawValues.push_back((uint16_t)entry.reg.rawValue);
                }

                // Perform compression benchmark
                auto result = Compression::TimeSeriesCompressor::benchmark(rawValues, 4);

                float ratio = (result.origBytes > 0)
                                ? (100.0f * (result.origBytes - result.compBytes) / result.origBytes)
                                : 0.0f;

                DEBUG_PRINTLN("\n[PollingManager] 🗜️ Compression Summary:");
                DEBUG_PRINTF("  Method          : %s\n", result.mode.c_str());
                DEBUG_PRINTF("  Samples         : %u\n", result.samples);
                DEBUG_PRINTF("  Original Size   : %u bytes\n", result.origBytes);
                DEBUG_PRINTF("  Compressed Size : %u bytes\n", result.compBytes);
                DEBUG_PRINTF("  Reduction       : %.2f%%\n", ratio);
                DEBUG_PRINTF("  CPU Time        : %lu µs\n", result.tCompressUs);
                DEBUG_PRINTF("  Lossless Verify : %s\n", result.lossless ? "✅ YES" : "❌ NO");

                if (result.compBytes < result.origBytes)
                    DEBUG_PRINTLN("  ✅ Compression effective!");
                else
                    DEBUG_PRINTLN("  ⚠️ No compression benefit.");

                // Flush the main buffer
                Buffer::clear();
                DEBUG_PRINTLN("[Buffer] 🧹 Main buffer cleared after compression.\n");
            }
        }
    }

}  // namespace PollingManager
