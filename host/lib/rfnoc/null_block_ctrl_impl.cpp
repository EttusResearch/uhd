//
// Copyright 2014-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/rfnoc/null_block_ctrl.hpp>
#include <boost/format.hpp>

using namespace uhd::rfnoc;

class null_block_ctrl_impl : public null_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(null_block_ctrl)
    {
        // Register hooks for line_rate:
        _tree->access<int>(_root_path / "args" / 0 /  "line_rate" / "value")
            .add_coerced_subscriber([this](const int delay){
                this->set_line_delay_cycles(delay);
            })
            .update()
        ;
        // Register hooks for bpp:
        _tree->access<int>(_root_path / "args" / 0 /  "bpp" / "value")
            .add_coerced_subscriber([this](const int bpp){
                this->set_bytes_per_packet(bpp);
            })
            .update()
        ;
    }

    void set_line_delay_cycles(int cycles)
    {
        sr_write(SR_LINE_RATE, uint32_t(cycles));
    }

    void set_bytes_per_packet(int bpp)
    {
        sr_write(SR_LINES_PER_PACKET, uint32_t(bpp / BYTES_PER_LINE));
    }

    double set_line_rate(double rate, double clock_rate)
    {
        int cycs_between_lines = clock_rate / rate - 1;
        if (cycs_between_lines > 0xFFFF) {
            cycs_between_lines = 0xFFFF;
            UHD_LOGGER_WARNING(unique_id())
                << str(boost::format("Requested rate %f is larger than possible "
                                     "with the current clock rate (%.2f MHz).")
                       % rate % (clock_rate / 1e6))
                << std::endl;
        }
        cycs_between_lines = std::max(0, cycs_between_lines);
        set_arg<int>("line_rate", cycs_between_lines);
        return _line_rate_from_reg_val(cycs_between_lines, clock_rate);
    }

    double get_line_rate(double clock_rate) const
    {
        return _line_rate_from_reg_val(get_arg<int>("line_rate"), clock_rate);
    }

    double _line_rate_from_reg_val(uint32_t reg_val, double clock_rate) const
    {
        return clock_rate / (reg_val + 1);
    }

    void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, const size_t)
    {
        if (not stream_cmd.stream_now) {
            throw uhd::not_implemented_error("null_block does not support timed commands.");
        }
        switch (stream_cmd.stream_mode) {
            case uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
                sr_write(SR_ENABLE_STREAM, true);
                break;

            case uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS:
                sr_write(SR_ENABLE_STREAM, false);
                break;

            case uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE:
            case uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE:
                throw uhd::not_implemented_error("null_block does not support streaming modes other than CONTINUOUS");

            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_destination(
            uint32_t next_address,
            size_t output_block_port
    ) {
        uhd::sid_t sid(next_address);
        if (sid.get_src() == 0) {
            sid.set_src(get_address());
        }
        sr_write(SR_NEXT_DST_SID, sid.get(), output_block_port);
    }
};

UHD_RFNOC_BLOCK_REGISTER(null_block_ctrl, "NullSrcSink");

