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

#include <mpm/mykonos/ad937x_ctrl.hpp>

#include <sstream>
#include <set>
#include <functional>

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
        throw uhd::runtime_error("ad937x_ctrl got an invalid channel string.");
    }
    return uhd::direction_t::RX_DIRECTION;
}

static ad937x_device::chain_t _get_chain_from_antenna(const std::string& antenna)
{
    auto sub = antenna.substr(2, 1);
    if (sub == "1") {
        return ad937x_device::chain_t::ONE;
    }
    else if (sub == "2") {
        return ad937x_device::chain_t::TWO;
    }
    else {
        throw uhd::runtime_error("ad937x_ctrl got an invalid channel string.");
    }
    return ad937x_device::chain_t::ONE;
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
        UHD_THROW_INVALID_CODE_PATH();
        return std::set<size_t>();
    }
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
        UHD_THROW_INVALID_CODE_PATH();
        return uhd::meta_range_t();
    }
}

class ad937x_ctrl_impl : public ad937x_ctrl
{
public:
    ad937x_ctrl_impl(spi_lock::sptr spi_l, uhd::spi_iface::sptr iface) :
        spi_l(spi_l),
        device(iface)
    {

    }

    virtual uint8_t get_product_id()
    {
        std::lock_guard<spi_lock> lock(*spi_l);
        return device.get_product_id();
    }

    virtual uint8_t get_device_rev()
    {
        std::lock_guard<spi_lock> lock(*spi_l);
        return device.get_device_rev();
    }
    virtual std::string get_api_version()
    {
        std::lock_guard<spi_lock> lock(*spi_l);
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
        std::lock_guard<spi_lock> lock(*spi_l);
        auto arm = device.get_arm_version();
        std::ostringstream ss;
        ss  << arm.major_ver << "."
            << arm.minor_ver << "."
            << arm.rc_ver;
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

        std::lock_guard<spi_lock> lock(*spi_l);
        return device.set_gain(dir, chain, value);
    }

    virtual void set_agc_mode(const std::string &which, const std::string &mode)
    {
        auto dir = _get_direction_from_antenna(which);
        if (dir != uhd::direction_t::RX_DIRECTION)
        {
            throw uhd::runtime_error("set_agc not valid for non-rx channels");
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
            throw uhd::runtime_error("invalid agc mode");
        }

        std::lock_guard<spi_lock> lock(*spi_l);
        device.set_agc_mode(dir, gain_mode);
    }

    virtual double set_clock_rate(double value)
    {
        auto rates = get_clock_rates();
        if (std::find(rates.cbegin(), rates.cend(), value) == rates.end())
        {
            value = *(rates.cbegin());
        }

        std::lock_guard<spi_lock> lock(*spi_l);
        return device.set_clock_rate(value);
    }

    virtual void enable_channel(const std::string &which, bool enable)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        std::lock_guard<spi_lock> lock(*spi_l);
        return device.enable_channel(dir, chain, enable);
    }

    virtual double set_freq(const std::string &which, double value)
    {
        auto dir = _get_direction_from_antenna(which);
        auto clipped_value = get_rf_freq_range().clip(value);

        std::lock_guard<spi_lock> lock(*spi_l);
        return device.tune(dir, clipped_value);
    }

    virtual double get_freq(const std::string &which)
    {
        auto dir = _get_direction_from_antenna(which);

        std::lock_guard<spi_lock> lock(*spi_l);
        return device.get_freq(dir);
    }

    virtual void set_fir(const std::string &which, int8_t gain, const std::vector<int16_t> & fir)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        auto lengths = _get_valid_fir_lengths(which);
        if (std::find(lengths.begin(), lengths.end(), fir.size()) == lengths.end())
        {
            throw uhd::value_error("invalid filter length");
        }

        std::lock_guard<spi_lock> lock(*spi_l);
        device.set_fir(dir, chain, gain, fir);
    }

    virtual std::vector<int16_t> get_fir(const std::string &which, int8_t &gain)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);

        std::lock_guard<spi_lock> lock(*spi_l);
        return device.get_fir(dir, chain, gain);
    }

    virtual int16_t get_temperature()
    {
        std::lock_guard<spi_lock> lock(*spi_l);
        return device.get_temperature();
    }

private:
    ad937x_device device;
    spi_lock::sptr spi_l;
};

ad937x_ctrl::sptr ad937x_ctrl::make(spi_lock::sptr spi_l, uhd::spi_iface::sptr iface)
{
    return std::make_shared<ad937x_ctrl_impl>(spi_l, iface);
}


