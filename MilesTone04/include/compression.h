#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <Arduino.h>
#include <vector>

namespace Compression {

    struct Delta16Entry {
        uint16_t value;
    };

    String compressDelta16(const std::vector<uint16_t>& values);
    std::vector<uint16_t> decompressDelta16(const String& data);
}

#endif
