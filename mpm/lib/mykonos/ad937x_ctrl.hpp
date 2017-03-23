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

#include "ad937x_device.hpp"
#include "../spi/spi_lock.h"

// TODO: fix path of UHD includes
#include <../../host/include/uhd/types/direction.hpp>
#include <../../host/include/uhd/types/ranges.hpp>
#include <../../host/include/uhd/exception.hpp>
#include <../../host/include/uhd/types/serial.hpp>

#include <boost/noncopyable.hpp>
#include <memory>
#include <functional>
#include <set>

class ad937x_ctrl : public boost::noncopyable
{
public:
    typedef std::shared_ptr<ad937x_ctrl> sptr;
    static sptr make(spi_lock::sptr spi_l, uhd::spi_iface::sptr iface);
    virtual ~ad937x_ctrl(void) {}

    static uhd::meta_range_t get_rf_freq_range(void);
    static uhd::meta_range_t get_bw_filter_range(void);
    static std::vector<double> get_clock_rates(void);
    static uhd::meta_range_t get_gain_range(const std::string &which);

    virtual uint8_t get_product_id() = 0;
    virtual uint8_t get_device_rev() = 0;
    virtual std::string get_api_version() = 0;
    virtual std::string get_arm_version() = 0;

    virtual double set_bw_filter(const std::string &which, double value) = 0;
    virtual double set_gain(const std::string &which, double value) = 0;

    virtual void set_agc_mode(const std::string &which, const std::string &mode) = 0;

    virtual double set_clock_rate(double value) = 0;
    virtual void enable_channel(const std::string &which, bool enable) = 0;

    virtual double set_freq(const std::string &which, double value) = 0;
    virtual double get_freq(const std::string &which) = 0;

    virtual void set_fir(const std::string &which, int8_t gain, const std::vector<int16_t> & fir) = 0;
    virtual std::vector<int16_t> get_fir(const std::string &which, int8_t &gain) = 0;

    virtual int16_t get_temperature() = 0;

protected:
    static uhd::direction_t _get_direction_from_antenna(const std::string& antenna);
    static ad937x_device::chain_t _get_chain_from_antenna(const std::string& antenna);

    static std::set<size_t> _get_valid_fir_lengths(const std::string& which);
};
