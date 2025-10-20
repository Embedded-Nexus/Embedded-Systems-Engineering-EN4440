#ifndef SECURITY_LAYER_H
#define SECURITY_LAYER_H

#include <Arduino.h>
#include <vector>
#include <stdint.h>

std::vector<uint8_t> encryptBuffer(const std::vector<uint8_t>& plain);
std::vector<uint8_t> decryptBuffer(const std::vector<uint8_t>& packet);

#endif // SECURITY_LAYER_H
