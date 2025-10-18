#include "compression.h"

namespace Compression {

    // 🧱 Run-Length Encoding (RLE) Compression
    String compressString(const String& input) {
        if (input.isEmpty()) return "";

        String output;
        int count = 1;

        for (int i = 1; i <= input.length(); ++i) {
            if (i < input.length() && input[i] == input[i - 1]) {
                count++;
            } else {
                output += input[i - 1];
                if (count > 1) {
                    output += "#";       // <-- separator before count
                    output += String(count);
                }
                count = 1;
            }
        }

        return output;
    }

    // 🔁 Run-Length Encoding (RLE) Decompression
    String decompressString(const String& input) {
        String output;

        for (int i = 0; i < input.length(); ++i) {
            char c = input[i];

            // detect '#count' after character
            if (i + 1 < input.length() && input[i + 1] == '#') {
                i += 2; // skip '#'
                String numStr;
                while (i < input.length() && isDigit(input[i])) {
                    numStr += input[i];
                    i++;
                }
                i--; // rewind one since loop will increment
                int count = numStr.toInt();
                for (int j = 0; j < count; j++) output += c;
            } else {
                output += c;
            }
        }

        return output;
    }

} // namespace Compression
