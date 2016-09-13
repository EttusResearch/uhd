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

#include "adf5355.hpp"
#include "adf5355_regs.hpp"
#include <uhd/utils/math.hpp>
#include <boost/math/common_factor_rt.hpp> //gcd
#include <boost/thread.hpp>

using namespace uhd;

template<typename data_t>
data_t clamp(data_t val, data_t min, data_t max) {
    return (val < min) ? min : ((val > max) ? max : val);
}

template<typename data_t>
double todbl(data_t val) {
    return static_cast<double>(val);
}

static const double ADF5355_DOUBLER_MAX_REF_FREQ    = 60e6;
static const double ADF5355_MAX_FREQ_PFD            = 125e6;
static const double ADF5355_PRESCALER_THRESH        = 7e9;

static const double ADF5355_MIN_VCO_FREQ            = 3.4e9;
static const double ADF5355_MAX_VCO_FREQ            = 6.8e9;
static const double ADF5355_MAX_OUT_FREQ            = 6.8e9;
static const double ADF5355_MIN_OUT_FREQ            = (3.4e9 / 64);
static const double ADF5355_MAX_OUTB_FREQ           = (6.8e9 * 2);
static const double ADF5355_MIN_OUTB_FREQ           = (3.4e9 * 2);

static const double ADF5355_PHASE_RESYNC_TIME       = 400e-6;

static const boost::uint32_t ADF5355_MOD1           = 16777216;
static const boost::uint32_t ADF5355_MAX_MOD2       = 16384;
static const boost::uint16_t ADF5355_MIN_INT_PRESCALER_89 = 75;

class adf5355_impl : public adf5355_iface
{
public:
    adf5355_impl(write_fn_t write_fn) :
        _write_fn(write_fn),
        _regs(),
        _rewrite_regs(true),
        _wait_time_us(0),
        _ref_freq(0.0),
        _pfd_freq(0.0),
        _fb_after_divider(false)
    {
        _regs.counter_reset = adf5355_regs_t::COUNTER_RESET_DISABLED;
        _regs.cp_three_state = adf5355_regs_t::CP_THREE_STATE_DISABLED;
        _regs.power_down = adf5355_regs_t::POWER_DOWN_DISABLED;
        _regs.pd_polarity = adf5355_regs_t::PD_POLARITY_POSITIVE;
        _regs.mux_logic = adf5355_regs_t::MUX_LOGIC_3_3V;
        _regs.ref_mode = adf5355_regs_t::REF_MODE_SINGLE;
        _regs.muxout = adf5355_regs_t::MUXOUT_DLD;
        _regs.double_buff_div = adf5355_regs_t::DOUBLE_BUFF_DIV_DISABLED;

        _regs.rf_out_a_enabled = adf5355_regs_t::RF_OUT_A_ENABLED_ENABLED;
        _regs.rf_out_b_enabled = adf5355_regs_t::RF_OUT_B_ENABLED_DISABLED;
        _regs.mute_till_lock_detect = adf5355_regs_t::MUTE_TILL_LOCK_DETECT_MUTE_DISABLED;
        _regs.ld_mode = adf5355_regs_t::LD_MODE_FRAC_N;
        _regs.frac_n_ld_precision = adf5355_regs_t::FRAC_N_LD_PRECISION_5NS;
        _regs.ld_cyc_count = adf5355_regs_t::LD_CYC_COUNT_1024;
        _regs.le_sync = adf5355_regs_t::LE_SYNC_LE_SYNCED_TO_REFIN;
        _regs.phase_resync = adf5355_regs_t::PHASE_RESYNC_DISABLED;
        _regs.reference_divide_by_2 = adf5355_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
        _regs.reference_doubler = adf5355_regs_t::REFERENCE_DOUBLER_DISABLED;

        _regs.autocal_en = adf5355_regs_t::AUTOCAL_EN_ENABLED;
        _regs.prescaler = adf5355_regs_t::PRESCALER_4_5;
        _regs.charge_pump_current = adf5355_regs_t::CHARGE_PUMP_CURRENT_0_94MA;

        _regs.gated_bleed = adf5355_regs_t::GATED_BLEED_DISABLED;
        _regs.negative_bleed = adf5355_regs_t::NEGATIVE_BLEED_ENABLED;
        _regs.feedback_select = adf5355_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
        _regs.output_power = adf5355_regs_t::OUTPUT_POWER_5DBM;
        _regs.cp_bleed_current = 2;
        _regs.r_counter_10_bit = 8;


        _regs.ld_cyc_count = adf5355_regs_t::LD_CYC_COUNT_1024;
        _regs.loss_of_lock_mode = adf5355_regs_t::LOSS_OF_LOCK_MODE_DISABLED;
        _regs.frac_n_ld_precision = adf5355_regs_t::FRAC_N_LD_PRECISION_5NS;
        _regs.ld_mode = adf5355_regs_t::LD_MODE_FRAC_N;

        _regs.vco_band_div = 3;
        _regs.timeout = 11;
        _regs.auto_level_timeout = 30;
        _regs.synth_lock_timeout = 12;

        _regs.adc_clock_divider = 16;
        _regs.adc_conversion = adf5355_regs_t::ADC_CONVERSION_ENABLED;
        _regs.adc_enable = adf5355_regs_t::ADC_ENABLE_ENABLED;

        _regs.phase_resync_clk_div = 0;
    }

    ~adf5355_impl()
    {
        _regs.power_down = adf5355_regs_t::POWER_DOWN_ENABLED;
        commit();
    }

    void set_feedback_select(feedback_sel_t fb_sel)
    {
        _fb_after_divider = (fb_sel == FB_SEL_DIVIDED);

        if (_fb_after_divider) {
            _regs.feedback_select = adf5355_regs_t::FEEDBACK_SELECT_DIVIDED;
        } else {
            _regs.feedback_select = adf5355_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
        }
    }

    void set_reference_freq(double fref, bool force = false)
    {
        //Skip the body if the reference frequency does not change
        if (uhd::math::frequencies_are_equal(fref, _ref_freq) and (not force))
            return;

        _ref_freq = fref;

        //-----------------------------------------------------------
        //Set reference settings

        //Reference doubler for 50% duty cycle
        bool doubler_en = (_ref_freq <= ADF5355_DOUBLER_MAX_REF_FREQ);

        /* Calculate and maximize PFD frequency */
        // TODO Target PFD should be configurable
        /* TwinRX requires PFD of 6.25 MHz or less */
        const double TWINRX_PFD_FREQ = 6.25e6;
        _pfd_freq = TWINRX_PFD_FREQ;

        int ref_div_factor = 16;

        //Reference divide-by-2 for 50% duty cycle
        // if R even, move one divide by 2 to to regs.reference_divide_by_2
        bool div2_en = (ref_div_factor % 2 == 0);
        if (div2_en) {
            ref_div_factor /= 2;
        }

        _regs.reference_divide_by_2 = div2_en ?
            adf5355_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED :
            adf5355_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
        _regs.reference_doubler = doubler_en ?
            adf5355_regs_t::REFERENCE_DOUBLER_ENABLED :
            adf5355_regs_t::REFERENCE_DOUBLER_DISABLED;
        _regs.r_counter_10_bit = ref_div_factor;
        UHD_ASSERT_THROW((_regs.r_counter_10_bit & ((boost::uint16_t)~0x3FF)) == 0);

        //-----------------------------------------------------------
        //Set timeouts (code from ADI driver)
        _regs.timeout = clamp<boost::uint16_t>(
            static_cast<boost::uint16_t>(ceil(_pfd_freq / (20e3 * 30))), 1, 1023);
        UHD_ASSERT_THROW((_regs.timeout & ((boost::uint16_t)~0x3FF)) == 0);
        _regs.synth_lock_timeout =
            static_cast<boost::uint8_t>(ceil((_pfd_freq * 2) / (100e3 * _regs.timeout)));
        UHD_ASSERT_THROW((_regs.synth_lock_timeout & ((boost::uint16_t)~0x1F)) == 0);
        _regs.auto_level_timeout =
            static_cast<boost::uint8_t>(ceil((_pfd_freq * 5) / (100e3 * _regs.timeout)));

        //-----------------------------------------------------------
        //Set VCO band divider
        _regs.vco_band_div =
            static_cast<boost::uint8_t>(ceil(_pfd_freq / 2.4e6));

        //-----------------------------------------------------------
        //Set ADC delay (code from ADI driver)
        _regs.adc_enable = adf5355_regs_t::ADC_ENABLE_ENABLED;
        _regs.adc_conversion = adf5355_regs_t::ADC_CONVERSION_ENABLED;
        _regs.adc_clock_divider = clamp<boost::uint8_t>(
            static_cast<boost::uint8_t>(ceil(((_pfd_freq / 100e3) - 2) / 4)), 1, 255);
        _wait_time_us = static_cast<boost::uint32_t>(
            ceil(16e6 / (_pfd_freq / ((4 * _regs.adc_clock_divider) + 2))));

        //-----------------------------------------------------------
        //Phase resync
        _regs.phase_resync = adf5355_regs_t::PHASE_RESYNC_DISABLED; // Disabled during development
        _regs.phase_adjust = adf5355_regs_t::PHASE_ADJUST_DISABLED;
        _regs.sd_load_reset = adf5355_regs_t::SD_LOAD_RESET_ON_REG0_UPDATE;
        _regs.phase_resync_clk_div = static_cast<boost::uint16_t>(
            floor(ADF5355_PHASE_RESYNC_TIME * _pfd_freq));

        _rewrite_regs = true;
    }

    void set_output_power(output_power_t power)
    {
        adf5355_regs_t::output_power_t setting;
        switch (power) {
            case OUTPUT_POWER_M4DBM: setting = adf5355_regs_t::OUTPUT_POWER_M4DBM; break;
            case OUTPUT_POWER_M1DBM: setting = adf5355_regs_t::OUTPUT_POWER_M1DBM; break;
            case OUTPUT_POWER_2DBM:  setting = adf5355_regs_t::OUTPUT_POWER_2DBM; break;
            case OUTPUT_POWER_5DBM:  setting = adf5355_regs_t::OUTPUT_POWER_5DBM; break;
            default: UHD_THROW_INVALID_CODE_PATH();
        }
        if (_regs.output_power != setting) _rewrite_regs = true;
        _regs.output_power = setting;
    }

    void set_output_enable(output_t output, bool enable) {

        switch (output) {
            case RF_OUTPUT_A: _regs.rf_out_a_enabled = enable ? adf5355_regs_t::RF_OUT_A_ENABLED_ENABLED :
                                                                adf5355_regs_t::RF_OUT_A_ENABLED_DISABLED;
                              break;
            case RF_OUTPUT_B: _regs.rf_out_b_enabled = enable ? adf5355_regs_t::RF_OUT_B_ENABLED_ENABLED :
                                                                adf5355_regs_t::RF_OUT_B_ENABLED_DISABLED;
                              break;
        }
    }

    void set_muxout_mode(muxout_t mode)
    {
        switch (mode) {
            case MUXOUT_3STATE: _regs.muxout = adf5355_regs_t::MUXOUT_3STATE; break;
            case MUXOUT_DVDD:   _regs.muxout = adf5355_regs_t::MUXOUT_DVDD; break;
            case MUXOUT_DGND:   _regs.muxout = adf5355_regs_t::MUXOUT_DGND; break;
            case MUXOUT_RDIV:   _regs.muxout = adf5355_regs_t::MUXOUT_RDIV; break;
            case MUXOUT_NDIV:   _regs.muxout = adf5355_regs_t::MUXOUT_NDIV; break;
            case MUXOUT_ALD:    _regs.muxout = adf5355_regs_t::MUXOUT_ANALOG_LD; break;
            case MUXOUT_DLD:    _regs.muxout = adf5355_regs_t::MUXOUT_DLD; break;
            default: UHD_THROW_INVALID_CODE_PATH();
        }
    }

    double set_frequency(double target_freq, double freq_resolution, bool flush = false)
    {
        if (target_freq > ADF5355_MAX_OUT_FREQ or target_freq < ADF5355_MIN_OUT_FREQ) {
            throw uhd::runtime_error("requested frequency out of range.");
        }
        if ((boost::uint32_t) freq_resolution == 0) {
            throw uhd::runtime_error("requested resolution cannot be less than 1.");
        }

        /* Calculate target VCOout frequency */
        //Increase RF divider until acceptable VCO frequency
        double target_vco_freq = target_freq;
        boost::uint32_t rf_divider = 1;
        while (target_vco_freq < ADF5355_MIN_VCO_FREQ && rf_divider < 64) {
            target_vco_freq *= 2;
            rf_divider *= 2;
        }

        switch (rf_divider) {
            case 1:  _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV1;  break;
            case 2:  _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV2;  break;
            case 4:  _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV4;  break;
            case 8:  _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV8;  break;
            case 16: _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV16; break;
            case 32: _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV32; break;
            case 64: _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV64; break;
            default: UHD_THROW_INVALID_CODE_PATH();
        }

        //Compute fractional PLL params
        double prescaler_input_freq = target_vco_freq;
        if (_fb_after_divider) {
            prescaler_input_freq /= rf_divider;
        }

        double N = prescaler_input_freq / _pfd_freq;
        boost::uint16_t INT = static_cast<boost::uint16_t>(floor(N));
        boost::uint32_t FRAC1 = static_cast<boost::uint32_t>(floor((N - INT) * ADF5355_MOD1));
        double residue = ADF5355_MOD1 * (N - (INT + FRAC1 / ADF5355_MOD1));

        double gcd = boost::math::gcd(static_cast<int>(_pfd_freq), static_cast<int>(freq_resolution));
        boost::uint16_t MOD2 = static_cast<boost::uint16_t>(floor(_pfd_freq / gcd));

        if (MOD2 > ADF5355_MAX_MOD2) {
            MOD2 = ADF5355_MAX_MOD2;
        }
        boost::uint16_t FRAC2 = ceil(residue * MOD2);

        double coerced_vco_freq = _pfd_freq * (
            todbl(INT) + (
                (todbl(FRAC1) +
                    (todbl(FRAC2) / todbl(MOD2)))
                / todbl(ADF5355_MOD1)
            )
        );

        double coerced_out_freq = coerced_vco_freq / rf_divider;

        /* Update registers */
        _regs.int_16_bit = INT;
        _regs.frac1_24_bit = FRAC1;
        _regs.frac2_14_bit = FRAC2;
        _regs.mod2_14_bit = MOD2;
        _regs.phase_24_bit = 0;

/*
        if (_regs.int_16_bit >= ADF5355_MIN_INT_PRESCALER_89) {
            _regs.prescaler = adf5355_regs_t::PRESCALER_8_9;
        } else {
            _regs.prescaler = adf5355_regs_t::PRESCALER_4_5;
        }

        // ADI: Tests have shown that the optimal bleed set is the following:
        // 4/N < IBLEED/ICP < 10/N */
/*
        boost::uint32_t cp_curr_ua =
            (static_cast<boost::uint32_t>(_regs.charge_pump_current) + 1) * 315;
        _regs.cp_bleed_current = clamp<boost::uint8_t>(
            ceil((todbl(400)*cp_curr_ua) / (_regs.int_16_bit*375)), 1, 255);
        _regs.negative_bleed = adf5355_regs_t::NEGATIVE_BLEED_ENABLED;
        _regs.gated_bleed = adf5355_regs_t::GATED_BLEED_DISABLED;
*/

        if (flush) commit();
        return coerced_out_freq;
    }

    void commit()
    {
        if (_rewrite_regs) {
            //For a full state sync write registers in reverse order 12 - 0
            addr_vtr_t regs;
            for (int addr = 12; addr >= 0; addr--) {
                regs.push_back(_regs.get_reg(boost::uint32_t(addr)));
            }
            _write_fn(regs);
            _rewrite_regs = false;

        } else {
            //Frequency update sequence from data sheet
            static const size_t ONE_REG = 1;
            _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(6)));
            _regs.counter_reset = adf5355_regs_t::COUNTER_RESET_ENABLED;
            _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(4)));
            _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(2)));
            _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(1)));
            _regs.autocal_en = adf5355_regs_t::AUTOCAL_EN_DISABLED;
            _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
            _regs.counter_reset = adf5355_regs_t::COUNTER_RESET_DISABLED;
            _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(4)));
            boost::this_thread::sleep(boost::posix_time::microsec(_wait_time_us));
            _regs.autocal_en = adf5355_regs_t::AUTOCAL_EN_ENABLED;
            _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
        }
    }

private: //Members
    typedef std::vector<boost::uint32_t> addr_vtr_t;

    write_fn_t      _write_fn;
    adf5355_regs_t  _regs;
    bool            _rewrite_regs;
    boost::uint32_t _wait_time_us;
    double          _ref_freq;
    double          _pfd_freq;
    double          _fb_after_divider;
};

adf5355_iface::sptr adf5355_iface::make(write_fn_t write)
{
    return sptr(new adf5355_impl(write));
}
