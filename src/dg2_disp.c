#include "dg2_disp.h"
#include "dg2_pkt.h"
#include <string.h>

typedef struct
{
    dg2_disp_sync_read_parser parser;
    void *dest;
} disp_sync_opts;

static void disp_lock(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    if (disp->cb_lock) {
        disp->cb_lock(disp->user_data);
    }
}

static void disp_unlock(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    if (disp->cb_unlock) {
        disp->cb_unlock(disp->user_data);
    }
}

static dg2_pkt disp_pkt_init(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    dg2_pkt pkt = {
        .buff = disp->tx_buff,
        .size = 0
    };

    return pkt;
}

static dg2_error disp_transmit_pkt(dg2_disp *disp, dg2_pkt *pkt)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(disp->cb_transmit);
    DG2_ASSERT(pkt);

    dg2_pkt_finish(pkt, disp->cb_crc);

    if (disp->cb_transmit(disp->user_data, pkt->buff, pkt->size) != pkt->size) {
        return DG2_ERROR_WRITE;
    }

    return DG2_OK;
}

static void disp_process_sync(dg2_disp *disp, dg2_pkt_parse_res *pkt_parse_res)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(pkt_parse_res);

    dg2_disp_sync *sync = &disp->sync;

    if (sync->status != DG2_DISP_SYNC_STATUS_SYNCING) {
        return;
    }

    if ((sync->cmd != pkt_parse_res->cmd) ) {
        return; // Note: Response is not related to a sync request
    }

    if (sync->cmd == DG2_CMD_READ) {
        if (sync->vp != pkt_parse_res->vp) {
            return; // Note: Response is not related to a sync request
        }

        if (sync->read_payload_size != pkt_parse_res->payload_size) {
            // Todo: Response could come from an automatic update - what to do here?
            return;
        }

        if ((sync->read_dest != NULL)) {
            sync->read_parser(sync->read_dest, pkt_parse_res->payload, sync->read_payload_size);
        }
    }
    else if (sync->cmd == DG2_CMD_WRITE) {
        if (pkt_parse_res->vp != DG2_PKT_WRITE_RESPONSE) {
            return;
        }
    }

    /* Unlock sync state */
    sync->status = DG2_DISP_SYNC_STATUS_SYNCED;
}

static dg2_error disp_sync(dg2_disp *disp)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(disp->cb_time);
    DG2_ASSERT(disp->cb_yield);

    disp->sync.start_time = disp->cb_time(disp->user_data);

    while (disp->sync.status == DG2_DISP_SYNC_STATUS_SYNCING) {
        disp_lock(disp);

        size_t time_elapsed = (disp->cb_time(disp->user_data) - disp->sync.start_time);

        if (time_elapsed >= disp->sync.timeout) {
            /* Unlock sync state */
            disp->sync.status = DG2_DISP_SYNC_STATUS_TIMEOUT;

            disp_unlock(disp);

            break;
        }

        disp_unlock(disp);

        if (disp->cb_yield) {
            // Note: Time spent on potential preemption is not taken into account
            size_t timeout_in = (disp->sync.timeout - time_elapsed);

            disp->cb_yield(disp->user_data, timeout_in);
        }
        else {
            dg2_disp_process(disp);
        }
    }

    return (dg2_error)disp->sync.status; // Note: Sync status is trivially convertible to error
}

static dg2_error disp_send_command(dg2_disp *disp, dg2_pkt *pkt, bool sync, disp_sync_opts *sync_opts) {
    DG2_ASSERT(disp);
    DG2_ASSERT(pkt);

    disp_lock(disp);

    if (sync) {
        if (disp->sync.status == DG2_DISP_SYNC_STATUS_SYNCING) {
            disp_unlock(disp);

            return DG2_ERROR_BUSY;
        }

        /* Lock sync state */
        disp->sync.status = DG2_DISP_SYNC_STATUS_SYNCING;

        disp->sync.cmd = pkt->buff[DG2_PKT_INDEX_CMD];
        disp->sync.vp = (pkt->buff[DG2_PKT_INDEX_VPH] << 8 ) | pkt->buff[DG2_PKT_INDEX_VPL];

        if(disp->sync.cmd == DG2_CMD_READ) {
            if (sync_opts) {
                disp->sync.read_dest = sync_opts->dest;
                disp->sync.read_parser = sync_opts->parser;
            }

            disp->sync.read_payload_size = pkt->buff[DG2_PKT_INDEX_PAYLOAD_SIZE];
        }
    }

    dg2_error error = disp_transmit_pkt(disp, pkt);

    if (error != DG2_OK) {
        if (sync) {
            /* Unlock sync state */
            disp->sync.status = DG2_DISP_SYNC_STATUS_SYNCED;
        }

        disp_unlock(disp);

        return error;
    }

    disp_unlock(disp);

    if (sync) {
        return disp_sync(disp);
    }

    return DG2_OK;
}

void dg2_disp_init(dg2_disp *disp, const dg2_disp_config *config)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(config);

    /* Callbacks */

    disp->cb_transmit   = config->cb_transmit;
    disp->cb_crc        = config->cb_crc;
    disp->cb_time       = config->cb_time;
    disp->cb_yield      = config->cb_yield;
    disp->cb_packet     = config->cb_packet;
    disp->cb_lock       = config->cb_lock;
    disp->cb_unlock     = config->cb_unlock;
    disp->user_data     = config->user_data;

    /* Buffers */

    dg2_circ_buff_init(&disp->rx_circ_buff, disp->rx_buff, DG2_DISP_RX_BUFF_CAPACITY);

    /* Sync */

    dg2_disp_sync *sync = &disp->sync;

    sync->status        = DG2_DISP_SYNC_STATUS_SYNCED;
    sync->start_time    = 0;
    sync->timeout       = config->timeout;
    sync->cmd           = 0;
    sync->vp            = 0;

    sync->read_dest           = NULL;
    sync->read_parser         = NULL;
    sync->read_payload_size   = 0;
}

dg2_error dg2_disp_process(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    disp_lock(disp);

    while (1) {
        /* Todo: Implement parser state to avoid unnecessary copies (vectored io)?
         * One caveat: The user packet callback still probaly should receive contigious
         * block of memory
         */
        uint8_t tmp_buff[DG2_PKT_MAX_SIZE /* 255 bytes */] = { };
        size_t tmp_buff_size = dg2_circ_buff_copy(&disp->rx_circ_buff, tmp_buff, DG2_ARRAY_SIZE(tmp_buff));

        dg2_pkt_parse_res res = dg2_pkt_parse(tmp_buff, tmp_buff_size, disp->cb_crc);

        dg2_circ_buff_discard_back(&disp->rx_circ_buff, res.bytes_consumed);

        if (res.err == DG2_PKT_PARSE_OK) {
            disp_process_sync(disp, &res);

            if (disp->cb_packet) {
                disp->cb_packet(disp->user_data, res.cmd, res.vp, res.payload, res.payload_size);
            }
        }
        else {
            break;
        }
    }

    disp_unlock(disp);

    return DG2_OK;
}

/* Read */

static dg2_error disp_read_vps(dg2_disp *disp, uint16_t vp, uint8_t count, bool sync, disp_sync_opts *sync_opts)
{
    DG2_ASSERT(disp);

    dg2_pkt pkt = disp_pkt_init(disp);
    dg2_pkt_build_read_vps(&pkt, vp, count);

    return disp_send_command(disp, &pkt, sync, sync_opts);
}

dg2_error dg2_disp_read_vps_async(dg2_disp *disp, uint16_t vp, uint8_t count)
{
    return disp_read_vps(disp, vp, count, false, NULL);
}

dg2_error dg2_disp_read_vp_async(dg2_disp *disp, uint16_t vp)
{
    return dg2_disp_read_vps_async(disp, vp, 1);
}

static void read_vps_parser(void *dest, void *payload, size_t payload_size)
{
    dg2_copy_and_swap_halfwords(dest, payload, payload_size);
}

dg2_error dg2_disp_read_vp(dg2_disp *disp, uint16_t vp, int16_t *dest)
{
    return dg2_disp_read_vps(disp, vp, dest, 1);
}

dg2_error dg2_disp_read_vps(dg2_disp *disp, uint16_t vp, int16_t *dest, uint8_t count)
{
    disp_sync_opts sync_opts = {
        .dest = dest,
        .parser = read_vps_parser
    };

    return disp_read_vps(disp, vp, count, true, &sync_opts);
}

/* Write */

static dg2_error disp_write_vps(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count, bool sync)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(src);

    dg2_pkt pkt = disp_pkt_init(disp);
    dg2_pkt_build_write_vps(&pkt, vp, src, count);

    return disp_send_command(disp, &pkt, sync, NULL);
}

dg2_error dg2_disp_write_vps_async(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count)
{
    return disp_write_vps(disp, vp, src, count, false);
}

dg2_error dg2_disp_write_vp_async(dg2_disp *disp, uint16_t vp, int16_t data)
{
    return dg2_disp_write_vps_async(disp, vp, &data, 1);
}

dg2_error dg2_disp_write_vp(dg2_disp *disp, uint16_t vp, int16_t data)
{
    return dg2_disp_write_vps(disp, vp, &data, 1);
}

dg2_error dg2_disp_write_vps(dg2_disp *disp, uint16_t vp, const int16_t *src, uint8_t count)
{
    return disp_write_vps(disp, vp, src, count, true);
}
