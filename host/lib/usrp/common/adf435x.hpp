//
// Copyright 2015 Ettus Research LLC
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

#ifndef INCLUDED_ADF435X_HPP
#define INCLUDED_ADF435X_HPP

#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/log.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/math/special_functions/round.hpp>
#include <vector>
#include "adf4350_regs.hpp"
#include "adf4351_regs.hpp"

class adf435x_iface
{
public:
    typedef boost::shared_ptr<adf435x_iface> sptr;
    typedef boost::function<void(std::vector<boost::uint32_t>)> write_fn_t;

    static sptr make_adf4350(write_fn_t write);
    static sptr make_adf4351(write_fn_t write);

    virtual ~adf435x_iface() = 0;

    enum output_t { RF_OUTPUT_A, RF_OUTPUT_B };

    enum prescaler_t { PRESCALER_4_5, PRESCALER_8_9 };

    enum feedback_sel_t { FB_SEL_FUNDAMENTAL, FB_SEL_DIVIDED };

    enum output_power_t { OUTPUT_POWER_M4DBM, OUTPUT_POWER_M1DBM, OUTPUT_POWER_2DBM, OUTPUT_POWER_5DBM };

    enum muxout_t { MUXOUT_3STATE, MUXOUT_DVDD, MUXOUT_DGND, MUXOUT_RDIV, MUXOUT_NDIV, MUXOUT_ALD, MUXOUT_DLD };

    virtual void set_reference_freq(double fref) = 0;

    virtual void set_prescaler(prescaler_t prescaler) = 0;

    virtual void set_feedback_select(feedback_sel_t fb_sel) = 0;

    virtual void set_output_power(output_power_t power) = 0;

    virtual void set_output_enable(output_t output, bool enable) = 0;

    virtual void set_muxout_mode(muxout_t mode) = 0;

    virtual uhd::range_t get_int_range() = 0;

    virtual double set_frequency(double target_freq, bool int_n_mode, bool flush = false) = 0;

    virtual void commit(void) = 0;
};

template <typename adf435x_regs_t>
class adf435x_impl : public adf435x_iface
{
public:
    adf435x_impl(write_fn_t write_fn) :
        _write_fn(write_fn),
        _regs(),
        _fb_after_divider(false),
        _reference_freq(0.0),
        _N_min(-1)
    {}

    virtual ~adf435x_impl() {};

    void set_reference_freq(double fref)
    {
        _reference_freq = fref;
    }

    void set_feedback_select(feedback_sel_t fb_sel)
    {
        _fb_after_divider = (fb_sel == FB_SEL_DIVIDED);
    }

    void set_prescaler(prescaler_t prescaler)
    {
        if (prescaler == PRESCALER_8_9) {
            _regs.prescaler = adf435x_regs_t::PRESCALER_8_9;
            _N_min = 75;
        } else {
            _regs.prescaler = adf435x_regs_t::PRESCALER_4_5;
            _N_min = 23;
        }
    }

    void set_output_power(output_power_t power)
    {
        switch (power) {
            case OUTPUT_POWER_M4DBM: _regs.output_power = adf435x_regs_t::OUTPUT_POWER_M4DBM; break;
            case OUTPUT_POWER_M1DBM: _regs.output_power = adf435x_regs_t::OUTPUT_POWER_M1DBM; break;
            case OUTPUT_POWER_2DBM:  _regs.output_power = adf435x_regs_t::OUTPUT_POWER_2DBM; break;
            case OUTPUT_POWER_5DBM:  _regs.output_power = adf435x_regs_t::OUTPUT_POWER_5DBM; break;
            default: UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_output_enable(output_t output, bool enable)
    {
        switch (output) {
            case RF_OUTPUT_A: _regs.rf_output_enable = enable ? adf435x_regs_t::RF_OUTPUT_ENABLE_ENABLED:
                                                               adf435x_regs_t::RF_OUTPUT_ENABLE_DISABLED;
                              break;
            case RF_OUTPUT_B: _regs.aux_output_enable = enable ? adf435x_regs_t::AUX_OUTPUT_ENABLE_ENABLED:
                                                                 adf435x_regs_t::AUX_OUTPUT_ENABLE_DISABLED;
                              break;
        }
    }

    void set_muxout_mode(muxout_t mode)
    {
        switch (mode) {
            case MUXOUT_3STATE: _regs.muxout = adf435x_regs_t::MUXOUT_3STATE; break;
            case MUXOUT_DVDD:   _regs.muxout = adf435x_regs_t::MUXOUT_DVDD; break;
            case MUXOUT_DGND:   _regs.muxout = adf435x_regs_t::MUXOUT_DGND; break;
            case MUXOUT_RDIV:   _regs.muxout = adf435x_regs_t::MUXOUT_RDIV; break;
            case MUXOUT_NDIV:   _regs.muxout = adf435x_regs_t::MUXOUT_NDIV; break;
            case MUXOUT_ALD:    _regs.muxout = adf435x_regs_t::MUXOUT_ANALOG_LD; break;
            case MUXOUT_DLD:    _regs.muxout = adf435x_regs_t::MUXOUT_DLD; break;
            default: UHD_THROW_INVALID_CODE_PATH();
        }
    }

    uhd::range_t get_int_range()
    {
        if (_N_min < 0) throw uhd::runtime_error("set_prescaler must be called before get_int_range");
        return uhd::range_t(_N_min, 4095);
    }

    double set_frequency(double target_freq, bool int_n_mode, bool flush = false)
    {
        static const double REF_DOUBLER_THRESH_FREQ = 12.5e6;
        static const double PFD_FREQ_MAX            = 25.0e6;
        static const double BAND_SEL_FREQ_MAX       = 100e3;
        static const double VCO_FREQ_MIN            = 2.2e9;
        static const double VCO_FREQ_MAX            = 4.4e9;

        //Default invalid value for actual_freq
        double actual_freq = 0;

        uhd::range_t rf_divider_range = _get_rfdiv_range();
        uhd::range_t int_range = get_int_range();

        double pfd_freq = 0;
        boost::uint16_t R = 0, BS = 0, N = 0, FRAC = 0, MOD = 0;
        boost::uint16_t RFdiv = static_cast<boost::uint16_t>(rf_divider_range.start());
        bool D = false, T = false;

        //Reference doubler for 50% duty cycle
        D = (_reference_freq <= REF_DOUBLER_THRESH_FREQ);

        //increase RF divider until acceptable VCO frequency
        double vco_freq = target_freq;
        while (vco_freq < VCO_FREQ_MIN && RFdiv < static_cast<boost::uint16_t>(rf_divider_range.stop())) {
            vco_freq *= 2;
            RFdiv *= 2;
        }

        /*
         * The goal here is to loop though possible R dividers,
         * band select clock dividers, N (int) dividers, and FRAC
         * (frac) dividers.
         *
         * Calculate the N and F dividers for each set of values.
         * The loop exits when it meets all of the constraints.
         * The resulting loop values are loaded into the registers.
         *
         * from pg.21
         *
         * f_pfd = f_ref*(1+D)/(R*(1+T))
         * f_vco = (N + (FRAC/MOD))*f_pfd
         *    N = f_vco/f_pfd - FRAC/MOD = f_vco*((R*(T+1))/(f_ref*(1+D))) - FRAC/MOD
         * f_actual = f_vco/RFdiv)
         */
        double feedback_freq = _fb_after_divider ? target_freq : vco_freq;

        for(R = 1; R <= 1023; R+=1){
            //PFD input frequency = f_ref/R ... ignoring Reference doubler/divide-by-2 (D & T)
            pfd_freq = _reference_freq*(D?2:1)/(R*(T?2:1));

            //keep the PFD frequency at or below 25MHz (Loop Filter Bandwidth)
            if (pfd_freq > PFD_FREQ_MAX) continue;

            //First, ignore fractional part of tuning
            N = boost::uint16_t(std::floor(feedback_freq/pfd_freq));

            //keep N > minimum int divider requirement
            if (N < static_cast<boost::uint16_t>(int_range.start())) continue;

            for(BS=1; BS <= 255; BS+=1){
                //keep the band select frequency at or below band_sel_freq_max
                //constraint on band select clock
                if (pfd_freq/BS > BAND_SEL_FREQ_MAX) continue;
                goto done_loop;
            }
        } done_loop:

        //Fractional-N calculation
        MOD = 4095; //max fractional accuracy
        FRAC = static_cast<boost::uint16_t>(boost::math::round((feedback_freq/pfd_freq - N)*MOD));
        if (int_n_mode) {
            if (FRAC > (MOD / 2)) { //Round integer such that actual freq is closest to target
                N++;
            }
            FRAC = 0;
        }

        //Reference divide-by-2 for 50% duty cycle
        // if R even, move one divide by 2 to to regs.reference_divide_by_2
        if(R % 2 == 0) {
            T = true;
            R /= 2;
        }

        //Typical phase resync time documented in data sheet pg.24
        static const double PHASE_RESYNC_TIME = 400e-6;

        //If feedback after divider, then compensation for the divider is pulled into the INT value
        int rf_div_compensation = _fb_after_divider ? 1 : RFdiv;

        //Compute the actual frequency in terms of _reference_freq, N, FRAC, MOD, D, R and T.
        actual_freq = (
            double((N + (double(FRAC)/double(MOD))) *
            (_reference_freq*(D?2:1)/(R*(T?2:1))))
        ) / rf_div_compensation;

        _regs.frac_12_bit            = FRAC;
        _regs.int_16_bit             = N;
        _regs.mod_12_bit             = MOD;
        _regs.clock_divider_12_bit   = std::max<boost::uint16_t>(1, boost::uint16_t(std::ceil(PHASE_RESYNC_TIME*pfd_freq/MOD)));
        _regs.feedback_select        = _fb_after_divider ?
                                        adf435x_regs_t::FEEDBACK_SELECT_DIVIDED :
                                        adf435x_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
        _regs.clock_div_mode         = _fb_after_divider ?
                                        adf435x_regs_t::CLOCK_DIV_MODE_RESYNC_ENABLE :
                                        adf435x_regs_t::CLOCK_DIV_MODE_FAST_LOCK;
        _regs.r_counter_10_bit       = R;
        _regs.reference_divide_by_2  = T ?
                                        adf435x_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED :
                                        adf435x_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
        _regs.reference_doubler      = D ?
                                        adf435x_regs_t::REFERENCE_DOUBLER_ENABLED :
                                        adf435x_regs_t::REFERENCE_DOUBLER_DISABLED;
        _regs.band_select_clock_div  = boost::uint8_t(BS);
        _regs.rf_divider_select      = static_cast<typename adf435x_regs_t::rf_divider_select_t>(_get_rfdiv_setting(RFdiv));
        _regs.ldf                    = int_n_mode ?
                                        adf435x_regs_t::LDF_INT_N :
                                        adf435x_regs_t::LDF_FRAC_N;

        std::string tuning_str = (int_n_mode) ? "Integer-N" : "Fractional";
        UHD_LOGV(often)
            << boost::format("ADF 435X Frequencies (MHz): REQUESTED=%0.9f, ACTUAL=%0.9f"
            ) % (target_freq/1e6) % (actual_freq/1e6) << std::endl
            << boost::format("ADF 435X Intermediates (MHz): Feedback=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f, REF=%0.2f"
            ) % (feedback_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) % (_reference_freq/1e6) << std::endl
            << boost::format("ADF 435X Tuning: %s") % tuning_str.c_str() << std::endl
            << boost::format("ADF 435X Settings: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d"
            ) % R % BS % N % FRAC % MOD % T % D % RFdiv << std::endl;

        UHD_ASSERT_THROW((_regs.frac_12_bit          & ((boost::uint16_t)~0xFFF)) == 0);
        UHD_ASSERT_THROW((_regs.mod_12_bit           & ((boost::uint16_t)~0xFFF)) == 0);
        UHD_ASSERT_THROW((_regs.clock_divider_12_bit & ((boost::uint16_t)~0xFFF)) == 0);
        UHD_ASSERT_THROW((_regs.r_counter_10_bit     & ((boost::uint16_t)~0x3FF)) == 0);

        UHD_ASSERT_THROW(vco_freq >= VCO_FREQ_MIN and vco_freq <= VCO_FREQ_MAX);
        UHD_ASSERT_THROW(RFdiv >= static_cast<boost::uint16_t>(rf_divider_range.start()));
        UHD_ASSERT_THROW(RFdiv <= static_cast<boost::uint16_t>(rf_divider_range.stop()));
        UHD_ASSERT_THROW(_regs.int_16_bit >= static_cast<boost::uint16_t>(int_range.start()));
        UHD_ASSERT_THROW(_regs.int_16_bit <= static_cast<boost::uint16_t>(int_range.stop()));

        if (flush) commit();
        return actual_freq;
    }

    void commit()
    {
        //reset counters
        _regs.counter_reset = adf435x_regs_t::COUNTER_RESET_ENABLED;
        std::vector<boost::uint32_t> regs;
        regs.push_back(_regs.get_reg(boost::uint32_t(2)));
        _write_fn(regs);
        _regs.counter_reset = adf435x_regs_t::COUNTER_RESET_DISABLED;

        //write the registers
        //correct power-up sequence to write registers (5, 4, 3, 2, 1, 0)
        regs.clear();
        for (int addr = 5; addr >= 0; addr--) {
            regs.push_back(_regs.get_reg(boost::uint32_t(addr)));
        }
        _write_fn(regs);
    }

protected:
    uhd::range_t _get_rfdiv_range();
    int _get_rfdiv_setting(boost::uint16_t div);

    write_fn_t      _write_fn;
    adf435x_regs_t  _regs;
    double          _fb_after_divider;
    double          _reference_freq;
    int             _N_min;
};

template <>
inline uhd::range_t adf435x_impl<adf4350_regs_t>::_get_rfdiv_range()
{
    return uhd::range_t(1, 16);
}

template <>
inline uhd::range_t adf435x_impl<adf4351_regs_t>::_get_rfdiv_range()
{
    return uhd::range_t(1, 64);
}

template <>
inline int adf435x_impl<adf4350_regs_t>::_get_rfdiv_setting(boost::uint16_t div)
{
    switch (div) {
        case 1:  return int(adf4350_regs_t::RF_DIVIDER_SELECT_DIV1);
        case 2:  return int(adf4350_regs_t::RF_DIVIDER_SELECT_DIV2);
        case 4:  return int(adf4350_regs_t::RF_DIVIDER_SELECT_DIV4);
        case 8:  return int(adf4350_regs_t::RF_DIVIDER_SELECT_DIV8);
        case 16: return int(adf4350_regs_t::RF_DIVIDER_SELECT_DIV16);
        default: UHD_THROW_INVALID_CODE_PATH();
    }
}

template <>
inline int adf435x_impl<adf4351_regs_t>::_get_rfdiv_setting(boost::uint16_t div)
{
    switch (div) {
        case 1:  return int(adf4351_regs_t::RF_DIVIDER_SELECT_DIV1);
        case 2:  return int(adf4351_regs_t::RF_DIVIDER_SELECT_DIV2);
        case 4:  return int(adf4351_regs_t::RF_DIVIDER_SELECT_DIV4);
        case 8:  return int(adf4351_regs_t::RF_DIVIDER_SELECT_DIV8);
        case 16: return int(adf4351_regs_t::RF_DIVIDER_SELECT_DIV16);
        case 32: return int(adf4351_regs_t::RF_DIVIDER_SELECT_DIV32);
        case 64: return int(adf4351_regs_t::RF_DIVIDER_SELECT_DIV64);
        default: UHD_THROW_INVALID_CODE_PATH();
    }
}

#endif // INCLUDED_ADF435X_HPP
