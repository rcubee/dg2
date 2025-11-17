#include <array>
#include "dg2_pkt.h"
#include <gtest/gtest.h>

TEST(dg2_pkt, Initialization)
{
    std::array<std::uint8_t, 256> buff = { };

    dg2_pkt pkt;

    dg2_pkt_init(&pkt, buff.data());

    ASSERT_EQ(pkt.buff, buff.data());
    ASSERT_EQ(pkt.size, 0);
}

TEST(dg2_pkt, BuildHeader)
{
    std::array<std::uint8_t, DG2_PKT_MAX_SIZE> buff = { };

    dg2_pkt pkt;

    const auto mock_headers =  {
        std::make_pair(DG2_CMD_INVALID, 0x0000),
        std::make_pair(DG2_CMD_READ, 0x0123),
        std::make_pair(DG2_CMD_WRITE, 0xFEDC)
    };

    for (const auto [cmd, vp] : mock_headers)
    {
        dg2_pkt_init(&pkt, buff.data());

        dg2_pkt_build_header(&pkt, cmd, vp);

        ASSERT_EQ(pkt.size, sizeof(dg2_pkt_header) + 2 /* + 2 for VP */);

        ASSERT_EQ(pkt.header->fhh, DG2_PKT_FHH);
        ASSERT_EQ(pkt.header->fhl, DG2_PKT_FHL);
        ASSERT_EQ(pkt.header->bc, 0); // Note: BC is set on dg2_pkt_finish()
        ASSERT_EQ(pkt.header->cmd, cmd);
        ASSERT_EQ(pkt.buff[DG2_PKT_INDEX_VPH], ((vp & 0xFF00)) >> 8);
        ASSERT_EQ(pkt.buff[DG2_PKT_INDEX_VPL], (vp & 0xFF));
    }
}

TEST(dg2_pkt, Parse)
{
    std::array<std::uint8_t, DG2_PKT_MAX_SIZE> buff = { };

    struct packet_parse_result
    {
        dg2_pkt_parse_err err;
        size_t bytes_consumed;
    };

    // TODO: Load from file

    std::vector<std::pair<std::vector<std::uint8_t>, packet_parse_result>> mock_data = {
        { { 0 }, { DG2_PKT_PARSE_ERR_NOT_FOUND, 0 } }, /* No data */

        { { DG2_PKT_FHL }, { DG2_PKT_PARSE_ERR_NOT_FOUND, 1 } }, /* No packet */

        { { 0x01, 0x02, 0x03, 0x04 }, { DG2_PKT_PARSE_ERR_NOT_FOUND, 4 } }, /* No packet */

        { { 0x43, DG2_PKT_FHH, 0x00, DG2_PKT_FHL, DG2_PKT_FHH, DG2_PKT_FHH, DG2_PKT_FHH, 0x23 }, { DG2_PKT_PARSE_ERR_NOT_FOUND, 8 } }, /* No packet */

        { { DG2_PKT_FHH, 0x00, DG2_PKT_FHL, DG2_PKT_FHH, 0x00 }, { DG2_PKT_PARSE_ERR_NOT_FOUND, 5 } }, /* No packet */

        { { DG2_PKT_FHH }, { DG2_PKT_PARSE_ERR_INCOMPLETE, 0 } }, /* Incomplete packet */

        { { DG2_PKT_FHH, DG2_PKT_FHL }, { DG2_PKT_PARSE_ERR_INCOMPLETE, 0 } }, /* Incomplete packet */

        { { DG2_PKT_FHH, DG2_PKT_FHH, DG2_PKT_FHL }, { DG2_PKT_PARSE_ERR_INCOMPLETE, 1 } }, /* Incomplete packet */

        { { 0x04, DG2_PKT_FHH, 3, DG2_PKT_FHH, DG2_PKT_FHL }, { DG2_PKT_PARSE_ERR_INCOMPLETE, 3 } }, /* Incomplete packet */

        { { 0x02, 0x5F, 0x25, DG2_PKT_FHH, DG2_PKT_FHL, 0x53 }, { DG2_PKT_PARSE_ERR_INCOMPLETE, 3 } }, /* Incomplete packet */

        { { 0x03, 0x12, DG2_PKT_FHH, DG2_PKT_FHL, 0x53, DG2_CMD_READ }, { DG2_PKT_PARSE_ERR_INCOMPLETE, 2 } }, /* Incomplete packet, header present */
    };

    mock_data.front().first.clear();

    for (const auto& [data, expected] : mock_data) {
        std::size_t bytes_consumed { 0 };
        dg2_pkt_header header;

        dg2_pkt_parse_res res { dg2_pkt_parse((uint8_t*)data.data(), data.size(), dg2_crc) };

        ASSERT_EQ(res.err, expected.err);
        ASSERT_EQ(res.bytes_consumed, expected.bytes_consumed);
    }
}
