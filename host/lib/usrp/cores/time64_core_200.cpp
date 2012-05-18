//
// Copyright 2011 Ettus Research LLC
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

#include "time64_core_200.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/assert_has.hpp>
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

class time64_core_200_impl : public time64_core_200{
public:
    time64_core_200_impl(
        wb_iface::sptr iface, const size_t base,
        const readback_bases_type &readback_bases,
        const size_t mimo_delay_cycles
    ):
        _iface(iface), _base(base),
        _readback_bases(readback_bases),
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
            const boost::uint32_t ticks_hi = _iface->peek32(_readback_bases.rb_hi_now);
            const boost::uint32_t ticks_lo = _iface->peek32(_readback_bases.rb_lo_now);
            if (ticks_hi != _iface->peek32(_readback_bases.rb_hi_now)) continue;
            const boost::uint64_t ticks = (boost::uint64_t(ticks_hi) << 32) | ticks_lo;
            return time_spec_t::from_ticks(ticks, _tick_rate);
        }
        throw uhd::runtime_error("time64_core_200: get time now timeout");
    }

    uhd::time_spec_t get_time_last_pps(void){
        for (size_t i = 0; i < 3; i++){ //special algorithm because we cant read 64 bits synchronously
            const boost::uint32_t ticks_hi = _iface->peek32(_readback_bases.rb_hi_pps);
            const boost::uint32_t ticks_lo = _iface->peek32(_readback_bases.rb_lo_pps);
            if (ticks_hi != _iface->peek32(_readback_bases.rb_hi_pps)) continue;
            const boost::uint64_t ticks = (boost::uint64_t(ticks_hi) << 32) | ticks_lo;
            return time_spec_t::from_ticks(ticks, _tick_rate);
        }
        throw uhd::runtime_error("time64_core_200: get time last pps timeout");
    }

    void set_time_now(const uhd::time_spec_t &time){
        const boost::uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME64_TICKS_LO, boost::uint32_t(ticks >> 0));
        _iface->poke32(REG_TIME64_IMM, FLAG_TIME64_LATCH_NOW);
        _iface->poke32(REG_TIME64_TICKS_HI, boost::uint32_t(ticks >> 32)); //latches all 3
    }

    void set_time_next_pps(const uhd::time_spec_t &time){
        const boost::uint64_t ticks = time.to_ticks(_tick_rate);
        _iface->poke32(REG_TIME64_TICKS_LO, boost::uint32_t(ticks >> 0));
        _iface->poke32(REG_TIME64_IMM, FLAG_TIME64_LATCH_NEXT_PPS);
        _iface->poke32(REG_TIME64_TICKS_HI, boost::uint32_t(ticks >> 32)); //latches all 3
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
