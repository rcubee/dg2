#include <array>
#include <iterator>
#include <cstdint>
#include "dg2_pkt.h"
#include <gtest/gtest.h>
#include "pkt.hpp"

/* Initialization */

TEST(dg2_pkt, Initialization)
{
    std::array<std::uint8_t, 256> buff = { };

    dg2_pkt pkt;

    dg2_pkt_init(&pkt, buff.data());

    ASSERT_EQ(pkt.buff, buff.data());
    ASSERT_EQ(pkt.size, 0);
}

class PacketBuildHeaderTest : public ::testing::TestWithParam<PacketBuildHeaderTestCase> {};

TEST_P(PacketBuildHeaderTest, BuildHeader)
{
    const auto [test_name, cmd, vp] = GetParam();

    std::array<std::uint8_t, DG2_PKT_MAX_SIZE> buff = { };

    dg2_pkt pkt;

    dg2_pkt_init(&pkt, buff.data());

    dg2_pkt_build_header(&pkt, cmd, vp);

    EXPECT_EQ(pkt.size, sizeof(dg2_pkt_header) + 2 /* + 2 for VP */);

    EXPECT_EQ(pkt.header->fhh, DG2_PKT_FHH);
    EXPECT_EQ(pkt.header->fhl, DG2_PKT_FHL);
    EXPECT_EQ(pkt.header->bc, 0); // Note: BC is set on dg2_pkt_finish()
    EXPECT_EQ(pkt.header->cmd, cmd);
    EXPECT_EQ(pkt.buff[DG2_PKT_INDEX_VPH], ((vp & 0xFF00)) >> 8);
    EXPECT_EQ(pkt.buff[DG2_PKT_INDEX_VPL], (vp & 0xFF));
}

/* Parsing */

class PacketParseTest : public ::testing::TestWithParam<PacketParseTestCase> {};

TEST_P(PacketParseTest, /* deliberately left blank */)
{
    auto [test_name, packet, crc, expected] = GetParam();

    if (packet.empty()) {
        packet.reserve(1); // Note: In case the vector is empty, call reserve so that .data() doesn't return a nullptr
    }

    dg2_pkt_parse_res res {
        dg2_pkt_parse(packet.data(),
        packet.size(),
        crc ? dg2_crc : nullptr)
    };

    EXPECT_EQ(res.err, expected.err);

    EXPECT_EQ(res.bytes_consumed, expected.bytes_consumed);

    if (expected.cmd.has_value()) {
        EXPECT_EQ(res.cmd, expected.cmd);
    }

    if (expected.vp.has_value()) {
        EXPECT_EQ(res.vp, expected.vp);
    }

    // Todo: Check payload and payload_size fields of dg2_pkt_parse_res
}

static void ValidateTestName(const std::string& test_name)
{
    for (const auto c : test_name) {
        if (!std::isalnum(c) && c != '_') {
            throw std::invalid_argument("Invalid test name: " + test_name);
        }
    }
}

/* Instantiation */

INSTANTIATE_TEST_SUITE_P(BuildHeader,
                         PacketBuildHeaderTest,
                         ::testing::ValuesIn(std::begin(gBuildHeaderTests), std::end(gBuildHeaderTests)),
                         [] (const ::testing::TestParamInfo<PacketBuildHeaderTestCase>& info) {
                             ValidateTestName(info.param.test_name);

                             return info.param.test_name;
                         });

INSTANTIATE_TEST_SUITE_P(NotFound,
                         PacketParseTest,
                         ::testing::ValuesIn(std::begin(gNoPacketTests), std::end(gNoPacketTests)),
                         [] (const ::testing::TestParamInfo<PacketParseTestCase>& info) {
                             ValidateTestName(info.param.test_name);

                             return info.param.test_name;
                         });

INSTANTIATE_TEST_SUITE_P(Incomplete,
                         PacketParseTest,
                         ::testing::ValuesIn(std::begin(gIncompletePacketTests), std::end(gIncompletePacketTests)),
                         [] (const ::testing::TestParamInfo<PacketParseTestCase>& info) {
                             ValidateTestName(info.param.test_name);

                             return info.param.test_name;
                         });

INSTANTIATE_TEST_SUITE_P(Ok,
                         PacketParseTest,
                         ::testing::ValuesIn(std::begin(gOkPacketTests), std::end(gOkPacketTests)),
                         [] (const ::testing::TestParamInfo<PacketParseTestCase>& info) {
                             ValidateTestName(info.param.test_name);

                             return info.param.test_name;
                         });
