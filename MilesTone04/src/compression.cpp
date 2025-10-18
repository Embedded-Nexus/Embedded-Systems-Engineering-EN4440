#include "compression.h"

namespace Compression {

    // 🧱 Smart Run-Length Encoding (RLE) Compression
    String compressString(const String& input) {
        if (input.isEmpty()) return "";

        String output;
        int count = 1;

        for (int i = 1; i <= input.length(); ++i) {
            bool sameChar = (i < input.length() && input[i] == input[i - 1]);
            bool isSafeChar = !isDigit(input[i - 1]) && input[i - 1] != '.' && input[i - 1] != ',';

            if (sameChar && isSafeChar) {
                count++;
            } else {
                output += input[i - 1];
                if (count > 1 && isSafeChar) {
                    output += '#';
                    output += String(count);
                }
                count = 1;
            }
        }

        return output;
    }

    // 🔁 Smart RLE Decompression
    String decompressString(const String& input) {
        String output;

        for (int i = 0; i < input.length(); ++i) {
            char c = input[i];

            // If next char is '#', decode count
            if (i + 1 < input.length() && input[i + 1] == '#') {
                i += 2; // skip '#'
                String numStr;
                while (i < input.length() && isDigit(input[i])) {
                    numStr += input[i];
                    i++;
                }
                i--; // step back for outer loop
                int count = numStr.toInt();
                for (int j = 0; j < count; j++) output += c;
            } else {
                output += c;
            }
        }

        return output;
    }

} // namespace Compression
