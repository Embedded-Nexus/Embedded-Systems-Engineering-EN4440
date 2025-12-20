#include "Acquisition.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "Acquisition.h"

namespace {
// CRC16 for Modbus RTU
uint16_t crc16_modbus(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// POST JSON to inverter API
String postJson(const String& url, const String& json, const String& apiKey) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", apiKey);
  int code = http.POST(json);
  String body = (code > 0) ? http.getString() : String("");
  http.end();
  return body;
}

} // namespace

namespace Acquisition {
  void processJsonCommand(const String& jsonCmd, const String& apiKey) {
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, jsonCmd);
  
    if (err) {
      Serial.println("[CMD] Invalid JSON command");
      return;
    }
  
    String func = doc["function"];
    if (func != "write") {
      Serial.println("[CMD] Unsupported function: " + func);
      return;
    }
  
    uint8_t slave = doc["slave"];
    uint16_t addr = doc["address"];
    uint16_t val  = doc["value"];
  
    
    bool ok = Acquisition::write(slave, addr, val, apiKey);
  
    if (ok) Serial.println("[CMD] Write command executed successfully");
    else    Serial.println("[CMD] Write command failed");
  }

  void fetchAndExecuteCommands(const String& apiUrl, const String& apiKey) {
    WiFiClient client;
    HTTPClient http;
  
    Serial.println("[CMD] Fetching command list from server...");
  
    http.begin(client, apiUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", apiKey);
  
    int code = http.GET();
  
    if (code <= 0) {
      Serial.printf("[CMD] HTTP error: %d\n", code);
      http.end();
      return;
    }
  
    String resp = http.getString();
    http.end();
  
    // If response is literally "No", skip
    if (resp == "No" || resp == "\"No\"" || resp.length() < 5) {
      Serial.println("[CMD] No new commands.");
      return;
    } else {
      Serial.println(resp);
      Serial.println("**********");
    }
  
    // Try to parse JSON array
    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, resp);
    if (err) {
      Serial.println("[CMD] JSON parse failed");
      return;
    }
  
    if (!doc.is<JsonArray>()) {
      Serial.println("[CMD] Response is not a JSON array");
      return;
    }
  
    // Iterate through each command in array
    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject cmd : arr) {
      String cmdString;
      serializeJson(cmd, cmdString);
      Serial.println("[CMD] Executing command: " + cmdString);
      processJsonCommand(cmdString, apiKey);
    }
  
    Serial.printf("[CMD] %d commands processed.\n", arr.size());
  }
  
 // namespace Acquisition
  

// Register metadata (gain + writable flag)
const RegInfo REGMAP[] = {
  {10, false}, // 0 Vac1
  {10, false}, // 1 Iac1
  {100, false},// 2 Fac1
  {10, false}, // 3 Vpv1
  {10, false}, // 4 Vpv2
  {10, false}, // 5 Ipv1
  {10, false}, // 6 Ipv2
  {10, false}, // 7 Temp
  {1,  true},  // 8 Export power %
  {1,  false}  // 9 Pac L
};

// Human-readable names and units
const char* UNITS[] = {"V","A","Hz","V","V","A","A","C","%","W"};
const char* NAMES[] = {
  "Vac1 /L1 Phase voltage",
  "Iac1 /L1 Phase current",
  "Fac1 /L1 Phase frequency",
  "Vpv1 /PV1 input voltage",
  "Vpv2 /PV2 input voltage",
  "Ipv1 /PV1 input current",
  "Ipv2 /PV2 input current",
  "Inverter internal temperature",
  "Export power percentage",
  "Pac L /Inverter output power"
};

// Build a Modbus RTU Read frame
std::vector<uint8_t> buildReadFrame(uint8_t slave, uint16_t startAddr, uint16_t numReg) {
  std::vector<uint8_t> f(8);
  f[0] = slave;
  f[1] = 0x03; // Read Holding Registers
  f[2] = (uint8_t)(startAddr >> 8);
  f[3] = (uint8_t)(startAddr & 0xFF);
  f[4] = (uint8_t)(numReg >> 8);
  f[5] = (uint8_t)(numReg & 0xFF);
  uint16_t crc = crc16_modbus(f.data(), 6);
  f[6] = (uint8_t)(crc & 0xFF);
  f[7] = (uint8_t)(crc >> 8);
  return f;
}

// Wrap frame in JSON
String frameToJson(const std::vector<uint8_t>& frame) {
  String hex;
  for (auto b : frame) {
    if (b < 16) hex += '0';
    hex += String(b, HEX);
  }
  return "{\"frame\":\"" + hex + "\"}";
}

// Extract frame bytes from JSON response
std::vector<uint8_t> jsonToFrame(const String& resp) {
  std::vector<uint8_t> out;
  int i = resp.indexOf("\"frame\":\"");
  if (i < 0) return out;
  i += 9;
  int j = resp.indexOf('"', i);
  if (j < 0) return out;
  String hex = resp.substring(i, j);
  for (unsigned int k = 0; k + 1 < hex.length(); k += 2) {
    out.push_back((uint8_t)strtol(hex.substring(k, k + 2).c_str(), nullptr, 16));
  }
  return out;
}

// Validate Modbus RTU frame (CRC + no exception)
static bool validateResponse(const std::vector<uint8_t>& f) {
  if (f.size() < 5) return false;
  uint16_t got = (uint16_t)f[f.size() - 2] | ((uint16_t)f[f.size() - 1] << 8);
  uint16_t calc = crc16_modbus(f.data(), f.size() - 2);
  if (got != calc) return false;
  if (f[1] & 0x80) return false; // exception
  return true;
}

// Perform a read and append samples to buffer
bool  readAndAppend(SampleBuffer& buf, const String& apiKey,
                   uint8_t slave, uint16_t startAddr, uint16_t numReg) {
  auto req = buildReadFrame(slave, startAddr, numReg);
  String payload = frameToJson(req);
  String resp = postJson("http://20.15.114.131:8080/api/inverter/read", payload, apiKey);
  auto frame = jsonToFrame(resp);

  if (!validateResponse(frame)) {
    Serial.println("[ACQ] invalid response: " + resp);
    return false;
  }

  uint8_t byteCount = frame[2];
  unsigned long t = millis();
  for (uint16_t k = 0; k < byteCount / 2; k++) {
    uint16_t reg = startAddr + k;
    uint16_t raw = ((uint16_t)frame[3 + 2 * k] << 8) | frame[4 + 2 * k];
    uint16_t scaled = raw; // store raw in buffer
    float value = (float)raw / (float)REGMAP[reg].gain;

    Sample s{t, reg, scaled};
    buf.addSample(s);

    // Debug print (Addr, Name, Raw, Scaled Value + Unit)
    Serial.printf("[ACQ] Addr %-2u | %-30s | Raw=%u | Value=%.2f %s\n",
                  reg, NAMES[reg], raw, value, UNITS[reg]);
  }
  return true;
}

// Periodic acquisition: every 5 s read 10 regs (0–9)
void tick(SampleBuffer& buf, const String& apiKey, unsigned long periodMs) {
  static unsigned long last = 0;
  if (millis() - last < periodMs) return;
  last = millis();

  readAndAppend(buf, apiKey, 0x11, 0, 10);
}

bool write(uint8_t slave, uint16_t address, uint16_t value, const String& apiKey) {
  // Build Modbus Write Single Register frame (function 0x06)
  std::vector<uint8_t> f(8);
  f[0] = slave;
  f[1] = 0x06; // Write single register
  f[2] = address >> 8;
  f[3] = address & 0xFF;
  f[4] = value >> 8;
  f[5] = value & 0xFF;

  uint16_t crc = crc16_modbus(f.data(), 6);
  f[6] = crc & 0xFF;
  f[7] = crc >> 8;

  // Convert frame → JSON
  String payload = frameToJson(f);

  // Send via HTTP
  String resp = postJson("http://20.15.114.131:8080/api/inverter/write", payload, apiKey);

  // Decode response JSON → frame
  auto reply = jsonToFrame(resp);

  if (!validateResponse(reply)) {
    Serial.println("[WRITE] Invalid response or CRC failed");
    return false;
  }

  // Confirm written address & value
  uint16_t addr = ((uint16_t)reply[2] << 8) | reply[3];
  uint16_t val  = ((uint16_t)reply[4] << 8) | reply[5];

  Serial.printf("[WRITE] Register %u successfully set to %u\n", addr, val);
  return true;
}


}