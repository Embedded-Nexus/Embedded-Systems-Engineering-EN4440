#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <Arduino.h>

namespace Encryption {

    // Encrypt string with XOR
    String encrypt(const String& input, const String& key);

    // Decrypt (same function, symmetric)
    String decrypt(const String& input, const String& key);

}  // namespace Encryption

#endif
