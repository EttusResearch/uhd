//
// Copyright 2017 Ettus Research (National Instruments)
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

#pragma once

#include "config/ad937x_config_t.hpp"
#include "config/ad937x_fir.hpp"
#include "config/ad937x_gain_ctrl_config.hpp"
#include "mpm/ad937x/adi_ctrl.hpp"
#include "ad937x_device_types.hpp"
#include "mpm/ad937x/ad937x_ctrl_types.hpp"
#include "adi/t_mykonos.h"
#include "adi/t_mykonos_gpio.h"

#include <uhd/exception.hpp>

#include <boost/noncopyable.hpp>
#include <memory>
#include <functional>

class ad937x_device : public boost::noncopyable
{
public:
    enum class gain_mode_t { MANUAL, AUTOMATIC, HYBRID };
    enum class pll_t {CLK_SYNTH, RX_SYNTH, TX_SYNTH, SNIFF_SYNTH, CALPLL_SDM};

    ad937x_device(uhd::spi_iface::sptr iface, mpm::ad937x::gpio::gain_pins_t gain_pins);

    uint8_t get_product_id();
    uint8_t get_device_rev();
    mpm::ad937x::device::api_version_t get_api_version();
    mpm::ad937x::device::arm_version_t get_arm_version();

    double set_bw_filter(uhd::direction_t direction, mpm::ad937x::device::chain_t chain, double value);
    double set_gain(uhd::direction_t direction, mpm::ad937x::device::chain_t chain, double value);
    void set_agc_mode(uhd::direction_t direction, gain_mode_t mode);
    double set_clock_rate(double value);
    void enable_channel(uhd::direction_t direction, mpm::ad937x::device::chain_t chain, bool enable);
    double tune(uhd::direction_t direction, double value);
    double get_freq(uhd::direction_t direction);

    bool get_pll_lock_status(pll_t pll);

    void set_fir(uhd::direction_t direction, mpm::ad937x::device::chain_t chain, int8_t gain, const std::vector<int16_t> & fir);
    std::vector<int16_t> get_fir(uhd::direction_t direction, mpm::ad937x::device::chain_t chain, int8_t &gain);

    int16_t get_temperature();

    void set_enable_gain_pins(uhd::direction_t direction, mpm::ad937x::device::chain_t chain, bool enable);
    void set_gain_pin_step_sizes(uhd::direction_t direction, mpm::ad937x::device::chain_t chain, double inc_step, double dec_step);

    const static double MIN_FREQ;
    const static double MAX_FREQ;
    const static double MIN_RX_GAIN;
    const static double MAX_RX_GAIN;
    const static double RX_GAIN_STEP;
    const static double MIN_TX_GAIN;
    const static double MAX_TX_GAIN;
    const static double TX_GAIN_STEP;

private:
    ad9371_spiSettings_t full_spi_settings;
    ad937x_config_t mykonos_config;
    ad937x_gain_ctrl_config_t gain_ctrl;

    void _initialize();
    void _load_arm(std::vector<uint8_t> & binary);
    void _run_initialization_calibrations();
    void _start_jesd();
    void _enable_tracking_calibrations();

    void _apply_gain_pins(uhd::direction_t direction, mpm::ad937x::device::chain_t chain);

    void _call_api_function(std::function<mykonosErr_t()> func);
    void _call_gpio_api_function(std::function<mykonosGpioErr_t()> func);

    static uint8_t _convert_rx_gain(double gain);
    static uint16_t _convert_tx_gain(double gain);
};
