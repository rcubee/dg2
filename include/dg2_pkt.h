#ifndef DG2_PKT_H_
#define DG2_PKT_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "dg2.h"

#define DG2_PKT_FHH 0x5AU
#define DG2_PKT_FHL 0xA5U

#define DG2_PKT_MAX_SIZE 255U

typedef enum dg2_cmd {
    DG2_CMD_INVALID = 0x00U,
    DG2_CMD_WRITE = 0x82U,
    DG2_CMD_READ = 0x83U
} dg2_cmd;

typedef enum dg2_pkt_index {
    DG2_PKT_INDEX_FHH = 0,
    DG2_PKT_INDEX_FHL = 1,
    DG2_PKT_INDEX_BC = 2,
    DG2_PKT_INDEX_CMD = 3,
    DG2_PKT_INDEX_VPH = 4,
    DG2_PKT_INDEX_VPL = 5,
    DG2_PKT_INDEX_PAYLOAD_SIZE = 6,
    DG2_PKT_INDEX_PAYLOAD = 7
} dg2_pkt_index;

typedef enum dg2_pkt_parse_err {
    DG2_PKT_PARSE_OK = 0U,
    DG2_PKT_PARSE_ERR,
    DG2_PKT_PARSE_ERR_NOT_FOUND,
    DG2_PKT_PARSE_ERR_INCOMPLETE
} dg2_pkt_parse_err;

typedef struct dg2_pkt_parse_res {
    dg2_pkt_parse_err err;
    size_t bytes_consumed;
    dg2_cmd cmd;
    uint16_t vp;
    uint8_t *payload;
    uint8_t payload_size;
} dg2_pkt_parse_res;

typedef struct __attribute__((packed)) dg2_pkt_header {
    uint8_t fhh;
    uint8_t fhl;
    uint8_t bc;
    uint8_t cmd;
} dg2_pkt_header;

typedef struct dg2_pkt {
    union {
        uint8_t *buff;
        dg2_pkt_header *header;
    };
    size_t size;
} dg2_pkt;

void dg2_pkt_init(dg2_pkt *pkt, uint8_t *buff);

void dg2_pkt_build_header(dg2_pkt *pkt, dg2_cmd cmd, uint16_t vp);

void dg2_pkt_insert_byte(dg2_pkt *pkt, uint8_t byte);
void dg2_pkt_insert_bytes(dg2_pkt *pkt, uint8_t *bytes, size_t count);

void dg2_pkt_insert_halfword(dg2_pkt *pkt, uint16_t halfword);
void dg2_pkt_insert_halfwords(dg2_pkt *pkt, uint16_t *halfwords, size_t count);

void dg2_pkt_finish(dg2_pkt *pkt, dg2_cb_crc cb_crc);

dg2_pkt_parse_res dg2_pkt_parse(const uint8_t *buff, size_t buff_size, dg2_cb_crc cb_crc);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_PKT_H_ */
