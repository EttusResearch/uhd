//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhdlib/usrp/cores/time64_core_200.hpp>
#include <boost/math/special_functions/round.hpp>

#define REG_TIME64_TICKS_HI    _base + 0
#define REG_TIME64_TICKS_LO    _base + 4
#define REG_TIME64_FLAGS       _base + 8
#define REG_TIME64_IMM         _base + 12
#define REG_TIME64_MIMO_SYNC   _base + 20 //lower byte is delay cycles

//pps flags (see above)
#define FLAG_TIME64_PPS_NEGEDGE (0 << 0)
#define FLAG_TIME64_PPS_POSEDGE (1 << 0)
#define FLAG_TIME64_PPS_SMA     (0 << 1)
#define FLAG_TIME64_PPS_MIMO    (1 << 1) //apparently not used

#define FLAG_TIME64_LATCH_NOW 1
#define FLAG_TIME64_LATCH_NEXT_PPS 0

#define FLAG_TIME64_MIMO_SYNC (1 << 8)

using namespace uhd;

time64_core_200::~time64_core_200(void){
    /* NOP */
}

class time64_core_200_impl : public time64_core_200{
public:
    time64_core_200_impl(
        wb_iface::sptr iface, const size_t base,
        const readback_bases_type &readback_bases,
        const size_t mimo_delay_cycles
    ):
        _iface(iface), _base(base),
        _readback_bases(readback_bases),
        _tick_rate(0.0),
        _mimo_delay_cycles(mimo_delay_cycles)
    {
        _sources.push_back("none");
        _sources.push_back("external");
        _sources.push_back("_external_");
        if (_mimo_delay_cycles != 0) _sources.push_back("mimo");
    }

    void enable_gpsdo(void){
        _sources.push_back("gpsdo");
    }

    void set_tick_rate(const double rate){
        _tick_rate = rate;
    }

    uhd::time_spec_t get_time_now(void){
        for (size_t i = 0; i < 3; i++){ //special algorithm because we cant read 64 bits synchronously
            const uint32_t ticks_hi = _iface->peek32(_readback_bases.rb_hi_now);
            const uint32_t ticks_lo = _iface->peek32(_readback_bases.rb_lo_now);
            if (ticks_hi != _iface->peek32(_readback_bases.rb_hi_now)) continue;
            const uint64_t ticks = (uint64_t(ticks_hi) << 32) | ticks_lo;
            return time_spec_t::from_ticks(ticks, _tick_rate);
        }
        throw uhd::runtime_error("time64_core_200: get time now timeout");
    }

    uhd::time_spec_t get_time_last_pps(void){
        for (size_t i = 0; i < 3; i++){ //special algorithm because we cant read 64 bits synchronously
            const uint32_t ticks_hi = _iface->peek32(_readback_bases.rb_hi_pps);
            const uint32_t ticks_lo = _iface->peek32(_readback_bases.rb_lo_pps);
            if (ticks_hi != _iface->peek32(_readback_bases.rb_hi_pps)) continue;
            const uint64_t ticks = (uint64_t(ticks_hi) << 32) | ticks_lo;
            return time_spec_t::from_ticks(ticks, _tick_rate);
        }
        throw uhd::runtime_error("time64_core_200: get time last pps timeout");
    }

    void set_time_now(const uhd::time_spec_t &time){
        const uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME64_TICKS_LO, uint32_t(ticks >> 0));
        _iface->poke32(REG_TIME64_IMM, FLAG_TIME64_LATCH_NOW);
        _iface->poke32(REG_TIME64_TICKS_HI, uint32_t(ticks >> 32)); //latches all 3
    }

    void set_time_next_pps(const uhd::time_spec_t &time){
        const uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME64_TICKS_LO, uint32_t(ticks >> 0));
        _iface->poke32(REG_TIME64_IMM, FLAG_TIME64_LATCH_NEXT_PPS);
        _iface->poke32(REG_TIME64_TICKS_HI, uint32_t(ticks >> 32)); //latches all 3
    }

    void set_time_source(const std::string &source){
        assert_has(_sources, source, "time source");

        //setup pps flags
        if (source == "external" or source == "gpsdo"){
            _iface->poke32(REG_TIME64_FLAGS, FLAG_TIME64_PPS_SMA | FLAG_TIME64_PPS_POSEDGE);
        }
        else if (source == "_external_"){
            _iface->poke32(REG_TIME64_FLAGS, FLAG_TIME64_PPS_SMA | FLAG_TIME64_PPS_NEGEDGE);
        }

        //setup mimo flags
        if (source == "mimo"){
            _iface->poke32(REG_TIME64_MIMO_SYNC, FLAG_TIME64_MIMO_SYNC | (_mimo_delay_cycles & 0xff));
        }
        else{
            _iface->poke32(REG_TIME64_MIMO_SYNC, 0);
        }
    }

    std::vector<std::string> get_time_sources(void){
        return _sources;
    }

private:
    wb_iface::sptr _iface;
    const size_t _base;
    const readback_bases_type _readback_bases;
    double _tick_rate;
    const size_t _mimo_delay_cycles;
    std::vector<std::string> _sources;
};

time64_core_200::sptr time64_core_200::make(wb_iface::sptr iface, const size_t base, const readback_bases_type &readback_bases, const size_t mimo_delay_cycles){
    return sptr(new time64_core_200_impl(iface, base, readback_bases, mimo_delay_cycles));
}
