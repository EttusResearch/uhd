//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/convert.hpp>
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

static const size_t CMD_Q_MAX = 32;

/*
 * This class extends mock_reg_iface_t by adding a constructor that initializes some of
 * the read memory to contain the memory size for the replay block. This is important,
 * because upon construction in software, the Replay Block will read from the
 * REG_MEM_SIZE_ADDR to determine the word size and memory address width. These constant
 * read-only values are crucial for the initialization of the other properties.
 * Additionally, the record fullness is initialized here. This read-only value changes
 * during recording.
 */
class replay_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    replay_mock_reg_iface_t(size_t mem_addr_size, size_t word_size, size_t num_channels)
    {
        for (size_t chan = 0; chan < num_channels; chan++) {
            const uint32_t base = chan * replay_block_control::REPLAY_BLOCK_OFFSET;
            const uint32_t reg_compat = base +
                replay_block_control::REG_COMPAT_ADDR;
            const uint32_t reg_mem_size = base +
                replay_block_control::REG_MEM_SIZE_ADDR;
            const uint32_t reg_rec_fullness = base +
                replay_block_control::REG_REC_FULLNESS_LO_ADDR;
            const uint32_t reg_rec_position = base +
                replay_block_control::REG_REC_POS_LO_ADDR;
            const uint32_t reg_play_position = base +
                replay_block_control::REG_PLAY_POS_LO_ADDR;
            const uint32_t reg_play_fifo_space = base +
                replay_block_control::REG_PLAY_CMD_FIFO_SPACE_ADDR;
            read_memory[reg_compat] = (replay_block_control::MINOR_COMPAT
                                       | (replay_block_control::MAJOR_COMPAT << 16));
            read_memory[reg_mem_size] = (mem_addr_size | (word_size << 16));
            read_memory[reg_rec_fullness]      = 0x0010;
            read_memory[reg_rec_fullness + 4]  = 0x0000;
            read_memory[reg_rec_position]      = 0xBEEF;
            read_memory[reg_rec_position + 4]  = 0xDEAD;
            read_memory[reg_play_position]     = 0xCAFE;
            read_memory[reg_play_position + 4] = 0xFEED;
            read_memory[reg_play_fifo_space]   = CMD_Q_MAX;
        }
    }
};

/*
 * replay_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, mock register interface,
 * and replay_block_control object, all of which are accessible to the test
 * case. The instance of the object is destroyed at the end of each test
 * case.
 */
constexpr size_t DEFAULT_MTU = 8000;

struct replay_block_fixture
{
    replay_block_fixture()
        : num_channels(4)
        , num_input_ports(num_channels)
        , num_output_ports(num_channels)
        , mem_addr_size(32)
        , max_buffer_size(1ULL << mem_addr_size)
        , default_item_size(4)
        , word_size(8)
        , reg_iface(std::make_shared<replay_mock_reg_iface_t>(
              mem_addr_size, (word_size * 8), num_channels))
        , block_container(get_mock_block(REPLAY_BLOCK,
              num_channels,
              num_channels,
              uhd::device_addr_t("foo=bar"),
              DEFAULT_MTU,
              ANY_DEVICE,
              reg_iface))
        , test_replay(block_container.get_block<replay_block_control>())
    {
        node_accessor.init_props(test_replay.get());
    }

    size_t num_channels;
    size_t num_input_ports;
    size_t num_output_ports;
    size_t mem_addr_size;
    uint64_t max_buffer_size; // in bytes
    size_t default_item_size; // in bytes
    size_t word_size; // in bytes
    std::shared_ptr<replay_mock_reg_iface_t> reg_iface;
    mock_block_container block_container;
    std::shared_ptr<replay_block_control> test_replay;
    node_accessor_t node_accessor{};
};

inline uint32_t get_addr(uint32_t offset, size_t chan)
{
    return offset + chan * replay_block_control::REPLAY_BLOCK_OFFSET;
}

/*
 * This test case ensures that the hardware is programmed correctly with
 * defaults when the replay block is constructed.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_construction, replay_block_fixture)
{
    BOOST_REQUIRE(test_replay);
    BOOST_CHECK_EQUAL(test_replay->get_block_args().get("foo"), "bar");

    BOOST_CHECK_EQUAL(test_replay->get_mem_size(), max_buffer_size);
    BOOST_CHECK_EQUAL(test_replay->get_word_size(), word_size);

    for (size_t chan = 0; chan < num_channels; chan++) {
        const uint32_t reg_compat = get_addr(replay_block_control::REG_COMPAT_ADDR, chan);
        const uint32_t reg_mem_size =
            get_addr(replay_block_control::REG_MEM_SIZE_ADDR, chan);
        const uint32_t reg_rec_buff_size =
            get_addr(replay_block_control::REG_REC_BUFFER_SIZE_LO_ADDR, chan);
        const uint32_t reg_rec_base_addr =
            get_addr(replay_block_control::REG_REC_BASE_ADDR_LO_ADDR, chan);
        const uint32_t reg_play_buff_size =
            get_addr(replay_block_control::REG_PLAY_BUFFER_SIZE_LO_ADDR, chan);
        const uint32_t reg_play_base_addr =
            get_addr(replay_block_control::REG_PLAY_BASE_ADDR_LO_ADDR, chan);
        const uint32_t reg_words_per_pkt =
            get_addr(replay_block_control::REG_PLAY_WORDS_PER_PKT_ADDR, chan);
        const uint32_t reg_play_item_size =
            get_addr(replay_block_control::REG_PLAY_ITEM_SIZE_ADDR, chan);
        BOOST_CHECK_EQUAL(reg_iface->read_memory[reg_compat] & 0xFFFF,
            replay_block_control::MINOR_COMPAT);
        BOOST_CHECK_EQUAL((reg_iface->read_memory[reg_compat] >> 16) & 0xFFFF,
            replay_block_control::MAJOR_COMPAT);
        BOOST_CHECK_EQUAL(reg_iface->read_memory[reg_mem_size] & 0xFFFF, mem_addr_size);
        BOOST_CHECK_EQUAL(
            (reg_iface->read_memory[reg_mem_size] >> 16) & 0xFFFF, word_size * 8);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_rec_buff_size], max_buffer_size & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_rec_buff_size + 4],
            (max_buffer_size >> 32) & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_rec_base_addr], 0);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_rec_base_addr + 4], 0);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_play_buff_size], max_buffer_size & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_buff_size + 4],
            (max_buffer_size >> 32) & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_base_addr], 0);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_base_addr + 4], 0);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_words_per_pkt],
            (DEFAULT_MTU - test_replay->get_chdr_hdr_len()) / word_size);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_item_size], default_item_size);
    }
}


/**************************************************************************
 * Record Buffer tests
 *************************************************************************/

/*
 * This test case ensures that the hardware is programmed correctly when a record restart
 * occurs. Any value written to REG_REC_RESTART_ADDR triggers a record restart.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_record_restart, replay_block_fixture)
{
    for (size_t port = 0; port < num_input_ports; port++) {
        const uint32_t reg_rec_restart =
            get_addr(replay_block_control::REG_REC_RESTART_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory.count(reg_rec_restart), 0);
        test_replay->record_restart(port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory.count(reg_rec_restart), 1);
    }
}

/*
 * This test case ensures that the get_record_fullness() API call reads correctly from
 * hardware.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_record_fullness, replay_block_fixture)
{
    for (size_t port = 0; port < num_input_ports; port++) {
        const uint64_t fullness = 0x123456789ABCDEF0 | port;
        const uint32_t reg_rec_fullness =
            get_addr(replay_block_control::REG_REC_FULLNESS_LO_ADDR, port);
        reg_iface->read_memory[reg_rec_fullness]     = fullness & 0xFFFFFFFF;
        reg_iface->read_memory[reg_rec_fullness + 4] = (fullness >> 32) & 0xFFFFFFFF;

        BOOST_CHECK_EQUAL(test_replay->get_record_fullness(port), fullness);
    }
}

/*
 * This test case ensures that the record type API calls interact correctly.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_record_type, replay_block_fixture)
{
    for (size_t port = 0; port < num_input_ports; port++) {
        // Test the defaults
        const io_type_t default_type = IO_TYPE_SC16;
        BOOST_CHECK_EQUAL(test_replay->get_record_type(port), default_type);
        BOOST_CHECK_EQUAL(test_replay->get_record_item_size(port),
            uhd::convert::get_bytes_per_item(default_type));
    }

    for (size_t port = 0; port < num_input_ports; port++) {
        const io_type_t type = IO_TYPE_U8;
        test_replay->set_record_type(type, port);
        BOOST_CHECK_EQUAL(test_replay->get_record_type(port), type);
        BOOST_CHECK_EQUAL(test_replay->get_record_item_size(port),
            uhd::convert::get_bytes_per_item(type));
    }
}

/*
 * This test case ensures that the hardware is programmed correctly when the record buffer
 * is configured. This includes testing that a record restart takes place. The test
 * also ensures that any configuration of the base address and buffer size are word
 * addressable. Additionally, it exercises the get_record_size() and
 * get_record_offset() API calls.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_record, replay_block_fixture)
{
    for (size_t port = 0; port < num_input_ports; port++) {
        // Test the defaults
        BOOST_CHECK_EQUAL(test_replay->get_record_size(port), max_buffer_size);
        BOOST_CHECK_EQUAL(test_replay->get_record_offset(port), 0);
    }

    for (size_t port = 0; port < num_input_ports; port++) {
        const uint32_t reg_rec_restart =
            get_addr(replay_block_control::REG_REC_RESTART_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory.count(reg_rec_restart), 0);

        const uint64_t base_addr   = 16 + max_buffer_size / num_input_ports * port;
        const uint64_t buffer_size = max_buffer_size / num_input_ports / 2;
        test_replay->record(base_addr, buffer_size, port);

        const uint32_t reg_rec_buff_size =
            get_addr(replay_block_control::REG_REC_BUFFER_SIZE_LO_ADDR, port);
        const uint32_t reg_rec_base_addr =
            get_addr(replay_block_control::REG_REC_BASE_ADDR_LO_ADDR, port);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_rec_buff_size], buffer_size & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_rec_buff_size + 4],
            (buffer_size >> 32) & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_rec_base_addr], base_addr & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_rec_base_addr + 4],
            (base_addr >> 32) & 0xFFFFFFFF);

        BOOST_CHECK_EQUAL(test_replay->get_record_size(port), buffer_size);
        BOOST_CHECK_EQUAL(test_replay->get_record_offset(port), base_addr);
        // There should be a record restart on config
        // (with any value written to the register)
        BOOST_CHECK_EQUAL(reg_iface->write_memory.count(reg_rec_restart), 1);

        // Valid base address and buffer size values are multiples of the word size
        for (uint64_t offset = 1; offset <= word_size - 1; offset++) {
            BOOST_CHECK_THROW(test_replay->record(base_addr + offset, buffer_size, port),
                uhd::value_error);
        }
        for (uint64_t offset = 1; offset <= word_size - 1; offset++) {
            BOOST_CHECK_THROW(test_replay->record(base_addr, buffer_size + offset, port),
                uhd::value_error);
        }
        // The play buffer must be within the bounds of the Replay memory
        BOOST_CHECK_THROW(
            test_replay->record(max_buffer_size, buffer_size, port), uhd::value_error);
        BOOST_CHECK_THROW(
            test_replay->record(base_addr, max_buffer_size, port), uhd::value_error);
    }
}

/*
 * This test case checks the record position.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_record_position, replay_block_fixture)
{
    for (size_t port = 0; port < num_input_ports; port++) {
        const uint32_t reg_rec_position =
            get_addr(replay_block_control::REG_REC_POS_LO_ADDR, port);
        uint64_t rec_pos = reg_iface->read_memory[reg_rec_position] |
            (uint64_t(reg_iface->read_memory[reg_rec_position + 4]) << 32);
        BOOST_CHECK_EQUAL(test_replay->get_record_position(port), rec_pos);
    }
}

/**************************************************************************
 * Playback tests
 *************************************************************************/

/*
 * This test case ensures that the hardware is programmed correctly when the play buffer
 * is configured. The test also ensures that any configuration of the base address and
 * buffer size are word addressable. Additionally, it exercises the get_play_size() and
 * get_play_offset() API calls.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_configure_play, replay_block_fixture)
{
    for (size_t port = 0; port < num_output_ports; port++) {
        // Test the defaults
        BOOST_CHECK_EQUAL(test_replay->get_play_size(port), max_buffer_size);
        BOOST_CHECK_EQUAL(test_replay->get_play_offset(port), 0);
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        const uint64_t base_addr = word_size + max_buffer_size / num_output_ports * port;
        const uint64_t buffer_size = max_buffer_size / num_output_ports / 2;
        test_replay->config_play(base_addr, buffer_size, port);

        const uint32_t reg_play_buff_size =
            get_addr(replay_block_control::REG_PLAY_BUFFER_SIZE_LO_ADDR, port);
        const uint32_t reg_play_base_addr =
            get_addr(replay_block_control::REG_PLAY_BASE_ADDR_LO_ADDR, port);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_play_buff_size], buffer_size & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_buff_size + 4],
            (buffer_size >> 32) & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_play_base_addr], base_addr & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_base_addr + 4],
            (base_addr >> 32) & 0xFFFFFFFF);

        BOOST_CHECK_EQUAL(test_replay->get_play_size(port), buffer_size);
        BOOST_CHECK_EQUAL(test_replay->get_play_offset(port), base_addr);

        // Valid base address and buffer size values are multiples of the word size
        for (uint64_t offset = 1; offset <= word_size - 1; offset++) {
            BOOST_CHECK_THROW(
                test_replay->config_play(base_addr + offset, buffer_size, port),
                uhd::value_error);
        }
        for (uint64_t offset = 1; offset <= word_size - 1; offset++) {
            BOOST_CHECK_THROW(
                test_replay->config_play(base_addr, buffer_size + offset, port),
                uhd::value_error);
        }
        // Valid base address and buffer size values are multiples of the item size for
        // playback
        size_t item_size = test_replay->get_play_item_size(port);
        for (uint64_t offset = 1; offset <= item_size - 1; offset++) {
            BOOST_CHECK_THROW(
                test_replay->config_play(base_addr + offset, buffer_size, port),
                uhd::value_error);
        }
        for (uint64_t offset = 1; offset <= item_size - 1; offset++) {
            BOOST_CHECK_THROW(
                test_replay->config_play(base_addr, buffer_size + offset, port),
                uhd::value_error);
        }
        // The play buffer must be within the bounds of the Replay memory
        BOOST_CHECK_THROW(test_replay->config_play(max_buffer_size, buffer_size, port),
            uhd::value_error);
        BOOST_CHECK_THROW(
            test_replay->config_play(base_addr, max_buffer_size, port), uhd::value_error);
    }
}

/*
 * This test case ensures that the hardware is programmed correctly through the playback
 * packet API calls.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_packet_size, replay_block_fixture)
{
    for (size_t port = 0; port < num_output_ports; port++) {
        // Test the defaults
        const uint32_t item_size = test_replay->get_play_item_size(port);
        const uint32_t expected_ipp =
            test_replay->get_max_payload_size({res_source_info::OUTPUT_EDGE, port})
            / item_size;
        BOOST_CHECK_EQUAL(test_replay->get_max_items_per_packet(port), expected_ipp);
        const uint32_t default_packet_size =
            expected_ipp * item_size + test_replay->get_chdr_hdr_len();
        BOOST_CHECK_EQUAL(test_replay->get_max_packet_size(port), default_packet_size);
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        const uint32_t ipp = 1024;
        test_replay->set_max_items_per_packet(ipp, port);
        BOOST_CHECK_EQUAL(test_replay->get_max_items_per_packet(port), ipp);

        const uint32_t item_size   = test_replay->get_play_item_size(port);
        const uint32_t packet_size = ipp * item_size + test_replay->get_chdr_hdr_len();
        BOOST_CHECK_EQUAL(test_replay->get_max_packet_size(port), packet_size);

        const uint32_t wpp = ipp * item_size / word_size;
        const uint32_t reg_words_per_pkt =
            get_addr(replay_block_control::REG_PLAY_WORDS_PER_PKT_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_words_per_pkt], wpp);
    }
}

/*
 * This test case ensures that the play type and item size API calls interact correctly
 * and program the hardware.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_play_type, replay_block_fixture)
{
    for (size_t port = 0; port < num_output_ports; port++) {
        // Test the defaults
        const io_type_t default_type = IO_TYPE_SC16;
        BOOST_CHECK_EQUAL(test_replay->get_play_type(port), default_type);
        BOOST_CHECK_EQUAL(test_replay->get_play_item_size(port),
            uhd::convert::get_bytes_per_item(default_type));
        const uint32_t reg_play_item_size =
            get_addr(replay_block_control::REG_PLAY_ITEM_SIZE_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_item_size],
            uhd::convert::get_bytes_per_item(default_type));
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        const io_type_t type = IO_TYPE_U8;
        test_replay->set_play_type(type, port);
        BOOST_CHECK_EQUAL(test_replay->get_play_type(port), type);
        BOOST_CHECK_EQUAL(test_replay->get_play_item_size(port),
            uhd::convert::get_bytes_per_item(type));
        const uint32_t reg_play_item_size =
            get_addr(replay_block_control::REG_PLAY_ITEM_SIZE_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_item_size],
            uhd::convert::get_bytes_per_item(type));
    }
}

/*
 * This test case ensures that the hardware is programmed correctly when a stream command
 * is issued. Note that there is not a distinction between STREAM_MODE_NUM_SAMPS_AND_DONE
 * and STREAM_MODE_NUM_SAMPS_AND_MORE in USRP3 devices and newer.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_issue_stream_cmd, replay_block_fixture)
{
    for (size_t port = 0; port < num_output_ports; port++) {
        uhd::stream_cmd_t cmd_stop(
            uhd::stream_cmd_t::stream_mode_t::STREAM_MODE_STOP_CONTINUOUS);
        test_replay->issue_stream_cmd(cmd_stop, port);

        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_stream_cmd], replay_block_control::PLAY_CMD_STOP);
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        uhd::stream_cmd_t cmd_cont(
            uhd::stream_cmd_t::stream_mode_t::STREAM_MODE_START_CONTINUOUS);
        test_replay->issue_stream_cmd(cmd_cont, port);

        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_CONTINUOUS);
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        uhd::stream_cmd_t cmd_finite(
            uhd::stream_cmd_t::stream_mode_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        const uint64_t num_words = 0x00123ABC;
        cmd_finite.num_samps     = num_words * word_size / 4;
        test_replay->issue_stream_cmd(cmd_finite, port);

        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_FINITE);
        // PLAY_CMD_FINITE writes the number of words to hardware
        const uint32_t reg_num_words =
            get_addr(replay_block_control::REG_PLAY_CMD_NUM_WORDS_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_num_words], num_words & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_num_words + 4], (num_words >> 32) & 0xFFFFFFFF);
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        uhd::stream_cmd_t cmd_finite(
            uhd::stream_cmd_t::stream_mode_t::STREAM_MODE_NUM_SAMPS_AND_MORE);
        const uint64_t num_words = 0x00DEF456;
        cmd_finite.num_samps     = num_words * word_size / 4;
        test_replay->issue_stream_cmd(cmd_finite, port);

        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_FINITE);
        // PLAY_CMD_FINITE writes the number of words to hardware
        const uint32_t reg_num_words =
            get_addr(replay_block_control::REG_PLAY_CMD_NUM_WORDS_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_num_words], num_words & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_num_words + 4], (num_words >> 32) & 0xFFFFFFFF);
    }
}

/*
 * This test case ensures that the hardware is programmed correctly when a stream command
 * is issued with delay. Note that there is not a distinction between
 * STREAM_MODE_NUM_SAMPS_AND_DONE and STREAM_MODE_NUM_SAMPS_AND_MORE in USRP3 devices and
 * newer.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_issue_stream_cmd_timed, replay_block_fixture)
{
    for (size_t port = 0; port < num_output_ports; port++) {
        uhd::stream_cmd_t cmd_cont(
            uhd::stream_cmd_t::stream_mode_t::STREAM_MODE_START_CONTINUOUS);
        cmd_cont.stream_now = false;
        test_replay->issue_stream_cmd(cmd_cont, port);

        const uint32_t cmd_time_mask = 1 << 31;
        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_CONTINUOUS | cmd_time_mask);

        const uint64_t num_ticks = 0;
        const uint32_t reg_cmd_time =
            get_addr(replay_block_control::REG_PLAY_CMD_TIME_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_cmd_time], num_ticks & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_cmd_time + 4], (num_ticks >> 32) & 0xFFFFFFFF);
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        uhd::stream_cmd_t cmd_finite(
            uhd::stream_cmd_t::stream_mode_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        const uint64_t num_words = 0x00123ABC;
        cmd_finite.num_samps     = num_words * word_size / 4;
        cmd_finite.stream_now    = false;
        test_replay->issue_stream_cmd(cmd_finite, port);

        const uint32_t cmd_time_mask = 1 << 31;
        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_FINITE | cmd_time_mask);
        // PLAY_CMD_FINITE writes the number of words to hardware
        const uint32_t reg_num_words =
            get_addr(replay_block_control::REG_PLAY_CMD_NUM_WORDS_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_num_words], num_words & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_num_words + 4], (num_words >> 32) & 0xFFFFFFFF);

        const uint64_t num_ticks = 0;
        const uint32_t reg_cmd_time =
            get_addr(replay_block_control::REG_PLAY_CMD_TIME_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_cmd_time], num_ticks & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_cmd_time + 4], (num_ticks >> 32) & 0xFFFFFFFF);
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        uhd::stream_cmd_t cmd_finite(
            uhd::stream_cmd_t::stream_mode_t::STREAM_MODE_NUM_SAMPS_AND_MORE);
        const uint64_t num_words = 0x00DEF456;
        cmd_finite.num_samps     = num_words * word_size / 4;
        cmd_finite.stream_now    = false;
        test_replay->issue_stream_cmd(cmd_finite, port);

        const uint32_t cmd_time_mask = 1 << 31;
        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_FINITE | cmd_time_mask);
        // PLAY_CMD_FINITE writes the number of words to hardware
        const uint32_t reg_num_words =
            get_addr(replay_block_control::REG_PLAY_CMD_NUM_WORDS_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_num_words], num_words & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_num_words + 4], (num_words >> 32) & 0xFFFFFFFF);

        const uint64_t num_ticks = 0;
        const uint32_t reg_cmd_time =
            get_addr(replay_block_control::REG_PLAY_CMD_TIME_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_cmd_time], num_ticks & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_cmd_time + 4], (num_ticks >> 32) & 0xFFFFFFFF);
    }
}

/*
 * This test case ensures that the hardware is programmed correctly when a stop command is
 * issued to the replay block via an API call.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_stop, replay_block_fixture)
{
    for (size_t port = 0; port < num_output_ports; port++) {
        test_replay->stop(port);

        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_stream_cmd], replay_block_control::PLAY_CMD_STOP);
    }
}

/*
 * This test case ensures that the hardware is programmed correctly when the record buffer
 * is configured. This includes testing that a record restart takes place. The test
 * also ensures that any configuration of the base address and buffer size are word
 * addressable. Additionally, it exercises the get_play_size() and
 * get_play_offset() API calls.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_play, replay_block_fixture)
{
    // Configure play buffer
    for (size_t port = 0; port < num_output_ports; port++) {
        const uint64_t base_addr = word_size + max_buffer_size / num_output_ports * port;
        const uint64_t buffer_size = max_buffer_size / num_output_ports / 2;
        test_replay->play(base_addr, buffer_size, port);

        const uint32_t reg_play_buff_size =
            get_addr(replay_block_control::REG_PLAY_BUFFER_SIZE_LO_ADDR, port);
        const uint32_t reg_play_base_addr =
            get_addr(replay_block_control::REG_PLAY_BASE_ADDR_LO_ADDR, port);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_play_buff_size], buffer_size & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_buff_size + 4],
            (buffer_size >> 32) & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_play_base_addr], base_addr & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_play_base_addr + 4],
            (base_addr >> 32) & 0xFFFFFFFF);

        BOOST_CHECK_EQUAL(test_replay->get_play_size(port), buffer_size);
        BOOST_CHECK_EQUAL(test_replay->get_play_offset(port), base_addr);

        // Valid base address and buffer size values are multiples of the word size
        for (uint64_t offset = 1; offset <= word_size - 1; offset++) {
            BOOST_CHECK_THROW(test_replay->play(base_addr + offset, buffer_size, port),
                uhd::value_error);
        }
        for (uint64_t offset = 1; offset <= word_size - 1; offset++) {
            BOOST_CHECK_THROW(test_replay->play(base_addr, buffer_size + offset, port),
                uhd::value_error);
        }
        // Valid base address and buffer size values are multiples of the item size for
        // playback
        size_t item_size = test_replay->get_play_item_size(port);
        for (uint64_t offset = 1; offset <= item_size - 1; offset++) {
            BOOST_CHECK_THROW(test_replay->play(base_addr + offset, buffer_size, port),
                uhd::value_error);
        }
        for (uint64_t offset = 1; offset <= item_size - 1; offset++) {
            BOOST_CHECK_THROW(test_replay->play(base_addr, buffer_size + offset, port),
                uhd::value_error);
        }
        // The play buffer must be within the bounds of the Replay memory
        BOOST_CHECK_THROW(
            test_replay->play(max_buffer_size, buffer_size, port), uhd::value_error);
        BOOST_CHECK_THROW(
            test_replay->play(base_addr, max_buffer_size, port), uhd::value_error);
    }

    // Non-timed commands
    for (size_t port = 0; port < num_output_ports; port++) {
        const uint64_t base_addr =
            2 * word_size + max_buffer_size / num_output_ports * port;
        const uint64_t buffer_size = max_buffer_size / num_output_ports / 4;
        uhd::stream_cmd_t cmd_cont(
            uhd::stream_cmd_t::stream_mode_t::STREAM_MODE_START_CONTINUOUS);
        test_replay->play(base_addr, buffer_size, port, 0.0, true);

        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_CONTINUOUS);
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        const uint64_t base_addr =
            3 * word_size + max_buffer_size / num_output_ports * port;
        const uint64_t buffer_size = 0x1230;
        test_replay->play(base_addr, buffer_size, port);

        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_FINITE);
        // PLAY_CMD_FINITE writes the number of words to hardware
        const uint64_t num_words = buffer_size / word_size;
        const uint32_t reg_num_words =
            get_addr(replay_block_control::REG_PLAY_CMD_NUM_WORDS_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_num_words], num_words & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_num_words + 4], (num_words >> 32) & 0xFFFFFFFF);
    }

    // Timed Commands
    for (size_t port = 0; port < num_output_ports; port++) {
        const uint64_t base_addr =
            4 * word_size + max_buffer_size / num_output_ports * port;
        const uint64_t buffer_size = max_buffer_size / num_output_ports / 4;
        test_replay->play(base_addr, buffer_size, port, 1.23, true);

        const uint32_t cmd_time_mask = 1 << 31;
        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_CONTINUOUS | cmd_time_mask);

        const uint64_t num_ticks = 0;
        const uint32_t reg_cmd_time =
            get_addr(replay_block_control::REG_PLAY_CMD_TIME_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_cmd_time], num_ticks & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_cmd_time + 4], (num_ticks >> 32) & 0xFFFFFFFF);
    }

    for (size_t port = 0; port < num_output_ports; port++) {
        const uint64_t base_addr =
            5 * word_size + max_buffer_size / num_output_ports * port;
        const uint64_t buffer_size = 0xABC0;
        test_replay->play(base_addr, buffer_size, port, 4.56, false);

        const uint32_t cmd_time_mask = 1 << 31;
        const uint32_t reg_stream_cmd =
            get_addr(replay_block_control::REG_PLAY_CMD_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_stream_cmd],
            replay_block_control::PLAY_CMD_FINITE | cmd_time_mask);
        // PLAY_CMD_FINITE writes the number of words to hardware
        const uint64_t num_words = buffer_size / word_size;
        const uint32_t reg_num_words =
            get_addr(replay_block_control::REG_PLAY_CMD_NUM_WORDS_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_num_words], num_words & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_num_words + 4], (num_words >> 32) & 0xFFFFFFFF);

        const uint64_t num_ticks = 0;
        const uint32_t reg_cmd_time =
            get_addr(replay_block_control::REG_PLAY_CMD_TIME_LO_ADDR, port);
        BOOST_CHECK_EQUAL(reg_iface->write_memory[reg_cmd_time], num_ticks & 0xFFFFFFFF);
        BOOST_CHECK_EQUAL(
            reg_iface->write_memory[reg_cmd_time + 4], (num_ticks >> 32) & 0xFFFFFFFF);
    }
}

/*
 * This test case checks the play position.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_play_position, replay_block_fixture)
{
    for (size_t port = 0; port < num_input_ports; port++) {
        const uint32_t reg_play_position =
            get_addr(replay_block_control::REG_PLAY_POS_LO_ADDR, port);
        uint64_t play_pos = reg_iface->read_memory[reg_play_position] |
            (uint64_t(reg_iface->read_memory[reg_play_position + 4]) << 32);
        BOOST_CHECK_EQUAL(test_replay->get_play_position(port), play_pos);
    }
}

/*
 * This test case checks to make sure play commands throw an error if the
 * command queue is full.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_play_cmd_limit, replay_block_fixture)
{
    for (size_t port = 0; port < num_input_ports; port++) {
        const uint32_t reg_play_cmd_fifo_space =
            get_addr(replay_block_control::REG_PLAY_CMD_FIFO_SPACE_ADDR, port);

        // Issue stop to clear command queue
        test_replay->stop(port);

        // Fill the command queue
        for (size_t i = 0; i < CMD_Q_MAX; i++) {
            test_replay->play(0, 0, port);
        }
        reg_iface->read_memory[reg_play_cmd_fifo_space] = 0;

        // Make sure the next command throws
        BOOST_CHECK_THROW(test_replay->play(0, 0, port), uhd::op_failed);

        reg_iface->read_memory[reg_play_cmd_fifo_space] = CMD_Q_MAX;

        // Issue stop to clear the queue and reset
        test_replay->stop(port);

        // Run once more to confirm no error is thrown
        test_replay->play(0, 0, port);

        // Issue stop to clear the queue and reset
        test_replay->stop(port);
    }
}

/*
 * This test case ensures that the Replay Block can be added to an RFNoC graph.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_graph, replay_block_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_port_info;
    edge_port_info.src_port        = 0;
    edge_port_info.dst_port        = 0;
    edge_port_info.is_forward_edge = true;
    edge_port_info.edge            = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_radio_node_t mock_radio_block{0};
    mock_ddc_node_t mock_ddc_block{};
    mock_terminator_t mock_sink_term(1, {}, "MOCK_SINK");

    UHD_LOG_INFO("TEST", "Priming mock block properties");
    node_accessor.init_props(&mock_radio_block);
    node_accessor.init_props(&mock_ddc_block);
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_radio_block, &mock_ddc_block, edge_port_info);
    graph.connect(&mock_ddc_block, test_replay.get(), edge_port_info);
    graph.connect(test_replay.get(), &mock_sink_term, edge_port_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    mock_sink_term.set_edge_property<double>(
        "tick_rate", 1.0, {res_source_info::INPUT_EDGE, 0});
    UHD_LOG_INFO("TEST", "Commit complete.");

    mock_radio_block.generate_overrun(0);
    uhd::rx_metadata_t rx_md;
    BOOST_REQUIRE(test_replay->get_record_async_metadata(rx_md, 1.0));
    BOOST_CHECK(rx_md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);

    mock_sink_term.post_action(res_source_info{res_source_info::INPUT_EDGE, 0},
        tx_event_action_info::make(uhd::async_metadata_t::EVENT_CODE_UNDERFLOW, 1234ul));
    uhd::async_metadata_t tx_md;
    BOOST_REQUIRE(test_replay->get_play_async_metadata(tx_md, 1.0));
    BOOST_CHECK(tx_md.event_code == uhd::async_metadata_t::EVENT_CODE_UNDERFLOW);
    BOOST_CHECK(tx_md.has_time_spec);
    BOOST_CHECK(tx_md.time_spec == 1234.0);
}

/*
 * This test case ensures that the Replay Block can be added to an RFNoC graph
 * in a loop.
 */
BOOST_FIXTURE_TEST_CASE(replay_test_graph_loop, replay_block_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_port_info;
    edge_port_info.src_port        = 0;
    edge_port_info.dst_port        = 0;
    edge_port_info.is_forward_edge = false;
    edge_port_info.edge            = detail::graph_t::graph_edge_t::DYNAMIC;

    // Now create a DDC block
    UHD_LOG_DEBUG("TEST", "Making DDC block control....");
    node_accessor_t node_accessor{};
    constexpr uint32_t num_hb  = 2;
    constexpr uint32_t max_cic = 128;
    constexpr size_t num_chans = 1;
    constexpr noc_id_t noc_id  = DDC_BLOCK;
    auto block_container =
        get_mock_block(noc_id, num_chans, num_chans, uhd::device_addr_t(""));
    auto& ddc_reg_iface = block_container.reg_iface;
    ddc_reg_iface->read_memory[ddc_block_control::RB_COMPAT_NUM] =
        (ddc_block_control::MAJOR_COMPAT << 16) | ddc_block_control::MINOR_COMPAT;
    ddc_reg_iface->read_memory[ddc_block_control::RB_NUM_HB]        = num_hb;
    ddc_reg_iface->read_memory[ddc_block_control::RB_CIC_MAX_DECIM] = max_cic;
    auto test_ddc = block_container.get_block<ddc_block_control>();

    node_accessor.init_props(test_ddc.get());
    UHD_LOG_DEBUG("TEST", "DDC done.");

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(test_ddc.get(), test_replay.get(), edge_port_info);
    // Graph must be DAG, disable prop prop on back-edge (normally,
    // rfnoc_graph::connect() would do this for us if we declare a back-edge
    edge_port_info.is_forward_edge = true;
    graph.connect(test_replay.get(), test_ddc.get(), edge_port_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
}
