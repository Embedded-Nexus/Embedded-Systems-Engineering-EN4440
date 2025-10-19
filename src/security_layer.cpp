#include "security_layer.h"

// ✅ Use libb64 C headers (guaranteed to exist on ESP8266)
extern "C" {
  #include "libb64/cencode.h"
  #include "libb64/cdecode.h"
}

// === Nonce Management ===
uint32_t getAndIncrementNonce() {
  EEPROM.begin(8);
  uint32_t nonce = 0;
  EEPROM.get(EEPROM_ADDR, nonce);
  nonce++;
  EEPROM.put(EEPROM_ADDR, nonce);
  EEPROM.commit();
  EEPROM.end();
  return nonce;
}

bool verifyAndStoreNonce(uint32_t newNonce) {
  EEPROM.begin(8);
  uint32_t lastNonce = 0;
  EEPROM.get(EEPROM_ADDR, lastNonce);

  if (newNonce <= lastNonce) {
    EEPROM.end();
    return false;
  }

  EEPROM.put(EEPROM_ADDR, newNonce);
  EEPROM.commit();
  EEPROM.end();
  return true;
}

// === HMAC ===
String computeHMAC(const std::vector<uint8_t>& payload, const char* key) {
  br_hmac_key_context kc;
  br_hmac_context ctx;
  uint8_t mac[32];

  br_hmac_key_init(&kc, &br_sha256_vtable, key, strlen(key));
  br_hmac_init(&ctx, &kc, 0);
  br_hmac_update(&ctx, payload.data(), payload.size());
  br_hmac_out(&ctx, mac);

  String hmacHex;
  for (int i = 0; i < 32; i++) {
    if (mac[i] < 16) hmacHex += "0";
    hmacHex += String(mac[i], HEX);
  }
  return hmacHex;
}

// === Base64 Encoding / Decoding ===
String simulateEncrypt(const std::vector<uint8_t>& data) {
  base64_encodestate state;
  base64_init_encodestate(&state);

  std::vector<char> encoded(data.size() * 2);
  int len = base64_encode_block((const char*)data.data(), data.size(), encoded.data(), &state);
  len += base64_encode_blockend(encoded.data() + len, &state);

  return String(encoded.data());
}

std::vector<uint8_t> simulateDecrypt(const String& base64data) {
  base64_decodestate state;
  base64_init_decodestate(&state);

  std::vector<uint8_t> decoded(base64data.length());
  int len = base64_decode_block(base64data.c_str(), base64data.length(), (char*)decoded.data(), &state);
  decoded.resize(len);

  return decoded;
}

// === Build signed + encrypted message ===
String buildSecureMessage(const std::vector<uint8_t>& compressedData) {
  uint32_t nonce = getAndIncrementNonce();
  String encoded = simulateEncrypt(compressedData);

  String messageToSign = String(nonce) + encoded;
  std::vector<uint8_t> msgBytes(messageToSign.begin(), messageToSign.end());
  String hmac = computeHMAC(msgBytes, PSK);

  String output = "{";
  output += "\"nonce\":" + String(nonce) + ",";
  output += "\"encrypted\":true,";
  output += "\"algorithm\":\"base64\",";
  output += "\"data\":\"" + encoded + "\",";
  output += "\"hmac\":\"" + hmac + "\"";
  output += "}";
  return output;
}

// === Verify incoming message ===
bool verifySecureMessage(const String& jsonMessage) {
  int nonceStart = jsonMessage.indexOf("\"nonce\":");
  int dataStart  = jsonMessage.indexOf("\"data\":\"");
  int hmacStart  = jsonMessage.indexOf("\"hmac\":\"");

  if (nonceStart == -1 || dataStart == -1 || hmacStart == -1) {
    Serial.println("[SECURITY] ❌ Invalid message format.");
    return false;
  }

  int nonceEnd = jsonMessage.indexOf(',', nonceStart);
  String nonceStr = jsonMessage.substring(nonceStart + 8, nonceEnd);
  uint32_t nonce = nonceStr.toInt();

  int dataEnd = jsonMessage.indexOf('"', dataStart + 8);
  String data = jsonMessage.substring(dataStart + 8, dataEnd);

  int hmacEnd = jsonMessage.indexOf('"', hmacStart + 8);
  String receivedHmac = jsonMessage.substring(hmacStart + 8, hmacEnd);

  if (!verifyAndStoreNonce(nonce)) {
    Serial.println("[SECURITY] ❌ Replay attack detected!");
    return false;
  }

  String messageToSign = String(nonce) + data;
  std::vector<uint8_t> msgBytes(messageToSign.begin(), messageToSign.end());
  String computedHmac = computeHMAC(msgBytes, PSK);

  if (!computedHmac.equalsIgnoreCase(receivedHmac)) {
    Serial.println("[SECURITY] ❌ HMAC verification failed!");
    return false;
  }

  Serial.println("[SECURITY] ✅ Message verified successfully!");
  return true;
}
