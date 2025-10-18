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

            // Step 6️⃣: Create a payload from the main buffer for compression
            String payload;
            for (const auto& entry : history) {
                payload += entry.timestamp + "," +
                           entry.reg.name + "," +
                           String(entry.reg.scaledValue) + "\n";
            }

            DEBUG_PRINTF("\n[Compression] Original Payload Size: %u bytes\n", payload.length());

            // Step 7️⃣: Compress the payload
            String compressed = Compression::compressString(payload);
            DEBUG_PRINTF("[Compression] Compressed Size: %u bytes\n", compressed.length());

            // Optional: Verify decompression
            String decompressed = Compression::decompressString(compressed);
            bool lossless = (payload == decompressed);
            DEBUG_PRINTF("[Compression] Lossless Verify: %s\n", lossless ? "✅ YES" : "❌ NO");

            DEBUG_PRINTLN("================ POLLING CYCLE END ==================\n");
        }
    }

}  // namespace PollingManager
