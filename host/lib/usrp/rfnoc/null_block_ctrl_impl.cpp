//
// Copyright 2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/usrp/rfnoc/null_block_ctrl.hpp>

using namespace uhd::rfnoc;

class null_block_ctrl_impl : public null_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(null_block_ctrl),
        _line_delay_cycles(0xFFFF) // This is set implicitly by the subscriber in the prop tree
    {
        // Add prop tree entry for line rate
        // We actually use _line_rate to store the rate, because we can't
        // read back the actual register (yet)
        UHD_MSG(status) << "populating " << _root_path / "line_delay_cycles/value" << std::endl;
        _tree->create<boost::uint32_t>(_root_path / "line_delay_cycles/value")
            .subscribe(boost::bind(&null_block_ctrl_impl::set_line_delay_cycles, this, _1))
            .set(0xFFFF); // Default: slowest rate possible
    }

    void set_line_delay_cycles(boost::uint32_t cycles)
    {
        _line_delay_cycles = cycles;
        sr_write(SR_LINE_RATE, cycles);
    }

    double set_line_rate(double rate, double clock_rate)
    {
        int cycs_between_lines = clock_rate / rate - 1;
        if (cycs_between_lines > 0xFFFF) {
            cycs_between_lines = 0xFFFF;
            UHD_MSG(warning)
                << str(boost::format("null_block_ctrl: Requested rate %f is larger than possible with the current clock rate (%.2f MHz).") % rate % (clock_rate / 1e6))
                << std::endl;
        }
        boost::uint32_t register_value = std::max(0, cycs_between_lines);
        sr_write(SR_LINE_RATE, register_value);
        return _line_rate_from_reg_val(register_value, clock_rate);
    }

    double get_line_rate(double clock_rate) const
    {
        return _line_rate_from_reg_val(_line_delay_cycles, clock_rate);
    }

    double _line_rate_from_reg_val(boost::uint32_t reg_val, double clock_rate) const
    {
        return clock_rate / (reg_val + 1);
    }

    void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd)
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

    bool set_output_signature(const stream_sig_t &out_sig_, size_t block_port)
    {
        stream_sig_t out_sig = out_sig_;
        boost::uint32_t lines_per_packet = DEFAULT_LINES_PER_PACKET;
        if (out_sig.packet_size) {
            lines_per_packet = std::max<size_t>(out_sig.packet_size / BYTES_PER_LINE, 1);;
        }
        out_sig.packet_size = lines_per_packet * BYTES_PER_LINE;

        if (block_ctrl_base::set_output_signature(out_sig, block_port)) {
            sr_write(SR_LINES_PER_PACKET, lines_per_packet);
            return true;
        }

        return false;
    }

    void set_destination(
            boost::uint32_t next_address,
            UHD_UNUSED(size_t output_block_port)
    ) {
        uhd::sid_t sid(next_address);
        if (sid.get_src() == 0) {
            sid.set_src(get_address());
        }
        sr_write(SR_NEXT_DST_BASE, sid.get());
    }

private:

    //! Store the line delay cycles. TODO remove once we have readback from the block to query this.
    size_t _line_delay_cycles;

};

UHD_RFNOC_BLOCK_REGISTER(null_block_ctrl, "NullSrcSink");

