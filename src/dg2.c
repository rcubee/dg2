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

void dg2_copy_bytes(uint8_t *dest, const uint8_t *src, size_t count)
{
    memcpy(dest, src, count);
}

// Note: Copies and swaps UP TO SIZE_MAX / 4 halfwords
void dg2_copy_and_swap_halfwords(uint8_t *restrict dest, const uint8_t *restrict src, size_t count)
{
    size_t bytes_to_copy = count << 1; // Note: Multiply by 2

    for (size_t i = 0; i < bytes_to_copy; i += 2) {
        dest[i] = src[i + 1];
        dest[i + 1] = src[i];
    }
}
