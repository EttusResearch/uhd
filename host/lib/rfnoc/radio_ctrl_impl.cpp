//
// Copyright 2014-2016 Ettus Research LLC
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
#include <boost/foreach.hpp>
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
    _continuous_streaming = std::vector<bool>(2, false);

    for (size_t i = 0; i < _num_rx_channels; i++) {
        _rx_streamer_active[i] = false;
    }
    for (size_t i = 0; i < _num_tx_channels; i++) {
        _tx_streamer_active[i] = false;
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

        // FIXME there's currently no way to set the underflow policy

        if (i == 0) {
            time_core_3000::readback_bases_type time64_rb_bases;
            time64_rb_bases.rb_now = regs::RB_TIME_NOW;
            time64_rb_bases.rb_pps = regs::RB_TIME_PPS;
            _time64 = time_core_3000::make(_perifs[i].ctrl, regs::sr_addr(regs::TIME), time64_rb_bases);
            this->set_time_now(0.0);
        }

        //Reset the RX control engine
        sr_write(regs::RX_CTRL_HALT, 1, i);
    }

    ////////////////////////////////////////////////////////////////////
    // Register the time keeper
    ////////////////////////////////////////////////////////////////////
    if (not _tree->exists(fs_path("time") / "now")) {
        _tree->create<time_spec_t>(fs_path("time") / "now")
            .set_publisher(boost::bind(&radio_ctrl_impl::get_time_now, this))
        ;
    }
    if (not _tree->exists(fs_path("time") / "pps")) {
        _tree->create<time_spec_t>(fs_path("time") / "pps")
            .set_publisher(boost::bind(&radio_ctrl_impl::get_time_last_pps, this))
        ;
    }
    if (not _tree->exists(fs_path("time") / "cmd")) {
        _tree->create<time_spec_t>(fs_path("time") / "cmd");
    }
    _tree->access<time_spec_t>(fs_path("time") / "now")
        .add_coerced_subscriber(boost::bind(&radio_ctrl_impl::set_time_now, this, _1))
    ;
    _tree->access<time_spec_t>(fs_path("time") / "pps")
        .add_coerced_subscriber(boost::bind(&radio_ctrl_impl::set_time_next_pps, this, _1))
    ;
    for (size_t i = 0; i < _get_num_radios(); i++) {
        _tree->access<time_spec_t>("time/cmd")
            .add_coerced_subscriber(boost::bind(&block_ctrl_base::set_command_tick_rate, this, boost::ref(_tick_rate), i))
            .add_coerced_subscriber(boost::bind(&block_ctrl_base::set_command_time, this, _1, i))
        ;
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
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::issue_stream_cmd() " << chan << " " << char(stream_cmd.stream_mode) << std::endl;
    if (not _is_streamer_active(uhd::RX_DIRECTION, chan)) {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::issue_stream_cmd() called on inactive channel. Skipping." << std::endl;
        return;
    }
    UHD_ASSERT_THROW(stream_cmd.num_samps <= 0x0fffffff);
    _continuous_streaming[chan] = (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

    //setup the mode to instruction flags
    typedef boost::tuple<bool, bool, bool, bool> inst_t;
    static const uhd::dict<stream_cmd_t::stream_mode_t, inst_t> mode_to_inst = boost::assign::map_list_of
                                                            //reload, chain, samps, stop
        (stream_cmd_t::STREAM_MODE_START_CONTINUOUS,   inst_t(true,  true,  false, false))
        (stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS,    inst_t(false, false, false, true))
        (stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE, inst_t(false, false, true,  false))
        (stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE, inst_t(false, true,  true,  false))
    ;

    //setup the instruction flag values
    bool inst_reload, inst_chain, inst_samps, inst_stop;
    boost::tie(inst_reload, inst_chain, inst_samps, inst_stop) = mode_to_inst[stream_cmd.stream_mode];

    //calculate the word from flags and length
    boost::uint32_t cmd_word = 0;
    cmd_word |= boost::uint32_t((stream_cmd.stream_now)? 1 : 0) << 31;
    cmd_word |= boost::uint32_t((inst_chain)?            1 : 0) << 30;
    cmd_word |= boost::uint32_t((inst_reload)?           1 : 0) << 29;
    cmd_word |= boost::uint32_t((inst_stop)?             1 : 0) << 28;
    cmd_word |= (inst_samps)? stream_cmd.num_samps : ((inst_stop)? 0 : 1);

    //issue the stream command
    const boost::uint64_t ticks = (stream_cmd.stream_now)? 0 : stream_cmd.time_spec.to_ticks(get_rate());
    sr_write(regs::RX_CTRL_CMD, cmd_word, chan);
    sr_write(regs::RX_CTRL_TIME_HI, boost::uint32_t(ticks >> 32), chan);
    sr_write(regs::RX_CTRL_TIME_LO, boost::uint32_t(ticks >> 0),  chan); //latches the command
}

std::vector<size_t> radio_ctrl_impl::get_active_rx_ports()
{
    std::vector<size_t> active_rx_ports;
    typedef std::map<size_t, bool> map_t;
    BOOST_FOREACH(map_t::value_type &m, _rx_streamer_active) {
        if (m.second) {
            active_rx_ports.push_back(m.first);
        }
    }
    return active_rx_ports;
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
    _rx_streamer_active[port] = active;
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
    _tx_streamer_active[port] = active;
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
        sr_write(regs::RX_CTRL_MAXLEN, uint32_t(spp), i);
    }
}

void radio_ctrl_impl::set_time_now(const time_spec_t &time_spec)
{
    _time64->set_time_now(time_spec);
}

void radio_ctrl_impl::set_time_next_pps(const time_spec_t &time_spec)
{
    _time64->set_time_next_pps(time_spec);
}


time_spec_t radio_ctrl_impl::get_time_now()
{
    return _time64->get_time_now();
}

time_spec_t radio_ctrl_impl::get_time_last_pps()
{
    return _time64->get_time_last_pps();
}

