#ifndef DG2_PKT_H_
#define DG2_PKT_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "dg2.h"

#define DG2_PKT_HEADER_H 0x5A
#define DG2_PKT_HEADER_L 0xA5

#define DG2_PKT_VP_OFFSET 4U
#define DG2_PKT_PAYLOAD_SIZE_OFFSET 6U
#define DG2_PKT_PAYLOAD_OFFSET 7U

typedef enum dg2_cmd {
    DG2_CMD_WRITE = 0x82,
    DG2_CMD_READ = 0x83
} dg2_cmd;

typedef struct  __attribute__((packed)) dg2_pkt_header {
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

void dg2_pkt_build_header(dg2_pkt *pkt, dg2_cmd cmd, uint16_t vp);

void dg2_pkt_app_byte(dg2_pkt *pkt, uint8_t byte);
void dg2_pkt_app_bytes(dg2_pkt *pkt, uint8_t *bytes, size_t count);

void dg2_pkt_app_halfword(dg2_pkt *pkt, uint16_t halfword);
void dg2_pkt_app_halfwords(dg2_pkt *pkt, uint16_t *halfwords, size_t count);

void dg2_pkt_app_word(dg2_pkt *pkt, uint32_t word);
void dg2_pkt_app_words(dg2_pkt *pkt, uint32_t *words, size_t count);

void dg2_pkt_finish(dg2_pkt *pkt, dg2_cb_crc cb_crc);

dg2_error dg2_pkt_verify(uint8_t *data, size_t size, dg2_cb_crc cb_crc);
dg2_error dg2_pkt_parse(uint8_t *data, dg2_cmd *cmd, uint16_t *vp, uint8_t **payload, uint8_t *payload_size);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_PKT_H_ */
