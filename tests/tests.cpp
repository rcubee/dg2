#include <algorithm>
#include <array>
#include "dg2_circ_buff.h"
#include <gtest/gtest.h>

TEST(dg2_circ_buff, Initialization)
{
    constexpr size_t buff_size { 1U };
    std::array<std::uint8_t, buff_size> buff;

    dg2_circ_buff _circ_buff;
    dg2_circ_buff *circ_buff = &_circ_buff;
    dg2_circ_buff_init(circ_buff, buff.data(), buff_size);

    ASSERT_EQ(circ_buff->buff, buff.data());

    // If the size of the circular buffer is 0, back should be equal to front
    ASSERT_EQ(circ_buff->buff, circ_buff->back);
    ASSERT_EQ(circ_buff->buff, circ_buff->front);

    ASSERT_EQ(circ_buff->capacity, buff_size);

    ASSERT_EQ(circ_buff->size, 0);
}

TEST(dg2_circ_buff, Getters)
{
    constexpr size_t buff_size { 1U };
    std::array<std::uint8_t, buff_size> buff;

    dg2_circ_buff _circ_buff;
    dg2_circ_buff *circ_buff = &_circ_buff;
    dg2_circ_buff_init(circ_buff, buff.data(), buff_size);

    ASSERT_EQ(dg2_circ_buff_get_size(circ_buff), 0);

    ASSERT_EQ(dg2_circ_buff_get_free_space(circ_buff), buff_size);
}

TEST(dg2_circ_buff, ReadAndWrite)
{
    constexpr size_t buff_size { 256U };
    std::array<std::uint8_t, buff_size> buff;

    dg2_circ_buff _circ_buff;
    dg2_circ_buff *circ_buff { &_circ_buff };
    dg2_circ_buff_init(circ_buff, buff.data(), buff_size);


    auto generate_mock_data = [] (size_t size) {
         std::vector<std::uint8_t> mock_data(size);

        size_t n { 0 };
        std::generate(mock_data.begin(), mock_data.end(), [&n]() {
            return n++;
        });

        return mock_data;
    };

    auto write_mock_data = [circ_buff, buff_size](const std::vector<uint8_t>& mock_data) {
        size_t write_result { dg2_circ_buff_write(circ_buff, mock_data.data(), mock_data.size()) };
        ASSERT_EQ(write_result, mock_data.size());

        ASSERT_EQ(dg2_circ_buff_get_size(circ_buff), mock_data.size());

        ASSERT_EQ(dg2_circ_buff_get_free_space(circ_buff), buff_size - mock_data.size());

        int memcmp_result { memcmp(mock_data.data(), mock_data.data(), mock_data.size()) };
        ASSERT_EQ(memcmp_result, 0U);
    };

    auto read_all_mock_data = [circ_buff, buff_size](const std::vector<std::uint8_t>& expected_mock_data) {
        std::vector<std::uint8_t> read_buff(expected_mock_data.size());

        size_t read_result { dg2_circ_buff_read(circ_buff, read_buff.data(), expected_mock_data.size()) };
        ASSERT_EQ(read_result, expected_mock_data.size());

        ASSERT_EQ(dg2_circ_buff_get_size(circ_buff), 0U);

        ASSERT_EQ(dg2_circ_buff_get_free_space(circ_buff), buff_size);

        // Note: After reading all data, back should be equal to front
        ASSERT_EQ(circ_buff->back, circ_buff->front);

        int memcmp_result { memcmp(read_buff.data(), expected_mock_data.data(), expected_mock_data.size()) };
        ASSERT_EQ(memcmp_result, 0U);
    };

    for (size_t size = 0; size <= buff_size; ++size) {
        auto mock_data = generate_mock_data(size);

        write_mock_data(mock_data);
        read_all_mock_data(mock_data);
    }
}
