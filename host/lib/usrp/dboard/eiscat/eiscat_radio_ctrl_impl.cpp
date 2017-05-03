//
// Copyright 2017 Ettus Research (National Instruments Corp.)
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

#include "eiscat_radio_ctrl_impl.hpp"

#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/utils/math.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

static const size_t IO_MASTER_RADIO = 0;
static const double EISCAT_TICK_RATE = 208e6; // Hz
static const double EISCAT_RADIO_RATE = 104e6; // Hz
static const double EISCAT_CENTER_FREQ = 104e6; // Hz
static const double EISCAT_DEFAULT_GAIN = 0.0; // Hz
static const double EISCAT_DEFAULT_BANDWIDTH = 52e6; // Hz
static const std::string EISCAT_ANTENNA_NAME = "Rx";
static const size_t EISCAT_NUM_CHANS = 5;

UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(eiscat_radio_ctrl)
{
    UHD_LOG_TRACE("EISCAT", "eiscat_radio_ctrl_impl::ctor() ");
    _radio_type = (get_block_id().get_block_count() == IO_MASTER_RADIO) ? PRIMARY : SECONDARY;
    _radio_slot = (get_block_id().get_block_count() == IO_MASTER_RADIO) ? "A" : "B";
    UHD_LOG_TRACE("EISCAT", "Radio slot: " << _radio_slot);
    const size_t num_chans = get_output_ports().size();
    UHD_ASSERT_THROW(num_chans == EISCAT_NUM_CHANS);
    UHD_LOG_TRACE("EISCAT", "Number of channels: " << num_chans);

    UHD_LOG_TRACE("EISCAT", "Setting tick rate to " << EISCAT_TICK_RATE/1e6 << " MHz");
    radio_ctrl_impl::set_rate(EISCAT_TICK_RATE);

    // For legacy prop tree init:
    fs_path fe_path = fs_path("dboards" / _radio_slot / "rx_frontends");

    // Init parent class
    for (size_t chan = 0; chan < num_chans; chan++) {
        radio_ctrl_impl::set_rx_frequency(EISCAT_CENTER_FREQ, chan);
        radio_ctrl_impl::set_rx_gain(EISCAT_DEFAULT_GAIN, chan);
        radio_ctrl_impl::set_rx_antenna(EISCAT_ANTENNA_NAME, chan);
        radio_ctrl_impl::set_rx_bandwidth(EISCAT_DEFAULT_BANDWIDTH, chan);

        // Legacy prop tree paths (for use with multi_usrp API)
        _tree->access<std::string>(fe_path / chan / "antenna" / "value")
            .add_coerced_subscriber(boost::bind(&eiscat_radio_ctrl_impl::set_rx_antenna, this, _1, chan))
            .set_publisher(boost::bind(&radio_ctrl_impl::get_rx_antenna, this, chan))
        ;
        _tree->access<double>(fe_path / chan / "freq" / "value")
            .set_coercer(boost::bind(&eiscat_radio_ctrl_impl::set_rx_frequency, this, _1, chan))
            .set_publisher(boost::bind(&radio_ctrl_impl::get_rx_frequency, this, chan))
        ;
        _tree->access<double>(fe_path / chan / "gain" / "value")
            .set_coercer(boost::bind(&eiscat_radio_ctrl_impl::set_rx_gain, this, _1, chan))
            .set_publisher(boost::bind(&radio_ctrl_impl::get_rx_gain, this, chan))
        ;
        _tree->access<double>(fe_path / chan / "bandwidth" / "value")
            .set_coercer(boost::bind(&eiscat_radio_ctrl_impl::set_rx_bandwidth, this, _1, chan))
            .set_publisher(boost::bind(&radio_ctrl_impl::get_rx_bandwidth, this, chan))
        ;
        // TODO: Add ranges or options for all of these. Not high-prio.
    }

    UHD_HERE();
    UHD_VAR((_tree->exists(fs_path("time/cmd"))));
}

eiscat_radio_ctrl_impl::~eiscat_radio_ctrl_impl()
{
    UHD_LOG_TRACE("EISCAT", "eiscat_radio_ctrl_impl::dtor() ");
}

void eiscat_radio_ctrl_impl::set_tx_antenna(const std::string &, const size_t)
{
    throw uhd::runtime_error("Cannot set Tx antenna on EISCAT daughterboard");
}

void eiscat_radio_ctrl_impl::set_rx_antenna(const std::string & /* ant */, const size_t /* chan */)
{
    UHD_LOG_WARNING("EISCAT", "Ignoring attempt to set Rx antenna");
}

double eiscat_radio_ctrl_impl::get_tx_frequency(const size_t /* chan */)
{
    UHD_LOG_WARNING("EISCAT", "Ignoring attempt to read Tx frequency");
    return 0.0;
}

double eiscat_radio_ctrl_impl::set_tx_frequency(const double /* freq */, const size_t /* chan */)
{
    throw uhd::runtime_error("Cannot set Tx frequency on EISCAT daughterboard");
}

double eiscat_radio_ctrl_impl::set_rx_frequency(const double freq, const size_t chan)
{
    if (freq != get_rx_frequency(chan)) {
        UHD_LOG_WARNING("EISCAT", "Ignoring attempt to set Rx frequency");
    }
    return get_rx_frequency(chan);
}

double eiscat_radio_ctrl_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    if (bandwidth != get_rx_bandwidth(chan)) {
        UHD_LOG_WARNING("EISCAT", "Ignoring attempt to set Rx bandwidth");
    }
    return get_rx_bandwidth(chan);
}


double eiscat_radio_ctrl_impl::set_tx_gain(const double /* gain */, const size_t /* chan */)
{
    throw uhd::runtime_error("Cannot set Tx gain on EISCAT daughterboard");
}

double eiscat_radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
    // TODO: Add ability to set digital gain or make it explicit this function is not supported.
    if (gain != get_rx_gain(chan)) {
        UHD_LOG_WARNING("EISCAT", "Ignoring attempt to set Rx gain.");
    }
    return get_rx_gain(chan);
}

double eiscat_radio_ctrl_impl::set_rate(double rate)
{
    if (rate != get_rate()) {
        UHD_LOG_WARNING("EISCAT", "Attempting to set sampling rate to invalid value " << rate);
    }
    return get_rate();
}

size_t eiscat_radio_ctrl_impl::get_chan_from_dboard_fe(const std::string &fe, const uhd::direction_t dir)
{
    if (dir != uhd::direction_t::RX_DIRECTION) {
        throw uhd::runtime_error("Unable to get chan from fe, EISCAT only has Rx frontends");
    }

    // A:0 -> A:7, B:0 -> B:7
    return boost::lexical_cast<size_t>(fe.substr(2));
}

std::string eiscat_radio_ctrl_impl::get_dboard_fe_from_chan(const size_t chan, const uhd::direction_t dir)
{
    if (dir != uhd::direction_t::RX_DIRECTION) {
        throw uhd::runtime_error("Unable to get fe from chan, EISCAT only has Rx frontends");
    }

    // A:0 -> A:7, B:0 -> B:7
    return _radio_slot + ':' + std::to_string(chan);
}

double eiscat_radio_ctrl_impl::get_output_samp_rate(size_t /* port */)
{
    return EISCAT_RADIO_RATE;
}

UHD_RFNOC_BLOCK_REGISTER(eiscat_radio_ctrl, "EISCATRadio");
