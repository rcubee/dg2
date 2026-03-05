#ifndef DG2_DISP_H_
#define DG2_DISP_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "dg2.h"
#include "dg2_circ_buff.h"
#include "dg2_conf.h"
#include "dg2_crc.h"
#include "dg2_pkt.h"
#include <stdbool.h>

typedef void* dg2_user_data;
typedef size_t (*dg2_cb_transmit)(dg2_user_data data, const uint8_t *src, size_t size);
typedef size_t (*dg2_cb_time)(dg2_user_data data);
typedef void (*dg2_cb_yield)(dg2_user_data data, size_t timeout_in);
typedef void (*dg2_cb_packet)(dg2_user_data data, dg2_cmd cmd, uint16_t vp, uint8_t *payload, uint8_t payload_size);
typedef void (*dg2_cb_lock)(dg2_user_data data);
typedef void (*dg2_cb_unlock)(dg2_user_data data);

typedef enum dg2_disp_sync_status
{
    DG2_DISP_SYNC_STATUS_SYNCED = DG2_OK,
    DG2_DISP_SYNC_STATUS_SYNCING = DG2_ERROR_BUSY,
    DG2_DISP_SYNC_STATUS_TIMEOUT = DG2_ERROR_TIMEOUT
} dg2_disp_sync_status;

typedef void (*dg2_disp_sync_read_parser)(void *dest, void *payload, size_t payload_size);

typedef struct dg2_disp_config
{
    dg2_cb_transmit cb_transmit;
    dg2_cb_crc cb_crc;
    dg2_cb_time cb_time;
    dg2_cb_yield cb_yield;
    dg2_cb_packet cb_packet;
    dg2_cb_lock cb_lock;
    dg2_cb_unlock cb_unlock;
    dg2_user_data user_data;

    size_t timeout;
} dg2_disp_config;

typedef struct dg2_disp_sync
{
    dg2_disp_sync_status status;
    size_t start_time;
    size_t timeout; /* In user-defined units */

    dg2_cmd cmd;
    uint16_t vp;

    void *read_dest;
    size_t read_payload_size; /* In halfwords */
    dg2_disp_sync_read_parser read_parser;
} dg2_disp_sync;

typedef struct dg2_disp
{
    dg2_cb_transmit cb_transmit;
    dg2_cb_crc cb_crc;
    dg2_cb_time cb_time;
    dg2_cb_yield cb_yield;
    dg2_cb_packet cb_packet;
    dg2_cb_lock cb_lock;
    dg2_cb_unlock cb_unlock;
    dg2_user_data user_data;

    uint8_t rx_buff[DG2_DISP_RX_BUFF_CAPACITY];
    dg2_circ_buff rx_circ_buff;

    uint8_t tx_buff[DG2_DISP_TX_BUFF_CAPACITY];

    dg2_disp_sync sync;
} dg2_disp;

void dg2_disp_init(dg2_disp *disp, const dg2_disp_config *config);

dg2_error dg2_disp_process(dg2_disp *disp);

dg2_error dg2_disp_read_vp_async(dg2_disp *disp, uint16_t vp);
dg2_error dg2_disp_read_vps_async(dg2_disp *disp, uint16_t vp, uint8_t count);
dg2_error dg2_disp_read_vp(dg2_disp *disp, uint16_t vp, int16_t *dest);
dg2_error dg2_disp_read_vps(dg2_disp *disp, uint16_t vp, int16_t *dest, uint8_t count);

dg2_error dg2_disp_write_vp_async(dg2_disp *disp, uint16_t vp, int16_t data);
dg2_error dg2_disp_write_vps_async(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count);
dg2_error dg2_disp_write_vp(dg2_disp *disp, uint16_t vp, int16_t data);
dg2_error dg2_disp_write_vps(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_DISP_H_ */
