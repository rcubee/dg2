#include "dg2_circ_buff.h"
#include "dg2.h"

/*
 * Note: Implementation of the circular buffer
 * - Data is pushed at the front of the buffer
 * - Data is popped at the back of the buffer
 * This implementation doesn't check whether back = front.
 *
 * Todo:
 * - Wrap-around with the help of binary AND operation (constrain the buffer capacity to be a power-of-two)?
 * - Optimize push and pop methods for ISRs
 * - Overflow on write handling policy
 */

void dg2_circ_buff_init(dg2_circ_buff *circ_buff, uint8_t *buff, size_t capacity)
{
    DG2_ASSERT(circ_buff);
    DG2_ASSERT(buff);
    DG2_ASSERT(capacity > 0);

    circ_buff->buff = buff;
    circ_buff->back = circ_buff->buff;
    circ_buff->front = circ_buff->buff;
    circ_buff->capacity = capacity;
    circ_buff->size = 0;
}

void dg2_circ_buff_clear(dg2_circ_buff *circ_buff)
{
    DG2_ASSERT(circ_buff);

    circ_buff->back = circ_buff->buff;
    circ_buff->front = circ_buff->buff;
    circ_buff->size = 0;
}

size_t dg2_circ_buff_get_free_space(dg2_circ_buff *circ_buff)
{
    return circ_buff->capacity - dg2_circ_buff_get_size(circ_buff);
}

size_t dg2_circ_buff_read(dg2_circ_buff* restrict circ_buff, uint8_t* restrict dest, size_t size)
{
    DG2_ASSERT(circ_buff);
    DG2_ASSERT((size > 0) ? (dest != NULL) : 1);

    size_t total_read_size = DG2_MIN(size, circ_buff->size);

    if (total_read_size == 0) {
        return total_read_size;
    }

    circ_buff->size -= total_read_size;

    /* First read */

    size_t first_read_size = DG2_MIN(circ_buff->buff + circ_buff->capacity - circ_buff->back /* Don't overflow */, total_read_size);

    memcpy(dest, circ_buff->back, first_read_size);
    circ_buff->back = circ_buff->buff + (((circ_buff->back + first_read_size) - circ_buff->buff) % circ_buff->capacity); // Note: Wrap-around

    /* Conditional second read */

    size_t second_read_size = total_read_size - first_read_size;

    if (second_read_size == 0) {
        return total_read_size;
    }

    memcpy(dest + first_read_size, circ_buff->back, second_read_size);
    circ_buff->back += second_read_size;

    return total_read_size;
}

size_t dg2_circ_buff_write(dg2_circ_buff* restrict circ_buff, const uint8_t* restrict src, size_t size)
{
    DG2_ASSERT(circ_buff != NULL);
    DG2_ASSERT((size > 0) ? (src != NULL) : 1);

    size_t total_write_size = DG2_MIN(size, dg2_circ_buff_get_free_space(circ_buff));

    if (total_write_size == 0) {
        return total_write_size;
    }

    circ_buff->size += total_write_size;

    /* First write */

    size_t first_write_size = DG2_MIN(circ_buff->buff + circ_buff->capacity - circ_buff->front /* Don't overflow */, total_write_size);

    memcpy(circ_buff->front, src, first_write_size);
    circ_buff->front = circ_buff->buff + (((circ_buff->front + first_write_size) - circ_buff->buff) % circ_buff->capacity); // Note: Wrap-around

    /* Conditional second write */

    size_t second_write_size = total_write_size - first_write_size;

    if (second_write_size == 0) {
        return total_write_size;
    }

    memcpy(circ_buff->front, src + first_write_size, second_write_size);
    circ_buff->front += second_write_size;

    return total_write_size;
}
