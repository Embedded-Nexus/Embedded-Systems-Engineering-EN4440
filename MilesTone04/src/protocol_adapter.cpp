#include "protocol_adapter.h"

namespace ProtocolAdapter {

    // Global frame queue
    static vector<vector<uint8_t>> frameQueue;

    // CRC16 (Modbus standard)
    uint16_t modbusCRC(uint8_t *buf, int len) {
        uint16_t crc = 0xFFFF;
        for (int pos = 0; pos < len; pos++) {
            crc ^= buf[pos];
            for (int i = 0; i < 8; i++) {
                crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
            }
        }
        return crc;
    }

    // Build Modbus Request Frame
    vector<uint8_t> BuildRequestFrame(uint8_t slaveAddr, uint8_t funcCode,
                                      uint16_t startAddr, uint16_t numReg,
                                      uint16_t data /* = 0 */) {
        vector<uint8_t> frame;

        if (funcCode == 0x03) {
            // READ Holding Registers
            frame.resize(8);
            frame[0] = slaveAddr;
            frame[1] = funcCode;
            frame[2] = (startAddr >> 8) & 0xFF;
            frame[3] = startAddr & 0xFF;
            frame[4] = (numReg >> 8) & 0xFF;
            frame[5] = numReg & 0xFF;

            uint16_t crc = modbusCRC(frame.data(), 6);
            frame[6] = crc & 0xFF;
            frame[7] = (crc >> 8) & 0xFF;
        } 
        else if (funcCode == 0x06) {
            // WRITE Single Register
            frame.resize(8);
            frame[0] = slaveAddr;
            frame[1] = funcCode;
            frame[2] = (startAddr >> 8) & 0xFF;
            frame[3] = startAddr & 0xFF;
            frame[4] = (data >> 8) & 0xFF;
            frame[5] = data & 0xFF;

            uint16_t crc = modbusCRC(frame.data(), 6);
            frame[6] = crc & 0xFF;
            frame[7] = (crc >> 8) & 0xFF;
        }

        return frame;
    }

    // Decode struct → Build & Queue frames
    const vector<vector<uint8_t>>& decodeRequestStruct(const RequestSIM &input) {
        const uint8_t slaveAddr = 0x01;
        frameQueue.clear();

        Serial.println("[ProtocolAdapter] Decoding RequestSIM...");

        // Handle WRITE requests first
        for (int i = 0; i < NUM_REGISTERS; i++) {
            if (input.write[i]) {
                uint16_t data = input.writeData[i];
                vector<uint8_t> frame = BuildRequestFrame(slaveAddr, 0x06, i, 1, data);
                frameQueue.push_back(frame);
                Serial.printf("[WRITE] Queued R%d (Data=%d)\n", i, data);
            }
        }

        // Handle READ requests
        for (int i = 0; i < NUM_REGISTERS; i++) {
            if (input.read[i]) {
                vector<uint8_t> frame = BuildRequestFrame(slaveAddr, 0x03, i, 1);
                frameQueue.push_back(frame);
                Serial.printf("[READ]  Queued R%d\n", i);
            }
        }

        Serial.printf("[ProtocolAdapter] Total frames queued: %d\n", (int)frameQueue.size());
        return frameQueue;
    }

    // Accessors
    const vector<vector<uint8_t>>& getFrameQueue() { return frameQueue; }
    void clearFrameQueue() { frameQueue.clear(); }

}  // namespace ProtocolAdapter
