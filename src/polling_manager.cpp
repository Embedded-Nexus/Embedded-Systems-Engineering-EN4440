#include "polling_manager.h"
#include "compression.h"
#include "buffer.h"
#include "request_config.h"
#include "protocol_adapter.h"
#include "inverter_comm.h"
#include "debug_utils.h"
#include "request_sim.h"
#include "frame_queue.h"

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

            // Debug: print current frame queue
            DEBUG_PRINTF("[UploadManager] 🧾 FrameQueue contains command %zu frames:\n", frameQueue.size());
            for (size_t i = 0; i < frameQueue.size(); ++i) {
                const auto& frame = frameQueue[i];
                DEBUG_PRINTF("   [%zu] Frame length: %zu bytes\n", i, frame.size());
                DEBUG_PRINTF("   Data: ");
                for (uint8_t byte : frame) {
                    DEBUG_PRINTF("%02X ", byte);  // Print each byte in hex
                }
                DEBUG_PRINTF("\n");
            }

            // Simulate sending & receiving
            InverterSim::processFrameQueue(frames);
            
            frameQueue.clear();


            // Append filtered data to buffer
            Buffer::appendFromTemporary(requestSim);

            // printGlobalRequestSim();
            

            // 🔍 Print main buffer contents
            const auto& allSnapshots = Buffer::getAll();
            Serial.printf("[MainBuffer] 📊 Total snapshots: %d\n", (int)allSnapshots.size());

            for (size_t s = 0; s < allSnapshots.size(); ++s) {
                const auto& snap = allSnapshots[s];
                Serial.printf("  Snapshot %d @ %s\n", (int)s + 1, snap.timestamp.c_str());
                yield();  // Prevent buffer overflow

                // Print register values
                for (size_t i = 0; i < snap.values.size(); ++i) {
                        Serial.printf("    R%-3d = %.2f\n", (int)i, snap.values[i]);
                        if (i % 5 == 0) yield();  // Prevent buffer overflow every 5 values
                }
            }
        }
    }

}  // namespace PollingManager
