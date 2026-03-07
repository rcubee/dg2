#include "dg2_pkt.h"
#include <cstdint>
#include <vector>
#include <optional>
#include <string>

/* Initialization */

struct PacketBuildHeaderTestCase
{
    std::string test_name;

    dg2_cmd cmd;
    std::uint16_t vp;
};

const PacketBuildHeaderTestCase gBuildHeaderTests[] =
{
    {
        .test_name = "Build_Header_Invalid",
        .cmd = DG2_CMD_INVALID,
        .vp = 0x0000
    },
    {
        .test_name = "Build_Header_Read",
        .cmd = DG2_CMD_READ,
        .vp = 0x1234
    },
    {
        .test_name = "Build_Header_Write",
        .cmd = DG2_CMD_WRITE,
        .vp = 0xFECD
    },
};

/* Parse */

struct PacketParseResult
{
    dg2_pkt_parse_err err;
    std::size_t bytes_consumed;
    std::optional<dg2_cmd> cmd { std::nullopt };
    std::optional<std::uint16_t> vp { std::nullopt };

    static PacketParseResult NotFound(const size_t bytes_consumed)
    {
        return PacketParseResult {
            .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
            .bytes_consumed = bytes_consumed
        };
    }

    static PacketParseResult Incomplete(const size_t bytes_consumed)
    {
        return PacketParseResult {
            .err = DG2_PKT_PARSE_ERR_INCOMPLETE,
            .bytes_consumed = bytes_consumed
        };
    }

    static PacketParseResult Ok(const size_t bytes_consumed, const dg2_cmd cmd, const std::uint16_t vp)
    {
        return PacketParseResult {
            .err = DG2_PKT_PARSE_OK,
            .bytes_consumed = bytes_consumed,
            .cmd = cmd,
            .vp = vp
        };
    }
};

struct PacketParseTestCase
{
    std::string test_name;
    std::vector<std::uint8_t> packet;
    bool crc { false };

    PacketParseResult expected;
};

const PacketParseTestCase gNoPacketTests[] =
{
    {
        .test_name = "Empty_Packet",
        .packet = {
            /* empty */
        },
        .expected = {
            .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
            .bytes_consumed = 0
        }
    },

    {
        .test_name = "Empty_Packet_CRC",
        .packet = {
            /* empty */
        },
        .crc = true,
        .expected = {
            .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
            .bytes_consumed = 0
        }
    },

    {
        .test_name = "No_Packet_0",
        .packet = {
            DG2_PKT_FHL
        },
        .expected = {
            .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
            .bytes_consumed = 1
        }
    },

    {
        .test_name = "No_Packet_1",
        .packet = {
            0x01, 0x02, 0x03, 0x04
        },
        .expected = {
            .err =DG2_PKT_PARSE_ERR_NOT_FOUND,
            .bytes_consumed = 4
        }
    },

    {
        .test_name = "No_Packet_2",
        .packet = {
            0x43, DG2_PKT_FHH, 0x00, DG2_PKT_FHL, DG2_PKT_FHH, DG2_PKT_FHH, DG2_PKT_FHH, 0x23
        },
        .expected = {
            .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
            .bytes_consumed = 8
        }
    },

    {
        .test_name = "No_Packet_3",
        .packet = {
            DG2_PKT_FHH, 0x00, DG2_PKT_FHL, DG2_PKT_FHH, 0x00
        },
        .expected = {
            .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
            .bytes_consumed = 5
        }
    },
};

const PacketParseTestCase gIncompletePacketTests[] =
{
    {
        .test_name = "Incomplete_Packet_0",
        .packet = {
            DG2_PKT_FHH
        },
        .expected = PacketParseResult::Incomplete(0)
    },

    {
        .test_name = "Incomplete_Packet_1",
        .packet = {
            DG2_PKT_FHH, DG2_PKT_FHL
        },
        .expected = PacketParseResult::Incomplete(0)
    },

    {
        .test_name = "Incomplete_Packet_2",
        .packet = {
            DG2_PKT_FHH, DG2_PKT_FHH, DG2_PKT_FHL
        },
        .expected = PacketParseResult::Incomplete(1)
    },

    {
        .test_name = "Incomplete_Packet_3",
        .packet = {
            0x04, DG2_PKT_FHH, 3, DG2_PKT_FHH, DG2_PKT_FHL
        },
        .expected = PacketParseResult::Incomplete(3)
    },

    {
        .test_name = "Incomplete_Packet_Header_Present",
        .packet = {
            0x03, 0x12, DG2_PKT_FHH, DG2_PKT_FHL, 0x53, DG2_CMD_READ
        },
        .expected = PacketParseResult::Incomplete(2)
    },
};

const PacketParseTestCase gOkPacketTests[] =
{
    {
        .test_name = "Read_1_Byte_At_0x1234",
        .packet = {
            0x01, 0x02, 0x03, 0x04, /* garbage */
            DG2_PKT_FHH, DG2_PKT_FHL, 0x04, DG2_CMD_READ, 0x12, 0x34, 0x01,
            0x04, 0x03, 0x02, 0x01, /* garbage */
        },
        .expected = PacketParseResult::Ok(4 + 7, DG2_CMD_READ, 0x1234)
    },

    {
        .test_name = "Read_7_Bytes_At_0x6677",
        .packet = {
            0x01, 0x02, 0x03, 0x04, /* garbage */
            DG2_PKT_FHH, DG2_PKT_FHL, 0x04, DG2_CMD_READ, 0x66, 0x77, 0x07,
            0x04, 0x03, 0x02, 0x01, /* garbage */
        },
        .expected = PacketParseResult::Ok(4 + 7, DG2_CMD_READ, 0x6677)
    },

    {
        .test_name = "Read_1_Byte_At_0x1234_With_Incorrect_CRC",
        .packet = {
            0x01, 0x02, 0x03, 0x04, /* garbage */
            DG2_PKT_FHH, DG2_PKT_FHL, 0x06, DG2_CMD_READ, 0x12, 0x34, 0x01, 0xAB, 0xCD,
            0x04, 0x03, 0x02, 0x01, /* garbage */
        },
        .crc = true,
        /* Incorrect CRC should be treated as a result of false positive header by the parser */
        .expected = PacketParseResult::NotFound(4 + 9 + 4)
    },

    {
        .test_name = "Read_1_Byte_At_0x000F_With_Correct_CRC",
        .packet = {
            0x01, 0x02, 0x03, 0x04, /* garbage */
            DG2_PKT_FHH, DG2_PKT_FHL, 0x06, DG2_CMD_READ, 0x00, 0x0F, 0x01, 0xED, 0x90,
            0x04, 0x03, 0x02, 0x01, /* garbage */
        },
        .crc = true,
        .expected = PacketParseResult::Ok(4 + 9, DG2_CMD_READ, 0x000F)
    }
};

inline std::vector<PacketParseTestCase> GetPacketParseTestCases()
{
    const auto NoPacketTests = std::vector<PacketParseTestCase> (std::begin(gNoPacketTests), std::end(gNoPacketTests));
    const auto IncompletePacketTests = std::vector<PacketParseTestCase> (std::begin(gIncompletePacketTests), std::end(gIncompletePacketTests));
    const auto OkPacketTests = std::vector<PacketParseTestCase> (std::begin(gOkPacketTests), std::end(gOkPacketTests));

    std::vector<PacketParseTestCase> Tests;
    Tests.reserve(std::size(NoPacketTests) + std::size(IncompletePacketTests));

    Tests.insert(Tests.end(), std::begin(NoPacketTests), std::end(NoPacketTests));
    Tests.insert(Tests.end(), std::begin(IncompletePacketTests), std::end(IncompletePacketTests));
    Tests.insert(Tests.end(), std::begin(OkPacketTests), std::end(OkPacketTests));

    return Tests;
}
