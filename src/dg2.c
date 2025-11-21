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

void dg2_copy_bytes(uint8_t *dest, uint8_t *bytes, size_t count)
{
    memcpy(dest, bytes, count);
}

void dg2_copy_halfwords(uint8_t *dest, uint8_t *src, size_t count)
{
    for (size_t i = 0; i < (count << 1 /* Multiply by 2 */); i += 2) {
        dest[i] = src[i + 1];
        dest[i + 1] = src[i];
    }
}
