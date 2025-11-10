#include <algorithm>
#include <array>
#include "dg2_circ_buff.h"
#include <gtest/gtest.h>
#include <utility>

auto generate_mock_data(size_t size) {
    std::vector<std::uint8_t> mock_data(size);

    size_t n { 0U };
    std::generate(mock_data.begin(), mock_data.end(), [&n]() {
        return n++;
    });

    return mock_data;
};

auto write_mock_data(dg2_circ_buff *circ_buff, const std::vector<uint8_t>& mock_data) {
    size_t write_result { dg2_circ_buff_write(circ_buff, mock_data.data(), mock_data.size()) };

    return write_result;
};

auto read_mock_data(dg2_circ_buff *circ_buff, size_t size) {
    std::vector<std::uint8_t> read_buff(size);

    size_t read_result { dg2_circ_buff_read(circ_buff, read_buff.data(), size) };

    read_buff.resize(read_result);

    return std::make_pair(read_result, read_buff);
};

TEST(dg2_circ_buff, Initialization)
{
    constexpr size_t buff_capacity { 1U };
    std::array<std::uint8_t, buff_capacity> buff;

    dg2_circ_buff _circ_buff;
    dg2_circ_buff *circ_buff { &_circ_buff };
    dg2_circ_buff_init(circ_buff, buff.data(), buff.size());

    ASSERT_EQ(circ_buff->buff, buff.data());

    ASSERT_EQ(circ_buff->back, circ_buff->front);

    ASSERT_EQ(circ_buff->capacity, buff.size());

    ASSERT_EQ(circ_buff->size, 0U);
}

TEST(dg2_circ_buff, GetSizeAndGetFreeSpace)
{
    for (auto buff_capacity : {1, 2, 3}) {
        std::vector<std::uint8_t> buff(buff_capacity);

        dg2_circ_buff _circ_buff;
        auto *circ_buff = &_circ_buff;
        dg2_circ_buff_init(circ_buff, buff.data(), buff.size());

        ASSERT_EQ(dg2_circ_buff_get_size(circ_buff), 0U);

        ASSERT_EQ(dg2_circ_buff_get_free_space(circ_buff), buff.size());
    }
}

TEST(dg2_circ_buff, ReadAndWrite)
{
    constexpr size_t buff_size { 256U };
    std::array<std::uint8_t, buff_size> buff;

    dg2_circ_buff _circ_buff;
    auto *circ_buff { &_circ_buff };
    dg2_circ_buff_init(circ_buff, buff.data(), buff_size);

    // Writes and reads
    for (size_t mock_data_size { 0U }; mock_data_size <= buff_size + 2 /* + 2 to check writes and reads not within boundaries */; ++mock_data_size) {
        auto mock_data = generate_mock_data(mock_data_size);
        auto expected_circ_buff_size { std::min(mock_data_size, buff_size) };

        /* Write */

        auto write_result { write_mock_data(circ_buff, mock_data) /* Might be more than possible */ };
        ASSERT_EQ(write_result, expected_circ_buff_size);

        ASSERT_EQ(dg2_circ_buff_get_size(circ_buff), expected_circ_buff_size);
        ASSERT_EQ(dg2_circ_buff_get_free_space(circ_buff), circ_buff->capacity - expected_circ_buff_size);

        // Note: When buffer is empty or full, back should be equal to front
        if (mock_data.size() == buff_size) {
            ASSERT_EQ(circ_buff->back, circ_buff->front);
        }

        /* Read */

        auto [read_result, read_buff] { read_mock_data(circ_buff, mock_data_size /* Might be more than available */) };
        ASSERT_EQ(read_result, expected_circ_buff_size);

        ASSERT_EQ(dg2_circ_buff_get_size(circ_buff), 0U);
        ASSERT_EQ(dg2_circ_buff_get_free_space(circ_buff), circ_buff->capacity);

        // Note: When buffer is empty or full, back should be equal to front
        ASSERT_EQ(circ_buff->back, circ_buff->front);

        ASSERT_EQ(read_buff.size(), expected_circ_buff_size);
        int memcmp_result { memcmp(read_buff.data(), mock_data.data(), expected_circ_buff_size) };
        ASSERT_EQ(memcmp_result, 0U);
    }
}

TEST(dg2_circ_buff, Peek)
{
    constexpr size_t buff_size { 256U };
    std::array<std::uint8_t, buff_size> buff;

    dg2_circ_buff _circ_buff;
    auto *circ_buff { &_circ_buff };
    dg2_circ_buff_init(circ_buff, buff.data(), buff_size);

    auto mock_data = generate_mock_data(buff.size());

    ASSERT_EQ(write_mock_data(circ_buff, mock_data), buff.size());
    ASSERT_EQ(dg2_circ_buff_get_size(circ_buff), buff.size());

    for (size_t offset { 0 }; offset < buff.size(); ++offset) {
        std::uint8_t dest;
        dg2_circ_buff_peek(circ_buff, &dest, offset);

        ASSERT_EQ(dest, mock_data.at(offset));
    }
}

TEST(dg2_circ_buff, PushAndPop)
{
    constexpr size_t buff_size { 256U };
    std::array<std::uint8_t, buff_size> buff;

    dg2_circ_buff _circ_buff;
    auto *circ_buff { &_circ_buff };
    dg2_circ_buff_init(circ_buff, buff.data(), buff.size());

    // for (auto mock_data_size : std::vector<std::size_t> { 0U, 1U, 2U, buff_size / 2U, buff_size, buff_size + 1U }) {
    for (auto mock_data_size : std::vector<std::size_t> { 0U, 1U, 2U, buff_size / 2U }) {
        auto mock_data = generate_mock_data(mock_data_size);
        auto expected_circ_buff_size { std::min(mock_data_size, buff_size) };

        for (auto it { mock_data.begin() }; it != mock_data.end(); ++it) {
            auto expected_push_result { bool(dg2_circ_buff_get_free_space(circ_buff)) };
            auto push_result { dg2_circ_buff_push(circ_buff, *it) };

            ASSERT_EQ(push_result, expected_push_result);
        }

        ASSERT_EQ(dg2_circ_buff_get_size(circ_buff), expected_circ_buff_size);

        std::vector<std::uint8_t> read_buff(mock_data_size); // Might be bigger than circ_buff

        for (auto it { read_buff.begin() }; it != read_buff.end(); ++it) {
            auto expected_pop_result { bool(dg2_circ_buff_get_size(circ_buff)) };
            std::uint8_t dest;
            auto pop_result { dg2_circ_buff_pop(circ_buff, &dest) };

            *it = dest;

            ASSERT_EQ(pop_result, expected_pop_result);
        }

        // read_buff.resize(expected_circ_buff_size);

        ASSERT_EQ(read_buff.size(), expected_circ_buff_size);
        int memcmp_result { memcmp(read_buff.data(), mock_data.data(), expected_circ_buff_size) };
        ASSERT_EQ(memcmp_result, 0U);
    }
}

TEST(dg2_circ_buff, Clear)
{
    constexpr size_t buff_size { 256U };
    std::array<std::uint8_t, buff_size> buff;

    dg2_circ_buff _circ_buff;
    dg2_circ_buff *circ_buff { &_circ_buff };
    dg2_circ_buff_init(circ_buff, buff.data(), buff_size);

    for (auto size : {0, 1, 2}) {
        auto mock_data = generate_mock_data(size);

        write_mock_data(circ_buff, mock_data);

        dg2_circ_buff_clear(circ_buff);

        ASSERT_EQ(circ_buff->buff, buff.data());

        ASSERT_EQ(circ_buff->back, circ_buff->front);

        ASSERT_EQ(circ_buff->capacity, buff_size);

        ASSERT_EQ(circ_buff->size, 0U);
    }
}
