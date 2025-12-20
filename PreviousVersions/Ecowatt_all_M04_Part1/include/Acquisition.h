#pragma once
#include <Arduino.h>
#include <vector>
#include "Buffer.h"

namespace Acquisition {
struct RegInfo { uint16_t gain; bool writable; };
extern const RegInfo REGMAP[];

std::vector<uint8_t> buildReadFrame(uint8_t slave, uint16_t startAddr, uint16_t numReg);
String frameToJson(const std::vector<uint8_t>& frame);
std::vector<uint8_t> jsonToFrame(const String& response);

bool readAndAppend(SampleBuffer& buf, const String& apiKey,
                   uint8_t slave, uint16_t startAddr, uint16_t numReg);

void tick(SampleBuffer& buf, const String& apiKey, unsigned long periodMs = 5000);

bool write(uint8_t slave, uint16_t address, uint16_t value, const String& apiKey);

void processJsonCommand(const String& jsonCmd, const String& apiKey);

// ✅ fetch and execute all commands from cloud
void fetchAndExecuteCommands(const String& apiUrl, const String& apiKey);


}
