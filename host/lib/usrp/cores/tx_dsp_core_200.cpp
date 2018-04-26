//
// Copyright 2011-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/cores/tx_dsp_core_200.hpp>
#include <uhdlib/usrp/cores/dsp_core_utils.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/log.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/math/special_functions/round.hpp>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>

#define REG_DSP_TX_FREQ          _dsp_base + 0
#define REG_DSP_TX_SCALE_IQ      _dsp_base + 4
#define REG_DSP_TX_INTERP        _dsp_base + 8

#define REG_TX_CTRL_CLEAR           _ctrl_base + 0
#define REG_TX_CTRL_FORMAT          _ctrl_base + 4
#define REG_TX_CTRL_REPORT_SID      _ctrl_base + 8
#define REG_TX_CTRL_POLICY          _ctrl_base + 12
#define REG_TX_CTRL_CYCLES_PER_UP   _ctrl_base + 16
#define REG_TX_CTRL_PACKETS_PER_UP  _ctrl_base + 20

#define FLAG_TX_CTRL_POLICY_WAIT          (0x1 << 0)
#define FLAG_TX_CTRL_POLICY_NEXT_PACKET   (0x1 << 1)
#define FLAG_TX_CTRL_POLICY_NEXT_BURST    (0x1 << 2)

//enable flag for registers: cycles and packets per update packet
#define FLAG_TX_CTRL_UP_ENB              (1ul << 31)

template <class T> T ceil_log2(T num){
    return std::ceil(std::log(num)/std::log(T(2)));
}

using namespace uhd;

tx_dsp_core_200::~tx_dsp_core_200(void){
    /* NOP */
}

class tx_dsp_core_200_impl : public tx_dsp_core_200{
public:
    tx_dsp_core_200_impl(
        wb_iface::sptr iface,
        const size_t dsp_base, const size_t ctrl_base,
        const uint32_t sid
    ):
        _iface(iface), _dsp_base(dsp_base), _ctrl_base(ctrl_base), _sid(sid)
    {
        // previously uninitialized - assuming zero for all
        _tick_rate = _link_rate = _host_extra_scaling = _fxpt_scalar_correction = 0.0;

        //init to something so update method has reasonable defaults
        _scaling_adjustment = 1.0;
        _dsp_extra_scaling = 1.0;

        //init the tx control registers
        this->clear();
        this->set_underflow_policy("next_packet");
    }

    void clear(void){
        _iface->poke32(REG_TX_CTRL_CLEAR, 1); //reset and flush technique
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        _iface->poke32(REG_TX_CTRL_CLEAR, 0);
        _iface->poke32(REG_TX_CTRL_REPORT_SID, _sid);
    }

    void set_underflow_policy(const std::string &policy){
        if (policy == "next_packet"){
            _iface->poke32(REG_TX_CTRL_POLICY, FLAG_TX_CTRL_POLICY_NEXT_PACKET);
        }
        else if (policy == "next_burst"){
            _iface->poke32(REG_TX_CTRL_POLICY, FLAG_TX_CTRL_POLICY_NEXT_BURST);
        }
        else throw uhd::value_error("USRP TX cannot handle requested underflow policy: " + policy);
    }

    void set_tick_rate(const double rate){
        _tick_rate = rate;
    }

    void set_link_rate(const double rate){
        //_link_rate = rate/sizeof(uint32_t); //in samps/s
        _link_rate = rate/sizeof(uint16_t); //in samps/s (allows for 8sc)
    }

    uhd::meta_range_t get_host_rates(void){
        meta_range_t range;
        for (int rate = 512; rate > 256; rate -= 4){
            range.push_back(range_t(_tick_rate/rate));
        }
        for (int rate = 256; rate > 128; rate -= 2){
            range.push_back(range_t(_tick_rate/rate));
        }
        for (int rate = 128; rate >= int(std::ceil(_tick_rate/_link_rate)); rate -= 1){
            range.push_back(range_t(_tick_rate/rate));
        }
        return range;
    }

    double set_host_rate(const double rate){
        const size_t interp_rate = boost::math::iround(_tick_rate/this->get_host_rates().clip(rate, true));
        size_t interp = interp_rate;

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

        if (interp > 1 and hb0 == 0 and hb1 == 0)
        {
            UHD_LOGGER_WARNING("CORES") << boost::format(
                "The requested interpolation is odd; the user should expect CIC rolloff.\n"
                "Select an even interpolation to ensure that a halfband filter is enabled.\n"
                "interpolation = dsp_rate/samp_rate -> %d = (%f MHz)/(%f MHz)\n"
            ) % interp_rate % (_tick_rate/1e6) % (rate/1e6);
        }

        // Calculate CIC interpolation (i.e., without halfband interpolators)
        // Calculate closest multiplier constant to reverse gain absent scale multipliers
        const double rate_pow = std::pow(double(interp & 0xff), 3);
        _scaling_adjustment = std::pow(2, ceil_log2(rate_pow))/(1.65*rate_pow);
        this->update_scalar();

        return _tick_rate/interp_rate;
    }

    void update_scalar(void){
        const double factor = 1.0 + std::max(ceil_log2(_scaling_adjustment), 0.0);
        const double target_scalar = (1 << 17)*_scaling_adjustment/_dsp_extra_scaling/factor;
        const int32_t actual_scalar = boost::math::iround(target_scalar);
        _fxpt_scalar_correction = target_scalar/actual_scalar*factor; //should be small
        _iface->poke32(REG_DSP_TX_SCALE_IQ, actual_scalar);
    }

    double get_scaling_adjustment(void){
        return _fxpt_scalar_correction*_host_extra_scaling*32767.;
    }

    double set_freq(const double requested_freq){
        double actual_freq;
        int32_t freq_word;
        get_freq_and_freq_word(requested_freq, _tick_rate, actual_freq, freq_word);
        _iface->poke32(REG_DSP_TX_FREQ, uint32_t(freq_word));
        return actual_freq;
    }

    uhd::meta_range_t get_freq_range(void){
        return uhd::meta_range_t(-_tick_rate/2, +_tick_rate/2, _tick_rate/std::pow(2.0, 32));
    }

    void set_updates(const size_t cycles_per_up, const size_t packets_per_up){
        _iface->poke32(REG_TX_CTRL_CYCLES_PER_UP,  (cycles_per_up  == 0)? 0 : (FLAG_TX_CTRL_UP_ENB | cycles_per_up));
        _iface->poke32(REG_TX_CTRL_PACKETS_PER_UP, (packets_per_up == 0)? 0 : (FLAG_TX_CTRL_UP_ENB | packets_per_up));
    }

    void setup(const uhd::stream_args_t &stream_args){
        if (not stream_args.args.has_key("noclear")) this->clear();

        unsigned format_word = 0;
        if (stream_args.otw_format == "sc16"){
            format_word = 0;
            _dsp_extra_scaling = 1.0;
            _host_extra_scaling = 1.0;
        }
        else if (stream_args.otw_format == "sc8"){
            format_word = (1 << 0);
            double peak = stream_args.args.cast<double>("peak", 1.0);
            peak = std::max(peak, 1.0/256);
            _host_extra_scaling = 1.0/peak/256;
            _dsp_extra_scaling = 1.0/peak;
        }
        else throw uhd::value_error("USRP TX cannot handle requested wire format: " + stream_args.otw_format);

        _host_extra_scaling /= stream_args.args.cast<double>("fullscale", 1.0);

        this->update_scalar();

        _iface->poke32(REG_TX_CTRL_FORMAT, format_word);

        if (stream_args.args.has_key("underflow_policy")){
            this->set_underflow_policy(stream_args.args["underflow_policy"]);
        }
    }

private:
    wb_iface::sptr _iface;
    const size_t _dsp_base, _ctrl_base;
    double _tick_rate, _link_rate;
    double _scaling_adjustment, _dsp_extra_scaling, _host_extra_scaling, _fxpt_scalar_correction;
    const uint32_t _sid;
};

tx_dsp_core_200::sptr tx_dsp_core_200::make(wb_iface::sptr iface, const size_t dsp_base, const size_t ctrl_base, const uint32_t sid){
    return sptr(new tx_dsp_core_200_impl(iface, dsp_base, ctrl_base, sid));
}
