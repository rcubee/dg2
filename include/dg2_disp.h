#ifndef DG2_DISP_H_
#define DG2_DISP_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "dg2.h"
#include "dg2_pkt.h"

typedef size_t (*dg2_cb_read)(uint8_t *buffer, size_t size);
typedef size_t (*dg2_cb_write)(uint8_t *buffer, size_t size);

typedef struct dg2_disp
{
    dg2_cb_read cb_read;
    dg2_cb_write cb_write;
    dg2_cb_crc cb_crc;

    uint8_t *tx_buff;
    size_t tx_buff_capacity;
    uint8_t *rx_buff;
    size_t rx_buff_capacity;
} dg2_disp;

dg2_pkt dg2_disp_pkt_init(dg2_disp *disp, dg2_cmd cmd, uint16_t vp);
dg2_error dg2_disp_pkt_exchange(dg2_disp *disp, dg2_pkt *pkt, size_t response_size);
dg2_error dg2_disp_pkt_exchange_ok(dg2_disp *disp, dg2_pkt *pkt);

void dg2_disp_init(dg2_disp *disp,
                        dg2_cb_read cb_read,
                        dg2_cb_write cb_write,
                        dg2_cb_crc cb_crc,
                        uint8_t *tx_buff,
                        size_t tx_buff_capacity,
                        uint8_t *rx_buff,
                        size_t rx_buff_capacity);

dg2_error dg2_disp_vp_read_bytes(dg2_disp *disp, uint16_t vp, uint8_t *dest, uint8_t count);
dg2_error dg2_disp_vp_read(dg2_disp *disp, uint16_t vp, int16_t *value);
dg2_error dg2_disp_vp_read_mul(dg2_disp *disp, uint16_t vp, int16_t *values, uint8_t count);

dg2_error dg2_disp_vp_write(dg2_disp *disp, uint16_t vp, int16_t value);
dg2_error dg2_disp_vp_write_mul(dg2_disp *disp, uint16_t vp, int16_t *values, uint8_t count);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_DISP_H_ */
