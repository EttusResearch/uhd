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

#include "n230_clk_pps_ctrl.hpp"

#include <uhd/utils/msg.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/cstdint.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <stdexcept>
#include <cmath>
#include <cstdlib>

namespace uhd { namespace usrp { namespace n230 {

class n230_clk_pps_ctrl_impl : public n230_clk_pps_ctrl
{
public:
    n230_clk_pps_ctrl_impl(
        ad9361_ctrl::sptr codec_ctrl,
        n230_ref_pll_ctrl::sptr ref_pll_ctrl,
        fpga::core_misc_reg_t& core_misc_reg,
        fpga::core_pps_sel_reg_t& core_pps_sel,
        fpga::core_status_reg_t& core_status_reg,
        const std::vector<time_core_3000::sptr>& time_cores
    ): _codec_ctrl(codec_ctrl),
       _ref_pll_ctrl(ref_pll_ctrl),
       _core_misc_reg(core_misc_reg),
       _core_pps_sel_reg(core_pps_sel),
       _core_status_reg(core_status_reg),
       _time_cores(time_cores),
       _tick_rate(0.0),
       _clock_source("<undefined>"),
       _time_source("<undefined>")
    {
    }

    virtual ~n230_clk_pps_ctrl_impl()
    {
    }

    double set_tick_rate(const double rate)
    {
        UHD_MSG(status) << "Configuring a tick rate of " << rate/1e6 << " MHz... ";
        _tick_rate = _codec_ctrl->set_clock_rate(rate);
        UHD_MSG(status) << "got " << _tick_rate/1e6 << " MHz\n";

        BOOST_FOREACH(time_core_3000::sptr& time_core, _time_cores) {
            time_core->set_tick_rate(_tick_rate);
            time_core->self_test();
        }

        return _tick_rate;
    }

    double get_tick_rate()
    {
        return _tick_rate;
    }

    void set_clock_source(const std::string &source)
    {
        if (_clock_source == source) return;

        if (source == "internal") {
            _ref_pll_ctrl->set_lock_to_ext_ref(false);
        } else if (source == "external" || source == "gpsdo") {
            _ref_pll_ctrl->set_lock_to_ext_ref(true);
        } else {
            throw uhd::key_error("set_clock_source: unknown source: " + source);
        }
        _core_misc_reg.write(fpga::core_misc_reg_t::REF_SEL, (source == "gpsdo") ? 1 : 0);

        _clock_source = source;
    }

    const std::string& get_clock_source()
    {
        return _clock_source;
    }

    uhd::sensor_value_t get_ref_locked()
    {
        bool locked = false;
        if (_clock_source == "external" || _clock_source == "gpsdo") {
            locked = (_core_status_reg.read(fpga::core_status_reg_t::REF_LOCKED) == 1);
        } else {
            //If the source is internal, the charge pump on the ADF4001 is tristated which
            //means that the 40MHz VCTXXO is free running i.e. always "locked"
            locked = true;
        }
        return sensor_value_t("Ref", locked, "locked", "unlocked");
    }

    void set_pps_source(const std::string &source)
    {
        if (_time_source == source) return;

        if (source == "none" or source == "gpsdo") {
            _core_pps_sel_reg.write(fpga::core_pps_sel_reg_t::EXT_PPS_EN, 0);
        } else if (source == "external") {
            _core_pps_sel_reg.write(fpga::core_pps_sel_reg_t::EXT_PPS_EN, 1);
        } else {
            throw uhd::key_error("update_time_source: unknown source: " + source);
        }

        _time_source = source;
    }

    const std::string& get_pps_source()
    {
        return _time_source;
    }

private:
    ad9361_ctrl::sptr                   _codec_ctrl;
    n230_ref_pll_ctrl::sptr             _ref_pll_ctrl;
    fpga::core_misc_reg_t&              _core_misc_reg;
    fpga::core_pps_sel_reg_t&           _core_pps_sel_reg;
    fpga::core_status_reg_t&            _core_status_reg;
    std::vector<time_core_3000::sptr>   _time_cores;
    double                              _tick_rate;
    std::string                         _clock_source;
    std::string                         _time_source;
};

}}} //namespace

using namespace uhd::usrp::n230;
using namespace uhd::usrp;

n230_clk_pps_ctrl::sptr n230_clk_pps_ctrl::make(
    ad9361_ctrl::sptr codec_ctrl,
    n230_ref_pll_ctrl::sptr ref_pll_ctrl,
    fpga::core_misc_reg_t& core_misc_reg,
    fpga::core_pps_sel_reg_t& core_pps_sel_reg,
    fpga::core_status_reg_t& core_status_reg,
    const std::vector<time_core_3000::sptr>& time_cores)
{
    return sptr(new n230_clk_pps_ctrl_impl(
        codec_ctrl, ref_pll_ctrl, core_misc_reg, core_pps_sel_reg, core_status_reg, time_cores));
}

