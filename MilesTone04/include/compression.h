#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <Arduino.h>

namespace Compression {

    // Compress using simple Run-Length Encoding (RLE)
    String compressString(const String& input);

    // Decompress (reverse of RLE)
    String decompressString(const String& input);

}

#endif
