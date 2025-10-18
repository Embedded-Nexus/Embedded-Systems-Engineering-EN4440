#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <Arduino.h>
#include <vector>
#include <stdint.h>

using namespace std;

// 🧾 Benchmark result
struct BenchResult {
    String mode;
    size_t samples;
    size_t origBytes;
    size_t compBytes;
    unsigned long tCompressUs;
    unsigned long tDecompressUs;
    bool lossless;
};

// ================================================================
// 🧮 Delta16VarCompressor
// - Simple delta encoding using ZigZag + Varint
// ================================================================
namespace Compression {

class Delta16VarCompressor {
public:
    static constexpr const char* name() { return "Delta16Var"; }

    static vector<uint8_t> compress(const vector<uint16_t>& values);
    static vector<uint16_t> decompress(const vector<uint8_t>& blob);
    static BenchResult benchmark(const vector<uint16_t>& values);
};

// ================================================================
// 🕒 TimeSeriesCompressor
// - Packs 4-bit signed deltas per register
// - Uses mask bits for large jumps
// ================================================================
class TimeSeriesCompressor {
public:
    static constexpr const char* name() { return "TimeSeriesS4"; }

    static vector<uint8_t> compress(const vector<uint16_t>& values, int regs);
    static vector<uint16_t> decompress(const vector<uint8_t>& blob, int regs);
    static BenchResult benchmark(const vector<uint16_t>& values, int regs);
};

}  // namespace Compression

#endif  // COMPRESSION_H
