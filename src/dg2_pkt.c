#include "dg2_pkt.h"

/*
 * DGUSII frame structure
 * section: | frame header | frame length | command | data | crc |
 * length:  | 2            | 1            | 1       | 249  | 2   |
 *
 * https://ecdn6.globalso.com/upload/p/1355/source/2025-02/T5L_DGUSII-Application-Development-Guide-V2.9-0207.pdf
 */

void dg2_pkt_init(dg2_pkt *pkt, uint8_t *buff)
{
    pkt->buff = buff;
    pkt->size = 0;
}

void dg2_pkt_build_header(dg2_pkt *pkt, dg2_cmd cmd, uint16_t vp)
{
    DG2_ASSERT(pkt);
    DG2_ASSERT(pkt->header);

    pkt->header->fhh = DG2_PKT_FHH;
    pkt->header->fhl = DG2_PKT_FHL;
    pkt->header->bc = 0;
    pkt->header->cmd = cmd;

    pkt->size = sizeof(dg2_pkt_header);

    dg2_pkt_insert_halfword(pkt, vp);
}

void dg2_pkt_insert_byte(dg2_pkt *pkt, uint8_t byte)
{
    dg2_pkt_insert_bytes(pkt, &byte, 1);
}

void dg2_pkt_insert_bytes(dg2_pkt *pkt, uint8_t *bytes, size_t count)
{
    DG2_ASSERT(pkt);
    DG2_ASSERT(pkt->buff);
    DG2_ASSERT(pkt->size + count * sizeof(uint8_t) <= 253); // Note: Maximum packet size is 253 (excluding CRC)

    dg2_write_bytes(pkt->buff + pkt->size, bytes, count);

    pkt->size += count * sizeof(uint8_t);
}

void dg2_pkt_insert_halfword(dg2_pkt *pkt, uint16_t halfword)
{
    dg2_pkt_insert_halfwords(pkt, &halfword, 1);
}

void dg2_pkt_insert_halfwords(dg2_pkt *pkt, uint16_t *halfwords, size_t count)
{
    DG2_ASSERT(pkt);
    DG2_ASSERT(pkt->buff);
    DG2_ASSERT(pkt->size + count * sizeof(uint16_t) <= 253); // Note: Maximum packet size is 253 (excluding CRC)

    dg2_write_halfwords(pkt->buff + pkt->size, halfwords, count);

    pkt->size += count * sizeof(uint16_t);
}

void dg2_pkt_insert_word(dg2_pkt *pkt, uint32_t word)
{
    dg2_pkt_insert_words(pkt, &word, 1);
}

void dg2_pkt_insert_words(dg2_pkt *pkt, uint32_t *words, size_t count)
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
        crc = DG2_SWAP16(crc);

        dg2_pkt_insert_halfword(pkt, crc);
    }

    pkt->header->bc = pkt->size - 3; // Note: First 3 bytes of the header are not counted
}

dg2_pkt_parse_res dg2_pkt_parse(const uint8_t *buff, size_t buff_size, dg2_cb_crc cb_crc)
{
    DG2_ASSERT(buff != NULL);

    dg2_pkt_parse_res res = {
        .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
        .bytes_consumed = buff_size,
        .cmd = DG2_CMD_INVALID,
        .vp = 0x0000,
        .payload = NULL,
        .payload_size = 0
    };

    size_t offset = 0;
    while (offset < buff_size) {
        const uint8_t *ptr = buff + offset;

        if (ptr[DG2_PKT_INDEX_FHH] != DG2_PKT_FHH) {
            ++offset;
            continue;
        }

        if (offset + 1 >= buff_size) {
            res.err = DG2_PKT_PARSE_ERR_INCOMPLETE;
            res.bytes_consumed = offset;
            return res;
        }

        if (ptr[DG2_PKT_INDEX_FHL] != DG2_PKT_FHL /* False positive */) {
            ++offset;
            continue;
        }

        size_t bytes_left = buff_size - offset; // Note: Bytes left for packet
        if (bytes_left < sizeof(dg2_pkt_header)) {
            res.err = DG2_PKT_PARSE_ERR_INCOMPLETE;
            res.bytes_consumed = offset;
            return res;
        }

        dg2_pkt_header *pkt_header = (dg2_pkt_header*)(ptr);
        size_t pkt_size = 3 + pkt_header->bc;

        if (bytes_left < pkt_size) {
            res.bytes_consumed = offset;
            res.err = DG2_PKT_PARSE_ERR_INCOMPLETE;
            return res;
        }

        if (cb_crc) {
            uint8_t *crc = (uint8_t*)(ptr + pkt_size - 2);
            uint16_t crc_received = crc[0] | (crc[1] << 8);

            uint16_t crc_expected = cb_crc((uint8_t*)pkt_header + 3 /* Skip first 3 bytes */, pkt_size - 5 /* Don't count first 3 and last 2 bytes */);

            if (crc_received != crc_expected /* False positive */) {
                offset += 2;
                continue;
            }
        }

        res.err = DG2_PKT_PARSE_OK;
        res.bytes_consumed = offset + pkt_size;
        res.cmd = pkt_header->cmd;
        res.vp = ((ptr[DG2_PKT_INDEX_VPH] << 8) & 0xFF00) | (ptr[DG2_PKT_INDEX_VPL] & 0xFF);
        res.payload_size = ptr[DG2_PKT_INDEX_PAYLOAD_SIZE];
        res.payload = (uint8_t*)(ptr + DG2_PKT_INDEX_PAYLOAD);

        return res;
    }

    return res;
}
