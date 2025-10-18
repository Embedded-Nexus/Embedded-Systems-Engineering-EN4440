#ifndef BUFFER_H
#define BUFFER_H

#include <Arduino.h>
#include <vector>
#include "inverter_comm.h"       // for DecodedRegisters
#include "temporary_buffer.h"    // for TemporaryBuffer::getAll()
#include "request_config.h"      // for RequestSIM

using namespace std;

namespace Buffer {

    // 🕒 Struct for time-stamped register entry
    struct TimedRegister {
        DecodedRegisters reg;
        String timestamp;   // e.g. "2025-10-18 14:35:42"
    };

    // Holds all filtered and timestamped data points
    extern vector<TimedRegister> mainBuffer;

    // 🧠 Append new filtered data (timestamped)
    void appendFromTemporary(const RequestSIM& config);

    // 🧭 Retrieve all logged data
    const vector<TimedRegister>& getAll();

    // 🧹 Clear all historical data
    void clear();

    // ⏰ Helper to get current time as string
    String getCurrentTimestamp();

}  // namespace Buffer

#endif
