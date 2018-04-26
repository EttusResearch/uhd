//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/time_core_3000.hpp>
#include <chrono>
#include <thread>

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
        UHD_LOGGER_DEBUG("CORES") << "Performing timer loopback test... ";
        const time_spec_t time0 = this->get_time_now();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_millis));
        const time_spec_t time1 = this->get_time_now();
        const double approx_secs = (time1 - time0).get_real_secs();
        const bool test_fail = (approx_secs > 0.15) or (approx_secs < 0.05);
        if (test_fail) {
            UHD_LOG_WARNING("CORES", "Timer loopback test failed!");
        } else {
            UHD_LOG_DEBUG("CORES", "Timer loopback test passed.");
        }

        //useful warning for debugging actual rate
        const size_t ticks_elapsed = size_t(_tick_rate*approx_secs);
        const size_t approx_rate = size_t(ticks_elapsed/(sleep_millis/1e3));
        if (test_fail) UHD_LOGGER_WARNING("CORES")
            << "Expecting clock rate: " << (_tick_rate/1e6) << " MHz\n"
            << "Approximate clock rate: " << (approx_rate/1e6) << " MHz\n"
        ;
    }

    uhd::time_spec_t get_time_now(void)
    {
        const uint64_t ticks = _iface->peek64(_readback_bases.rb_now);
        return time_spec_t::from_ticks(ticks, _tick_rate);
    }

    uhd::time_spec_t get_time_last_pps(void)
    {
        const uint64_t ticks = _iface->peek64(_readback_bases.rb_pps);
        return time_spec_t::from_ticks(ticks, _tick_rate);
    }

    void set_time_now(const uhd::time_spec_t &time)
    {
        const uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME_HI, uint32_t(ticks >> 32));
        _iface->poke32(REG_TIME_LO, uint32_t(ticks >> 0));
        _iface->poke32(REG_TIME_CTRL, CTRL_LATCH_TIME_NOW);
    }

    void set_time_sync(const uhd::time_spec_t &time)
    {
        const uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME_HI, uint32_t(ticks >> 32));
        _iface->poke32(REG_TIME_LO, uint32_t(ticks >> 0));
        _iface->poke32(REG_TIME_CTRL, CTRL_LATCH_TIME_SYNC);
    }

    void set_time_next_pps(const uhd::time_spec_t &time)
    {
        const uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME_HI, uint32_t(ticks >> 32));
        _iface->poke32(REG_TIME_LO, uint32_t(ticks >> 0));
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
