#include "polling_manager.h"
#include "compression.h"
#include "buffer.h"
#include "request_config.h"
#include "protocol_adapter.h"
#include "inverter_comm.h"
#include "debug_utils.h"
#include "request_sim.h"

namespace {
    unsigned long lastPollTime = 0;
    unsigned long pollInterval = 5000;
    unsigned long lastCompressionTime = 0;
    const unsigned long compressionInterval = 30000; // compress every 15s
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
        if (now - lastPollTime >= pollingInterval) {
            lastPollTime = now;
            DEBUG_PRINTLN("\n================ POLLING CYCLE START =================");

            // Build Modbus request
           // Use the global instance defined in request_sim.cpp
            const auto& frames = ProtocolAdapter::decodeRequestStruct(requestSim);

            // Simulate sending & receiving
            InverterSim::processFrameQueue(frames);

            // Append filtered data to buffer
            Buffer::appendFromTemporary(requestSim);

            // printGlobalRequestSim();


            // 🔍 Print main buffer contents
            const auto& allSnapshots = Buffer::getAll();
            Serial.printf("[MainBuffer] 📊 Total snapshots: %d\n", (int)allSnapshots.size());

            for (size_t s = 0; s < allSnapshots.size(); ++s) {
                const auto& snap = allSnapshots[s];
                Serial.printf("  Snapshot %d @ %s\n", (int)s + 1, snap.timestamp.c_str());

                // Print register values
                for (size_t i = 0; i < snap.values.size(); ++i) {
                        Serial.printf("    R%-3d = %.2f\n", (int)i, snap.values[i]);
                }
            }
        }
    }

}  // namespace PollingManager
