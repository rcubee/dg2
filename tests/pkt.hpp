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
    std::optional<std::uint8_t> payload_size { std::nullopt };

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

    static PacketParseResult Ok(const size_t bytes_consumed, const dg2_cmd cmd, const std::uint16_t vp, const std::uint8_t payload_size)
    {
        return PacketParseResult {
            .err = DG2_PKT_PARSE_OK,
            .bytes_consumed = bytes_consumed,
            .cmd = cmd,
            .vp = vp,
            .payload_size = payload_size
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
        .test_name = "Read_1_Word_At_0x1234_Response",
        .packet = {
            DG2_PKT_FHH, DG2_PKT_FHL, 0x06, DG2_CMD_READ, 0x12, 0x34, 0x01, 0x14, 0x10
        },
        .expected = PacketParseResult::Ok(9, DG2_CMD_READ, 0x1234, 1)
    },

    {
        .test_name = "Read_1_Word_At_0x000F_Response_With_CRC",
        .packet = {
            0x5A, 0xA5, 0x08, 0x83, 0x00, 0x0F, 0x01, 0x14,
            0x10, 0x43, 0xF0,
        },
        .crc = true,
        .expected = PacketParseResult::Ok(11, DG2_CMD_READ, 0x000F, 1)
    },

    {
        .test_name = "Read_3_Words_At_0x1234_Response",
        .packet = {
            0x5A, 0xA5, 0x0A, 0x83, 0x12, 0x34, 0x03, 0x00,
            0x01, 0x00, 0x02, 0x00, 0x03
        },
        .expected = PacketParseResult::Ok(13, DG2_CMD_READ, 0x1234, 3)
    },

    {
        .test_name = "Read_2_Words_At_0xABCD_Response_With_CRC",
        .packet = {
            0x5A, 0xA5, 0x0A, 0x83, 0xAB, 0xCD, 0x02, 0x00,
            0x01, 0x00, 0x02, 0xAB, 0x68
        },
        .crc = true,
        .expected = PacketParseResult::Ok(13, DG2_CMD_READ, 0xABCD, 2)
    },
};

const PacketParseTestCase gMiscPacketTests[] =
{
    {
        .test_name = "Too_Large_By_One",
        .packet = {
            DG2_PKT_FHH, DG2_PKT_FHL, ((DG2_PKT_MAX_SIZE - 3) + 1 /* Frame length */), DG2_CMD_READ, 0x12, 0x34, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
            0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA
        },
        .crc = true,
        .expected = PacketParseResult::NotFound(256)
    },
    {
        .test_name = "Read_2_Words_At_0xABCD_Response_With_Incorrect_CRC",
        .packet = {
            0x5A, 0xA5, 0x0A, 0x83, 0xAB, 0xCD, 0x02, 0x00,
            0x01, 0x00, 0x02, 0x00, 0x00
        },
        .crc = true,
        .expected = PacketParseResult::NotFound(13)
    },
};
