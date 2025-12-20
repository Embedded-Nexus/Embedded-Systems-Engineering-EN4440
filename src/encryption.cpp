#include "encryption.h"

std::vector<uint8_t> encryptBuffer(const std::vector<uint8_t>& data, uint8_t key) {
    std::vector<uint8_t> encrypted;
    encrypted.reserve(data.size());  // reserve memory to match input

    for (auto b : data) {
        encrypted.push_back(b ^ key);  // XOR encryption
    }

    return encrypted;
}
