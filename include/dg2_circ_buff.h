#ifndef DG2_CIRC_BUFF_H_
#define DG2_CIRC_BUFF_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t *buff;
    uint8_t *back;
    uint8_t *front;
    size_t capacity;
    size_t size;
} dg2_circ_buff;

void dg2_circ_buff_init(dg2_circ_buff *circ_buff, uint8_t *buff, size_t capacity);

void dg2_circ_buff_clear(dg2_circ_buff *circ_buff);

static inline size_t dg2_circ_buff_get_size(dg2_circ_buff *circ_buff)
{
    return circ_buff->size;
}

size_t dg2_circ_buff_get_free_space(dg2_circ_buff *circ_buff);

size_t dg2_circ_buff_read(dg2_circ_buff *circ_buff, uint8_t *dest, size_t size);

static inline size_t dg2_circ_buff_pop(dg2_circ_buff *circ_buff, uint8_t *dest)
{
    return dg2_circ_buff_read(circ_buff, dest, 1);
}

size_t dg2_circ_buff_write(dg2_circ_buff *circ_buff, const uint8_t *src, size_t size);

static inline size_t dg2_circ_buff_push(dg2_circ_buff *circ_buff, const uint8_t *src)
{
    return dg2_circ_buff_write(circ_buff, src, 1);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_CIRC_BUFF_H_ */
