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

#include "time_core_3000.hpp"
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/msg.hpp>
#include <boost/thread/thread.hpp>

#define REG_TIME_HI       _base + 0
#define REG_TIME_LO       _base + 4
#define REG_TIME_CTRL     _base + 8

#define CTRL_LATCH_TIME_NOW     (1 << 0)
#define CTRL_LATCH_TIME_PPS     (1 << 1)
#define CTRL_LATCH_TIME_SYNC    (1 << 2)

using namespace uhd;

time_core_3000::~time_core_3000(void){
    /* NOP */
}

struct time_core_3000_impl : time_core_3000
{
    time_core_3000_impl(
        wb_iface::sptr iface, const size_t base,
        const readback_bases_type &readback_bases
    ):
        _iface(iface),
        _base(base),
        _readback_bases(readback_bases)
    {
        this->set_tick_rate(1); //init to non zero
    }

    ~time_core_3000_impl(void)
    {
        UHD_SAFE_CALL
        (
            ;//NOP
        )
    }

    void set_tick_rate(const double rate)
    {
        _tick_rate = rate;
    }

    void self_test(void)
    {
        const size_t sleep_millis = 100;
        UHD_MSG(status) << "Performing timer loopback test... " << std::flush;
        const time_spec_t time0 = this->get_time_now();
        boost::this_thread::sleep(boost::posix_time::milliseconds(sleep_millis));
        const time_spec_t time1 = this->get_time_now();
        const double approx_secs = (time1 - time0).get_real_secs();
        const bool test_fail = (approx_secs > 0.15) or (approx_secs < 0.05);
        UHD_MSG(status) << ((test_fail)? " fail" : "pass") << std::endl;

        //useful warning for debugging actual rate
        const size_t ticks_elapsed = size_t(_tick_rate*approx_secs);
        const size_t approx_rate = size_t(ticks_elapsed/(sleep_millis/1e3));
        if (test_fail) UHD_MSG(warning)
            << "Expecting clock rate: " << (_tick_rate/1e6) << " MHz\n"
            << "Approximate clock rate: " << (approx_rate/1e6) << " MHz\n"
        << std::endl;
    }

    uhd::time_spec_t get_time_now(void)
    {
        const boost::uint64_t ticks = _iface->peek64(_readback_bases.rb_now);
        return time_spec_t::from_ticks(ticks, _tick_rate);
    }

    uhd::time_spec_t get_time_last_pps(void)
    {
        const boost::uint64_t ticks = _iface->peek64(_readback_bases.rb_pps);
        return time_spec_t::from_ticks(ticks, _tick_rate);
    }

    void set_time_now(const uhd::time_spec_t &time)
    {
        const boost::uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME_HI, boost::uint32_t(ticks >> 32));
        _iface->poke32(REG_TIME_LO, boost::uint32_t(ticks >> 0));
        _iface->poke32(REG_TIME_CTRL, CTRL_LATCH_TIME_NOW);
    }

    void set_time_sync(const uhd::time_spec_t &time)
    {
        const boost::uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME_HI, boost::uint32_t(ticks >> 32));
        _iface->poke32(REG_TIME_LO, boost::uint32_t(ticks >> 0));
        _iface->poke32(REG_TIME_CTRL, CTRL_LATCH_TIME_SYNC);
    }

    void set_time_next_pps(const uhd::time_spec_t &time)
    {
        const boost::uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME_HI, boost::uint32_t(ticks >> 32));
        _iface->poke32(REG_TIME_LO, boost::uint32_t(ticks >> 0));
        _iface->poke32(REG_TIME_CTRL, CTRL_LATCH_TIME_PPS);
    }

    wb_iface::sptr _iface;
    const size_t _base;
    const readback_bases_type _readback_bases;
    double _tick_rate;
};

time_core_3000::sptr time_core_3000::make(
    wb_iface::sptr iface, const size_t base,
    const readback_bases_type &readback_bases
)
{
    return time_core_3000::sptr(new time_core_3000_impl(iface, base, readback_bases));
}
