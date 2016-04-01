//
// Copyright 2014-2015 Ettus Research LLC
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

#include "wb_iface_adapter.hpp"
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/direction.hpp>
#include "radio_ctrl_impl.hpp"
#include "../../transport/super_recv_packet_handler.hpp"

using namespace uhd;
using namespace uhd::rfnoc;

static const size_t BYTES_PER_SAMPLE = 4;

/****************************************************************************
 * Structors and init
 ***************************************************************************/
// Note: block_ctrl_base must be called before this, but has to be called by
// the derived class because of virtual inheritance
radio_ctrl_impl::radio_ctrl_impl() :
    _tick_rate(rfnoc::rate_node_ctrl::RATE_UNDEFINED)
{
    _num_rx_channels = get_output_ports().size();
    _num_tx_channels = get_input_ports().size();

    for (size_t i = 0; i < _num_rx_channels; i++) {
        _rx_streamers_active[i] = false;
    }
    for (size_t i = 0; i < _num_tx_channels; i++) {
        _tx_streamers_active[i] = false;
    }

    /////////////////////////////////////////////////////////////////////////
    // Setup peripherals
    /////////////////////////////////////////////////////////////////////////
    for (size_t i = 0; i < _get_num_radios(); i++) {
        _register_loopback_self_test(i);
        _perifs[i].ctrl = boost::make_shared<wb_iface_adapter>(
            // poke32 functor
            boost::bind(
                static_cast< void (block_ctrl_base::*)(const boost::uint32_t, const boost::uint32_t, const size_t) >(&block_ctrl_base::sr_write),
                this, _1, _2, i
            ),
            // peek32 functor
            boost::bind(
                static_cast< boost::uint32_t (block_ctrl_base::*)(const boost::uint32_t, const size_t) >(&block_ctrl_base::user_reg_read32),
                this,
                _1, i
            ),
            // peek64 functor
            boost::bind(
                static_cast< boost::uint64_t (block_ctrl_base::*)(const boost::uint32_t, const size_t) >(&block_ctrl_base::user_reg_read64),
                this,
                _1, i
            ),
            // get_time functor
            boost::bind(
                static_cast< time_spec_t (block_ctrl_base::*)(const size_t) >(&block_ctrl_base::get_command_time),
                this, i
            ),
            // set_time functor
            boost::bind(
                static_cast< void (block_ctrl_base::*)(const time_spec_t&, const size_t) >(&block_ctrl_base::set_command_time),
                this,
                _1, i
            )
        );

        _perifs[i].framer = rx_vita_core_3000::make(_perifs[i].ctrl, regs::sr_addr(regs::RX_CTRL));
        _perifs[i].deframer = tx_vita_core_3000::make(_perifs[i].ctrl, regs::sr_addr(uhd::rfnoc::SR_ERROR_POLICY));

        // FIXME there's currently no way to set the underflow policy, which would be set here:
        _perifs[i].framer->setup(stream_args_t());
        _perifs[i].deframer->setup(stream_args_t());

        if (i == 0) {
            time_core_3000::readback_bases_type time64_rb_bases;
            time64_rb_bases.rb_now = regs::RB_TIME_NOW;
            time64_rb_bases.rb_pps = regs::RB_TIME_PPS;
            _time64 = time_core_3000::make(_perifs[i].ctrl, regs::sr_addr(regs::TIME), time64_rb_bases);
            _time64->set_time_now(0.0);
        }
    }

    ////////////////////////////////////////////////////////////////////
    // Register the time keeper
    ////////////////////////////////////////////////////////////////////
    if (not _tree->exists(fs_path("time") / "now")) {
        _tree->create<time_spec_t>(fs_path("time") / "now")
            .set_publisher(boost::bind(&time_core_3000::get_time_now, _time64))
            .add_coerced_subscriber(boost::bind(&time_core_3000::set_time_now, _time64, _1))
        ;
    }
    if (not _tree->exists(fs_path("time") / "pps")) {
        _tree->create<time_spec_t>(fs_path("time") / "pps")
            .set_publisher(boost::bind(&time_core_3000::get_time_last_pps, _time64))
            .add_coerced_subscriber(boost::bind(&time_core_3000::set_time_next_pps, _time64, _1))
        ;
    }

    if (not _tree->exists(fs_path("time") / "cmd")) {
        _tree->create<time_spec_t>(fs_path("time") / "cmd");
        for (size_t i = 0; i < _get_num_radios(); i++) {
            _tree->access<time_spec_t>("time/cmd")
                .add_coerced_subscriber(boost::bind(&block_ctrl_base::set_command_tick_rate, this, boost::ref(_tick_rate), i))
                .add_coerced_subscriber(boost::bind(&block_ctrl_base::set_command_time, this, _1, i))
            ;
        }
    }
    // spp gets created in the XML file
    _tree->access<int>(get_arg_path("spp") / "value")
        .add_coerced_subscriber(boost::bind(&radio_ctrl_impl::_update_spp, this, _1))
        .update()
    ;
}

void radio_ctrl_impl::_register_loopback_self_test(size_t chan)
{
    UHD_MSG(status) << "[RFNoC Radio] Performing register loopback test... " << std::flush;
    size_t hash = size_t(time(NULL));
    for (size_t i = 0; i < 100; i++)
    {
        boost::hash_combine(hash, i);
        sr_write(regs::TEST, boost::uint32_t(hash), chan);
        boost::uint32_t result = user_reg_read32(regs::RB_TEST, chan);
        if (result != boost::uint32_t(hash)) {
            UHD_MSG(status) << "fail" << std::endl;
            UHD_MSG(status) << boost::format("expected: %x result: %x") % boost::uint32_t(hash) % result << std::endl;
            return; // exit on any failure
        }
    }
    UHD_MSG(status) << "pass" << std::endl;
}

/****************************************************************************
 * API calls
 ***************************************************************************/
double radio_ctrl_impl::set_rate(double rate)
{
    boost::mutex::scoped_lock lock(_mutex);
    _tick_rate = rate;
    _time64->set_tick_rate(_tick_rate);
    for (size_t i = 0; i < _num_rx_channels; i++) {
        _perifs[i].framer->set_tick_rate(_tick_rate);
    }
    _time64->self_test();
    return _tick_rate;
}

void radio_ctrl_impl::set_tx_antenna(const std::string &ant, const size_t chan)
{
    _tx_antenna[chan] = ant;
}

void radio_ctrl_impl::set_rx_antenna(const std::string &ant, const size_t chan)
{
    _rx_antenna[chan] = ant;
}

double radio_ctrl_impl::set_tx_frequency(const double freq, const size_t chan)
{
    return _tx_freq[chan] = freq;
}

double radio_ctrl_impl::set_rx_frequency(const double freq, const size_t chan)
{
    return _rx_freq[chan] = freq;
}

double radio_ctrl_impl::set_tx_gain(const double gain, const size_t chan)
{
    return _tx_gain[chan] = gain;
}

double radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
    return _rx_gain[chan] = gain;
}

void radio_ctrl_impl::set_time_sync(const uhd::time_spec_t &time)
{
    _time64->set_time_sync(time);
}

double radio_ctrl_impl::get_rate() const
{
    return _tick_rate;
}

std::string radio_ctrl_impl::get_tx_antenna(const size_t chan) /* const */
{
    return _tx_antenna[chan];
}

std::string radio_ctrl_impl::get_rx_antenna(const size_t chan) /* const */
{
    return _rx_antenna[chan];
}

double radio_ctrl_impl::get_tx_frequency(const size_t chan) /* const */
{
    return _tx_freq[chan];
}

double radio_ctrl_impl::get_rx_frequency(const size_t chan) /* const */
{
    return _rx_freq[chan];
}

double radio_ctrl_impl::get_tx_gain(const size_t chan) /* const */
{
    return _tx_gain[chan];
}

double radio_ctrl_impl::get_rx_gain(const size_t chan) /* const */
{
    return _rx_gain[chan];
}

/***********************************************************************
 * RX Streamer-related methods (from source_block_ctrl_base)
 **********************************************************************/
//! Pass stream commands to the radio
void radio_ctrl_impl::issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, const size_t chan)
{
    boost::mutex::scoped_lock lock(_mutex);
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::issue_stream_cmd() " << chan << std::endl;
    _perifs[chan].framer->issue_stream_command(stream_cmd);
}

void radio_ctrl_impl::handle_overrun(boost::weak_ptr<uhd::rx_streamer> streamer, const size_t port)
{
    UHD_MSG(status) << "radio_ctrl_impl::handle_overrun()" << std::endl;
    boost::shared_ptr<transport::sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<transport::sph::recv_packet_streamer>(streamer.lock());
    if (not my_streamer) return; //If the rx_streamer has expired then overflow handling makes no sense.

    //find out if we were in continuous mode before stopping
    const bool in_continuous_streaming_mode = _perifs[port].framer->in_continuous_streaming_mode();

    if (my_streamer->get_num_channels() == 1 and in_continuous_streaming_mode) {
        issue_stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS, port);
        return;
    }

    /////////////////////////////////////////////////////////////
    // MIMO overflow recovery time
    /////////////////////////////////////////////////////////////
    //stop streaming on all channels
    for (size_t i = 0; i < my_streamer->get_num_channels(); i++) {
        // clear command FIFO to ensure we stop streaming
        sr_write(regs::RX_CTRL_CLEAR_CMDS, 0, i);
        issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS, i);
    }
    //flush transports
    my_streamer->flush_all(0.001);
    //restart streaming on all channels
    if (in_continuous_streaming_mode)
    {
        stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = _time64->get_time_now() + time_spec_t(0.05);
        for (size_t i = 0; i < my_streamer->get_num_channels(); i++) {
            issue_stream_cmd(stream_cmd, i);
        }
    }
}

/***********************************************************************
 * Radio controls (radio_ctrl specific)
 **********************************************************************/
void radio_ctrl_impl::set_rx_streamer(bool active, const size_t port)
{
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::set_rx_streamer() " << port << " -> " << active << std::endl;
    if (port > _num_rx_channels) {
        throw uhd::value_error(str(
            boost::format("[%s] Can't (un)register RX streamer on port %d (invalid port)")
            % unique_id() % port
        ));
    }
    _rx_streamers_active[port] = active;
    if (not check_radio_config()) {
        throw std::runtime_error(str(
            boost::format("[%s]: Invalid radio configuration.")
            % unique_id()
        ));
    }
}

void radio_ctrl_impl::set_tx_streamer(bool active, const size_t port)
{
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::set_tx_streamer() " << port << " -> " << active << std::endl;
    if (port > _num_tx_channels) {
        throw uhd::value_error(str(
            boost::format("[%s] Can't (un)register TX streamer on port %d (invalid port)")
            % unique_id() % port
        ));
    }
    _tx_streamers_active[port] = active;
    if (not check_radio_config()) {
        throw std::runtime_error(str(
            boost::format("[%s]: Invalid radio configuration.")
            % unique_id()
        ));
    }
}

// Subscribers to block args:
// TODO move to nocscript
void radio_ctrl_impl::_update_spp(int spp)
{
    boost::mutex::scoped_lock lock(_mutex);
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::_update_spp(): Requested spp: " << spp << std::endl;
    if (spp == 0) {
        spp = DEFAULT_PACKET_SIZE / BYTES_PER_SAMPLE;
    }
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::_update_spp(): Setting spp to: " << spp << std::endl;
    for (size_t i = 0; i < _num_rx_channels; i++) {
        if (_perifs[i].framer)
            _perifs[i].framer->set_nsamps_per_packet(size_t(spp));
    }
}

