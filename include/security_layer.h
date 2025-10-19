#ifndef SECURITY_LAYER_H
#define SECURITY_LAYER_H

#include <Arduino.h>
#include <vector>
#include <EEPROM.h>
#include <base64.h>        // ✅ lowercase header for ESP8266 C-style Base64
#include <BearSSLHelpers.h>
#include <Hash.h>

// --- Configuration ---
#define PSK "my_secret_psk"   // Pre-shared key for HMAC
#define EEPROM_ADDR 0         // EEPROM address for nonce storage

// --- Function declarations ---
uint32_t getAndIncrementNonce();
bool verifyAndStoreNonce(uint32_t newNonce);

String computeHMAC(const std::vector<uint8_t>& payload, const char* key);
String simulateEncrypt(const std::vector<uint8_t>& data);
std::vector<uint8_t> simulateDecrypt(const String& base64data);

String buildSecureMessage(const std::vector<uint8_t>& compressedData);
bool verifySecureMessage(const String& jsonMessage);

#endif
