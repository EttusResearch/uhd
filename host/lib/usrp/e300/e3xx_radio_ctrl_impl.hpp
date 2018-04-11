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

#ifndef INCLUDED_LIBUHD_RFNOC_E3XX_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_E3XX_RADIO_CTRL_IMPL_HPP

#include "e300_global_regs.hpp"
#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <uhdlib/usrp/common/ad9361_ctrl.hpp>
#include <uhdlib/usrp/common/ad936x_manager.hpp>
#include <uhdlib/rfnoc/radio_ctrl_impl.hpp>
#include <uhd/usrp/gpio_defs.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to an E3XX radio.
 */
class e3xx_radio_ctrl_impl : public radio_ctrl_impl
{
public:
    /************************************************************************
     * Structors
     ***********************************************************************/
    UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR_DECL(e3xx_radio_ctrl)
    virtual ~e3xx_radio_ctrl_impl();

    /************************************************************************
     * API calls
     ***********************************************************************/
    double set_rate(double rate);
    void set_rx_antenna(const std::string &ant, const size_t chan);

    double set_tx_frequency(const double freq, const size_t chan);
    double set_rx_frequency(const double freq, const size_t chan);

    double set_tx_gain(const double gain, const size_t chan);
    double set_rx_gain(const double gain, const size_t chan);
    double set_rx_bandwidth(const double bandwidth, const size_t chan);

    double get_tx_gain(const size_t chan);
    double get_rx_gain(const size_t chan);
    double get_rx_bandwidth(const size_t chan);

    std::vector<std::string> get_gpio_banks() const;
    void set_gpio_attr(const std::string &bank, const std::string &attr, const uint32_t value, const uint32_t mask);
    uint32_t get_gpio_attr(const std::string &bank, const std::string &attr);

    size_t get_chan_from_dboard_fe(const std::string &fe, const direction_t);
    std::string get_dboard_fe_from_chan(const size_t chan, const direction_t);

    /************************************************************************
     * RFIC setup and control
     ***********************************************************************/
    /*! Set up the radio. No API calls may be made before this one.
     */
    void setup_radio(uhd::usrp::ad9361_ctrl::sptr safe_codec_ctrl);

private:
    void _setup_radio_channel(const size_t chan);
    void _reset_radio(void);

protected:
    /************************************************************************
     * Helpers
     ***********************************************************************/
    virtual bool check_radio_config();
    void _enforce_tick_rate_limits(const size_t chans, const double tick_rate);

private:
    /************************************************************************
     * Peripheral controls
     ***********************************************************************/
    void _update_fe_lo_freq(const std::string &fe, const double freq);
    void _update_atrs(void);
    void _update_atr_leds(uhd::usrp::gpio_atr::gpio_atr_3000::sptr leds, const std::string &rx_ant);

    void _update_gpio_state(void);
    void _update_enables(void);

    void _update_time_source(const std::string &source);

    // get frontend lock sensor
    uhd::sensor_value_t _get_fe_pll_lock(const bool is_tx);

    /************************************************************************
     * Internal GPIO control
     ***********************************************************************/
    struct gpio_t
    {
        gpio_t() : pps_sel(uhd::usrp::e300::global_regs::PPS_INT),
            mimo(0), radio_rst(0), tx_bandsels(0),
            rx_bandsel_a(0), rx_bandsel_b(0), rx_bandsel_c(0)
        {}

        uint32_t pps_sel;
        uint32_t mimo;
        uint32_t radio_rst;

        uint32_t tx_bandsels;
        uint32_t rx_bandsel_a;
        uint32_t rx_bandsel_b;
        uint32_t rx_bandsel_c;

        static const size_t PPS_SEL     = 0;
        static const size_t MIMO        = 2;
        static const size_t RADIO_RST   = 3;
        static const size_t TX_BANDSEL  = 4;
        static const size_t RX_BANDSELA = 7;
        static const size_t RX_BANDSELB = 13;
        static const size_t RX_BANDSELC = 17;
    };
    uint8_t _get_internal_gpio(uhd::usrp::gpio_atr::gpio_atr_3000::sptr);

private: // members
    struct e3xx_perifs_t
    {
        usrp::gpio_atr::gpio_atr_3000::sptr      atr;
        uhd::usrp::gpio_atr::gpio_atr_3000::sptr leds;
    };
    //! SPI to talk to the AD936x
    spi_core_3000::sptr                    _spi;
    //! One ATR per channel
    std::map<size_t, e3xx_perifs_t>        _e3xx_perifs;
    //! AD936x controls
    uhd::usrp::ad9361_ctrl::sptr           _codec_ctrl;
    uhd::usrp::ad936x_manager::sptr        _codec_mgr;
    gpio_t                                 _misc;

}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_E3XX_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:
