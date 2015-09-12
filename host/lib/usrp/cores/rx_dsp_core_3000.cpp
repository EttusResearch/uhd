//
// Copyright 2011-2014 Ettus Research LLC
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

#include "rx_dsp_core_3000.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/thread/thread.hpp> //thread sleep
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/sign.hpp>
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
    }

    ~rx_dsp_core_3000_impl(void)
    {
        UHD_SAFE_CALL
        (
            ;//NOP
        )
    }

    void set_mux(const std::string &mode, const bool fe_swapped, const bool invert_i, const bool invert_q){
        static const uhd::dict<std::string, boost::uint32_t> mode_to_mux = boost::assign::map_list_of
            ("IQ", 0)
            ("QI", FLAG_DSP_RX_MUX_SWAP_IQ)
            ("I", FLAG_DSP_RX_MUX_REAL_MODE)
            ("Q", FLAG_DSP_RX_MUX_SWAP_IQ | FLAG_DSP_RX_MUX_REAL_MODE)
        ;
        _iface->poke32(REG_DSP_RX_MUX, mode_to_mux[mode]
            | (fe_swapped ? FLAG_DSP_RX_MUX_SWAP_IQ : 0)
            | (invert_i ? FLAG_DSP_RX_MUX_INVERT_I : 0)
            | (invert_q ? FLAG_DSP_RX_MUX_INVERT_Q : 0));
    }

    void set_tick_rate(const double rate){
        _tick_rate = rate;
    }

    void set_link_rate(const double rate){
        //_link_rate = rate/sizeof(boost::uint32_t); //in samps/s
        _link_rate = rate/sizeof(boost::uint16_t); //in samps/s (allows for 8sc)
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
                UHD_MSG(warning) << boost::format(
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
                UHD_MSG(warning) << boost::format(
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
        // Calculate compensation gain values for algorithmic gain of CORDIC and CIC taking into account
        // gain compensation blocks already hardcoded in place in DDC (that provide simple 1/2^n gain compensation).
        // CORDIC algorithmic gain limits asymptotically around 1.647 after many iterations.
        //
        // The polar rotation of [I,Q] = [1,1] by Pi/8 also yields max magnitude of SQRT(2) (~1.4142) however
        // input to the CORDIC thats outside the unit circle can only be sourced from a saturated RF frontend.
        // To provide additional dynamic range head room accordingly using scale factor applied at egress from DDC would
        // cost us small signal performance, thus we do no provide compensation gain for a saturated front end and allow
        // the signal to clip in the H/W as needed. If we wished to avoid the signal clipping in these circumstances then adjust code to read:
        // _scaling_adjustment = std::pow(2, ceil_log2(rate_pow))/(1.648*rate_pow*1.415);
        _scaling_adjustment = std::pow(2, ceil_log2(rate_pow))/(1.648*rate_pow);

        this->update_scalar();

        return _tick_rate/decim_rate;
    }

    // Calculate compensation gain values for algorithmic gain of CORDIC and CIC taking into account
    // gain compensation blocks already hardcoded in place in DDC (that provide simple 1/2^n gain compensation).
    // Further more factor in OTW format which adds further gain factor to weight output samples correctly.
    void update_scalar(void){
        const double target_scalar = (1 << (_is_b200 ? 16 : 15))*_scaling_adjustment/_dsp_extra_scaling;
        const boost::int32_t actual_scalar = boost::math::iround(target_scalar);
        // Calculate the error introduced by using integer representation for the scalar, can be corrected in host later.
        _fxpt_scalar_correction = target_scalar/actual_scalar;
        // Write DDC with scaling correction for CIC and CORDIC that maximizes dynamic range in 32/16/12/8bits.
        _iface->poke32(REG_DSP_RX_SCALE_IQ, actual_scalar);
    }

    double get_scaling_adjustment(void){
        return _fxpt_scalar_correction*_host_extra_scaling/32767.;
    }

    double set_freq(const double freq_){
        //correct for outside of rate (wrap around)
        double freq = std::fmod(freq_, _tick_rate);
        if (std::abs(freq) > _tick_rate/2.0)
            freq -= boost::math::sign(freq)*_tick_rate;

        //confirm that the target frequency is within range of the CORDIC
        UHD_ASSERT_THROW(std::abs(freq) <= _tick_rate/2.0);

        /* Now calculate the frequency word. It is possible for this calculation
         * to cause an overflow. As the requested DSP frequency approaches the
         * master clock rate, that ratio multiplied by the scaling factor (2^32)
         * will generally overflow within the last few kHz of tunable range.
         * Thus, we check to see if the operation will overflow before doing it,
         * and if it will, we set it to the integer min or max of this system.
         */
        boost::int32_t freq_word = 0;

        static const double scale_factor = std::pow(2.0, 32);
        if((freq / _tick_rate) >= (uhd::math::BOOST_INT32_MAX / scale_factor)) {
            /* Operation would have caused a positive overflow of int32. */
            freq_word = uhd::math::BOOST_INT32_MAX;

        } else if((freq / _tick_rate) <= (uhd::math::BOOST_INT32_MIN / scale_factor)) {
            /* Operation would have caused a negative overflow of int32. */
            freq_word = uhd::math::BOOST_INT32_MIN;

        } else {
            /* The operation is safe. Perform normally. */
            freq_word = boost::int32_t(boost::math::round((freq / _tick_rate) * scale_factor));
        }

        //program the frequency word into the device DSP
        const double actual_freq = (double(freq_word) / scale_factor) * _tick_rate;
        _iface->poke32(REG_DSP_RX_FREQ, boost::uint32_t(freq_word));

        return actual_freq;
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
            .publish(boost::bind(&rx_dsp_core_3000::get_host_rates, this))
        ;
        subtree->create<double>("rate/value")
            .set(DEFAULT_RATE)
            .coerce(boost::bind(&rx_dsp_core_3000::set_host_rate, this, _1))
        ;
        subtree->create<double>("freq/value")
            .set(DEFAULT_CORDIC_FREQ)
            .coerce(boost::bind(&rx_dsp_core_3000::set_freq, this, _1))
        ;
        subtree->create<meta_range_t>("freq/range")
            .publish(boost::bind(&rx_dsp_core_3000::get_freq_range, this))
        ;
    }

private:
    wb_iface::sptr _iface;
    const size_t _dsp_base;
    const bool _is_b200;    //TODO: Obsolete this when we switch to the new DDC on the B200
    double _tick_rate, _link_rate;
    double _scaling_adjustment, _dsp_extra_scaling, _host_extra_scaling, _fxpt_scalar_correction;
};

rx_dsp_core_3000::sptr rx_dsp_core_3000::make(wb_iface::sptr iface, const size_t dsp_base, const bool is_b200 /* = false */)
{
    return sptr(new rx_dsp_core_3000_impl(iface, dsp_base, is_b200));
}
