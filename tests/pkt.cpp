#include <array>
#include <cstdint>
#include "dg2_pkt.h"
#include <gtest/gtest.h>
#include "pkts.hpp"

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
    for (auto& [title, packet, expected, useCRC] : GetPacketParseTests()) {
        std::cout << title << std::endl;

        if (packet.empty()) {
            packet.reserve(1); // Note: In case the vector is empty, call reserve so that .data() doesn't return a nullptr
        }

        dg2_pkt_parse_res res { dg2_pkt_parse(packet.data(), packet.size(), useCRC ? dg2_crc : nullptr) };

        // Throw instead?
        ASSERT_EQ(res.err, expected.err);
        ASSERT_EQ(res.bytes_consumed, expected.bytesConsumed);

        if (expected.cmd.has_value()) {
            ASSERT_EQ(res.cmd, expected.cmd);
        }

        if (expected.vp.has_value()) {
            ASSERT_EQ(res.vp, expected.vp);
        }
    }
}
