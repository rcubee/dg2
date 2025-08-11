#include "dg2_pkt.h"

/*
 * DGUSII frame structure
 * | frame header | frame length | command | vp address | payload length | payload |
 * | 2            | 1            | 1       | 2          | 1              | ...     |
 */

void dg2_pkt_build_header(dg2_pkt *pkt, dg2_cmd cmd, uint16_t vp)
{
    DG2_ASSERT(pkt);
    DG2_ASSERT(pkt->header);

    pkt->header->fhh = DG2_PKT_HEADER_H;
    pkt->header->fhl = DG2_PKT_HEADER_L;
    pkt->header->bc = 0;
    pkt->header->cmd = cmd;

    pkt->size = sizeof(dg2_pkt_header);

    dg2_pkt_app_halfword(pkt, vp);
}

inline void dg2_pkt_app_byte(dg2_pkt *pkt, uint8_t byte)
{
    dg2_pkt_app_bytes(pkt, &byte, 1);
}

void dg2_pkt_app_bytes(dg2_pkt *pkt, uint8_t *bytes, size_t count)
{
    DG2_ASSERT(pkt);
    DG2_ASSERT(pkt->buff);
    DG2_ASSERT(pkt->size + count * sizeof(uint8_t) <= 253); // Note: Maximum packet size is 253 (excluding CRC)

    dg2_write_bytes(pkt->buff + pkt->size, bytes, count);

    pkt->size += count * sizeof(uint8_t);
}

inline void dg2_pkt_app_halfword(dg2_pkt *pkt, uint16_t halfword)
{
    dg2_pkt_app_halfwords(pkt, &halfword, 1);
}

void dg2_pkt_app_halfwords(dg2_pkt *pkt, uint16_t *halfwords, size_t count)
{
    DG2_ASSERT(pkt);
    DG2_ASSERT(pkt->buff);
    DG2_ASSERT(pkt->size + count * sizeof(uint16_t) <= 253); // Note: Maximum packet size is 253 (excluding CRC)

    dg2_write_halfwords(pkt->buff + pkt->size, halfwords, count);

    pkt->size += count * sizeof(uint16_t);
}

inline void dg2_pkt_app_word(dg2_pkt *pkt, uint32_t word)
{
    dg2_pkt_app_words(pkt, &word, 1);
}

void dg2_pkt_app_words(dg2_pkt *pkt, uint32_t *words, size_t count)
{
    DG2_ASSERT(pkt);
    DG2_ASSERT(pkt->buff);
    DG2_ASSERT(pkt->size + count * sizeof(uint32_t) <= 253); // Note: Maximum packet size is 253 (excluding CRC)

    dg2_write_words(pkt->buff + pkt->size, words, count);

    pkt->size += count * sizeof(uint32_t);
}

void dg2_pkt_finish(dg2_pkt *pkt, dg2_cb_crc cb_crc)
{
    DG2_ASSERT(pkt);

    if (cb_crc) {
        uint16_t crc = cb_crc((uint8_t*)pkt->header + 3, pkt->size - 3);
        crc = DG2_LE_TO_BE_HALFWORD(crc);

        dg2_pkt_app_halfword(pkt, crc);
    }

    pkt->header->bc = pkt->size - 3; // Note: First 3 bytes of the header are not counted
}

dg2_error dg2_pkt_verify(uint8_t *data, size_t size, dg2_cb_crc cb_crc)
{
    DG2_ASSERT(data);

    if (size < sizeof(dg2_pkt_header)) {
        return DG2_ERROR_PKT_INCOMPLETE;
    }

    dg2_pkt_header *header = (dg2_pkt_header*)data;

    if ((header->fhh != DG2_PKT_HEADER_H) || (header->fhl != DG2_PKT_HEADER_L)) {
        return DG2_ERROR_PKT_INVALID_HEADER;
    }

    if (header->bc < size - 3) {
        return DG2_ERROR_PKT_INCOMPLETE;
    }

    if (header->bc > size - 3) {
        return DG2_ERROR_PKT_TOO_LONG;
    }

    if (cb_crc) {
        size_t offset = size - 2;
        uint16_t crc_received = data[offset] | (data[offset + 1] << 8);

        uint16_t crc_expected = cb_crc(data + 3, size - 5);

        if (crc_received != crc_expected) {
            return DG2_ERROR_CRC_MISMATCH;
        }
    }

    return DG2_OK;
}

dg2_error dg2_pkt_parse(uint8_t *data, dg2_cmd *cmd, uint16_t *vp, uint8_t **payload, uint8_t *payload_size)
{
    DG2_ASSERT(data);

    if (cmd) {
        *cmd = ((dg2_pkt_header*)data)->cmd;
    }

    if (vp) {
        dg2_read_halfwords(vp, data + DG2_PKT_VP_OFFSET, 1);
    }

    if (payload) {
        *payload = data + DG2_PKT_PAYLOAD_OFFSET;
    }

    if (payload_size) {
        *payload_size = data[DG2_PKT_PAYLOAD_SIZE_OFFSET];
    }

    return DG2_OK;
}

