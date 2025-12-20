#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <Arduino.h>
#include <vector>

// Returns a new encrypted copy of the data
std::vector<uint8_t> encryptBuffer(const std::vector<uint8_t>& data, uint8_t key);

#endif
