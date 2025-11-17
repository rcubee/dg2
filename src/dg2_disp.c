#include "dg2_disp.h"
#include "dg2_pkt.h"
#include <string.h>

static void dg2_disp_process_sync(dg2_disp *disp, dg2_pkt_parse_res *pkt_parse_res)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(pkt_parse_res);

    dg2_disp_sync *sync = &disp->sync;

    if (sync->status != DG2_DISP_SYNC_STATUS_BUSY) {
        return;
    }

    if ((sync->cmd != pkt_parse_res->cmd) ) {
        // Note: Response is not related to a sync request
        return;
    }

    if (sync->cmd == DG2_CMD_READ) {
        if (sync->vp != pkt_parse_res->vp) {
            // Note: Response is not related to a sync request
            return;
        }

        if ((sync->read_size /* In bytes */) != 2 * (pkt_parse_res->payload_size /* In halfwords */)) {
            // Note: Response could come from an automatic update - what to do here?
            return;
        }

        if ((sync->read_dest != NULL) && (sync->read_size > 0)) {
            switch (sync->read_type) {
            case DG2_DISP_SYNC_READ_TYPE_U8: {
                dg2_read_bytes(sync->read_dest, pkt_parse_res->payload, sync->read_size);
                break;
            }

            case DG2_DISP_SYNC_READ_TYPE_U16: {
                dg2_read_halfwords(sync->read_dest, pkt_parse_res->payload, (sync->read_size >> 1 /* Divide by 2 */));
                break;
            }
            }
        }
    }
    else if (sync->cmd == DG2_CMD_WRITE) {
        // Note: Responses to writes don't contain related vp address, "OK" is sent instead
        if (pkt_parse_res->vp != 0x4f4b ) {
            return;
        }
    }

    sync->status = DG2_DISP_SYNC_STATUS_OK;
}

static dg2_pkt dg2_disp_pkt_init(dg2_disp *disp, dg2_cmd cmd, uint16_t vp)
{
    dg2_pkt pkt;
    pkt.buff = disp->tx_buff;

    dg2_pkt_build_header(&pkt, cmd, vp);

    return pkt;
}

void dg2_disp_init(dg2_disp *disp,
                        dg2_cb_write cb_write,
                        dg2_cb_time cb_time,
                        dg2_cb_process cb_process,
                        dg2_cb_packet cb_packet,
                        dg2_cb_crc cb_crc,
                        dg2_cb_data cb_data)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(cb_time);
    DG2_ASSERT(cb_write);

    disp->cb_write = cb_write;
    disp->cb_time = cb_time;
    disp->cb_process = cb_process;
    disp->cb_packet = cb_packet;
    disp->cb_crc = cb_crc;
    disp->cb_data = cb_data;

    disp->sync.status = DG2_DISP_SYNC_STATUS_OK;
    disp->sync.start_time = 0;
    disp->sync.timeout = 100;
    disp->sync.cmd = 0;
    disp->sync.vp = 0;

    dg2_circ_buff_init(&disp->rx_circ_buff, disp->rx_buff, DG2_DISP_RX_BUFF_CAPACITY);
}

dg2_disp_sync_status dg2_disp_process(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    if (disp->cb_process) {
        disp->cb_process(disp->cb_data);
    }

    dg2_disp_sync *sync = &disp->sync;

    while (1) {
        // Todo: Implement parser state to avoid unnecessary copies (vectored io)
        uint8_t tmp_buff[DG2_PKT_MAX_SIZE] = { };
        size_t tmp_buff_size = dg2_circ_buff_copy(&disp->rx_circ_buff, tmp_buff, DG2_ARRAY_SIZE(tmp_buff));

        dg2_pkt_parse_res res = dg2_pkt_parse(tmp_buff, tmp_buff_size, disp->cb_crc);

        dg2_circ_buff_discard_back(&disp->rx_circ_buff, res.bytes_consumed);

        if (res.err == DG2_PKT_PARSE_OK) {
            dg2_disp_process_sync(disp, &res);

            if (disp->cb_packet) {
                disp->cb_packet(disp->cb_data, res.cmd, res.vp, res.payload, res.payload_size);
            }
        }
        else {
            break;
        }
    }

    if (sync->status == DG2_DISP_SYNC_STATUS_BUSY && (disp->cb_time(disp->cb_data) - sync->start_time) > sync->timeout) {
        sync->status = DG2_DISP_SYNC_STATUS_TIMEOUT;
    }

    return sync->status;
}

static dg2_error transmit(dg2_disp *disp, dg2_pkt *pkt)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(pkt);

    dg2_pkt_finish(pkt, disp->cb_crc);

    if (disp->cb_write(disp->cb_data, pkt->buff, pkt->size) != pkt->size) {
        return DG2_ERROR_WRITE;
    }

    return DG2_OK;
}

static dg2_error sync(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    dg2_disp_sync_status sync_status;
    while ((sync_status = dg2_disp_process(disp)) == DG2_DISP_SYNC_STATUS_BUSY)
        ; // Todo: Sleep

    return (dg2_error)sync_status; // Note: Sync status is trivially convertable to error
}

dg2_error dg2_disp_read_vps_async(dg2_disp *disp, uint16_t vp, int16_t *values, uint8_t count)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(values);
    DG2_ASSERT(count > 0);

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_READ, vp);
    dg2_pkt_insert_byte(&pkt, count);

    return transmit(disp, &pkt);
}

dg2_error dg2_disp_read_vp(dg2_disp *disp, uint16_t vp, int16_t *value)
{
    DG2_ASSERT(disp);

    return dg2_disp_read_vps(disp, vp, value, 1);
}

dg2_error dg2_disp_read_vps(dg2_disp *disp, uint16_t vp, int16_t *values, uint8_t count)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(values);
    DG2_ASSERT(count > 0);

    if (disp->sync.status == DG2_DISP_SYNC_STATUS_BUSY) {
        return DG2_ERROR_BUSY;
    }

    dg2_error error;
    if ((error = dg2_disp_read_vps_async(disp, vp, values, count)) != DG2_OK) {
        return error;
    }

    disp->sync.status = DG2_DISP_SYNC_STATUS_BUSY;
    disp->sync.start_time = disp->cb_time(disp->cb_data);
    disp->sync.cmd = DG2_CMD_READ;
    disp->sync.vp = vp;
    disp->sync.read_dest = values;
    disp->sync.read_size = 2 * count;
    disp->sync.read_type = DG2_DISP_SYNC_READ_TYPE_U16;

    return sync(disp);
}

dg2_error dg2_disp_write_vp_async(dg2_disp *disp, uint16_t vp, int16_t value)
{
    DG2_ASSERT(disp);

    return dg2_disp_write_vps_async(disp, vp, &value, 1);
}

dg2_error dg2_disp_write_vps_async(dg2_disp *disp, uint16_t vp, int16_t *values, uint8_t count)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(values);

    if (count < 1) {
        return DG2_OK;
    }

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, vp);
    dg2_pkt_insert_halfwords(&pkt, (uint16_t*)values, count);

    return transmit(disp, &pkt);
}
