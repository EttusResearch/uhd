//
// Copyright 2015-2016 Ettus Research
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
#include "e3xx_radio_ctrl_impl.hpp"
#include "e300_defaults.hpp"
#include "e300_regs.hpp"
#include "../device3/device3_radio_regs.hpp"
#include <boost/make_shared.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::usrp::e300;
using namespace uhd::usrp::device3;
using uhd::usrp::dboard_iface;

//! mapping of frontend to radio perif index
static const size_t FE0 = 1;
static const size_t FE1 = 0;


/****************************************************************************
 * Structors
 ***************************************************************************/
UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(e3xx_radio_ctrl)
{
    UHD_RFNOC_BLOCK_TRACE() << "e3xx_radio_ctrl_impl::ctor() " << std::endl;

    ////////////////////////////////////////////////////////////////////
    // Set up peripherals
    ////////////////////////////////////////////////////////////////////
    for (size_t i = 0; i < _num_radios; i++) {
        wb_iface_adapter::sptr ctrl_iface_adapter = boost::make_shared<wb_iface_adapter>(
            // poke32 functor
            boost::bind(
                static_cast< void (block_ctrl_base::*)(const boost::uint32_t, const boost::uint32_t, const size_t) >(&block_ctrl_base::sr_write),
                this, _1, _2, i
            ),
            // peek32 functor
            boost::bind(
                static_cast< boost::uint32_t (block_ctrl_base::*)(const boost::uint32_t, const size_t port) >(&block_ctrl_base::user_reg_read32),
                this,
                _1, i
            ),
            // peek64 functor
            boost::bind(
                static_cast< boost::uint64_t (block_ctrl_base::*)(const boost::uint32_t, const size_t port) >(&block_ctrl_base::user_reg_read64),
                this,
                _1, i
            )
        );

        if (i == 0) {
            _spi = spi_core_3000::make(ctrl_iface_adapter, radio::sr_addr(radio::SPI), radio::RB_SPI);
            _spi->set_divider(6);
        }
        _atrs[i] = usrp::gpio_atr::gpio_atr_3000::make_write_only(ctrl_iface_adapter, radio::sr_addr(radio::GPIO));
    }

    ////////////////////////////////////////////////////////////////////
    // Time source
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::string>("time_source/value")
        .add_coerced_subscriber(boost::bind(&e3xx_radio_ctrl_impl::_update_time_source, this, _1))
        .set(DEFAULT_TIME_SRC);
#ifdef E300_GPSD
    static const std::vector<std::string> time_sources = boost::assign::list_of("none")("internal")("external")("gpsdo");
#else
    static const std::vector<std::string> time_sources = boost::assign::list_of("none")("internal")("external");
#endif
    _tree->create<std::vector<std::string> >("time_source/options").set(time_sources);

    ////////////////////////////////////////////////////////////////////
    // create RF frontend interfacing
    ////////////////////////////////////////////////////////////////////
    {
        const fs_path codec_path = fs_path("rx_codecs") / "A";
        _tree->create<std::string>(codec_path / "name").set("E3x0 RX dual ADC");
        _tree->create<int>(codec_path / "gains"); //empty cuz gains are in frontend
    }
    {
        const fs_path codec_path = fs_path("tx_codecs") / "A";
        _tree->create<std::string>(codec_path / "name").set("E3x0 TX dual DAC");
        _tree->create<int>(codec_path / "gains"); //empty cuz gains are in frontend
    }

    ////////////////////////////////////////////////////////////////////
    // internal gpios
    ////////////////////////////////////////////////////////////////////
    UHD_RFNOC_BLOCK_TRACE() << "  Creating internal GPIOs..." << std::endl;

    usrp::gpio_atr::gpio_atr_3000::sptr fp_gpio = usrp::gpio_atr::gpio_atr_3000::make(
        get_ctrl_iface(0),
        radio::sr_addr(radio::FP_GPIO),
        radio::RB_FP_GPIO
    );
    BOOST_FOREACH(const usrp::gpio_atr::gpio_attr_map_t::value_type attr, usrp::gpio_atr::gpio_attr_map)
    {
        _tree->create<boost::uint32_t>(fs_path("gpio") / "INT0" / attr.second)
            .add_coerced_subscriber(boost::bind(&usrp::gpio_atr::gpio_atr_3000::set_gpio_attr, fp_gpio, attr.first, _1))
            .set(0);
    }
    _tree->create<boost::uint8_t>(fs_path("gpio") / "INT0" / "READBACK")
        .set_publisher(boost::bind(&usrp::gpio_atr::gpio_atr_3000::read_gpio, fp_gpio));

    ////////////////////////////////////////////////////////////////////
    // Tick rate
    ////////////////////////////////////////////////////////////////////
    UHD_RFNOC_BLOCK_TRACE() << "  Setting tick rate..." << std::endl;
    _tree->access<double>("tick_rate")
        .add_coerced_subscriber(boost::bind(&e3xx_radio_ctrl_impl::set_rate, this, _1))
        .set_publisher(boost::bind(&e3xx_radio_ctrl_impl::get_rate, this))
    ;

}

/****************************************************************************
 * API calls
 ***************************************************************************/
double e3xx_radio_ctrl_impl::set_rate(double rate)
{
    UHD_MSG(status) << "Setting SPI divider to " << ceil(rate/AD9361_SPI_RATE) << "\n";
    _spi->set_divider(ceil(rate/AD9361_SPI_RATE)); // ceil() to prevent less than 1 rounding to 0
    UHD_MSG(status) << "Asking for clock rate " << rate/1e6 << " MHz\n";
    _tick_rate = _codec_ctrl->set_clock_rate(rate);
    UHD_MSG(status) << "Actually got clock rate " << _tick_rate/1e6 << " MHz\n";

    if (not check_radio_config()) {
        throw std::runtime_error(str(
            boost::format("[%s]: Invalid radio configuration.")
            % unique_id()
        ));
    }

    _time64->set_tick_rate(_tick_rate);
    for (size_t i = 0; i < _num_rx_channels; i++) {
        _perifs[i].framer->set_tick_rate(_tick_rate);
    }
    _time64->self_test();

    return _tick_rate;
}

/*! Select antenna \p for channel \p chan.
 */
void e3xx_radio_ctrl_impl::set_antenna(const std::string &ant, const size_t chan)
{
    if (ant != "TX/RX" and ant != "RX2")
        throw uhd::value_error("Unknown RX antenna option: " + ant);
    _antenna[chan] = ant;
    this->_update_atrs();
    this->_update_atr_leds(_perifs[chan].leds, ant);
}

double e3xx_radio_ctrl_impl::set_tx_frequency(const double freq, const size_t)
{
    return _tree->access<double>(fs_path("dboards/A/tx_frontends/A/freq/value")).set(freq).get();
}

double e3xx_radio_ctrl_impl::set_rx_frequency(const double freq, const size_t)
{
    return _tree->access<double>(fs_path("dboards/A/rx_frontends/A/freq/value")).set(freq).get();
}

double e3xx_radio_ctrl_impl::set_tx_gain(const double gain, const size_t chan)
{
    const std::string fe_side = (chan == 0) ? "A" : "B";
    double new_gain = _tree->access<double>(fs_path("dboards/A/tx_frontends/" + fe_side + "/gains/PGA/value")).set(gain).get();
    _tx_gain[chan] = new_gain;
    return new_gain;
}

double e3xx_radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
    const std::string fe_side = (chan == 0) ? "A" : "B";
    double new_gain = _tree->access<double>(fs_path("dboards/A/rx_frontends/" + fe_side + "/gains/PGA/value")).set(gain).get();
    _rx_gain[chan] = new_gain;
    return new_gain;
}

/****************************************************************************
 * Radio control and setup
 ***************************************************************************/
void e3xx_radio_ctrl_impl::setup_radio(uhd::usrp::ad9361_ctrl::sptr safe_codec_ctrl)
{
    if (_codec_ctrl) {
        throw std::runtime_error("Attempting to set up radio twice!");
    }

    ////////////////////////////////////////////////////////////////////
    // Create timed interface
    ////////////////////////////////////////////////////////////////////
    _codec_ctrl = safe_codec_ctrl;
    _codec_ctrl->set_timed_spi(_spi, 1);
    _codec_mgr = uhd::usrp::ad936x_manager::make(_codec_ctrl, _num_radios);

    ////////////////////////////////////////////////////////////////////
    // setup radios
    ////////////////////////////////////////////////////////////////////
    for (size_t chan = 0; chan < _num_radios; chan++) {
        _setup_radio_channel(chan);
    }
    // Loopback test
    for (size_t chan = 0; chan < _num_radios; chan++) {
        _codec_mgr->loopback_self_test(
            boost::bind(
                static_cast< void (block_ctrl_base::*)(const boost::uint32_t, const boost::uint32_t, const size_t) >(&block_ctrl_base::sr_write),
                this,
                radio::CODEC_IDLE, _1, chan
            ),
            boost::bind(
                static_cast< boost::uint64_t (block_ctrl_base::*)(const boost::uint32_t, const size_t port) >(&block_ctrl_base::user_reg_read64),
                this,
                radio::RB_CODEC_READBACK, chan
            )
        );
    }

    this->_update_enables();
}

static void gain_updater(std::map<size_t, double> &gain_map, const size_t index, double value)
{
    gain_map[index] = value;
}

void e3xx_radio_ctrl_impl::_setup_radio_channel(const size_t chan)
{
    const fs_path rx_dsp_path = fs_path("rx_dsps") / chan;
    _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd")
        .add_coerced_subscriber(boost::bind(&radio_ctrl_impl::issue_stream_cmd, this, _1, chan));

    ////////////////////////////////////////////////////////////////////
    // add some dummy nodes on the prop tree (FIXME remove these)
    ////////////////////////////////////////////////////////////////////
    const fs_path tx_dsp_path = fs_path("tx_dsps") / chan;
    _tree->create<double>(tx_dsp_path / "freq/value").set(0.0);
    _tree->create<meta_range_t>(tx_dsp_path / "freq/range").set(meta_range_t(0.0, 0.0, 0.0));
    _tree->create<double>(rx_dsp_path / "freq/value").set(0.0);
    _tree->create<meta_range_t>(rx_dsp_path / "freq/range").set(meta_range_t(0.0, 0.0, 0.0));
    _tree->create<double>(tx_dsp_path / "rate/value")
        .add_coerced_subscriber(boost::bind(&e3xx_radio_ctrl_impl::set_rate, this, _1))
        .set_publisher(boost::bind(&radio_ctrl_impl::get_rate, this))
    ;
    _tree->create<double>(rx_dsp_path / "rate/value")
        .add_coerced_subscriber(boost::bind(&e3xx_radio_ctrl_impl::set_rate, this, _1))
        .set_publisher(boost::bind(&radio_ctrl_impl::get_rate, this))
    ;

    ////////////////////////////////////////////////////////////////////
    // create RF frontend interfacing
    ////////////////////////////////////////////////////////////////////
    static const std::vector<direction_t> dirs = boost::assign::list_of(RX_DIRECTION)(TX_DIRECTION);
    BOOST_FOREACH(direction_t dir, dirs) {
        const std::string x = (dir == RX_DIRECTION) ? "rx" : "tx";
        const std::string key = boost::to_upper_copy(x) + std::string(((chan == FE0)? "1" : "2"));
        const fs_path rf_fe_path
            = fs_path("dboards") / "A" / (x + "_frontends") / ((chan == 0) ? "A" : "B");

        // This will connect all the AD936x-specific items
        _codec_mgr->populate_frontend_subtree(
            _tree->subtree(rf_fe_path), key, dir
        );

        // This will connect all the e3xx_radio_ctrl-specific items
        _tree->create<sensor_value_t>(rf_fe_path / "sensors" / "lo_locked")
            .set_publisher(boost::bind(&e3xx_radio_ctrl_impl::_get_fe_pll_lock, this, dir == TX_DIRECTION))
        ;
        _tree->access<double>(rf_fe_path / "freq" / "value")
            .add_coerced_subscriber(boost::bind(&e3xx_radio_ctrl_impl::_update_fe_lo_freq, this, key, _1))
        ;
        if (dir == RX_DIRECTION) {
            _tree->access<double>(rf_fe_path / "gains" / "PGA" / "value")
                .add_coerced_subscriber(boost::bind(&gain_updater, boost::ref(_rx_gain), chan, _1))
            ;
        }
        else if (dir == TX_DIRECTION) {
            _tree->access<double>(rf_fe_path / "gains" / "PGA" / "value")
                .add_coerced_subscriber(boost::bind(&gain_updater, boost::ref(_tx_gain), chan, _1))
            ;
        }

        // Antenna Setup
        if (dir == RX_DIRECTION) {
            static const std::vector<std::string> ants = boost::assign::list_of("TX/RX")("RX2");
            _tree->create<std::vector<std::string> >(rf_fe_path / "antenna" / "options").set(ants);
            _tree->create<std::string>(rf_fe_path / "antenna" / "value")
                .add_coerced_subscriber(boost::bind(&e3xx_radio_ctrl_impl::set_antenna, this, _1, chan))
                .set_publisher(boost::bind(&e3xx_radio_ctrl_impl::get_antenna, this, chan))
                .set("RX2");
        }
        else if (dir == TX_DIRECTION) {
            static const std::vector<std::string> ants(1, "TX/RX");
            _tree->create<std::vector<std::string> >(rf_fe_path / "antenna" / "options").set(ants);
            _tree->create<std::string>(rf_fe_path / "antenna" / "value").set("TX/RX");
        }
    }
}

void e3xx_radio_ctrl_impl::_reset_radio(void)
{
    _misc.radio_rst = 1;
    _update_gpio_state();
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    _misc.radio_rst = 0;
    _update_gpio_state();
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));
}

/****************************************************************************
 * Helpers
 ***************************************************************************/
bool e3xx_radio_ctrl_impl::check_radio_config()
{
    const size_t num_rx = _rx_streamers_active[0] + _rx_streamers_active[1];
    const size_t num_tx = _tx_streamers_active[0] + _tx_streamers_active[1];
    _enforce_tick_rate_limits(
        std::max(num_rx, num_tx),
        _tick_rate
    );

    this->_update_enables();
    return true;
}

void e3xx_radio_ctrl_impl::_enforce_tick_rate_limits(
        const size_t chan_count,
        const double tick_rate
) {
    const size_t max_chans = 2;
    if (chan_count > max_chans) {
        throw uhd::value_error(boost::str(
            boost::format("cannot not setup %d channels per direction (maximum is %d)")
                % chan_count
                % max_chans
        ));
    } else {
        const double max_tick_rate = uhd::usrp::ad9361_device_t::AD9361_MAX_CLOCK_RATE / ((chan_count <= 1) ? 1 : 2);
        if (tick_rate - max_tick_rate >= 1.0)
        {
            throw uhd::value_error(boost::str(
                boost::format("current master clock rate (%.6f MHz) exceeds maximum possible master clock rate (%.6f MHz) when using %d channels")
                    % (tick_rate/1e6)
                    % (max_tick_rate/1e6)
                    % chan_count
            ));
        }
        // TODO minimum rate check
    }
}

/****************************************************************************
 * Peripheral controls
 ***************************************************************************/
void e3xx_radio_ctrl_impl::_update_fe_lo_freq(const std::string &fe, const double freq)
{
    if (fe[0] == 'R') {
        for (size_t i = 0; i < _num_radios; i++) {
            _rx_freq[i] = freq;
        }
    }
    if (fe[0] == 'T') {
        for (size_t i = 0; i < _num_radios; i++) {
            _tx_freq[i] = freq;
        }
    }
    this->_update_atrs();
}

void e3xx_radio_ctrl_impl::_update_atrs(void)
{
    for (size_t instance = 0; instance < _num_radios; instance++)
    {
        // if we're not ready, no point ...
        if (not _atrs[instance])
            return;

        const bool rx_ant_rx2  = _antenna[instance] == "RX2";
        const double rx_freq = _rx_freq[instance];
        const double tx_freq = _tx_freq[instance];
        const bool rx_low_band = rx_freq < 2.6e9;
        const bool tx_low_band = tx_freq < 2940.0e6;

        // VCRX
        boost::uint32_t vcrx_v1_rxing = 1;
        boost::uint32_t vcrx_v2_rxing = 0;
        boost::uint32_t vcrx_v1_txing = 1;
        boost::uint32_t vcrx_v2_txing = 0;

        if (rx_low_band) {
            vcrx_v1_rxing = rx_ant_rx2 ? 0 : 1;
            vcrx_v2_rxing = rx_ant_rx2 ? 1 : 0;
            vcrx_v1_txing = 0;
            vcrx_v2_txing = 1;
        } else {
            vcrx_v1_rxing = rx_ant_rx2 ? 1 : 0;
            vcrx_v2_rxing = rx_ant_rx2 ? 0 : 1;
            vcrx_v1_txing = 1;
            vcrx_v2_txing = 0;
        }

        // VCTX
        boost::uint32_t vctxrx_v1_rxing = 0;
        boost::uint32_t vctxrx_v2_rxing = 1;
        boost::uint32_t vctxrx_v1_txing = 0;
        boost::uint32_t vctxrx_v2_txing = 1;

        if (tx_low_band) {
            vctxrx_v1_rxing = rx_ant_rx2 ? 1 : 0;
            vctxrx_v2_rxing = rx_ant_rx2 ? 0 : 1;
            vctxrx_v1_txing = 1;
            vctxrx_v2_txing = 0;
        } else {
            vctxrx_v1_rxing = rx_ant_rx2 ? 1 : 0;
            vctxrx_v2_rxing = rx_ant_rx2 ? 0 : 1;
            vctxrx_v1_txing = 1;
            vctxrx_v2_txing = 1;
        }

        //swapped for routing reasons, reswap it here
        if (instance == 1) {
            std::swap(vctxrx_v1_rxing, vctxrx_v2_rxing);
            std::swap(vctxrx_v1_txing, vctxrx_v2_txing);
        }

        boost::uint32_t tx_enable_a = (!tx_low_band) ? 1 : 0;
        boost::uint32_t tx_enable_b = (tx_low_band) ? 1 : 0;

        /* Set RX / TX band selects */
        boost::uint32_t rx_band_select_a = 0;
        boost::uint32_t rx_band_select_b = 0;
        boost::uint32_t rx_band_select_c = 0;
        boost::uint32_t tx_band_select = 0;

        if (instance == 0) {
            // RX
            if (rx_freq < 450e6) {
                rx_band_select_a = 5; // 3'b101
                rx_band_select_b = 0; // 2'bXX -- Don't care
                rx_band_select_c = 1; // 2'b01
            } else if (rx_freq < 700e6) {
                rx_band_select_a = 3; // 3'b011
                rx_band_select_b = 0; // 2'bXX -- Don't care
                rx_band_select_c = 3; // 2'b11
            } else if (rx_freq < 1200e6) {
                rx_band_select_a = 1; // 3'b001
                rx_band_select_b = 0; // 2'bXX -- Don't care
                rx_band_select_c = 2; // 2'b10
            } else if (rx_freq < 1800e6) {
                rx_band_select_a = 0; // 3'b000
                rx_band_select_b = 1; // 2'b01
                rx_band_select_c = 0; // 2'bXX -- Don't care
            } else if (rx_freq < 2350e6){
                rx_band_select_a = 2; // 3'b010
                rx_band_select_b = 3; // 2'b11
                rx_band_select_c = 0; // 2'bXX -- Don't care
            } else if (rx_freq < 2600e6){
                rx_band_select_a = 4; // 3'b100
                rx_band_select_b = 2; // 2'b10
                rx_band_select_c = 0; // 2'bXX -- Don't care
            } else { // >= 2600e6
                rx_band_select_a = 5; // 3'bXX -- Don't care
                rx_band_select_b = 0; // 2'bXX -- Don't care
                rx_band_select_c = 1; // 2'bXX -- Don't care
            }
        } else if (instance == 1) {
            if (rx_freq < 450e6) {
                rx_band_select_a = 4; // 3'b100
                rx_band_select_b = 0; // 2'bXX -- Don't care
                rx_band_select_c = 2; // 2'b10
            } else if (rx_freq < 700e6) {
                rx_band_select_a = 2; // 3'b010
                rx_band_select_b = 0; // 2'bXX -- Don't care
                rx_band_select_c = 3; // 2'b11
            } else if (rx_freq < 1200e6) {
                rx_band_select_a = 0; // 3'b000
                rx_band_select_b = 0; // 2'bXX -- Don't care
                rx_band_select_c = 1; // 2'b01
            } else if (rx_freq < 1800e6) {
                rx_band_select_a = 1; // 3'b001
                rx_band_select_b = 2; // 2'b10
                rx_band_select_c = 0; // 2'bXX -- Don't care
            } else if (rx_freq < 2350e6){
                rx_band_select_a = 3; // 3'b011
                rx_band_select_b = 3; // 2'b11
                rx_band_select_c = 0; // 2'bXX -- Don't care
            } else if (rx_freq < 2600e6){
                rx_band_select_a = 5; // 3'b101
                rx_band_select_b = 1; // 2'b01
                rx_band_select_c = 0; // 2'bXX -- Don't care
            } else { // >= 2600e6
                rx_band_select_a = 5; // 3'bXX -- Don't care
                rx_band_select_b = 0; // 2'bXX -- Don't care
                rx_band_select_c = 1; // 2'bXX -- Don't care
            }
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }

        // TX band selects are the same for both radio frontends
        if (tx_freq < 117.7e6)
            tx_band_select = 7; // 3'b111
        else if (tx_freq < 178.2e6)
            tx_band_select = 6; // 3'b110
        else if (tx_freq < 284.3e6)
            tx_band_select = 5; // 3'b101
        else if (tx_freq < 453.7e6)
            tx_band_select = 4; // 3'b100
        else if (tx_freq < 723.8e6)
            tx_band_select = 3; // 3'b011
        else if (tx_freq < 1154.9e6)
            tx_band_select = 2; // 3'b010
        else if (tx_freq < 1842.6e6)
            tx_band_select = 1; // 3'b001
        else if (tx_freq < 2940.0e6)
            tx_band_select = 0; // 3'b000
        else // > 2940.0e6
            tx_band_select = 7; // 3'bXXX -- Don't care, set to lowest band

        const boost::uint32_t rx_selects = 0
            | (vcrx_v1_rxing << VCRX_V1)
            | (vcrx_v2_rxing << VCRX_V2)
            | (vctxrx_v1_rxing << VCTXRX_V1)
            | (vctxrx_v2_rxing << VCTXRX_V2)
        ;
        const boost::uint32_t tx_selects = 0
            | (vcrx_v1_txing << VCRX_V1)
            | (vcrx_v2_txing << VCRX_V2)
            | (vctxrx_v1_txing << VCTXRX_V1)
            | (vctxrx_v2_txing << VCTXRX_V2)
        ;
        const boost::uint32_t tx_enables = 0
            | (tx_enable_a << TX_ENABLEA)
            | (tx_enable_b << TX_ENABLEB)
        ;
        const boost::uint32_t rxtx_band_selects = 0
            | (rx_band_select_a << RX_BANDSEL)
            | (rx_band_select_b << RXB_BANDSEL)
            | (rx_band_select_c << RXC_BANDSEL)
            | (tx_band_select << TX_BANDSEL)
        ;

        // Form register values;
        boost::uint32_t oo_reg = rx_selects | rxtx_band_selects;
        boost::uint32_t rx_reg = rx_selects | rxtx_band_selects;
        boost::uint32_t tx_reg = tx_selects | tx_enables | rxtx_band_selects;
        boost::uint32_t fd_reg = tx_selects | tx_enables | rxtx_band_selects; //tx selects dominate in fd mode

        //add tx enables based on fe enable
        tx_reg |= tx_enables;
        fd_reg |= tx_enables;

        usrp::gpio_atr::gpio_atr_3000::sptr atr = _atrs[instance];
        atr->set_atr_reg(usrp::gpio_atr::ATR_REG_IDLE, oo_reg);
        atr->set_atr_reg(usrp::gpio_atr::ATR_REG_RX_ONLY, rx_reg);
        atr->set_atr_reg(usrp::gpio_atr::ATR_REG_TX_ONLY, tx_reg);
        atr->set_atr_reg(usrp::gpio_atr::ATR_REG_FULL_DUPLEX, fd_reg);
    }
}

void e3xx_radio_ctrl_impl::_update_atr_leds(usrp::gpio_atr::gpio_atr_3000::sptr leds, const std::string &rx_ant)
{
    const bool is_txrx = (rx_ant == "TX/RX");
    const int rx_led = (1 << 2);
    const int tx_led = (1 << 1);
    const int txrx_led = (1 << 0);
    using namespace uhd::usrp::gpio_atr;
    leds->set_atr_reg(ATR_REG_IDLE, 0);
    leds->set_atr_reg(ATR_REG_RX_ONLY, is_txrx ? txrx_led : rx_led);
    leds->set_atr_reg(ATR_REG_TX_ONLY, tx_led);
    leds->set_atr_reg(ATR_REG_FULL_DUPLEX, rx_led | tx_led);
}

void e3xx_radio_ctrl_impl::_update_gpio_state(void)
{
    UHD_RFNOC_BLOCK_TRACE() << "e3xx_radio_ctrl_impl::_update_gpio_state() " << std::endl;
    boost::uint32_t misc_reg = 0
        | (_misc.pps_sel      << gpio_t::PPS_SEL)
        | (_misc.mimo         << gpio_t::MIMO)
        | (_misc.radio_rst    << gpio_t::RADIO_RST);
    _tree->access<boost::uint32_t>("global_regs/misc").set(misc_reg);
}

void e3xx_radio_ctrl_impl::_update_enables(void)
{
    UHD_RFNOC_BLOCK_TRACE() << "e3xx_radio_ctrl_impl::_update_enables() " << std::endl;
    if (not _codec_ctrl) {
        UHD_MSG(warning) << "Attempting to access CODEC controls before setting up the radios." << std::endl;
        return;
    }

    const size_t num_rx = _rx_streamers_active[0] + _rx_streamers_active[1];
    const size_t num_tx = _tx_streamers_active[0] + _tx_streamers_active[1];

    const bool mimo = (num_rx == 2) or (num_tx == 2);

    // This currently doesn't work with GNU Radio, so leave it uncommented
    //if ((num_tx + num_rx) == 3)
    //    throw uhd::runtime_error("e300: 2 RX 1 TX and 1 RX 2 TX configurations not possible");

    //setup the active chains in the codec
    if ((num_rx + num_tx) == 0) {
        // Ensure at least one RX chain is enabled so AD9361 outputs a sample clock
        _codec_ctrl->set_active_chains(false, false, true, false);
    } else {
        _codec_ctrl->set_active_chains(
                _tx_streamers_active[FE0],
                _tx_streamers_active[FE1],
                _rx_streamers_active[FE0],
                _rx_streamers_active[FE1]
        );
    }

    // Set radio data direction register cleared due to reset
    for (size_t instance = 0; instance < _num_radios; instance++)
    {
        _atrs[instance]->set_gpio_ddr(usrp::gpio_atr::DDR_OUTPUT, 0xFFFFFFFF);
    }

    //figure out if mimo is enabled based on new state
    _misc.mimo = (mimo) ? 1 : 0;
    _update_gpio_state();

    //atrs change based on enables
    _update_atrs();
}

void e3xx_radio_ctrl_impl::_update_time_source(const std::string &source)
{
    UHD_MSG(status) << boost::format("Setting time source to %s") % source << std::endl;
    if (source == "none" or source == "internal") {
        _misc.pps_sel = global_regs::PPS_INT;
#ifdef E300_GPSD
    } else if (source == "gpsdo") {
        _misc.pps_sel = global_regs::PPS_GPS;
#endif
    } else if (source == "external") {
        _misc.pps_sel = global_regs::PPS_EXT;
    } else {
        throw uhd::key_error("update_time_source: unknown source: " + source);
    }
    _update_gpio_state();
}

uhd::sensor_value_t e3xx_radio_ctrl_impl::_get_fe_pll_lock(const bool is_tx)
{
    const boost::uint32_t st = _tree->access<boost::uint32_t>("global_regs/pll").get();
    const bool locked = is_tx ? ((st & 0x1) > 0) : ((st & 0x2) > 0);
    return sensor_value_t("LO", locked, "locked", "unlocked");
}

/****************************************************************************
 * Register block
 ***************************************************************************/
UHD_RFNOC_BLOCK_REGISTER(e3xx_radio_ctrl, "E3XXRadio");
