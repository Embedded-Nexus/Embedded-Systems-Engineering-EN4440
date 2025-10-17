#include "modbus_utils.h"
#include "debug_utils.h"
#include <vector>

namespace Modbus {

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
}
