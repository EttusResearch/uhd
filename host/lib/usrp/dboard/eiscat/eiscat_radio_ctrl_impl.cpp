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
#include <uhd/utils/math.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

static const double EISCAT_TICK_RATE = 208e6; // Hz
static const double EISCAT_RADIO_RATE = 104e6; // Hz
static const double EISCAT_CENTER_FREQ = 104e6; // Hz
static const double EISCAT_DEFAULT_GAIN = 0.0; // Hz
static const double EISCAT_DEFAULT_BANDWIDTH = 52e6; // Hz
static const char* EISCAT_ANTENNA_NAME = "Rx";
static const size_t EISCAT_NUM_CHANS = 5;
static const size_t EISCAT_NUM_FRONTENDS = 16;

UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(eiscat_radio_ctrl)
{
    UHD_LOG_TRACE("EISCAT", "eiscat_radio_ctrl_impl::ctor() ");
    const size_t num_chans = get_output_ports().size();
    UHD_ASSERT_THROW(num_chans == EISCAT_NUM_CHANS);
    UHD_LOG_TRACE("EISCAT", "Number of channels: " << num_chans);

    UHD_LOG_TRACE("EISCAT", "Setting tick rate to " << EISCAT_TICK_RATE/1e6 << " MHz");

    /**** Configure the radio itself ***************************************/
    radio_ctrl_impl::set_rate(EISCAT_TICK_RATE);
    for (size_t chan = 0; chan < num_chans; chan++) {
        radio_ctrl_impl::set_rx_frequency(EISCAT_CENTER_FREQ, chan);
        radio_ctrl_impl::set_rx_gain(EISCAT_DEFAULT_GAIN, chan);
        radio_ctrl_impl::set_rx_antenna(EISCAT_ANTENNA_NAME, chan);
        radio_ctrl_impl::set_rx_bandwidth(EISCAT_DEFAULT_BANDWIDTH, chan);
    }

    /**** Set up legacy compatible properties ******************************/
    // For use with multi_usrp APIs etc.
    // For legacy prop tree init:
    fs_path fe_path = fs_path("dboards") / "A" / "rx_frontends";

    // The EISCAT dboards have 16 frontends total, but they map to 5 channels
    // each through a matrix of FIR filters and summations. UHD will get much
    // less confused if we create 5 fake frontends, because that's also the
    // number of channels. Since we have no control over the frontends,
    // nothing is lost here.
    for (size_t fe_idx = 0; fe_idx < EISCAT_NUM_CHANS; fe_idx++) {
        _tree->create<std::string>(fe_path / fe_idx / "name")
            .set(str(boost::format("EISCAT Beam Contributions %d") % fe_idx))
        ;
        _tree->create<std::string>(fe_path / fe_idx / "connection")
            .set("I")
        ;
        _tree->create<std::string>(fe_path / fe_idx / "antenna" / "value")
            .set(EISCAT_ANTENNA_NAME)
            //.add_coerced_subscriber(boost::bind(&eiscat_radio_ctrl_impl::set_rx_antenna, this, _1, 0))
            ////.set_publisher(boost::bind(&radio_ctrl_impl::get_rx_antenna, this, 0))
            //.set_publisher([](){ return EISCAT_ANTENNA_NAME; })
        ;
        _tree->create<std::vector<std::string>>(fe_path / fe_idx / "antenna" / "options")
            .set(std::vector<std::string>(1, "Rx"))
        ;
        _tree->create<double>(fe_path / fe_idx / "freq" / "value")
            .set(EISCAT_CENTER_FREQ)
            //.set_coercer(boost::bind(&eiscat_radio_ctrl_impl::set_rx_frequency, this, _1, 0))
            ////.set_publisher(boost::bind(&radio_ctrl_impl::get_rx_frequency, this, 0))
        ;
        _tree->create<meta_range_t>(fe_path / fe_idx / "freq" / "range")
            .set(meta_range_t(EISCAT_CENTER_FREQ, EISCAT_CENTER_FREQ))
        ;
        _tree->create<double>(fe_path / fe_idx / "gains" / "null" / "value")
            .set(EISCAT_DEFAULT_GAIN)
            //.set_coercer(boost::bind(&eiscat_radio_ctrl_impl::set_rx_gain, this, _1, 0))
            //.set_publisher(boost::bind(&radio_ctrl_impl::get_rx_gain, this, 0))
        ;
        _tree->create<meta_range_t>(fe_path / fe_idx / "gains" / "null" / "range")
            .set(meta_range_t(EISCAT_DEFAULT_GAIN, EISCAT_DEFAULT_GAIN))
        ;
        _tree->create<double>(fe_path / fe_idx / "bandwidth" / "value")
            .set(EISCAT_DEFAULT_BANDWIDTH)
            //.set_coercer(boost::bind(&eiscat_radio_ctrl_impl::set_rx_bandwidth, this, _1, 0))
            //.set_publisher(boost::bind(&radio_ctrl_impl::get_rx_bandwidth, this, 0))
        ;
        _tree->create<meta_range_t>(fe_path / fe_idx / "bandwidth" / "range")
            .set(meta_range_t(EISCAT_DEFAULT_BANDWIDTH, EISCAT_DEFAULT_BANDWIDTH))
        ;
        _tree->create<bool>(fe_path / fe_idx / "use_lo_offset")
            .set(false)
        ;
    }

    // We can actually stream data to an EISCAT board, so it needs some tx
    // frontends too:
    fe_path = fs_path("dboards") / "A" / "tx_frontends";
    for (size_t fe_idx = 0; fe_idx < EISCAT_NUM_CHANS; fe_idx++) {
        _tree->create<std::string>(fe_path / fe_idx / "name")
            .set(str(boost::format("EISCAT Uplink %d") % fe_idx))
        ;
    }

    // There is only ever one EISCAT radio per dboard, so this should be unset
    // when we reach this line:
    UHD_ASSERT_THROW(not _tree->exists("tick_rate"));
    _tree->create<double>("tick_rate")
        //.set_coercer(boost::bind(&eiscat_radio_ctrl_impl::set_rate, this, _1))
        .set(EISCAT_TICK_RATE)
    ;

    if (not _tree->exists(fs_path("clock_source/value"))) {
        _tree->create<std::string>(fs_path("clock_source/value")).set("external");
    }

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
    return boost::lexical_cast<size_t>(fe);
}

std::string eiscat_radio_ctrl_impl::get_dboard_fe_from_chan(const size_t chan, const uhd::direction_t dir)
{
    return std::to_string(chan);
}

double eiscat_radio_ctrl_impl::get_output_samp_rate(size_t /* port */)
{
    return EISCAT_RADIO_RATE;
}

bool eiscat_radio_ctrl_impl::check_radio_config()
{
    UHD_RFNOC_BLOCK_TRACE() << "x300_radio_ctrl_impl::check_radio_config() " ;
    const fs_path rx_fe_path = fs_path("dboards/A/rx_frontends");
    uint32_t chan_enables = 0;
    for (const auto &enb: _rx_streamer_active) {
        if (enb.second) {
            chan_enables |= (1<<enb.first);
        }
    }
    UHD_LOG_TRACE("EISCAT", str(boost::format("check_radio_config(): Setting channel enables to 0x%02X") % chan_enables));
    sr_write("SR_RX_STREAM_ENABLE", chan_enables);

    return true;
}

UHD_RFNOC_BLOCK_REGISTER(eiscat_radio_ctrl, "EISCATRadio");
