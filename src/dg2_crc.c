#include "dg2_crc.h"

uint16_t dg2_crc(uint8_t *data, size_t size)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else {
                crc >>= 1;
            }
        }
    }

    return crc;
}
