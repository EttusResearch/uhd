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

#include "tx_dsp_core_200.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <algorithm>
#include <cmath>

#define REG_DSP_TX_FREQ          _dsp_base + 0
#define REG_DSP_TX_SCALE_IQ      _dsp_base + 1
#define REG_DSP_TX_INTERP        _dsp_base + 2

#define REG_TX_CTRL_NUM_CHAN        _ctrl_base + 0
#define REG_TX_CTRL_CLEAR_STATE     _ctrl_base + 1
#define REG_TX_CTRL_REPORT_SID      _ctrl_base + 2
#define REG_TX_CTRL_POLICY          _ctrl_base + 3
#define REG_TX_CTRL_CYCLES_PER_UP   _ctrl_base + 4
#define REG_TX_CTRL_PACKETS_PER_UP  _ctrl_base + 5

#define FLAG_TX_CTRL_POLICY_WAIT          (0x1 << 0)
#define FLAG_TX_CTRL_POLICY_NEXT_PACKET   (0x1 << 1)
#define FLAG_TX_CTRL_POLICY_NEXT_BURST    (0x1 << 2)

//enable flag for registers: cycles and packets per update packet
#define FLAG_TX_CTRL_UP_ENB              (1ul << 31)

template <class T> T ceil_log2(T num){
    return std::ceil(std::log(num)/std::log(T(2)));
}

using namespace uhd;

class tx_dsp_core_200_impl : public tx_dsp_core_200{
public:
    tx_dsp_core_200_impl(
        wb_iface::sptr iface,
        const size_t dsp_base, const size_t ctrl_base,
        const boost::uint32_t sid
    ):
        _iface(iface), _dsp_base(dsp_base), _ctrl_base(ctrl_base)
    {
        //init the tx control registers
        _iface->poke32(REG_TX_CTRL_CLEAR_STATE, 1); //reset
        _iface->poke32(REG_TX_CTRL_NUM_CHAN, 0);    //1 channel
        _iface->poke32(REG_TX_CTRL_REPORT_SID, sid);
        _iface->poke32(REG_TX_CTRL_POLICY, FLAG_TX_CTRL_POLICY_NEXT_PACKET);
    }

    void set_tick_rate(const double rate){
        _tick_rate = rate;
    }

    double set_host_rate(const double rate){
        int interp = boost::math::iround(_tick_rate/rate);

        //determine which half-band filters are activated
        int hb0 = 0, hb1 = 0;
        if (interp % 2 == 0){
            hb0 = 1;
            interp /= 2;
        }
        if (interp % 2 == 0){
            hb1 = 1;
            interp /= 2;
        }

        _iface->poke32(REG_DSP_TX_INTERP, (hb1 << 9) | (hb0 << 8) | (interp & 0xff));

        // Calculate CIC interpolation (i.e., without halfband interpolators)
        // Calculate closest multiplier constant to reverse gain absent scale multipliers
        double rate_cubed = std::pow(double(interp & 0xff), 3);
        const boost::int16_t scale = boost::math::iround((4096*std::pow(2, ceil_log2(rate_cubed)))/(1.65*rate_cubed));
        _iface->poke32(REG_DSP_TX_SCALE_IQ, (boost::uint32_t(scale) << 16) | (boost::uint32_t(scale) << 0));

        return _tick_rate/interp;
    }

    double set_freq(const double freq_){
        //correct for outside of rate (wrap around)
        double freq = std::fmod(freq_, _tick_rate);
        if (std::abs(freq) > _tick_rate/2.0)
            freq -= boost::math::sign(freq)*_tick_rate;

        //calculate the freq register word (signed)
        UHD_ASSERT_THROW(std::abs(freq) <= _tick_rate/2.0);
        static const double scale_factor = std::pow(2.0, 32);
        const boost::int32_t freq_word = boost::int32_t(boost::math::round((freq / _tick_rate) * scale_factor));

        //update the actual frequency
        const double actual_freq = (double(freq_word) / scale_factor) * _tick_rate;

        _iface->poke32(REG_DSP_TX_FREQ, boost::uint32_t(freq_word));

        return actual_freq;
    }

    void set_updates(const size_t cycles_per_up, const size_t packets_per_up){
        _iface->poke32(REG_TX_CTRL_CYCLES_PER_UP,  (cycles_per_up  == 0)? 0 : (FLAG_TX_CTRL_UP_ENB | cycles_per_up));
        _iface->poke32(REG_TX_CTRL_PACKETS_PER_UP, (packets_per_up == 0)? 0 : (FLAG_TX_CTRL_UP_ENB | packets_per_up));
    }

private:
    wb_iface::sptr _iface;
    const size_t _dsp_base, _ctrl_base;
    double _tick_rate;
};

tx_dsp_core_200::sptr tx_dsp_core_200::make(wb_iface::sptr iface, const size_t dsp_base, const size_t ctrl_base, const boost::uint32_t sid){
    return sptr(new tx_dsp_core_200_impl(iface, dsp_base, ctrl_base, sid));
}
