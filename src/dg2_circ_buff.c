#include "dg2_circ_buff.h"
#include "dg2.h"

/*
 * Note: Implementation of the circular buffer
 * - Data is pushed at the front of the buffer
 * - Data is popped at the back of the buffer
 * To differentiate between the empty and full states, the size of the buffer is tracked.
 *
 * Todo:
 * - Overrun support
 */

/* This function won't work as intended if val >= 2 * wrap_at */
static size_t wrap_around_unsafe(size_t val, size_t wrap_at)
{
    DG2_ASSERT(wrap_at > 0U);
    DG2_ASSERT(val < 2U * wrap_at);

    if (val >= wrap_at) {
        val -= wrap_at;
    }

    return val;
}

/* This function doesn't check the size of the circular buffer */
static void dg2_circ_buff_discard_back_unsafe(dg2_circ_buff *circ_buff, size_t size)
{
    DG2_ASSERT(circ_buff);

    circ_buff->back = wrap_around_unsafe(circ_buff->back + size, circ_buff->capacity);

    circ_buff->size -= size;
}

/* This function doesn't check the size of the circular buffer */
static void dg2_circ_buff_advance_front_unsafe(dg2_circ_buff *circ_buff, size_t size)
{
    DG2_ASSERT(circ_buff);

    circ_buff->front = wrap_around_unsafe(circ_buff->front + size, circ_buff->capacity);

    circ_buff->size += size;
}

void dg2_circ_buff_init(dg2_circ_buff *circ_buff, uint8_t *buff, size_t capacity)
{
    DG2_ASSERT(circ_buff);
    DG2_ASSERT(buff);
    DG2_ASSERT(capacity > 0U);

    circ_buff->buff = buff;
    circ_buff->back = 0U;
    circ_buff->front = 0U;
    circ_buff->capacity = capacity;
    circ_buff->size = 0U;
}

size_t dg2_circ_buff_get_free_space(const dg2_circ_buff *circ_buff)
{
    DG2_ASSERT(circ_buff);

    return circ_buff->capacity - dg2_circ_buff_get_size(circ_buff);
}

size_t dg2_circ_buff_discard_back(dg2_circ_buff *circ_buff, size_t size)
{
    DG2_ASSERT(circ_buff);

    // Note: Don't discard more than possible
    size = DG2_MIN(size, dg2_circ_buff_get_size(circ_buff));

    dg2_circ_buff_discard_back_unsafe(circ_buff, size);

    return size;
}

size_t dg2_circ_buff_advance_front(dg2_circ_buff *circ_buff, size_t size)
{
    DG2_ASSERT(circ_buff);

    // Note: Don't advance more than possible
    size = DG2_MIN(size, dg2_circ_buff_get_free_space(circ_buff));

    dg2_circ_buff_advance_front_unsafe(circ_buff, size);

    return size;
}

void dg2_circ_buff_clear(dg2_circ_buff *circ_buff)
{
    DG2_ASSERT(circ_buff);

    circ_buff->back = 0U;
    circ_buff->front = 0U;
    circ_buff->size = 0U;
}

uint8_t dg2_circ_buff_peek(const dg2_circ_buff *circ_buff, uint8_t *dest, size_t rel_index)
{
    DG2_ASSERT(circ_buff);
    DG2_ASSERT(dest);

    if (rel_index >= dg2_circ_buff_get_size(circ_buff)) {
        *dest = 0U;

        return 0U;
    }

    size_t abs_index = wrap_around_unsafe(circ_buff->back + rel_index, circ_buff->capacity);

    *dest = *(circ_buff->buff + abs_index);

    return 1U;
}

uint8_t dg2_circ_buff_pop(dg2_circ_buff *circ_buff, uint8_t *dest)
{
    DG2_ASSERT(circ_buff);
    DG2_ASSERT(dest);

    if (dg2_circ_buff_get_size(circ_buff) == 0U) {
        *dest = 0U;

        return 0U;
    }

    *dest = *(circ_buff->buff + circ_buff->back);

    dg2_circ_buff_discard_back_unsafe(circ_buff, 1U);

    return 1U;
}

size_t dg2_circ_buff_copy(dg2_circ_buff* restrict circ_buff, uint8_t* restrict dest, size_t size)
{
    DG2_ASSERT(circ_buff);
    DG2_ASSERT((size > 0U) ? (dest != NULL) : 1U);

    size_t total_copy_size = DG2_MIN(size, circ_buff->size);

    if (total_copy_size < 1U) {
        return total_copy_size;
    }

    /* First copy */

    size_t first_copy_size = DG2_MIN(circ_buff->capacity - circ_buff->back /* Don't overflow */, total_copy_size);

    memcpy(dest, circ_buff->buff + circ_buff->back, first_copy_size);

    /* Conditional second copy */

    size_t second_copy_size = total_copy_size - first_copy_size;

    if (second_copy_size > 0U) {
        memcpy(dest + first_copy_size, circ_buff->buff /* Wrap-around */, second_copy_size);
    }

    return total_copy_size;
}

size_t dg2_circ_buff_read(dg2_circ_buff* restrict circ_buff, uint8_t* restrict dest, size_t size)
{
    DG2_ASSERT(circ_buff);
    DG2_ASSERT((size > 0U) ? (dest != NULL) : 1U);

    size_t total_read_size = dg2_circ_buff_copy(circ_buff, dest, size);

    dg2_circ_buff_discard_back_unsafe(circ_buff, total_read_size); /* Handles wrap-around */

    return total_read_size;
}

uint8_t dg2_circ_buff_push(dg2_circ_buff *circ_buff, uint8_t data)
{
    DG2_ASSERT(circ_buff);

    if (dg2_circ_buff_get_free_space(circ_buff) < 1U) {
        return 0U;
    }

    *(circ_buff->buff + circ_buff->front) = data;

    dg2_circ_buff_advance_front_unsafe(circ_buff, 1U);

    return 1U;
}

size_t dg2_circ_buff_write(dg2_circ_buff* restrict circ_buff, const uint8_t* restrict src, size_t size)
{
    DG2_ASSERT(circ_buff != NULL);
    DG2_ASSERT((size > 0U) ? (src != NULL) : 1U);

    size_t total_write_size = DG2_MIN(size, dg2_circ_buff_get_free_space(circ_buff));

    if (total_write_size < 1U) {
        return total_write_size;
    }

    /* First write */

    size_t first_write_size = DG2_MIN(circ_buff->capacity - circ_buff->front /* Don't overflow */, total_write_size);

    memcpy(circ_buff->buff + circ_buff->front, src, first_write_size);

    /* Conditional second write */

    size_t second_write_size = total_write_size - first_write_size;

    if (second_write_size > 0U) {
        memcpy(circ_buff->buff /* Wrap-around */, src + first_write_size, second_write_size);
    }

    dg2_circ_buff_advance_front_unsafe(circ_buff, total_write_size); // Note: Handles wrap-around

    return total_write_size;
}
