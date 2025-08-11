#include "dg2.h"
#include <string.h>

char *dg2_error_to_str(dg2_error error)
{
    switch (error) {
        case DG2_OK: return "Ok";
        case DG2_ERROR: return "Error";
        case DG2_ERROR_READ: return "Read error";
        case DG2_ERROR_WRITE: return "Write error";
        case DG2_ERROR_PKT_INCOMPLETE: return "Incomplete packet";
        case DG2_ERROR_PKT_INVALID_HEADER: return "Invalid packet header";
        case DG2_ERROR_PKT_TOO_LONG: return "Packet is too long";
        case DG2_ERROR_CRC_MISMATCH: return "CRC mismatch";
        default: return "Unknown error";
    }
}

inline void dg2_read_bytes(uint8_t *dest, uint8_t *bytes, size_t count)
{
    memcpy(dest, bytes, count);
}

void dg2_read_halfwords(uint16_t *dest, uint8_t *halfwords, size_t count)
{
    size_t offset = 0;
    for (size_t i = 0; i < count; ++i) {
        /* Note: Unalligned access */
        dest[i] = ((halfwords[offset] << 8)
                   | halfwords[offset + 1]);

        offset += sizeof(uint16_t);
    }
}

// void dg2_read_words(uint32_t *dest, uint8_t *words, size_t count)
// {
//     size_t offset = 0;
//     for (size_t i = 0; i < count; ++i) {
//         /* Note: Unalligned access */
//         dest[i] = ((words[offset] << 24)
//                    | (words[offset + 1] << 16)
//                    | (words[offset + 2] << 8)
//                    | words[offset + 3]);
//
//         offset += sizeof(uint32_t);
//     }
// }

void dg2_write_bytes(void *dest, uint8_t *bytes, size_t count)
{
    DG2_ASSERT(dest);
    DG2_ASSERT(count > 0 ? bytes != NULL : 1);

    memcpy(dest, bytes, count);
}

void dg2_write_halfwords(void *dest, uint16_t *halfwords, size_t count)
{
    DG2_ASSERT(dest);
    DG2_ASSERT(count > 0 ? halfwords != NULL : 1);

    size_t offset = 0;
    for (size_t i = 0; i < count; ++i) {
        uint16_t halfword = DG2_LE_TO_BE_HALFWORD(halfwords[i]);

        memcpy(dest + offset, &halfword, sizeof(uint16_t));
        offset += sizeof(uint16_t);
    }
}

void dg2_write_words(void *dest, uint32_t *words, size_t count)
{
    DG2_ASSERT(dest);
    DG2_ASSERT(count > 0 ? words != NULL : 1);

    size_t offset = 0;
    for (size_t i = 0; i < count; ++i) {
        uint32_t word = DG2_LE_TO_BE_WORD(words[i]);

        memcpy(dest + offset, &word, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
}

uint16_t dg2_crc(uint8_t *data, size_t size)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < size; ++i)
    {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; ++j)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
                crc >>= 1;
        }
    }

    return crc;
}
