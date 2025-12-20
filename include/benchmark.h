#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include "buffer.h"

using namespace std;

struct BenchmarkReport {
    String method;
    size_t sampleCount;
    size_t originalSize;
    size_t compressedSize;
    float compressionRatio;
    unsigned long cpuTimeMicros;
    bool lossless;
    bool withinCap;
};

namespace Benchmark {
    BenchmarkReport runEndToEnd(
        const vector<BufferEntry>& buffer,
        const String& methodName,
        std::function<String(const String&)> compressFn,
        std::function<String(const String&)> decompressFn,
        size_t payloadCap
    );

    void printReport(const BenchmarkReport& report);
}

#endif
