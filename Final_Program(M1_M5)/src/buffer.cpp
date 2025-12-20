#include "buffer.h"
#include "temporary_buffer.h"
#include "debug_utils.h"
#include "inverter_comm.h"
#include "request_config.h"


namespace Buffer {

    static std::vector<TimedSnapshot> mainBuffer;
    static const size_t MAX_BUFFER_SIZE = 100;   // your limit
    static bool bufferOverflow = false;          // flag

    // --------------------------------------------------------------------
    // Append filtered snapshot(s) from TemporaryBuffer
    // --------------------------------------------------------------------
    void appendFromTemporary(const RequestSIM& config) {
        const auto& tempData = TemporaryBuffer::getAll();
        if (tempData.empty()) {
            DEBUG_PRINTLN("[Buffer] ⚠️ Temporary buffer is empty, nothing to append.");
            return;
        }

        // Process each snapshot stored in TemporaryBuffer
        for (const auto& snapshot : tempData) {

        // --- Create filtered snapshot ---
        TimedSnapshot filtered;
        filtered.timestamp = snapshot.timestamp;
        filtered.values.assign(NUM_REGISTERS, -1.0f);

        for (size_t i = 0; i < snapshot.values.size() && i < NUM_REGISTERS; ++i) {
            if (config.read[i]) {
                filtered.values[i] = snapshot.values[i];
            }
        }

        // --- Enforce max buffer size ---
        if (mainBuffer.size() >= MAX_BUFFER_SIZE) {
            bufferOverflow = true;     // mark overflow
            mainBuffer.erase(mainBuffer.begin());  // remove oldest
        }

        // --- Push newest snapshot ---
        mainBuffer.push_back(filtered);

        DEBUG_PRINTF("[Buffer] Added snapshot @ %s (size=%d)\n",
                    filtered.timestamp.c_str(), mainBuffer.size());
        }


        DEBUG_PRINTF("[Buffer] 📦 Main buffer now has %d snapshot(s)\n",
                     (int)mainBuffer.size());
    }

    // --------------------------------------------------------------------
    // Retrieve all stored snapshots
    // --------------------------------------------------------------------
    const std::vector<TimedSnapshot>& getAll() {
        return mainBuffer;
    }

    // --------------------------------------------------------------------
    // Clear main buffer
    // --------------------------------------------------------------------
    void clear() {
        mainBuffer.clear();
        DEBUG_PRINTLN("[Buffer] 🧹 Main buffer cleared.");
    }
    bool hasOverflowed() {
    return bufferOverflow;
    }

}  // namespace Buffer
