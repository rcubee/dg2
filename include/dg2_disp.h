#ifndef DG2_DISP_H_
#define DG2_DISP_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "dg2.h"
#include "dg2_circ_buff.h"
#include "dg2_pkt.h"

typedef void* dg2_cb_data;
typedef size_t (*dg2_cb_write)(dg2_cb_data data, uint8_t *dest, size_t size);
typedef size_t (*dg2_cb_time)(dg2_cb_data data);
typedef void (*dg2_cb_process)(dg2_cb_data data);
typedef void (*dg2_cb_packet)(dg2_cb_data data, dg2_cmd cmd, uint16_t vp, uint8_t *payload, uint8_t payload_size);

typedef enum dg2_disp_sync_status
{
    DG2_DISP_SYNC_STATUS_OK = DG2_OK,
    DG2_DISP_SYNC_STATUS_BUSY = DG2_ERROR_BUSY,
    DG2_DISP_SYNC_STATUS_TIMEOUT =  DG2_ERROR_TIMEOUT
} dg2_disp_sync_status;

typedef enum dg2_disp_sync_read_type
{
    DG2_DISP_SYNC_READ_TYPE_U8 = 0,
    DG2_DISP_SYNC_READ_TYPE_U16
} dg2_disp_sync_read_type;

typedef struct dg2_disp_sync
{
    dg2_disp_sync_status status;
    size_t start_time;
    size_t timeout;
    dg2_cmd cmd;
    uint16_t vp;
    void *read_dest;
    size_t read_size;
    dg2_disp_sync_read_type read_type;
} dg2_disp_sync;

typedef struct dg2_disp
{
    dg2_cb_write cb_write;
    dg2_cb_time cb_time;
    dg2_cb_process cb_process;
    dg2_cb_packet cb_packet;
    dg2_cb_crc cb_crc;
    dg2_cb_data cb_data;

    dg2_disp_sync sync;

    uint8_t rx_buff[DG2_DISP_RX_BUFF_CAPACITY];
    dg2_circ_buff rx_circ_buff;

    uint8_t tx_buff[DG2_DISP_TX_BUFF_CAPACITY];
} dg2_disp;

void dg2_disp_init(dg2_disp *disp,
                   dg2_cb_write cb_write,
                   dg2_cb_time cb_time,
                   dg2_cb_process cb_process,
                   dg2_cb_packet cb_packet,
                   dg2_cb_crc cb_crc,
                   dg2_cb_data cb_data);

dg2_disp_sync_status dg2_disp_process(dg2_disp *disp);

dg2_error dg2_disp_read_vp(dg2_disp *disp, uint16_t vp, int16_t *value);
dg2_error dg2_disp_read_vps(dg2_disp *disp, uint16_t vp, int16_t *values, uint8_t count);

dg2_error dg2_disp_write_vp_async(dg2_disp *disp, uint16_t vp, int16_t value);
dg2_error dg2_disp_write_vps_async(dg2_disp *disp, uint16_t vp, int16_t *values, uint8_t count);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_DISP_H_ */
