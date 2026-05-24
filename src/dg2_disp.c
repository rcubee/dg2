#include "dg2_disp.h"
#include "dg2_pkt.h"

#define DG2_RETURN_ERROR(expr) { \
    dg2_error error; \
    if ((error = (expr)) != DG2_OK) { \
        return error; \
    } \
}

static bool disp_lock(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    if (disp->cb_lock) {
        return disp->cb_lock(disp->user_data);
    }

    return true;
}

static void disp_unlock(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    if (disp->cb_unlock) {
        disp->cb_unlock(disp->user_data);
    }
}

static dg2_error disp_set_busy(dg2_disp *disp)
{
    if (disp_lock(disp) == false) {
        return DG2_ERROR_BUSY;
    }

    if (disp->busy) {
        disp_unlock(disp);

        return DG2_ERROR_BUSY;
    }

    disp->busy = true;

    disp_unlock(disp);

    return DG2_OK;
}

#ifdef DG2_ASYNC_ENABLE
static inline void disp_exec_async_ctor(
    dg2_disp_exec *exec,
    dg2_cb_solicited_resp cb_solicited_resp,
    dg2_user_data cb_solicited_resp_user_data,
    dg2_cb_trampoline cb_trampoline
) {
    exec->mode = DG2_DISP_EXEC_MODE_ASYNC;

    exec->params.async.cb_solicited_resp            = cb_solicited_resp;
    exec->params.async.cb_solicited_resp_user_data  = cb_solicited_resp_user_data;

    exec->params.async.cb_trampoline = cb_trampoline;
}
#endif

#ifdef DG2_SYNC_ENABLE
static inline void disp_exec_sync_ctor(dg2_disp_exec *exec, void *out)
{
    exec->mode = DG2_DISP_EXEC_MODE_SYNC;
    exec->params.sync.out = out;
}
#endif

static inline dg2_pkt *disp_req_init(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    disp->req.pkt = (dg2_pkt) {
        .buff = disp->tx_buff,
        .size = 0
    };

    return &disp->req.pkt;
}

static size_t disp_req_get_time_elapsed(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    return (disp->cb_time(disp->user_data) - disp->req.start_time);
}

static size_t disp_req_get_time_left(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    size_t time_elapsed = disp_req_get_time_elapsed(disp);

    return (disp->req.timeout > time_elapsed) ? (disp->req.timeout - time_elapsed) : 0;
}

static dg2_error disp_transmit_pkt(dg2_disp *disp, dg2_pkt *pkt)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(disp->cb_transmit);
    DG2_ASSERT(pkt);

    dg2_pkt_finish(pkt, disp->cb_crc);

    if (disp->cb_transmit(pkt->buff, pkt->size, disp->user_data) != pkt->size) {
        DG2_LOG("Failed transmitting packet.");

        return DG2_ERROR_WRITE;
    }

    DG2_LOG("Transmitted packet.");

    return DG2_OK;
}

#ifdef DG2_SYNC_ENABLE
static dg2_error disp_sync(dg2_disp *disp, volatile dg2_disp_req_sync_status *req_sync_status)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(disp->cb_time);
    DG2_ASSERT(disp->cb_yield);

    while (*req_sync_status == DG2_DISP_REQ_SYNC_STATUS_BUSY) {
        size_t time_left = disp_req_get_time_left(disp);

        disp->cb_yield(time_left, disp->user_data);
    }

    return (*req_sync_status == DG2_DISP_REQ_SYNC_STATUS_DONE) ? DG2_OK : DG2_ERROR_TIMEOUT;
}
#endif // DG2_SYNC_ENABLE

static bool disp_is_resp_solicited(dg2_disp *disp, dg2_pkt_parse_res *pkt_parse_res)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(pkt_parse_res);

    dg2_disp_req *req = &disp->req;

    if (disp->busy == false) {
        return false;
    }

    if ((req->cmd != pkt_parse_res->cmd) ) {
        return false; // Note: Response is not related to prior request
    }

    if (req->cmd == DG2_CMD_READ) {
        if (req->vp != pkt_parse_res->vp) {
            return false; // Note: Response is not related to prior request
        }

        if (req->resp_payload_size != pkt_parse_res->payload_size) {
            // TODO: Response could come from an automatic update - what to do here?
            return false;
        }
    }
    else if (req->cmd == DG2_CMD_WRITE) {
        if (pkt_parse_res->vp != DG2_PKT_WRITE_RESPONSE) {
            return false;
        }
    }

    return true;
}

static dg2_error disp_enqueue_req(
    dg2_disp *disp,

    dg2_pkt *pkt,
    dg2_cb_resp_parser cb_resp_parser,

    dg2_disp_exec *exec
) {
    DG2_ASSERT(disp);
    DG2_ASSERT(pkt);
    DG2_ASSERT(exec);

    disp->req.cmd = pkt->buff[DG2_PKT_INDEX_CMD]; // TODO: Move to seperate function.
    disp->req.vp = (pkt->buff[DG2_PKT_INDEX_VPH] << 8 ) | pkt->buff[DG2_PKT_INDEX_VPL]; // TODO: Move to seperate function.

    disp->req.resp_parser = cb_resp_parser;
    if (disp->req.cmd == DG2_CMD_READ) {
        disp->req.resp_payload_size = pkt->buff[DG2_PKT_INDEX_PAYLOAD_SIZE]; // TODO: Move to seperate function.
    }

    disp->exec = *exec;

#ifdef DG2_SYNC_ENABLE
    volatile dg2_disp_req_sync_status req_sync_status = DG2_DISP_REQ_SYNC_STATUS_BUSY;
    if (disp->exec.mode == DG2_DISP_EXEC_MODE_SYNC) {
        disp->exec.params.sync.req_sync_status = &req_sync_status;
    }
#endif

    disp->req.action = DG2_DISP_REQ_ACTION_TRANSMIT;

    disp->req.start_time = disp->cb_time(disp->user_data); // TODO: Move to .proces(), call on packet transmission.

#ifdef DG2_SYNC_ENABLE
    if (disp->exec.mode == DG2_DISP_EXEC_MODE_SYNC) {
        DG2_LOG("Syncing");

        return disp_sync(disp, &req_sync_status);
    }
#endif

    return DG2_OK;
}

#ifdef DG2_ASYNC_ENABLE
static inline void disp_process_timeout_async(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    if (disp->exec.params.async.cb_solicited_resp == NULL) {
        return;
    }

    disp->exec.params.async.cb_trampoline(
        DG2_ERROR_TIMEOUT,
        disp->exec.params.async.cb_solicited_resp,
        disp->exec.params.async.cb_solicited_resp_user_data,
        0x0000,
        DG2_CMD_INVALID,
        NULL,
        0,
        NULL
    );
}
#endif // DG2_ASYNC_ENABLE

#ifdef DG2_SYNC_ENABLE
static inline void disp_process_timeout_sync(dg2_disp *disp)
{
    *disp->exec.params.sync.req_sync_status = DG2_DISP_REQ_SYNC_STATUS_TIMEOUT;
}
#endif // DG2_SYNC_ENABLE

static inline void disp_process_timeout(dg2_disp *disp)
{
    if (disp->busy == false || disp_req_get_time_left(disp) > 0) {
        return;
    }

    DG2_LOG("Reponse did not arrive on time.");

#if defined(DG2_ASYNC_ENABLE) && defined(DG2_SYNC_ENABLE)
    if (disp->exec.mode == DG2_DISP_EXEC_MODE_ASYNC) {
        disp_process_timeout_async(disp);
    }
    else {
        disp_process_timeout_sync(disp);
    }
#elif defined (DG2_ASYNC_ENABLE)
    disp_process_timeout_async(disp);
#else
    disp_process_timeout_sync(disp);
#endif

    DG2_COMPILER_BARRIER();
    disp->busy = false;
}

#ifdef DG2_ASYNC_ENABLE
static inline void disp_process_solicited_async_resp(dg2_disp *disp, dg2_pkt_parse_res *res, uint8_t *parse_buff)
{
    // Note: Callback is optional.
    if (disp->exec.params.async.cb_solicited_resp == NULL) {
        return;
    }

    if (disp->req.resp_parser) {
        disp->req.resp_parser(parse_buff, res->payload, res->payload_size);
    }

    disp->exec.params.async.cb_trampoline(
        DG2_OK,
        disp->exec.params.async.cb_solicited_resp,
        disp->exec.params.async.cb_solicited_resp_user_data,
        disp->req.vp,
        disp->req.cmd,
        res->payload,
        res->payload_size,
        parse_buff
    );
}
#endif // DG2_ASYNC_ENABLE

#ifdef DG2_SYNC_ENABLE
static inline void disp_process_solicited_sync_resp(dg2_disp *disp, dg2_pkt_parse_res *res)
{
    if (disp->exec.params.sync.out) {
        disp->req.resp_parser(disp->exec.params.sync.out, res->payload, res->payload_size);
    }

    /* Note:
     * The volatile type qualifier prevents the compiler from reordering volatile operations ONLY among themselves, thus compiler barrier is needed.
     * req_status must be set as last to prevent the synchronous API call from returning before response is parsed.
     */
    DG2_COMPILER_BARRIER();
    *disp->exec.params.sync.req_sync_status = DG2_DISP_REQ_SYNC_STATUS_DONE;
}
#endif // DG2_SYNC_ENABLE

static inline void disp_process_solicited_resp(dg2_disp *disp, dg2_pkt_parse_res *res, uint8_t *parse_buff)
{
#if defined(DG2_ASYNC_ENABLE) && defined(DG2_SYNC_ENABLE)
    if (disp->exec.mode == DG2_DISP_EXEC_MODE_ASYNC) {
        disp_process_solicited_async_resp(disp, res, parse_buff);
    }
    else {
        disp_process_solicited_sync_resp(disp, res);
    }
#elif defined (DG2_ASYNC_ENABLE)
    disp_process_solicited_async_resp(disp, res, parse_buff);
#else
    disp_process_solicited_sync_resp(disp, res);
#endif
}

static inline void disp_process_unsolicited_resp(dg2_disp *disp, dg2_pkt_parse_res *res)
{
    if (disp->cb_unsolicited_resp == NULL) {
        return;
    }

    disp->cb_unsolicited_resp(
        res->cmd,
        res->vp,
        res->payload,
        res->payload_size,
        disp->user_data
    );
}

static inline void disp_process_resp(dg2_disp *disp)
{
    /* Todo: Implement parser state to avoid unnecessary copies (vectored io)?
     * One caveat: The user packet callback still probaly should receive contigious
     * block of memory
     */
    uint8_t parse_buff[DG2_PKT_MAX_SIZE];
    size_t parse_buff_size = dg2_circ_buff_copy(&disp->rx_circ_buff, parse_buff, DG2_ARRAY_SIZE(parse_buff));

    dg2_pkt_parse_res res = dg2_pkt_parse(parse_buff, parse_buff_size, disp->cb_crc);

    dg2_circ_buff_discard_back(&disp->rx_circ_buff, res.bytes_consumed);

    if (res.err != DG2_PKT_PARSE_OK) {
        return;
    }

    DG2_LOG("Reponse arrived on time.");

    if (disp_is_resp_solicited(disp, &res)) {
        disp_process_solicited_resp(disp, &res, parse_buff);
    }
    else {
        disp_process_unsolicited_resp(disp, &res);
    }

    /* Note:
     * Synchronization of producer (API calls)-consumer (.process() method) relies on the fact that
     * write access to disp->busy is atomic (no intermediate state of busy flag), removing the need for locking here.
     * disp->busy must be set as last to prevent other API calls from invalidating state required to handle the response.
     */
    DG2_COMPILER_BARRIER();
    disp->busy = false;
}

void dg2_disp_init(dg2_disp *disp, const dg2_disp_config *config)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(config);

    /* Callbacks */

    disp->cb_transmit   = config->cb_transmit;
    disp->cb_crc        = config->cb_crc;
    disp->cb_time       = config->cb_time;
#ifdef DG2_SYNC_ENABLE
    disp->cb_yield      = config->cb_yield;
#endif // DG2_SYNC_ENABLE
    disp->cb_unsolicited_resp     = config->cb_unsolicited_resp;
    disp->cb_lock       = config->cb_lock;
    disp->cb_unlock     = config->cb_unlock;
    disp->user_data     = config->user_data;

    dg2_circ_buff_init(&disp->rx_circ_buff, disp->rx_buff, DG2_DISP_RX_BUFF_CAPACITY);

    dg2_disp_req *req = &disp->req;

    req->action        = DG2_DISP_REQ_ACTION_NONE;
    req->start_time    = 0;
    req->timeout       = config->timeout;
    req->cmd           = DG2_CMD_INVALID;
    req->vp            = 0x0000;

    req->resp_parser         = NULL;
    req->resp_payload_size   = 0;

    disp->busy = false;
}

dg2_error dg2_disp_process(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    if (disp->req.action == DG2_DISP_REQ_ACTION_TRANSMIT) {
        disp->req.action = DG2_DISP_REQ_ACTION_NONE;

        dg2_error error = disp_transmit_pkt(disp, &disp->req.pkt);
        if (error != DG2_OK) {
            disp->busy = false;

            return error;
        }
    }
    else if (disp->req.action == DG2_DISP_REQ_ACTION_CANCEL) {
        disp->req.action = DG2_DISP_REQ_ACTION_NONE;

        disp->busy = false;

        return DG2_OK;
    }

    disp_process_timeout(disp);

    disp_process_resp(disp);

    return DG2_OK;
}

void dg2_disp_cancel_req(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    // Note: .process() may be processing the request & response right now - don't invalidate any state.

    disp->req.action = DG2_DISP_REQ_ACTION_CANCEL;
}


static void read_vps_parser(void *dest, void *payload, uint8_t payload_size)
{
    dg2_copy_and_swap_halfwords(dest, payload, payload_size);
}

static dg2_error disp_read_vps_generic(dg2_disp *disp, uint16_t vp, uint8_t count, dg2_cb_resp_parser resp_parser, dg2_disp_exec *exec)
{
    DG2_ASSERT(disp);

    DG2_RETURN_ERROR(disp_set_busy(disp));

    dg2_pkt *pkt = disp_req_init(disp);
    DG2_RETURN_ERROR(dg2_pkt_build_read_vps(pkt, vp, count));

    return disp_enqueue_req(
        disp,
        pkt,
        resp_parser,
        exec
    );
}

static dg2_error disp_write_vps(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count, dg2_disp_exec *exec)
{
    DG2_ASSERT(disp);

    DG2_RETURN_ERROR(disp_set_busy(disp));

    dg2_pkt *pkt = disp_req_init(disp);
    DG2_RETURN_ERROR(dg2_pkt_build_write_vps(pkt, vp, src, count));

    return disp_enqueue_req(
        disp,
        pkt,
        NULL,
        exec
    );
}

#ifdef DG2_ASYNC_ENABLE

static void read_vps_trampoline(
    dg2_error error,
    dg2_cb_solicited_resp cb_solicited_resp,
    dg2_user_data cb_solicited_resp_user_data,
    uint16_t vp,
    dg2_cmd cmd,
    void *payload,
    size_t payload_size,
    void *parsed_resp
) {
    DG2_ASSERT(cb_solicited_resp);

    ((dg2_disp_cb_read_vps_async)cb_solicited_resp)(
        error,
        (int16_t *)parsed_resp,
        (uint8_t)payload_size,
        cb_solicited_resp_user_data
    );
}

dg2_error dg2_disp_read_vps_async(dg2_disp *disp, uint16_t vp, uint8_t count, dg2_disp_cb_read_vps_async cb, dg2_user_data user_data)
{
    DG2_ASSERT(disp);

    dg2_disp_exec exec;
    disp_exec_async_ctor(&exec, cb, user_data, read_vps_trampoline);

    return disp_read_vps_generic(disp, vp, count, read_vps_parser, &exec);
}

static void write_vps_trampoline(dg2_error error, dg2_cb_solicited_resp cb_solicited_resp, dg2_user_data cb_solicited_resp_user_data, uint16_t vp, dg2_cmd cmd, void *payload, size_t payload_size, void *parsed_resp)
{
    DG2_ASSERT(cb_solicited_resp);

    ((dg2_disp_cb_write_vps_async)cb_solicited_resp)(error, vp, cb_solicited_resp_user_data);
}

dg2_error dg2_disp_write_vps_async(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count, dg2_disp_cb_write_vps_async cb, dg2_user_data user_data)
{
    DG2_ASSERT(disp);

    dg2_disp_exec exec;
    disp_exec_async_ctor(&exec, cb, user_data, write_vps_trampoline);

    return disp_write_vps(disp, vp, src, count, &exec);
}

#endif // DG2_ASYNC_ENABLE

#ifdef DG2_SYNC_ENABLE

dg2_error dg2_disp_read_vps(dg2_disp *disp, uint16_t vp, uint8_t count, int16_t *out)
{
    DG2_ASSERT(disp);

    dg2_disp_exec exec;
    disp_exec_sync_ctor(&exec, out);

    return disp_read_vps_generic(
        disp,
        vp,
        count,
        read_vps_parser,
        &exec
    );
}

dg2_error dg2_disp_write_vps(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count)
{
    DG2_ASSERT(disp);

    dg2_disp_exec exec;
    disp_exec_sync_ctor(&exec, NULL);

    return disp_write_vps(disp, vp, src, count, &exec);
}

#endif // DG2_SYNC_ENABLE

void dg2_copy_and_swap_words(uint8_t *restrict dest, const uint8_t *restrict src, size_t count)
{
    size_t bytes_to_copy = count << 2; // Note: Multiply by 4

    for (size_t i = 0; i < bytes_to_copy; i += 4) {
        dest[i] = src[i + 3];
        dest[i + 1] = src[i + 2];
        dest[i + 2] = src[i + 1];
        dest[i + 3] = src[i];
    }
}

static void read_floats_parser(void *dest, void *payload, uint8_t payload_size)
{
    dg2_copy_and_swap_words(dest, payload, payload_size >> 1);
}

static inline dg2_error disp_read_floats(dg2_disp *disp, uint16_t vp, uint8_t count, dg2_disp_exec *exec)
{
    DG2_ASSERT(disp);

    return disp_read_vps_generic(
        disp,
        vp,
        count << 1,
        read_floats_parser,
        exec
    );
}

#ifdef DG2_ASYNC_ENABLE
static void read_floats_trampoline(
    dg2_error error,
    dg2_cb_solicited_resp cb_solicited_resp,
    dg2_user_data cb_solicited_resp_user_data,
    uint16_t vp,
    dg2_cmd cmd,
    void *payload,
    size_t payload_size,
    void *parsed_resp
) {
    DG2_ASSERT(cb_solicited_resp);

    ((dg2_disp_cb_read_floats_async)cb_solicited_resp)(
        error,
        (float *)parsed_resp,
        (uint8_t)payload_size >> 1,
        cb_solicited_resp_user_data
    );
}

dg2_error dg2_disp_read_floats_async(dg2_disp *disp, uint16_t vp, uint8_t count, dg2_disp_cb_read_floats_async cb, dg2_user_data user_data)
{
    DG2_ASSERT(disp);

    dg2_disp_exec exec;
    disp_exec_async_ctor(&exec, cb, user_data, read_floats_trampoline);

    return disp_read_floats(disp, vp, count, &exec);
}
#endif // DG2_ASYNC_ENABLE

#ifdef DG2_SYNC_ENABLE

dg2_error dg2_disp_read_floats(dg2_disp *disp, uint16_t vp, uint8_t count, float *out)
{
    DG2_ASSERT(disp);

    dg2_disp_exec exec;
    disp_exec_sync_ctor(&exec, out);

    return disp_read_floats(disp, vp, count, &exec);
}
#endif // DG2_SYNC_ENABLE
