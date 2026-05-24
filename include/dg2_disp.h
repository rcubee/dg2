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

typedef size_t (*dg2_cb_transmit)(const uint8_t *src, size_t size, dg2_user_data user_data);
typedef size_t (*dg2_cb_time)(dg2_user_data user_data);
#ifdef DG2_SYNC_ENABLE
typedef void (*dg2_cb_yield)(size_t time_left, dg2_user_data user_data);
#endif // DG2_SYNC_ENABLE
typedef void (*dg2_cb_unsolicited_resp)(dg2_cmd cmd, uint16_t vp, uint8_t *payload, uint8_t payload_size, dg2_user_data user_data);
typedef bool (*dg2_cb_lock)(dg2_user_data data);
typedef void (*dg2_cb_unlock)(dg2_user_data data);

typedef void (*dg2_cb_resp_parser)(void *dest, void *payload, uint8_t payload_size);

typedef void (*dg2_cb_solicited_resp);
typedef void (*dg2_cb_trampoline)(
    dg2_error error,
    dg2_cb_solicited_resp cb_solicited_resp,
    dg2_user_data cb_solicited_resp_user_data,
    uint16_t vp,
    dg2_cmd cmd,
    void *payload,
    size_t payload_size,
    void *parsed_resp
);

/* Async callbacks */
typedef void (*dg2_disp_cb_read_vps_async)(dg2_error error, int16_t *data, uint8_t count, dg2_user_data user_data);
typedef void (*dg2_disp_cb_write_vps_async)(dg2_error error, uint16_t vp, dg2_user_data user_data);
typedef void (*dg2_disp_cb_read_floats_async)(dg2_error error, float *data, uint8_t count, dg2_user_data user_data);

typedef enum dg2_disp_req_sync_status
{
    DG2_DISP_REQ_SYNC_STATUS_BUSY = 0,
    DG2_DISP_REQ_SYNC_STATUS_DONE,
    DG2_DISP_REQ_SYNC_STATUS_TIMEOUT
} dg2_disp_req_sync_status;

typedef void (*dg2_disp_resp_parser)(void *dest, void *payload, size_t payload_size);

typedef struct dg2_disp_config
{
    dg2_cb_transmit cb_transmit;
    dg2_cb_crc cb_crc;
    dg2_cb_time cb_time;
#ifdef DG2_SYNC_ENABLE
    dg2_cb_yield cb_yield;
#endif // DG2_SYNC_ENABLE
    dg2_cb_unsolicited_resp cb_unsolicited_resp;
    dg2_cb_lock cb_lock;
    dg2_cb_unlock cb_unlock;
    dg2_user_data user_data;

    size_t timeout;
} dg2_disp_config;

typedef enum dg2_disp_exec_mode {
    DG2_DISP_EXEC_MODE_ASYNC,
    DG2_DISP_EXEC_MODE_SYNC
} dg2_disp_exec_mode;

typedef struct dg2_disp_exec_params_async {
    dg2_cb_solicited_resp cb_solicited_resp;
    dg2_user_data *cb_solicited_resp_user_data;
    dg2_cb_trampoline cb_trampoline;
} dg2_disp_exec_params_async;

/**
 * @struct dg2_disp_exec_params_sync
 * @brief Collections of parameters related to synchronous execution policy.
 * @warning req_status must be allocated on the stack of the caller.
 */
typedef struct dg2_disp_exec_params_sync {
    //// volatile dg2_disp_req_status *req_status;
    volatile dg2_disp_req_sync_status *req_sync_status;
    void *out;
} dg2_disp_exec_params_sync;

typedef union dg2_disp_exec_params {
    dg2_disp_exec_params_async async;
    dg2_disp_exec_params_sync sync;
} dg2_disp_exec_params;

typedef struct dg2_disp_exec {
    dg2_disp_exec_mode mode;
    dg2_disp_exec_params params;
} dg2_disp_exec;

typedef enum dg2_disp_req_action{
    DG2_DISP_REQ_ACTION_NONE,
    DG2_DISP_REQ_ACTION_TRANSMIT,
    DG2_DISP_REQ_ACTION_CANCEL
} dg2_disp_req_action;

typedef struct dg2_disp_req
{
    dg2_disp_req_action action;
    dg2_pkt pkt;

    size_t start_time;
    size_t timeout; /* In user-defined units */

    dg2_cmd cmd;
    uint16_t vp;

    uint8_t resp_payload_size; /* In halfwords */
    dg2_cb_resp_parser resp_parser;
} dg2_disp_req;

typedef struct dg2_disp
{
    dg2_cb_transmit cb_transmit;
    dg2_cb_crc cb_crc;
    dg2_cb_time cb_time;
#ifdef DG2_SYNC_ENABLE
    dg2_cb_yield cb_yield;
#endif // DG2_SYNC_ENABLE
    dg2_cb_unsolicited_resp cb_unsolicited_resp;
    dg2_cb_lock cb_lock;
    dg2_cb_unlock cb_unlock;
    dg2_user_data user_data;

    uint8_t rx_buff[DG2_DISP_RX_BUFF_CAPACITY];
    dg2_circ_buff rx_circ_buff;

    uint8_t tx_buff[DG2_DISP_TX_BUFF_CAPACITY];

    dg2_disp_exec exec;
    dg2_disp_req req;
    bool busy;
} dg2_disp;

void dg2_disp_init(dg2_disp *disp, const dg2_disp_config *config);

dg2_error dg2_disp_process(dg2_disp *disp);

void dg2_disp_cancel_req(dg2_disp *disp);

#ifdef DG2_ASYNC_ENABLE
dg2_error dg2_disp_read_vps_async(dg2_disp *disp, uint16_t vp, uint8_t count, dg2_disp_cb_read_vps_async cb, dg2_user_data user_data);
dg2_error dg2_disp_write_vps_async(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count, dg2_disp_cb_write_vps_async cb, dg2_user_data user_data);
dg2_error dg2_disp_read_floats_async(dg2_disp *disp, uint16_t vp, uint8_t count, dg2_disp_cb_read_floats_async cb, dg2_user_data user_data);
#endif // DG2_ASYNC_ENABLE

#ifdef DG2_SYNC_ENABLE
dg2_error dg2_disp_read_vps(dg2_disp *disp, uint16_t vp, uint8_t count, int16_t *out);
dg2_error dg2_disp_write_vps(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count);
dg2_error dg2_disp_read_floats(dg2_disp *disp, uint16_t vp, uint8_t count, float *out);
#endif // DG2_SYNC_ENABLE

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_DISP_H_ */
