//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/null_block_control.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <atomic>

namespace {

} // namespace

using namespace uhd::rfnoc;
using uhd::stream_cmd_t;


const uint32_t null_block_control::REG_CTRL_STATUS       = 0x00;
const uint32_t null_block_control::REG_SRC_LINES_PER_PKT = 0x04;
const uint32_t null_block_control::REG_SRC_BYTES_PER_PKT = 0x08;
const uint32_t null_block_control::REG_SRC_THROTTLE_CYC  = 0x0C;
const uint32_t null_block_control::REG_SNK_LINE_CNT_LO   = 0x10;
const uint32_t null_block_control::REG_SNK_LINE_CNT_HI   = 0x14;
const uint32_t null_block_control::REG_SNK_PKT_CNT_LO    = 0x18;
const uint32_t null_block_control::REG_SNK_PKT_CNT_HI    = 0x1C;
const uint32_t null_block_control::REG_SRC_LINE_CNT_LO   = 0x20;
const uint32_t null_block_control::REG_SRC_LINE_CNT_HI   = 0x24;
const uint32_t null_block_control::REG_SRC_PKT_CNT_LO    = 0x28;
const uint32_t null_block_control::REG_SRC_PKT_CNT_HI    = 0x2C;
const uint32_t null_block_control::REG_LOOP_LINE_CNT_LO  = 0x30;
const uint32_t null_block_control::REG_LOOP_LINE_CNT_HI  = 0x34;
const uint32_t null_block_control::REG_LOOP_PKT_CNT_LO   = 0x38;
const uint32_t null_block_control::REG_LOOP_PKT_CNT_HI   = 0x3C;


class null_block_control_impl : public null_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(null_block_control)
    {
        // This block doesn't pass through packets, so the MTU can differ on
        // input and output.
        set_mtu_forwarding_policy(forwarding_policy_t::DROP);
        uint32_t initial_state = regs().peek32(REG_CTRL_STATUS);
        _streaming             = initial_state & 0x2;
        _nipc                  = (initial_state >> 24) & 0xFF;
        _item_width            = (initial_state >> 16) & 0xFF;
        // Give the source some reasonable values to start
        set_bytes_per_packet(1024);
        set_throttle_cycles(0);
        reset_counters();
        register_issue_stream_cmd();
    }

    void issue_stream_cmd(const stream_cmd_t& stream_cmd) override
    {
        if (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS) {
            RFNOC_LOG_TRACE("Received start stream request!");
            regs().poke32(REG_CTRL_STATUS, 0x2);
            _streaming = true;
        } else if (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS) {
            RFNOC_LOG_TRACE("Received stop stream request!");
            regs().poke32(REG_CTRL_STATUS, 0x0);
            _streaming = false;
        } else {
            throw uhd::runtime_error("Null source can only do continuous streaming!");
        }
    }

    void reset_counters() override
    {
        const uint32_t streaming_flag = _streaming ? 0x2 : 0x0;
        regs().poke32(REG_CTRL_STATUS, streaming_flag | 0x1);
        regs().poke32(REG_CTRL_STATUS, streaming_flag | 0x0);
    }

    void set_lines_per_packet(const uint32_t lpp)
    {
        if (lpp < 2) {
            throw uhd::value_error("Null source lines per packet must be at "
                                   "least one line in the payload!");
        }
        if (lpp > 0xFFF) {
            throw uhd::value_error("Null source lines per packet cannot exceed 12 bits!");
        }
        // The register value is decreased by one line for the header, and one
        // line for encoding (if we write a 0 here, the payload will still be at
        // least one line long).
        regs().poke32(REG_SRC_LINES_PER_PKT, lpp - 2);
    }

    void set_bytes_per_packet(const uint32_t bpp) override
    {
        if (bpp > 0xFFFF) {
            throw uhd::value_error("Null source lines per packet cannot exceed 16 bits!");
        }
        regs().poke32(REG_SRC_BYTES_PER_PKT, bpp);
        const uint32_t bytes_per_line = (_item_width * _nipc) / 8;
        // If bpp is not an integer multiple of bytes_per_line, then we add a
        // full additional line!
        const uint32_t lpp = bpp / bytes_per_line + (bpp % bytes_per_line ? 1 : 0);
        set_lines_per_packet(lpp);
    }

    void set_throttle_cycles(const uint32_t cycs) override
    {
        if (cycs > 0x3FF) {
            throw uhd::value_error("Null source throttle cycles cannot exceed 10 bits!");
        }
        regs().poke32(REG_SRC_THROTTLE_CYC, cycs);
    }

    uint32_t get_item_width() override
    {
        return _item_width;
    }

    uint32_t get_nipc() override
    {
        return _nipc;
    }

    uint32_t get_lines_per_packet() override
    {
        return regs().peek32(REG_SRC_LINES_PER_PKT) + 2;
    }

    uint32_t get_bytes_per_packet() override
    {
        return regs().peek32(REG_SRC_BYTES_PER_PKT);
    }

    uint32_t get_throttle_cycles() override
    {
        return regs().peek32(REG_SRC_THROTTLE_CYC);
    }

    uint64_t get_count(
        const port_type_t port_type, const count_type_t count_type) override
    {
        const uint32_t count_addr_lo = [&]() {
            switch (port_type) {
                case SOURCE:
                    return count_type == LINES ? REG_SRC_LINE_CNT_LO : REG_SRC_PKT_CNT_LO;
                case SINK:
                    return count_type == LINES ? REG_SNK_LINE_CNT_LO : REG_SNK_PKT_CNT_LO;
                case LOOP:
                    return count_type == LINES ? REG_LOOP_LINE_CNT_LO
                                               : REG_LOOP_PKT_CNT_LO;
                default:
                    UHD_THROW_INVALID_CODE_PATH();
            }
        }();
        return regs().peek64(count_addr_lo);
    }

private:
    /*! Action API: Register a handler for stream commands
     */
    void register_issue_stream_cmd()
    {
        register_action_handler(ACTION_KEY_STREAM_CMD,
            [this](const res_source_info& src, action_info::sptr action) {
                stream_cmd_action_info::sptr stream_cmd_action =
                    std::dynamic_pointer_cast<stream_cmd_action_info>(action);
                if (!stream_cmd_action) {
                    throw uhd::runtime_error(
                        "Received stream_cmd of invalid action type!");
                }
                if (src.instance != 0 || src.type != res_source_info::OUTPUT_EDGE) {
                    throw uhd::runtime_error(
                        "The null source can only stream from output port 0!");
                }
                RFNOC_LOG_DEBUG("Received stream command action request!");
                issue_stream_cmd(stream_cmd_action->stream_cmd);
            });
    }

    void deinit() override
    {
        issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    }


    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! True if the source port 0 is producing data
    std::atomic_bool _streaming{false};

    //! Number of items per clock
    uint32_t _nipc;

    //! Bits per item
    uint32_t _item_width;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    null_block_control, 0x00000001, "NullSrcSink", CLOCK_KEY_GRAPH, "bus_clk")
