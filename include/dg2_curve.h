#ifndef DG2_CURVE_H_
#define DG2_CURVE_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <stdint.h>

typedef enum dg2_curve_ch {
    DG2_CURVE_CH_0 = 0,
    DG2_CURVE_CH_1,
    DG2_CURVE_CH_2,
    DG2_CURVE_CH_3,
    DG2_CURVE_CH_4,
    DG2_CURVE_CH_5,
    DG2_CURVE_CH_6,
    DG2_CURVE_CH_7
} dg2_curve_ch;


typedef struct dg2_curve
{
    dg2_curve_ch ch;
    uint16_t *buff;
    uint8_t buff_capacity;
    uint8_t data_len;
} dg2_curve;

typedef struct dg2_curves
{
    dg2_curve *curves;
    uint8_t curve_count;
} dg2_curves;

void dg2_curve_init(dg2_curve *curve, dg2_curve_ch ch, uint16_t *buff, uint8_t buff_capacity);

void dg2_curve_clear(dg2_curve *curve);

void dg2_curve_app(dg2_curve *curve, uint16_t *src, uint8_t data_len);

void dg2_curve_write(dg2_curve *curve, uint16_t *src, uint8_t data_len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_CURVE_H_ */
