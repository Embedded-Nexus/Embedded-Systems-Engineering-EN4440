#ifndef MODBUS_UTILS_H
#define MODBUS_UTILS_H

#include <Arduino.h>
#include <vector>

namespace Modbus {
    // Calculates Modbus RTU CRC16 (LSB first, polynomial 0xA001)
    uint16_t modbusCRC(uint8_t *buf, int len);
}

#endif  // MODBUS_UTILS_H
