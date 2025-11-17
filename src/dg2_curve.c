#include "dg2.h"
#include "dg2_curve.h"

#if 0

void dg2_curve_init(dg2_curve *curve, dg2_curve_ch ch, uint16_t *buff, uint8_t buff_capacity)
{
    DG2_ASSERT(curve);
    DG2_ASSERT(buff);

    curve->ch = ch;
    curve->buff = buff;
    curve->buff_capacity = buff_capacity;
    curve->data_len = 0;
}

void dg2_curve_clear(dg2_curve *curve)
{
    DG2_ASSERT(curve);
    DG2_ASSERT(curve->buff);

    memset(curve->buff, 0, curve->buff_capacity);
    curve->data_len = 0;
}

void dg2_curve_app(dg2_curve *curve, uint16_t *src, uint8_t data_len)
{
    DG2_ASSERT(curve);
    DG2_ASSERT(curve->buff);
    DG2_ASSERT(src);
    DG2_ASSERT((curve->data_len + data_len) * sizeof(uint16_t) <= curve->buff_capacity);

    memcpy(curve->buff + curve->data_len, src, data_len * sizeof(uint16_t));
    curve->data_len += data_len;
}

void dg2_curve_write(dg2_curve *curve, uint16_t *src, uint8_t data_len)
{
    DG2_ASSERT(curve);
    DG2_ASSERT(curve->buff);
    DG2_ASSERT(src);
    DG2_ASSERT(data_len * sizeof(uint16_t) <= curve->buff_capacity);

    memcpy(curve->buff, src, data_len * sizeof(uint16_t));
    curve->data_len = data_len;
}

#endif // 0
