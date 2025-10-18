#include "inverter_comm.h"
#include <sstream>
#include <iomanip>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "debug_utils.h"
#include "modbus_utils.h"
#include "register_map.h"
#include "inverterSIM_utils.h"


namespace InverterSim {
        ////////////////////////// CRC Validation //////////////////////////
        bool validateCRC(const vector<uint8_t>& frame) {
            if (frame.size() < 4) return false;  // too short to contain CRC
    
            uint16_t receivedCRC = (frame[frame.size() - 1] << 8) | frame[frame.size() - 2];
            uint16_t calcCRC = Modbus::modbusCRC((uint8_t*)frame.data(), frame.size() - 2);
    
            if (receivedCRC == calcCRC) {
                DEBUG_PRINTLN("[InverterSim] CRC check passed.");
                return true;
            } else {
                DEBUG_PRINTF("[InverterSim] CRC check failed. Received: %04X, Expected: %04X\n", receivedCRC, calcCRC);
                return false;
            }
        }

    ////////////////////////// Validate Response //////////////////////////
    bool ValidateResponseFrame(const vector<uint8_t>& responseFrame) {
        if (responseFrame.empty()) {
            DEBUG_PRINTLN("[InverterSim] Empty or invalid response frame.");
            return false;
        }

        if (!validateCRC(responseFrame)) {
            DEBUG_PRINTLN("[InverterSim] CRC validation failed.");
            return false;
        }

        uint8_t funcCode = responseFrame[1];
        if (funcCode & 0x80) {
            uint8_t exceptionCode = responseFrame[2];
            DEBUG_PRINTF("[InverterSim] Modbus Exception: 0x%02X\n", exceptionCode);
            return false;
        }

        DEBUG_PRINTLN("[InverterSim] Valid Modbus response frame.");
        return true;
    }

    ////////////////////////// Decode Response //////////////////////////
    void decodeResponseFrame(const vector<uint8_t>& frame, uint16_t startAddr) {
        if (frame.size() < 5) return;
        uint8_t funcCode = frame[1];
    
        if (funcCode == 0x03) { // READ
            uint8_t byteCount = frame[2];
            uint8_t numRegisters = byteCount / 2;
            DEBUG_PRINTF("[InverterSim] Decoding %d registers (starting at R%d):\n", numRegisters, startAddr);
    
            for (uint8_t i = 0; i < numRegisters; i++) {
                uint16_t regAddr = startAddr + i;
                uint16_t rawValue = (frame[3 + i * 2] << 8) | frame[4 + i * 2];
    
                if (regAddr < REGISTER_COUNT) {
                    const auto& reg = registerMap[regAddr];
                    float scaled = (float)rawValue / reg.scale;
                    DEBUG_PRINTF("  R%-2d %-35s = %.2f %s (raw=%d)\n",
                                 reg.index, reg.name, scaled, reg.unit, rawValue);
                } else {
                    DEBUG_PRINTF("  R%-2d (Unknown) = %d\n", regAddr, rawValue);
                }
            }
        }
        else if (funcCode == 0x06) { // WRITE
            uint16_t addr = (frame[2] << 8) | frame[3];
            uint16_t value = (frame[4] << 8) | frame[5];
    
            if (addr < REGISTER_COUNT) {
                const auto& reg = registerMap[addr];
                DEBUG_PRINTF("[InverterSim] Write Confirmed: %s (R%d) = %d %s\n",
                             reg.name, addr, value, reg.unit);
            } else {
                DEBUG_PRINTF("[InverterSim] Write Confirmed: Unknown R%d = %d\n", addr, value);
            }
        }
        else {
            DEBUG_PRINTF("[InverterSim] Unsupported function code: 0x%02X\n", funcCode);
        }
    }
    
    

    //////////////////// Main inverter send function ////////////////////
    bool sendFrameToInverter(const vector<uint8_t>& frame) {
        String jsonFrame = InverterUtils::frameToJson(frame);
        String apiKey = "NjhhZWIwNDU1ZDdmMzg3MzNiMTQ5Yjg2OjY4YWViMDQ1NWQ3ZjM4NzMzYjE0OWI3Yw";
        String response;
    
        uint8_t funcCode = frame[1];
        uint16_t startAddr = (frame[2] << 8) | frame[3];   // ✅ extract address
    
        if (funcCode == 0x03) {
            DEBUG_PRINTLN("[InverterSim] Function Code 0x03 → READ operation");
            response = InverterUtils::readAPI(jsonFrame, apiKey);
        } 
        else if (funcCode == 0x06) {
            DEBUG_PRINTLN("[InverterSim] Function Code 0x06 → WRITE operation");
            response = InverterUtils::writeAPI(jsonFrame, apiKey);
        } 
        else {
            DEBUG_PRINTF("[InverterSim] Unknown Function Code: 0x%02X\n", funcCode);
            return false;
        }
    
        bool success = response.length() > 0;
        if (success) {
            DEBUG_PRINTLN("[InverterSim] Frame sent successfully.");
            processResponseFrame(response, startAddr);   // ✅ pass address
        } 
        else {
            DEBUG_PRINTLN("[InverterSim] Frame send failed.");
        }
    
        return success;
    }
    

    //////////////////// Retry logic for queued frames ////////////////////
    void processFrameQueue(const vector<vector<uint8_t>>& frames) {
        DEBUG_PRINTF("[InverterSim] Processing %d frame(s)...\n", (int)frames.size());

        int frameIndex = 0;
        for (const auto& frame : frames) {
            frameIndex++;
            DEBUG_PRINTF("[InverterSim] Sending frame #%d...\n", frameIndex);

            int attempt = 0;
            bool success = false;

            while (attempt < 3 && !success) {
                DEBUG_PRINTF("Attempt %d for frame #%d...\n", attempt + 1, frameIndex);
                success = sendFrameToInverter(frame);
                attempt++;
                if (!success) {
                    DEBUG_PRINTLN("[InverterSim] Retry in 400ms...");
                    delay(400);
                }
            }

            if (!success) {
                DEBUG_PRINTF("[InverterSim] Frame #%d FAILED after 3 attempts.\n", frameIndex);
                // TODO: log or queue for retry later
            } else {
                DEBUG_PRINTF("[InverterSim] Frame #%d SUCCESS.\n", frameIndex);
            }
        }

        DEBUG_PRINTLN("[InverterSim] Frame queue processing complete.");
    }

    //////////////////// Process Response Frames ////////////////////
    void processResponseFrame(const String& response, uint16_t startAddr) {
        DEBUG_PRINTLN("[InverterSim] === Processing Response Frame ===");
    
        // Step 1: Convert JSON → binary Modbus frame
        auto frame = InverterUtils::jsonToFrame(response);
        if (frame.empty()) {
            DEBUG_PRINTLN("[InverterSim] JSON → Frame conversion failed.");
            return;
        }
    
        // Step 2: Validate CRC and Modbus error codes
        if (!ValidateResponseFrame(frame)) {
            DEBUG_PRINTLN("[InverterSim] Response frame validation failed.");
            return;
        }
    
        // Step 3: Decode and display registers — ✅ now with startAddr
        decodeResponseFrame(frame, startAddr);
    
        DEBUG_PRINTLN("[InverterSim] === Response Frame Processing Complete ===");
    }
}  // namespace InverterSim
