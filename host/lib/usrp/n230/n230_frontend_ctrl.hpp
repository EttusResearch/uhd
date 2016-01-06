//
// Copyright 2013-2014 Ettus Research LLC
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

#ifndef INCLUDED_N230_FRONTEND_CTRL_HPP
#define INCLUDED_N230_FRONTEND_CTRL_HPP

#include "radio_ctrl_core_3000.hpp"
#include "ad9361_ctrl.hpp"
#include "gpio_atr_3000.hpp"
#include <uhd/types/sensors.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>
#include "n230_fpga_defs.h"

namespace uhd { namespace usrp { namespace n230 {

enum fe_state_t {
    NONE_STREAMING, TX_STREAMING, RX_STREAMING, TXRX_STREAMING
};

enum self_test_mode_t {
    LOOPBACK_DISABLED, LOOPBACK_RADIO, LOOPBACK_CODEC
};


class n230_frontend_ctrl : boost::noncopyable
{
public:
    typedef boost::shared_ptr<n230_frontend_ctrl> sptr;

    static sptr make(
        radio_ctrl_core_3000::sptr core_ctrl,
        fpga::core_misc_reg_t& core_misc_reg,
        ad9361_ctrl::sptr codec_ctrl,
        const std::vector<gpio_atr::gpio_atr_3000::sptr>& gpio_cores);

    virtual ~n230_frontend_ctrl() {}

    virtual void set_antenna_sel(
        const size_t which,
        const std::string &ant) = 0;

    virtual void set_stream_state(
        const size_t which,
        const fe_state_t state) = 0;

    virtual void set_stream_state(
        const fe_state_t fe0_state,
        const fe_state_t fe1_state) = 0;

    virtual void set_bandsel(
        const std::string& which,
        double freq) = 0;

    virtual void set_self_test_mode(
        self_test_mode_t mode) = 0;
};

}}} //namespace

#endif /* INCLUDED_N230_FRONTEND_CTRL_HPP */
