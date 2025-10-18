#include "compression.h"

namespace Compression {

    String compressDelta16(const std::vector<uint16_t>& values) {
        if (values.empty()) return "";

        String output;
        uint16_t prev = values[0];
        output += String(prev) + ",";  // store first value

        for (size_t i = 1; i < values.size(); ++i) {
            int16_t delta = (int16_t)(values[i] - prev);
            output += String(delta) + ",";
            prev = values[i];
        }
        return output;
    }

    std::vector<uint16_t> decompressDelta16(const String& data) {
        std::vector<uint16_t> result;
        if (data.isEmpty()) return result;

        int start = 0;
        int comma = data.indexOf(',');

        // First absolute value
        if (comma > 0) {
            uint16_t value = (uint16_t)data.substring(start, comma).toInt();
            result.push_back(value);
            start = comma + 1;
        }

        // Remaining deltas
        while (start < data.length()) {
            comma = data.indexOf(',', start);
            if (comma == -1) break;

            int delta = data.substring(start, comma).toInt();
            uint16_t newVal = result.back() + delta;
            result.push_back(newVal);

            start = comma + 1;
        }
        return result;
    }

}
