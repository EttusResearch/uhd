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

#ifndef INCLUDED_N230_CLK_PPS_CTRL_HPP
#define INCLUDED_N230_CLK_PPS_CTRL_HPP

#include "time_core_3000.hpp"
#include "ad9361_ctrl.hpp"
#include <uhd/types/sensors.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>
#include "n230_cores.hpp"
#include "n230_fpga_defs.h"

namespace uhd { namespace usrp { namespace n230 {

class n230_clk_pps_ctrl : boost::noncopyable
{
public:
    typedef boost::shared_ptr<n230_clk_pps_ctrl> sptr;

    static sptr make(
        ad9361_ctrl::sptr codec_ctrl,
        n230_ref_pll_ctrl::sptr ref_pll_ctrl,
        fpga::core_misc_reg_t& core_misc_reg,
        fpga::core_pps_sel_reg_t& core_pps_sel_reg,
        fpga::core_status_reg_t& core_status_reg,
        const std::vector<time_core_3000::sptr>& time_cores);

    virtual ~n230_clk_pps_ctrl() {}

    /***********************************************************************
     * Tick Rate
     **********************************************************************/
    /*! Set the master clock rate of the device.
     * \return the clock frequency in Hz
     */
    virtual double set_tick_rate(const double rate) = 0;

    /*! Get the master clock rate of the device.
     * \return the clock frequency in Hz
     */
    virtual double get_tick_rate() = 0;

    /***********************************************************************
     * Reference clock
     **********************************************************************/
    /*! Set the reference clock source of the device.
     */
    virtual void set_clock_source(const std::string &source) = 0;

    /*! Get the reference clock source of the device.
     */
    virtual const std::string& get_clock_source() = 0;

    /*! Get the reference clock lock status.
     */
    virtual uhd::sensor_value_t get_ref_locked() = 0;

    /***********************************************************************
     * Time source
     **********************************************************************/
    /*! Set the time source of the device.
     */
    virtual void set_pps_source(const std::string &source) = 0;

    /*! Get the reference clock source of the device.
     */
    virtual const std::string& get_pps_source() = 0;
};

}}} //namespace

#endif /* INCLUDED_N230_CLK_PPS_CTRL_HPP */
