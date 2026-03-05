#include "dg2_pkt.h"
#include <cstdint>
#include <vector>
#include <optional>
#include <string>

struct PacketParseResult
{
    dg2_pkt_parse_err err;
    std::size_t bytesConsumed;
    std::optional<dg2_cmd> cmd;
    std::optional<std::uint16_t> vp;
};

struct PacketParseTest
{
    std::string title;
    std::vector<std::uint8_t> packet;
    PacketParseResult expected;
    bool useCRC { false };
};

inline std::vector<PacketParseTest> GetPacketParseTests()
{
    std::vector<PacketParseTest> ppts = {
        { /* No data */
            "No Data",
            { },
            {
                .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
                .bytesConsumed = 0
            }
        },

        { /* No packet */
            "No packet",
            { DG2_PKT_FHL },
            {
                .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
                .bytesConsumed = 1
            }
        },

        { /* No packet */
            "No packet",
            { 0x01, 0x02, 0x03, 0x04 },
            {
                .err =DG2_PKT_PARSE_ERR_NOT_FOUND,
                .bytesConsumed = 4
            }
        },

        { /* No packet */
            "No packet",
            { 0x43, DG2_PKT_FHH, 0x00, DG2_PKT_FHL, DG2_PKT_FHH, DG2_PKT_FHH, DG2_PKT_FHH, 0x23 },
            {
                .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
                .bytesConsumed = 8
            }
        },

        { /* No packet */
            "No packet",
            { DG2_PKT_FHH, 0x00, DG2_PKT_FHL, DG2_PKT_FHH, 0x00 },
            {
                .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
                .bytesConsumed = 5
            }
        },

        { /* Incomplete packet */
            "Incomplete packet",
            { DG2_PKT_FHH },
            {
                .err = DG2_PKT_PARSE_ERR_INCOMPLETE,
                .bytesConsumed = 0
            }
        },

        { /* Incomplete packet */
            "Incomplete packet",
            { DG2_PKT_FHH, DG2_PKT_FHL },
            {
                .err = DG2_PKT_PARSE_ERR_INCOMPLETE,
                .bytesConsumed = 0
            }
        },

        { /* Incomplete packet */
            "Incomplete packet",
            { DG2_PKT_FHH, DG2_PKT_FHH, DG2_PKT_FHL },
            {
                .err = DG2_PKT_PARSE_ERR_INCOMPLETE,
                .bytesConsumed = 1
            }
        },

        { /* Incomplete packet */
            "Incomplete packet",
            { 0x04, DG2_PKT_FHH, 3, DG2_PKT_FHH, DG2_PKT_FHL },
            {
                .err = DG2_PKT_PARSE_ERR_INCOMPLETE,
                .bytesConsumed = 3
            }
        },

        { /* Incomplete packet */
            "Incomplete packet",
            { 0x02, 0x5F, 0x25, DG2_PKT_FHH, DG2_PKT_FHL, 0x53 },
            {
                .err = DG2_PKT_PARSE_ERR_INCOMPLETE,
                .bytesConsumed = 3
            }
        },

        { /* Incomplete packet, entire header present */
            "Incomplete packet, entire header present",
            { 0x03, 0x12, DG2_PKT_FHH, DG2_PKT_FHL, 0x53, DG2_CMD_READ },
            {
                .err = DG2_PKT_PARSE_ERR_INCOMPLETE,
                .bytesConsumed = 2
            }
        },

        { /* Read 1 byte at vp 0x1234 */
            "Read 1 byte at vp 0x1234",
            {
                0x01, 0x02, 0x03, 0x04, /* garbage */
                DG2_PKT_FHH, DG2_PKT_FHL, 0x04, DG2_CMD_READ, 0x12, 0x34, 0x01,
                0x04, 0x03, 0x02, 0x01, /* garbage */
            },
            {
                .err = DG2_PKT_PARSE_OK,
                .bytesConsumed = (4 + 7),
                .cmd = DG2_CMD_READ,
                .vp = 0x1234
            }
        },

        { /* Read 7 bytes at vp 0x6677 */
            "Read 7 bytes at vp 0x6677",
            {
                0x01, 0x02, 0x03, 0x04, /* garbage */
                DG2_PKT_FHH, DG2_PKT_FHL, 0x04, DG2_CMD_READ, 0x66, 0x77, 0x07,
                0x04, 0x03, 0x02, 0x01, /* garbage */
            },
            {
                .err = DG2_PKT_PARSE_OK,
                .bytesConsumed = (4 + 7),
                .cmd = DG2_CMD_READ,
                .vp = 0x6677
            }
        },

        /* CRC */

        { /* Read 1 byte at vp 0x1234 with incorrect CRC */
            "Read 1 byte at vp 0x1234 with incorrect CRC",
            {
                0x01, 0x02, 0x03, 0x04, /* garbage */
                DG2_PKT_FHH, DG2_PKT_FHL, 0x06, DG2_CMD_READ, 0x12, 0x34, 0x01, 0xAB, 0xCD,
                0x04, 0x03, 0x02, 0x01, /* garbage */
            },
            {
                /* Incorrect CRC should be treated as a result of false positive header by the parser */
                .err = DG2_PKT_PARSE_ERR_NOT_FOUND,
                .bytesConsumed = (4 + 9 + 4),
            },
            true // useCRC
        },

        { /* Read 1 byte at vp 0x000F with correct CRC */
            "Read 1 byte at vp 0x000F with correct CRC",
            {
                0x01, 0x02, 0x03, 0x04, /* garbage */
                DG2_PKT_FHH, DG2_PKT_FHL, 0x06, DG2_CMD_READ, 0x00, 0x0F, 0x01, 0xED, 0x90,
                0x04, 0x03, 0x02, 0x01, /* garbage */
            },
            {
                .err = DG2_PKT_PARSE_OK,
                .bytesConsumed = (4 + 9),
                .cmd = DG2_CMD_READ,
                .vp = 0x000F
            },
            true // useCRC
        },
    };

    return ppts;
}
