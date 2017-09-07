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

#include "ad937x_device.hpp"
#include "adi/mykonos.h"
#include "mpm/ad937x/ad937x_ctrl.hpp"
#include <mpm/exception.hpp>

#include <sstream>
#include <set>
#include <functional>
#include <iostream>
#include <algorithm>

using namespace mpm::chips;
using namespace mpm::ad937x::device;

static uhd::direction_t _get_direction_from_antenna(const std::string& antenna)
{
    auto sub = antenna.substr(0, 2);
    if (sub == "RX") {
        return uhd::direction_t::RX_DIRECTION;
    }
    else if (sub == "TX") {
        return uhd::direction_t::TX_DIRECTION;
    }
    else {
        throw mpm::runtime_error("ad937x_ctrl got an invalid channel string.");
    }
    return uhd::direction_t::RX_DIRECTION;
}

static chain_t _get_chain_from_antenna(const std::string& antenna)
{
    auto sub = antenna.substr(2, 1);
    if (sub == "1") {
        return chain_t::ONE;
    }
    else if (sub == "2") {
        return chain_t::TWO;
    }
    else {
        throw mpm::runtime_error("ad937x_ctrl got an invalid channel string.");
    }
    return chain_t::ONE;
}

std::set<size_t> _get_valid_fir_lengths(const std::string& which)
{
    auto dir = _get_direction_from_antenna(which);
    switch (dir)
    {
    case uhd::direction_t::RX_DIRECTION:
        return{ 24, 48, 72 };
    case uhd::direction_t::TX_DIRECTION:
        return{ 16, 32, 48, 64, 80, 96 };
    default:
        MPM_THROW_INVALID_CODE_PATH();
        return std::set<size_t>();
    }
}

uhd::meta_range_t _get_valid_rx_gain_steps()
{
    // 0-7 step size is valid, in 0.5 dB increments
    return uhd::meta_range_t(0, 3.5, 0.5);
}

uhd::meta_range_t _get_valid_tx_gain_steps()
{
    // 0-31 step size is valid, in 0.05 dB increments
    return uhd::meta_range_t(0, 1.55, 0.05);
}

uhd::meta_range_t ad937x_ctrl::get_rf_freq_range(void)
{
    return uhd::meta_range_t(ad937x_device::MIN_FREQ, ad937x_device::MAX_FREQ);
}


uhd::meta_range_t ad937x_ctrl::get_bw_filter_range(void)
{
    // TODO: fix
    return uhd::meta_range_t(0, 1);
}

std::vector<double> ad937x_ctrl::get_clock_rates(void)
{
    // TODO: fix
    return { 125e6 };
}

uhd::meta_range_t ad937x_ctrl::get_gain_range(const std::string &which)
{
    auto dir = _get_direction_from_antenna(which);
    switch (dir)
    {
    case uhd::direction_t::RX_DIRECTION:
        return uhd::meta_range_t(ad937x_device::MIN_RX_GAIN, ad937x_device::MAX_RX_GAIN, ad937x_device::RX_GAIN_STEP);
    case uhd::direction_t::TX_DIRECTION:
        return uhd::meta_range_t(ad937x_device::MIN_TX_GAIN, ad937x_device::MAX_TX_GAIN, ad937x_device::TX_GAIN_STEP);
    default:
        MPM_THROW_INVALID_CODE_PATH();
        return uhd::meta_range_t();
    }
}

class ad937x_ctrl_impl : public ad937x_ctrl
{
public:
    ad937x_ctrl_impl(
        std::shared_ptr<std::mutex> spi_mutex,
        mpm::types::regs_iface::sptr iface,
        mpm::ad937x::gpio::gain_pins_t gain_pins) :
        spi_mutex(spi_mutex),
        device(iface.get(), gain_pins),
        _iface(iface)
    {
        /* nop */
    }

    virtual void begin_initialization()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.begin_initialization();
    }

    virtual void finish_initialization()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.finish_initialization();
    }

    virtual void start_jesd_rx()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.start_jesd_rx();
    }

    virtual void start_jesd_tx()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.start_jesd_tx();
    }

    virtual void start_radio()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.start_radio();
    }

    virtual void stop_radio()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.stop_radio();
    }

    virtual uint8_t get_multichip_sync_status()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_multichip_sync_status();
    }

    virtual uint8_t get_framer_status()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_framer_status();
    }

    virtual uint8_t get_deframer_status()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_deframer_status();
    }

    virtual uint16_t get_ilas_config_match()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_ilas_config_match();
    }

    virtual void enable_jesd_loopback(uint8_t enable)
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.enable_jesd_loopback(enable);
    }

    virtual uint8_t get_product_id()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_product_id();
    }

    virtual uint8_t get_device_rev()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_device_rev();
    }
    virtual std::string get_api_version()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        auto api = device.get_api_version();
        std::ostringstream ss;
        ss  << api.silicon_ver << "."
            << api.major_ver << "."
            << api.minor_ver << "."
            << api.build_ver;

        return ss.str();
    }

    virtual std::string get_arm_version()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        auto arm = device.get_arm_version();
        std::ostringstream ss;
        ss  << arm.major_ver << "."
            << arm.minor_ver << "."
            << arm.rc_ver;

        switch (arm.build_type)
        {
        case mpm::ad937x::device::build_type_t::RELEASE:
            ss << " Release";
            break;
        case mpm::ad937x::device::build_type_t::DEBUG:
            ss << " Debug";
            break;
        case mpm::ad937x::device::build_type_t::TEST_OBJECT:
            ss << " Test Object";
            break;
        }

        return ss.str();
    }

    virtual double set_bw_filter(const std::string &which, double value)
    {
        // TODO implement
        return double();
    }

    virtual double set_gain(const std::string &which, double value)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.set_gain(dir, chain, value);
    }

    virtual double get_gain(const std::string &which)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_gain(dir, chain);
    }

    // TODO: does agc mode need to have a which parameter?
    // this affects all RX channels on the device
    virtual void set_agc_mode(const std::string &which, const std::string &mode)
    {
        auto dir = _get_direction_from_antenna(which);
        if (dir != uhd::direction_t::RX_DIRECTION)
        {
            throw mpm::runtime_error("set_agc not valid for non-rx channels");
        }

        ad937x_device::gain_mode_t gain_mode;
        if (mode == "automatic")
        {
            gain_mode = ad937x_device::gain_mode_t::AUTOMATIC;
        }
        else if (mode == "manual") {
            gain_mode = ad937x_device::gain_mode_t::MANUAL;
        }
        else if (mode == "hybrid") {
            gain_mode = ad937x_device::gain_mode_t::HYBRID;
        }
        else {
            throw mpm::runtime_error("invalid agc mode");
        }

        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.set_agc_mode(dir, gain_mode);
    }

    virtual double set_clock_rate(double value)
    {
        auto rates = get_clock_rates();
        if (std::find(rates.cbegin(), rates.cend(), value) == rates.end())
        {
            value = *(rates.cbegin());
        }

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.set_clock_rate(value);
    }

    virtual void enable_channel(const std::string &which, bool enable)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.enable_channel(dir, chain, enable);
    }

    virtual double set_freq(const std::string &which, double value, bool wait_for_lock)
    {
        auto dir = _get_direction_from_antenna(which);
        auto clipped_value = get_rf_freq_range().clip(value);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.tune(dir, clipped_value, wait_for_lock);
    }

    virtual double get_freq(const std::string &which)
    {
        auto dir = _get_direction_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_freq(dir);
    }

    virtual void set_fir(const std::string &which, int8_t gain, const std::vector<int16_t> & fir)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        auto lengths = _get_valid_fir_lengths(which);
        if (std::find(lengths.begin(), lengths.end(), fir.size()) == lengths.end())
        {
            throw mpm::value_error("invalid filter length");
        }

        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.set_fir(dir, chain, gain, fir);
    }

    virtual std::vector<int16_t> get_fir(const std::string &which, int8_t &gain)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_fir(dir, chain, gain);
    }

    virtual int16_t get_temperature()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_temperature();
    }

    virtual void set_enable_gain_pins(const std::string &which, bool enable)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.set_enable_gain_pins(dir, chain, enable);
    }

    virtual void set_gain_pin_step_sizes(const std::string &which, double inc_step, double dec_step)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        if (dir == uhd::RX_DIRECTION)
        {
            auto steps = _get_valid_rx_gain_steps();
            inc_step = steps.clip(inc_step);
            dec_step = steps.clip(dec_step);
        }
        else if (dir == uhd::TX_DIRECTION)
        {
            auto steps = _get_valid_tx_gain_steps();
            inc_step = steps.clip(inc_step);
            dec_step = steps.clip(dec_step);

            // double comparison here should be okay because of clipping
            if (inc_step != dec_step)
            {
                throw mpm::value_error("TX gain increment and decrement steps must be equal");
            }
        }

        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.set_gain_pin_step_sizes(dir, chain, inc_step, dec_step);
    }

    uint8_t peek8(const uint32_t addr)
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return _iface->peek8(addr);
    }

    void poke8(const uint32_t addr, const uint8_t val)
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        _iface->poke8(addr, val);
    }

private:
    ad937x_device device;
    std::shared_ptr<std::mutex> spi_mutex;
    mpm::types::regs_iface::sptr _iface;
};

ad937x_ctrl::sptr ad937x_ctrl::make(
        std::shared_ptr<std::mutex> spi_mutex,
        mpm::types::regs_iface::sptr iface,
        mpm::ad937x::gpio::gain_pins_t gain_pins
) {
    return std::make_shared<ad937x_ctrl_impl>(spi_mutex, iface, gain_pins);
}


