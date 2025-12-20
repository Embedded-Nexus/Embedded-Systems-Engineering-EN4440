#ifndef CLOUD_DECODE_UTILS_H
#define CLOUD_DECODE_UTILS_H

#include <Arduino.h>
#include <vector>

struct DecodedSnapshot {
    String timestamp;              // "YYYY-MM-DD HH:MM:SS"
    std::vector<uint16_t> registers;  // Register values (0xFFFF = unread)
};

// 🧠 Decode decompressed uint16_t data back into timestamped snapshots
std::vector<DecodedSnapshot> decodeDecompressedData(
    const std::vector<uint16_t>& data, int regs);

// 🧾 Optional: pretty-print decoded data to Serial/console
void printDecodedSnapshots(const std::vector<DecodedSnapshot>& snapshots);

#endif  // CLOUD_DECODE_UTILS_H
