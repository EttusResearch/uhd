//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhdlib/usrp/cores/rx_dsp_core_3000.hpp>
#include <uhdlib/usrp/cores/dsp_core_utils.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/thread/thread.hpp> //thread sleep
#include <boost/math/special_functions/round.hpp>
#include <algorithm>
#include <cmath>

#define REG_DSP_RX_FREQ       _dsp_base + 0
#define REG_DSP_RX_SCALE_IQ   _dsp_base + 4
#define REG_DSP_RX_DECIM      _dsp_base + 8
#define REG_DSP_RX_MUX        _dsp_base + 12
#define REG_DSP_RX_COEFFS     _dsp_base + 16
//FIXME: Add code to support REG_DSP_RX_COEFFS

#define FLAG_DSP_RX_MUX_SWAP_IQ   (1 << 0)
#define FLAG_DSP_RX_MUX_REAL_MODE (1 << 1)
#define FLAG_DSP_RX_MUX_INVERT_Q  (1 << 2)
#define FLAG_DSP_RX_MUX_INVERT_I  (1 << 3)

template <class T> T ceil_log2(T num){
    return std::ceil(std::log(num)/std::log(T(2)));
}

using namespace uhd;

const double rx_dsp_core_3000::DEFAULT_CORDIC_FREQ = 0.0;
const double rx_dsp_core_3000::DEFAULT_DDS_FREQ = 0.0;
const double rx_dsp_core_3000::DEFAULT_RATE = 1e6;

rx_dsp_core_3000::~rx_dsp_core_3000(void){
    /* NOP */
}

class rx_dsp_core_3000_impl : public rx_dsp_core_3000{
public:
    rx_dsp_core_3000_impl(
        wb_iface::sptr iface,
        const size_t dsp_base,
        const bool is_b200
    ):
        _iface(iface), _dsp_base(dsp_base), _is_b200(is_b200)
    {
        // previously uninitialized - assuming zero for all
        _link_rate = _host_extra_scaling = _fxpt_scalar_correction = 0.0;

        //init to something so update method has reasonable defaults
        _scaling_adjustment = 1.0;
        _dsp_extra_scaling = 1.0;
        _tick_rate = 1.0;
        _dsp_freq_offset = 0.0;
    }

    ~rx_dsp_core_3000_impl(void)
    {
        UHD_SAFE_CALL
        (
            ;//NOP
        )
    }

    void set_mux(const uhd::usrp::fe_connection_t& fe_conn){
        uint32_t reg_val = 0;
        switch (fe_conn.get_sampling_mode()) {
        case uhd::usrp::fe_connection_t::REAL:
        case uhd::usrp::fe_connection_t::HETERODYNE:
            reg_val = FLAG_DSP_RX_MUX_REAL_MODE;
            break;
        default:
            reg_val = 0;
            break;
        }

        if (fe_conn.is_iq_swapped()) reg_val |= FLAG_DSP_RX_MUX_SWAP_IQ;
        if (fe_conn.is_i_inverted()) reg_val |= FLAG_DSP_RX_MUX_INVERT_I;
        if (fe_conn.is_q_inverted()) reg_val |= FLAG_DSP_RX_MUX_INVERT_Q;

        _iface->poke32(REG_DSP_RX_MUX, reg_val);

        if (fe_conn.get_sampling_mode() == uhd::usrp::fe_connection_t::HETERODYNE) {
            //1. Remember the sign of the IF frequency.
            //   It will be discarded in the next step
            int if_freq_sign = boost::math::sign(fe_conn.get_if_freq());
            //2. Map IF frequency to the range [0, _tick_rate)
            double if_freq = std::abs(std::fmod(fe_conn.get_if_freq(), _tick_rate));
            //3. Map IF frequency to the range [-_tick_rate/2, _tick_rate/2)
            //   This is the aliased frequency
            if (if_freq > (_tick_rate / 2.0)) {
                if_freq -= _tick_rate;
            }
            //4. Set DSP offset to spin the signal in the opposite
            //   direction as the aliased frequency
            _dsp_freq_offset = if_freq * (-if_freq_sign);
        } else {
            _dsp_freq_offset = 0.0;
        }
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
        if (!_is_b200) {
            for (int rate = 1024; rate > 512; rate -= 8){
                range.push_back(range_t(_tick_rate/rate));
            }
        }
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
        const size_t decim_rate = boost::math::iround(_tick_rate/this->get_host_rates().clip(rate, true));
        size_t decim = decim_rate;

        //determine which half-band filters are activated
        int hb0 = 0, hb1 = 0, hb2 = 0, hb_enable=0;
        if (decim % 2 == 0){
            hb0 = 1;
            decim /= 2;
        }
        if (decim % 2 == 0){
            hb1 = 1;
            decim /= 2;
        }
        //the third half-band is not supported by the B200
        if (decim % 2 == 0 && !_is_b200){
            hb2 = 1;
            decim /= 2;
        }

        if (_is_b200) {
            _iface->poke32(REG_DSP_RX_DECIM, (hb0 << 9) /*small HB */ | (hb1 << 8) /*large HB*/ | (decim & 0xff));

            if (decim > 1 and hb0 == 0 and hb1 == 0) {
                UHD_LOGGER_WARNING("CORES") << boost::format(
                    "The requested decimation is odd; the user should expect CIC rolloff.\n"
                    "Select an even decimation to ensure that a halfband filter is enabled.\n"
                    "decimation = dsp_rate/samp_rate -> %d = (%f MHz)/(%f MHz)\n"
                ) % decim_rate % (_tick_rate/1e6) % (rate/1e6);
            }
        } else {
            // Encode Halfband config for setting register programming.
            if (hb2) { // Implies HB1 and HB0 also asserted
                hb_enable=3;
            } else if (hb1) { // Implies HB0 is also asserted
                hb_enable=2;
            } else if (hb0) {
                hb_enable=1;
            } else {
                hb_enable=0;
            }
            _iface->poke32(REG_DSP_RX_DECIM,  (hb_enable << 8) | (decim & 0xff));

            if (decim > 1 and hb0 == 0 and hb1 == 0 and hb2 == 0) {
                UHD_LOGGER_WARNING("CORES") << boost::format(
                    "The requested decimation is odd; the user should expect passband CIC rolloff.\n"
                    "Select an even decimation to ensure that a halfband filter is enabled.\n"
                    "Decimations factorable by 4 will enable 2 halfbands, those factorable by 8 will enable 3 halfbands.\n"
                    "decimation = dsp_rate/samp_rate -> %d = (%f MHz)/(%f MHz)\n"
                ) % decim_rate % (_tick_rate/1e6) % (rate/1e6);
            }
        }

        // Caclulate algorithmic gain of CIC for a given decimation.
        // For Ettus CIC R=decim, M=1, N=4. Gain = (R * M) ^ N
        const double rate_pow = std::pow(double(decim & 0xff), 4);
        // Calculate compensation gain values for algorithmic gain of and CIC taking into account
        // gain compensation blocks already hardcoded in place in DDC (that provide simple 1/2^n gain compensation).
        //
        // The polar rotation of [I,Q] = [1,1] by Pi/8 also yields max magnitude of SQRT(2) (~1.4142) however
        // input to the DDS thats outside the unit circle can only be sourced from a saturated RF frontend.
        // To provide additional dynamic range head room accordingly using scale factor applied at egress from DDC would
        // cost us small signal performance, thus we do no provide compensation gain for a saturated front end and allow
        // the signal to clip in the H/W as needed. If we wished to avoid the signal clipping in these circumstances then adjust code to read:
        // _scaling_adjustment = std::pow(2, ceil_log2(rate_pow))/(1.648*rate_pow*1.415);
        _scaling_adjustment = std::pow(2, ceil_log2(rate_pow))/(2.0*rate_pow);

        this->update_scalar();

        return _tick_rate/decim_rate;
    }

    // Calculate compensation gain values for algorithmic gain of DDS and CIC taking into account
    // gain compensation blocks already hardcoded in place in DDC (that provide simple 1/2^n gain compensation).
    // Further more factor in OTW format which adds further gain factor to weight output samples correctly.
    void update_scalar(void){
        const double target_scalar = (1 << (_is_b200 ? 16 : 15))*_scaling_adjustment/_dsp_extra_scaling;
        const int32_t actual_scalar = boost::math::iround(target_scalar);
        // Calculate the error introduced by using integer representation for the scalar, can be corrected in host later.
        _fxpt_scalar_correction = target_scalar/actual_scalar;
        // Write DDC with scaling correction for CIC and DDS that maximizes dynamic range in 32/16/12/8bits.
        _iface->poke32(REG_DSP_RX_SCALE_IQ, actual_scalar);
    }

    double get_scaling_adjustment(void){
        return _fxpt_scalar_correction*_host_extra_scaling/32767.;
    }

    double set_freq(const double requested_freq){
        double actual_freq;
        int32_t freq_word;
        get_freq_and_freq_word(requested_freq + _dsp_freq_offset, _tick_rate, actual_freq, freq_word);
        _iface->poke32(REG_DSP_RX_FREQ, uint32_t(freq_word));
        _current_freq = actual_freq;
        return actual_freq;
    }

    double get_freq(void){
        return _current_freq;
    }

    uhd::meta_range_t get_freq_range(void){
        //Too keep the DSP range symmetric about 0, we use abs(_dsp_freq_offset)
        const double offset = std::abs<double>(_dsp_freq_offset);
        return uhd::meta_range_t(-(_tick_rate-offset)/2, +(_tick_rate-offset)/2, _tick_rate/std::pow(2.0, 32));
    }

    void setup(const uhd::stream_args_t &stream_args){

        if (stream_args.otw_format == "sc16"){
            _dsp_extra_scaling = 1.0;
            _host_extra_scaling = 1.0;
        }
        else if (stream_args.otw_format == "sc8"){
            double peak = stream_args.args.cast<double>("peak", 1.0);
            peak = std::max(peak, 1.0/256);
            _host_extra_scaling = peak*256;
            _dsp_extra_scaling = peak;
        }
        else if (stream_args.otw_format == "sc12"){
            double peak = stream_args.args.cast<double>("peak", 1.0);
            peak = std::max(peak, 1.0/16);
            _host_extra_scaling = peak*16;
            _dsp_extra_scaling = peak;
        }
        else if (stream_args.otw_format == "fc32"){
            _host_extra_scaling = 1.0;
            _dsp_extra_scaling = 1.0;
        }
        else throw uhd::value_error("USRP RX cannot handle requested wire format: " + stream_args.otw_format);

        _host_extra_scaling *= stream_args.args.cast<double>("fullscale", 1.0);

        this->update_scalar();
    }

    void populate_subtree(property_tree::sptr subtree)
    {
        subtree->create<meta_range_t>("rate/range")
            .set_publisher(boost::bind(&rx_dsp_core_3000::get_host_rates, this))
        ;
        subtree->create<double>("rate/value")
            .set(DEFAULT_RATE)
            .set_coercer(boost::bind(&rx_dsp_core_3000::set_host_rate, this, _1))
        ;
        subtree->create<double>("freq/value")
            .set(DEFAULT_DDS_FREQ)
            .set_coercer(boost::bind(&rx_dsp_core_3000::set_freq, this, _1))
            .set_publisher([this](){ return this->get_freq(); })
        ;
        subtree->create<meta_range_t>("freq/range")
            .set_publisher(boost::bind(&rx_dsp_core_3000::get_freq_range, this))
        ;
    }

private:
    wb_iface::sptr _iface;
    const size_t _dsp_base;
    const bool _is_b200;    //TODO: Obsolete this when we switch to the new DDC on the B200
    double _tick_rate, _link_rate;
    double _scaling_adjustment, _dsp_extra_scaling, _host_extra_scaling, _fxpt_scalar_correction;
    double _dsp_freq_offset;
    double _current_freq;
};

rx_dsp_core_3000::sptr rx_dsp_core_3000::make(wb_iface::sptr iface, const size_t dsp_base, const bool is_b200 /* = false */)
{
    return sptr(new rx_dsp_core_3000_impl(iface, dsp_base, is_b200));
}
