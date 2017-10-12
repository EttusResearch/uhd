//
// Copyright 2017 Ettus Research
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

#include "magnesium_radio_ctrl_impl.hpp"

#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/types/direction.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <sstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

static const size_t IO_MASTER_RADIO = 0;

// TODO: some of these values are duplicated in ad937x_device.cpp
static const double MAGNESIUM_TICK_RATE = 125e6; // Hz
static const double MAGNESIUM_RADIO_RATE = 125e6; // Hz
static const double MAGNESIUM_MIN_FREQ = 300e6; // Hz
static const double MAGNESIUM_MAX_FREQ = 6e9; // Hz
static const double MAGNESIUM_MIN_RX_GAIN = 0.0; // dB
static const double MAGNESIUM_MAX_RX_GAIN = 30.0; // dB
static const double MAGNESIUM_RX_GAIN_STEP = 0.5;
static const double MAGNESIUM_MIN_TX_GAIN = 0.0; // dB
static const double MAGNESIUM_MAX_TX_GAIN = 41.95; // dB
static const double MAGNESIUM_TX_GAIN_STEP = 0.05;
static const double MAGNESIUM_CENTER_FREQ = 2.5e9; // Hz
static const double MAGNESIUM_DEFAULT_GAIN = 0.0; // dB
static const double MAGNESIUM_DEFAULT_BANDWIDTH = 40e6; // Hz TODO: fix
static const size_t MAGNESIUM_NUM_TX_CHANS = 2;
static const size_t MAGNESIUM_NUM_RX_CHANS = 2;

std::string _get_which(direction_t dir, size_t chan)
{
    std::stringstream ss;
    switch (dir)
    {
    case RX_DIRECTION:
        ss << "RX";
        break;
    case TX_DIRECTION:
        ss << "TX";
        break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }

    switch (chan)
    {
    case 0:
        ss << "1";
        break;
    case 1:
        ss << "2";
        break;
    default:
        throw uhd::runtime_error("invalid channel number");
    }

    return ss.str();
}

fs_path magnesium_radio_ctrl_impl::_get_fe_path(size_t chan, direction_t dir)
{
    switch (dir)
    {
        case TX_DIRECTION:
            return fs_path("dboards" / _radio_slot / "tx_frontends" / get_dboard_fe_from_chan(chan, TX_DIRECTION));
        case RX_DIRECTION:
            return fs_path("dboards" / _radio_slot / "rx_frontends" / get_dboard_fe_from_chan(chan, RX_DIRECTION));
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(magnesium_radio_ctrl)
{
    UHD_LOG_TRACE("MAGNESIUM", "magnesium_radio_ctrl_impl::ctor() ");
    _radio_slot = (get_block_id().get_block_count() == IO_MASTER_RADIO) ? "A" : "B";
    _slot_prefix = (get_block_id().get_block_count() == IO_MASTER_RADIO) ? "db_0_" : "db_1_";
    UHD_LOG_TRACE("MAGNESIUM", "Radio slot: " << _radio_slot);


    const size_t num_rx_chans = get_output_ports().size();
    //UHD_ASSERT_THROW(num_rx_chans == MAGNESIUM_NUM_RX_CHANS);
    const size_t num_tx_chans = get_input_ports().size();
    //UHD_ASSERT_THROW(num_tx_chans == MAGNESIUM_NUM_TX_CHANS);

    UHD_LOG_TRACE("MAGNESIUM", "Setting tick rate to " << MAGNESIUM_TICK_RATE / 1e6 << " MHz");
    radio_ctrl_impl::set_rate(MAGNESIUM_TICK_RATE);

    for (size_t chan = 0; chan < num_rx_chans; chan++) {
        radio_ctrl_impl::set_rx_frequency(MAGNESIUM_CENTER_FREQ, chan);
        radio_ctrl_impl::set_rx_gain(MAGNESIUM_DEFAULT_GAIN, chan);
        // TODO: fix antenna name
        radio_ctrl_impl::set_rx_antenna(str(boost::format("RX%d") % (chan+1)), chan);
        radio_ctrl_impl::set_rx_bandwidth(MAGNESIUM_DEFAULT_BANDWIDTH, chan);
    }

    for (size_t chan = 0; chan < num_tx_chans; chan++) {
        radio_ctrl_impl::set_tx_frequency(MAGNESIUM_CENTER_FREQ, chan);
        radio_ctrl_impl::set_tx_gain(MAGNESIUM_DEFAULT_GAIN, chan);
        // TODO: fix antenna name
        radio_ctrl_impl::set_tx_antenna(str(boost::format("TX%d") % (chan + 1)), chan);
    }

    /**** Set up legacy compatible properties ******************************/
    // For use with multi_usrp APIs etc.
    // For legacy prop tree init:
    // TODO: determine DB number
    const fs_path fe_base = fs_path("dboards") / _radio_slot;
    const std::vector<uhd::direction_t> dir({ RX_DIRECTION, TX_DIRECTION });
    const std::vector<std::string> fe({ "rx_frontends", "tx_frontends" });
    const std::vector<std::string> ant({ "RX" , "TX" });
    const std::vector<size_t> num_chans({ MAGNESIUM_NUM_RX_CHANS , MAGNESIUM_NUM_TX_CHANS });
    const size_t RX_IDX = 0, TX_IDX = 1;

    for (size_t fe_idx = 0; fe_idx < fe.size(); ++fe_idx)
    {
        const fs_path fe_direction_path = fe_base / fe[fe_idx];
        for (size_t chan = 0; chan < num_chans[fe_idx]; ++chan)
        {
            const fs_path fe_path = fe_direction_path / chan;
            UHD_LOG_TRACE("MAGNESIUM", "Adding FE at " << fe_path);
            // Shared TX/RX attributes
            _tree->create<std::string>(fe_path / "name")
                .set(str(boost::format("Magnesium %s %d") % ant[fe_idx] % chan))
                ;
            _tree->create<std::string>(fe_path / "connection")
                .set("IQ")
                ;
            {
                // TODO: fix antenna name
                // Now witness the firepower of this fully armed and operational lambda
                auto dir_ = dir[fe_idx];
                auto coerced_lambda = [this, chan, dir_](const std::string &ant)
                {
                    return this->_set_antenna(ant, chan, dir_);
                };
                auto publisher_lambda = [this, chan, dir_]()
                {
                    return this->_get_antenna(chan, dir_);
                };
                _tree->create<std::string>(fe_path / "antenna" / "value")
                    .set(str(boost::format("%s%d") % ant[fe_idx] % (chan + 1)))
                    .add_coerced_subscriber(coerced_lambda)
                    .set_publisher(publisher_lambda);
                // TODO: fix options
                _tree->create<std::vector<std::string>>(fe_path / "antenna" / "options")
                    .set(std::vector<std::string>(1, str(boost::format("%s%d") % ant[fe_idx] % (chan + 1))));
            }
            {
                auto dir_ = dir[fe_idx];
                auto coerced_lambda = [this, chan, dir_](const double freq)
                {
                    return this->_set_frequency(freq, chan, dir_);
                };
                auto publisher_lambda = [this, chan, dir_]()
                {
                    return this->_get_frequency(chan, dir_);
                };
                _tree->create<double>(fe_path / "freq" / "value")
                    .set(MAGNESIUM_CENTER_FREQ)
                    .set_coercer(coerced_lambda)
                    .set_publisher(publisher_lambda);
                _tree->create<meta_range_t>(fe_path / "freq" / "range")
                    .set(meta_range_t(MAGNESIUM_MIN_FREQ, MAGNESIUM_MAX_FREQ));
            }
            {
                auto dir_ = dir[fe_idx];
                auto coerced_lambda = [this, chan, dir_](const double gain)
                {
                    return this->_set_gain(gain, chan, dir_);
                };
                auto publisher_lambda = [this, chan, dir_]()
                {
                    return this->_get_gain(chan, dir_);
                };
                auto min_gain = (fe_idx == RX_IDX) ? MAGNESIUM_MIN_RX_GAIN : MAGNESIUM_MIN_TX_GAIN;
                auto max_gain = (fe_idx == RX_IDX) ? MAGNESIUM_MAX_RX_GAIN : MAGNESIUM_MAX_TX_GAIN;
                auto gain_step = (fe_idx == RX_IDX) ? MAGNESIUM_RX_GAIN_STEP : MAGNESIUM_TX_GAIN_STEP;
                // TODO: change from null
                _tree->create<double>(fe_path / "gains" / "null" / "value")
                    .set(MAGNESIUM_DEFAULT_GAIN)
                    .set_coercer(coerced_lambda)
                    .set_publisher(publisher_lambda);
                _tree->create<meta_range_t>(fe_path / "gains" / "null" / "range")
                    .set(meta_range_t(min_gain, max_gain, gain_step));
            }
            // TODO: set up read/write of bandwidth properties correctly
            if (fe_idx == RX_IDX)
            {
                auto coerced_lambda = [this, chan](const double bw)
                {
                    return this->set_rx_bandwidth(bw, chan);
                };
                auto publisher_lambda = [this, chan]()
                {
                    return this->get_rx_bandwidth(chan);
                };
                _tree->create<double>(fe_path / "bandwidth" / "value")
                    .set(MAGNESIUM_DEFAULT_BANDWIDTH)
                    .set_coercer(coerced_lambda)
                    .set_publisher(publisher_lambda);
            }
            else {
                _tree->create<double>(fe_path / "bandwidth" / "value")
                    .set(MAGNESIUM_DEFAULT_BANDWIDTH);
            }
            _tree->create<meta_range_t>(fe_path / "bandwidth" / "range")
                .set(meta_range_t(MAGNESIUM_DEFAULT_BANDWIDTH, MAGNESIUM_DEFAULT_BANDWIDTH));
        }
    }

    // TODO change codec names
    _tree->create<int>("rx_codecs" / _radio_slot / "gains");
    _tree->create<int>("tx_codecs" / _radio_slot / "gains");
    _tree->create<std::string>("rx_codecs" / _radio_slot / "name").set("AD9361 Dual ADC");
    _tree->create<std::string>("tx_codecs" / _radio_slot / "name").set("AD9361 Dual DAC");

    // TODO remove this dirty hack
    if (not _tree->exists("tick_rate"))
    {
        _tree->create<double>("tick_rate").set(MAGNESIUM_TICK_RATE);
    }
}

magnesium_radio_ctrl_impl::~magnesium_radio_ctrl_impl()
{
    UHD_LOG_TRACE("MAGNESIUM", "magnesium_radio_ctrl_impl::dtor() ");
}


double magnesium_radio_ctrl_impl::set_rate(double rate)
{
    // TODO: implement
    if (rate != get_rate()) {
        UHD_LOG_WARNING("MAGNESIUM", "Attempting to set sampling rate to invalid value " << rate);
    }
    return get_rate();
}

void magnesium_radio_ctrl_impl::set_tx_antenna(const std::string &ant, const size_t chan)
{
    _set_antenna(ant, chan, TX_DIRECTION);
}

void magnesium_radio_ctrl_impl::set_rx_antenna(const std::string &ant, const size_t chan)
{
    _set_antenna(ant, chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_tx_frequency(const double freq, const size_t chan)
{
    return _set_frequency(freq, chan, TX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_rx_frequency(const double freq, const size_t chan)
{
    return _set_frequency(freq, chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    return _set_bandwidth(bandwidth, chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_tx_gain(const double gain, const size_t chan)
{
    return _set_gain(gain, chan, TX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
    return _set_gain(gain, chan, RX_DIRECTION);
}

std::string magnesium_radio_ctrl_impl::get_tx_antenna(const size_t chan) /* const */
{
    return _get_antenna(chan, TX_DIRECTION);
}

std::string magnesium_radio_ctrl_impl::get_rx_antenna(const size_t chan) /* const */
{
    return _get_antenna(chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_tx_frequency(const size_t chan) /* const */
{
    return _get_frequency(chan, TX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_rx_frequency(const size_t chan) /* const */
{
    return _get_frequency(chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_tx_gain(const size_t chan) /* const */
{
    return _get_gain(chan, TX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_rx_gain(const size_t chan) /* const */
{
    return _get_gain(chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::get_rx_bandwidth(const size_t chan) /* const */
{
    return _get_bandwidth(chan, RX_DIRECTION);
}

size_t magnesium_radio_ctrl_impl::get_chan_from_dboard_fe(const std::string &fe, const direction_t dir)
{
    // UHD_LOG_TRACE("MAGNESIUM", "get_chan_from_dboard_fe " << fe << " returns " << boost::lexical_cast<size_t>(fe));
    return boost::lexical_cast<size_t>(fe);
}

std::string magnesium_radio_ctrl_impl::get_dboard_fe_from_chan(const size_t chan, const direction_t dir)
{
    // UHD_LOG_TRACE("MAGNESIUM", "get_dboard_fe_from_chan " << chan << " returns " << std::to_string(chan));
    return std::to_string(chan);
}

double magnesium_radio_ctrl_impl::get_output_samp_rate(size_t port)
{
    return 125e6;
}

void magnesium_radio_ctrl_impl::set_rpc_client(
    uhd::rpc_client::sptr rpcc,
    const uhd::device_addr_t &block_args
)
{
    _rpcc = rpcc;
    _block_args = block_args;
}

double magnesium_radio_ctrl_impl::_set_frequency(const double freq, const size_t chan, const direction_t dir)
{
    // TODO: there is only one LO per RX or TX, so changing frequency will affect the adjacent channel in the same direction
    // Update the adjacent channel when this occurs
    auto which = _get_which(dir, chan);
    UHD_LOG_TRACE("MAGNESIUM", "calling " << _slot_prefix << "set_freq on " << which << " with " << freq);
    auto retval = _rpcc->request_with_token<double>(_slot_prefix + "set_freq", which, freq, false);
    UHD_LOG_TRACE("MAGNESIUM", _slot_prefix << "set_freq returned " << retval);
    return retval;
}

double magnesium_radio_ctrl_impl::_set_gain(const double gain, const size_t chan, const direction_t dir)
{
    auto which = _get_which(dir, chan);
    UHD_LOG_TRACE("MAGNESIUM", "calling " << _slot_prefix << "set_gain on " << which << " with " << gain);
    auto retval = _rpcc->request_with_token<double>(_slot_prefix + "set_gain", which, gain);
    UHD_LOG_TRACE("MAGNESIUM", _slot_prefix << "set_gain returned " << retval);
    return retval;
}

void magnesium_radio_ctrl_impl::_set_antenna(const std::string &ant, const size_t chan, const direction_t dir)
{
    // TODO: implement
    UHD_LOG_WARNING("MAGNESIUM", "Ignoring attempt to set antenna");
    // CPLD control?
}

double magnesium_radio_ctrl_impl::_set_bandwidth(const double bandwidth, const size_t chan, const direction_t dir)
{
    // TODO: implement
    UHD_LOG_WARNING("MAGNESIUM", "Ignoring attempt to set bandwidth");
    return get_rx_bandwidth(chan);
}

double magnesium_radio_ctrl_impl::_get_frequency(const size_t chan, const direction_t dir)
{
    auto which = _get_which(dir, chan);
    UHD_LOG_TRACE("MAGNESIUM", "calling " << _slot_prefix << "get_freq on " << which);
    auto retval = _rpcc->request_with_token<double>(_slot_prefix + "get_freq", which);
    UHD_LOG_TRACE("MAGNESIUM", _slot_prefix << "get_freq returned " << retval);
    return retval;
}

double magnesium_radio_ctrl_impl::_get_gain(const size_t chan, const direction_t dir)
{
    auto which = _get_which(dir, chan);
    UHD_LOG_TRACE("MAGNESIUM", "calling " << _slot_prefix << "get_gain on " << which);
    auto retval = _rpcc->request_with_token<double>(_slot_prefix + "get_gain", which);
    UHD_LOG_TRACE("MAGNESIUM", _slot_prefix << "get_gain returned " << retval);
    return retval;
}

std::string magnesium_radio_ctrl_impl::_get_antenna(const size_t chan, const direction_t dir)
{
    // TODO: implement
    UHD_LOG_WARNING("MAGNESIUM", "Ignoring attempt to get antenna");
    return "RX1";
    // CPLD control?
}

double magnesium_radio_ctrl_impl::_get_bandwidth(const size_t chan, const direction_t dir)
{
    // TODO: implement
    UHD_LOG_WARNING("MAGNESIUM", "Ignoring attempt to get bandwidth");
    return MAGNESIUM_DEFAULT_BANDWIDTH;
}

UHD_RFNOC_BLOCK_REGISTER(magnesium_radio_ctrl, "MagnesiumRadio");
