#include "dg2.h"
#include <string.h>

char *dg2_error_to_str(dg2_error error)
{
    switch (error) {
    case DG2_OK: return "Ok";
    case DG2_ERROR: return "Error";
    case DG2_ERROR_READ: return "Read error";
    case DG2_ERROR_WRITE: return "Write error";
    case DG2_ERROR_BUSY: return "Busy";
    case DG2_ERROR_TIMEOUT: return "Timeout";

    default: return "Unknown error";
    }
}

void dg2_read_bytes(uint8_t *dest, uint8_t *bytes, size_t count)
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
        uint16_t halfword = DG2_SWAP16(halfwords[i]);

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
        uint32_t word = DG2_SWAP32(words[i]);

        memcpy(dest + offset, &word, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
}
