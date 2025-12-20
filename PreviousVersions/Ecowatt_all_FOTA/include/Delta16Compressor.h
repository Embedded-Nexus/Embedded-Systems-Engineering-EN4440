#pragma once
#include <vector>
#include <stdint.h>
#include <Arduino.h>

struct BenchResult {
  uint32_t samples;
  uint32_t origBytes;
  uint32_t compBytes;
  bool lossless;
  const char* mode;
  unsigned long tCompressUs;
  unsigned long tDecompressUs;
};

// Delta + ZigZag + base-128 varint
class Delta16VarCompressor {
public:
  static std::vector<uint8_t> compress(const std::vector<uint16_t>& values);
  static std::vector<uint16_t> decompress(const std::vector<uint8_t>& blob);
  static BenchResult benchmark(const std::vector<uint16_t>& values);
  static const char* name() { return "delta16v"; }
};

// Frame-wise time-series coder for fixed number of regs per frame.
// First frame absolute (regs*2 B). Each later frame:
//  mask(2 B, bit j=1 => absolute follows) + packed 4-bit signed deltas for all regs (regs/2 B) + absolutes(2 B each).
class TimeSeriesCompressor {
public:
  static std::vector<uint8_t> compress(const std::vector<uint16_t>& values, int regs);
  static std::vector<uint16_t> decompress(const std::vector<uint8_t>& blob, int regs);
  static BenchResult benchmark(const std::vector<uint16_t>& values, int regs);
  static const char* name() { return "ts16"; }
};
