//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/cores/tx_dsp_core_3000.hpp>
#include <uhdlib/usrp/cores/dsp_core_utils.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/log.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/thread/thread.hpp> //sleep
#include <algorithm>
#include <cmath>

#define REG_DSP_TX_FREQ          _dsp_base + 0
#define REG_DSP_TX_SCALE_IQ      _dsp_base + 4
#define REG_DSP_TX_INTERP        _dsp_base + 8

template <class T> T ceil_log2(T num){
    return std::ceil(std::log(num)/std::log(T(2)));
}

using namespace uhd;

const double tx_dsp_core_3000::DEFAULT_CORDIC_FREQ = 0.0;
const double tx_dsp_core_3000::DEFAULT_DDS_FREQ = 0.0;
const double tx_dsp_core_3000::DEFAULT_RATE = 1e6;

tx_dsp_core_3000::~tx_dsp_core_3000(void){
    /* NOP */
}

class tx_dsp_core_3000_impl : public tx_dsp_core_3000{
public:
    tx_dsp_core_3000_impl(
        wb_iface::sptr iface,
        const size_t dsp_base
    ):
        _iface(iface), _dsp_base(dsp_base)
    {
        // previously uninitialized - assuming zero for all
        _link_rate = _host_extra_scaling = _fxpt_scalar_correction = 0.0;

        //init to something so update method has reasonable defaults
        _scaling_adjustment = 1.0;
        _dsp_extra_scaling = 1.0;
        this->set_tick_rate(1.0);
    }

    void set_tick_rate(const double rate){
        _tick_rate = rate;
        set_freq(_current_freq);
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

        // Caclulate algorithmic gain of CIC for a given interpolation
        // For Ettus CIC R=decim, M=1, N=3. Gain = (R * M) ^ N
        const double rate_pow = std::pow(double(interp & 0xff), 3);
        // Calculate compensation gain values for algorithmic gain of DDS and CIC taking into account
        // gain compensation blocks already hardcoded in place in DDC (that provide simple 1/2^n gain compensation).
        _scaling_adjustment = std::pow(2, ceil_log2(rate_pow))/(rate_pow);
        this->update_scalar();

        return _tick_rate/interp_rate;
    }

  // Calculate compensation gain values for algorithmic gain of DDS and CIC taking into account
  // gain compensation blocks already hardcoded in place in DDC (that provide simple 1/2^n gain compensation).
  // Further more factor in OTW format which adds further gain factor to weight output samples correctly.
    void update_scalar(void){
        const double target_scalar = (1 << 16)*_scaling_adjustment/_dsp_extra_scaling;
        const int32_t actual_scalar = boost::math::iround(target_scalar);
        _fxpt_scalar_correction = target_scalar/actual_scalar; //should be small
        _iface->poke32(REG_DSP_TX_SCALE_IQ, actual_scalar);
    }

    double get_scaling_adjustment(void){
        return _fxpt_scalar_correction*_host_extra_scaling*32767.;
    }

    double set_freq(const double requested_freq) {
        double actual_freq;
        int32_t freq_word;
        get_freq_and_freq_word(requested_freq, _tick_rate, actual_freq, freq_word);
        _iface->poke32(REG_DSP_TX_FREQ, uint32_t(freq_word));
        _current_freq = actual_freq;
        return actual_freq;
    }

    double get_freq(void){
        return _current_freq;
    }

    uhd::meta_range_t get_freq_range(void){
        return uhd::meta_range_t(-_tick_rate/2, +_tick_rate/2, _tick_rate/std::pow(2.0, 32));
    }

    void setup(const uhd::stream_args_t &stream_args){

        if (stream_args.otw_format == "sc16"){
            _dsp_extra_scaling = 1.0;
            _host_extra_scaling = 1.0;
        }
        else if (stream_args.otw_format == "sc8"){
            double peak = stream_args.args.cast<double>("peak", 1.0);
            peak = std::max(peak, 1.0/256);
            _host_extra_scaling = 1.0/peak/256;
            _dsp_extra_scaling = 1.0/peak;
        }
        else if (stream_args.otw_format == "sc12"){
            double peak = stream_args.args.cast<double>("peak", 1.0);
            peak = std::max(peak, 1.0/16);
            _host_extra_scaling = 1.0/peak/16;
            _dsp_extra_scaling = 1.0/peak;
        }
        else if (stream_args.otw_format == "fc32"){
            _host_extra_scaling = 1.0;
            _dsp_extra_scaling = 1.0;
        }
        else throw uhd::value_error("USRP TX cannot handle requested wire format: " + stream_args.otw_format);

        _host_extra_scaling /= stream_args.args.cast<double>("fullscale", 1.0);

        this->update_scalar();
    }

    void populate_subtree(property_tree::sptr subtree)
    {
        subtree->create<meta_range_t>("rate/range")
            .set_publisher(boost::bind(&tx_dsp_core_3000::get_host_rates, this))
        ;
        subtree->create<double>("rate/value")
            .set(DEFAULT_RATE)
            .set_coercer(boost::bind(&tx_dsp_core_3000::set_host_rate, this, _1))
        ;
        subtree->create<double>("freq/value")
            .set(DEFAULT_DDS_FREQ)
            .set_coercer(boost::bind(&tx_dsp_core_3000::set_freq, this, _1))
            .set_publisher([this](){ return this->get_freq(); })
        ;
        subtree->create<meta_range_t>("freq/range")
            .set_publisher(boost::bind(&tx_dsp_core_3000::get_freq_range, this))
        ;
    }

private:
    wb_iface::sptr _iface;
    const size_t _dsp_base;
    double _tick_rate, _link_rate;
    double _scaling_adjustment, _dsp_extra_scaling, _host_extra_scaling, _fxpt_scalar_correction;
    double _current_freq;
};

tx_dsp_core_3000::sptr tx_dsp_core_3000::make(wb_iface::sptr iface, const size_t dsp_base)
{
    return sptr(new tx_dsp_core_3000_impl(iface, dsp_base));
}
