#include "dg2_disp.h"
#include "dg2_pkt.h"
#include <string.h>

static dg2_error dg2_disp_pkt_receive(dg2_disp *disp, size_t size)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(size > sizeof(dg2_pkt_header));

    size_t to_receive = disp->cb_crc ? size + 2 : size;

    if (disp->cb_read(disp->pkt_buff, to_receive) != to_receive) {
        return DG2_ERROR_READ;
    }

    return dg2_pkt_verify(disp->pkt_buff, to_receive, disp->cb_crc);
}

static dg2_error dg2_disp_pkt_transmit(dg2_disp *disp, dg2_pkt *pkt)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(pkt);

    dg2_pkt_finish(pkt, disp->cb_crc);

    if (disp->cb_write(pkt->buff, pkt->size) != pkt->size) {
        return DG2_ERROR_WRITE;
    }

    return DG2_OK;
}

static dg2_error vp_read(dg2_disp *disp, uint16_t vp, uint8_t count, uint8_t **resp_payload)
{
    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_READ, vp);
    dg2_pkt_app_byte(&pkt, count);

    dg2_error error;
    if ((error = dg2_disp_pkt_exchange(disp, &pkt, DG2_PKT_PAYLOAD_OFFSET + 2 * count)) != DG2_OK) {
        return error;
    }

    dg2_cmd resp_cmd;
    uint16_t resp_vp;
    uint8_t resp_payload_size;
    if ((error = dg2_pkt_parse(disp->pkt_buff, &resp_cmd, &resp_vp, resp_payload, &resp_payload_size)) != DG2_OK) {
        return error;
    }

    if (resp_cmd != DG2_CMD_READ || resp_vp != vp || resp_payload_size != count) {
        return DG2_ERROR;
    }

    return DG2_OK;
}

dg2_pkt dg2_disp_pkt_init(dg2_disp *disp, dg2_cmd cmd, uint16_t vp)
{
    dg2_pkt pkt;
    pkt.buff = disp->pkt_buff;

    dg2_pkt_build_header(&pkt, cmd, vp);

    return pkt;
}

dg2_error dg2_disp_pkt_exchange(dg2_disp *disp, dg2_pkt *pkt, size_t response_size)
{
    dg2_error error;
    if ((error = dg2_disp_pkt_transmit(disp, pkt)) != DG2_OK) {
        return error;
    }

    if ((error = dg2_disp_pkt_receive(disp, response_size)) != DG2_OK) {
        return error;
    }

    return DG2_OK;
}

inline dg2_error dg2_disp_pkt_exchange_ok(dg2_disp *disp, dg2_pkt *pkt)
{
    return dg2_disp_pkt_exchange(disp, pkt, 6);
}

void dg2_disp_init(dg2_disp *disp,
                        dg2_cb_read cb_read,
                        dg2_cb_write cb_write,
                        dg2_cb_crc cb_crc)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(cb_read);
    DG2_ASSERT(cb_write);

    disp->cb_read = cb_read;
    disp->cb_write = cb_write;
    disp->cb_crc = cb_crc;
}

/*
 * Read
 */

dg2_error dg2_disp_vp_read_bytes(dg2_disp *disp, uint16_t vp, uint8_t *dest, uint8_t count)
{
    dg2_error error;
    uint8_t *resp_payload;
    if ((error = vp_read(disp, vp, (count / 2) + (count % 2), &resp_payload)) != DG2_OK) {
        return error;
    }

    dg2_read_bytes(dest, resp_payload, count);

    return DG2_OK;
}

dg2_error dg2_disp_vp_read(dg2_disp *disp, uint16_t vp, int16_t *value)
{
    return dg2_disp_vp_read_mul(disp, vp, value, 1);
}

dg2_error dg2_disp_vp_read_mul(dg2_disp *disp, uint16_t vp, int16_t *values, uint8_t count)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(count > 0 ? values != NULL : 1);

    if (!count) {
        return DG2_OK;
    }

    dg2_error error;
    uint8_t *resp_payload;
    if ((error = vp_read(disp, vp, count, &resp_payload)) != DG2_OK) {
        return error;
    }

    dg2_read_halfwords((uint16_t*)values, resp_payload, count);

    return DG2_OK;
}

/*
 * Write
 */

dg2_error dg2_disp_vp_write(dg2_disp *disp, uint16_t vp, int16_t value)
{
    return dg2_disp_vp_write_mul(disp, vp, &value, 1);
}

dg2_error dg2_disp_vp_write_mul(dg2_disp *disp, uint16_t vp, int16_t *values, uint8_t count)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(count > 0 ? values != NULL : 1);

    if (!count) {
        return DG2_OK;
    }

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, vp);
    dg2_pkt_app_halfwords(&pkt, (uint16_t*)values, count);

    return dg2_disp_pkt_exchange_ok(disp, &pkt);
}
