#include "temporary_buffer.h"

namespace TemporaryBuffer {

    // Global buffer storage
    vector<DecodedRegisters> buffer;

    void update(const vector<DecodedRegisters>& newData) {
        for (const auto& newReg : newData) {
            // Check if this register already exists in the buffer
            auto it = find_if(buffer.begin(), buffer.end(),
                              [&](const DecodedRegisters& r) { return r.index == newReg.index; });

            if (it != buffer.end()) {
                *it = newReg;  // Replace existing value
            } else {
                buffer.push_back(newReg);  // Insert new entry
            }
        }
    }

    const vector<DecodedRegisters>& getAll() {
        return buffer;
    }

    void clear() {
        buffer.clear();
    }

}  // namespace TemporaryBuffer
