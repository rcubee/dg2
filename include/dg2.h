#ifndef DG2_H_
#define DG2_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <assert.h>
#include "dg2_conf.h"
#include "dg2_crc.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DG2_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#ifdef DG2_ASSERT_ENABLE
#define DG2_ASSERT(expr) assert(expr)
#else
#define DG2_ASSERT(expr) ((void)0)
#endif

#define DG2_MIN(expr1, expr2) ({ \
    typeof(expr1) _val1 = (expr1); \
    typeof(expr2) _val2 = (expr2); \
    _val1 < _val2 ? _val1 : _val2; \
})

#define DG2_SWAP16(expr) __builtin_bswap16(expr)
#define DG2_SWAP32(expr) __builtin_bswap32(expr)

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
    DG2_ERROR_BUSY,
    DG2_ERROR_TIMEOUT
} dg2_error;

char *dg2_error_to_str(dg2_error error);

void dg2_copy_bytes(uint8_t *dest, uint8_t *src, size_t count);
void dg2_copy_halfwords(uint8_t *dest, uint8_t *src, size_t count);

uint16_t dg2_crc(uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_H_ */
