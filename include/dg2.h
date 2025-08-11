// https://ecdn6.globalso.com/upload/p/1355/source/2025-02/T5L_DGUSII-Application-Development-Guide-V2.9-0207.pdf

#ifndef DG2_H_
#define DG2_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DG2_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#if 1
#define DG2_ASSERT(expr) assert(expr)
#else
#define DG2_ASSERT(expr)
#endif

#define DG2_LE_TO_BE_HALFWORD(halfword) ((((halfword) & 0x00FF) << 8) | (((halfword) & 0xFF00) >> 8))
#define DG2_LE_TO_BE_WORD(word) ((((word) & 0x000000FF) << 24) | (((word) & 0x0000FF00) << 8) | (((word) & 0x00FF0000) >> 8) | (((word) & 0xFF000000) >> 24))

#define DG2_RESULT(expr) { dg2_error e; \
    if ((e = (expr)) != DG2_OK) { \
        printf("%s resulted with error: %s\n",(#expr), dg2_error_to_str(e)); \
    }  \
}

typedef enum dg2_error
{
    DG2_OK = 0,
    DG2_ERROR,
    DG2_ERROR_READ,
    DG2_ERROR_WRITE,
    DG2_ERROR_PKT_INCOMPLETE,
    DG2_ERROR_PKT_INVALID_HEADER,
    DG2_ERROR_PKT_TOO_LONG,
    DG2_ERROR_CRC_MISMATCH
} dg2_error;

typedef uint16_t (*dg2_cb_crc)(uint8_t *data, size_t size);

char *dg2_error_to_str(dg2_error error);

void dg2_read_bytes(uint8_t *dest, uint8_t *bytes, size_t count);
void dg2_read_halfwords(uint16_t *dest, uint8_t *halfwords, size_t count);
// void dg2_read_words(uint32_t *dest, uint8_t *words, size_t count);

void dg2_write_bytes(void *dest, uint8_t *bytes, size_t count);
void dg2_write_halfwords(void *dest, uint16_t *halfwords, size_t count);
void dg2_write_words(void *dest, uint32_t *words, size_t count);

uint16_t dg2_crc(uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_H_ */
