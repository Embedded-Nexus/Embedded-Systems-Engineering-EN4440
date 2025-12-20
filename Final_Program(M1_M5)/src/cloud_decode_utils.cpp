#include "cloud_decode_utils.h"

std::vector<DecodedSnapshot> decodeDecompressedData(
    const std::vector<uint16_t>& data, int regs)
{
    std::vector<DecodedSnapshot> result;
    if (data.size() < (size_t)(regs + 6)) return result;

    size_t frameWords = regs + 6;
    size_t frameCount = data.size() / frameWords;

    for (size_t f = 0; f < frameCount; ++f) {
        size_t base = f * frameWords;

        // Extract timestamp parts
        uint16_t year   = data[base + 0];
        uint16_t month  = data[base + 1];
        uint16_t day    = data[base + 2];
        uint16_t hour   = data[base + 3];
        uint16_t minute = data[base + 4];
        uint16_t second = data[base + 5];

        char buf[25];
        snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
                 year, month, day, hour, minute, second);

        DecodedSnapshot snap;
        snap.timestamp = String(buf);
        snap.registers.reserve(regs);

        // Extract register values
        for (int r = 0; r < regs; ++r) {
            snap.registers.push_back(data[base + 6 + r]);
        }

        result.push_back(std::move(snap));
    }

    return result;
}

void printDecodedSnapshots(const std::vector<DecodedSnapshot>& snapshots) {
    Serial.printf("[DecodedData] 🧩 Total snapshots: %d\n", (int)snapshots.size());

    for (size_t i = 0; i < snapshots.size(); ++i) {
        const auto& s = snapshots[i];
        Serial.printf("  Snapshot %d @ %s\n", (int)(i + 1), s.timestamp.c_str());

        for (size_t r = 0; r < s.registers.size(); ++r) {
            uint16_t val = s.registers[r];
            if (val == 0xFFFF)
                Serial.printf("    R%-2d = (unread)\n", (int)r);
            else
                Serial.printf("    R%-2d = %u\n", (int)r, val);
        }
        Serial.println();
    }
}
