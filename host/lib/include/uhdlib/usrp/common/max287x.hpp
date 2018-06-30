//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef MAX287X_HPP_INCLUDED
#define MAX287X_HPP_INCLUDED

#include "max2870_regs.hpp"
#include "max2871_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/assign.hpp>
#include <boost/function.hpp>
#include <boost/math/special_functions/round.hpp>
#include <vector>
#include <chrono>
#include <thread>
#include <stdint.h>

/**
 * MAX287x interface
 */
class max287x_iface
{
public:
    typedef boost::shared_ptr<max287x_iface> sptr;

    typedef boost::function<void(std::vector<uint32_t>)> write_fn;

    /**
     * LD Pin Modes
     */
    typedef enum{
        LD_PIN_MODE_LOW,
        LD_PIN_MODE_DLD,
        LD_PIN_MODE_ALD,
        LD_PIN_MODE_HIGH
    } ld_pin_mode_t;

    /**
     * MUXOUT Modes
     */
    typedef enum{
        MUXOUT_TRI_STATE,
        MUXOUT_HIGH,
        MUXOUT_LOW,
        MUXOUT_RDIV,
        MUXOUT_NDIV,
        MUXOUT_ALD,
        MUXOUT_DLD,
        MUXOUT_SYNC,
        MUXOUT_SPI
    } muxout_mode_t;

    /**
     * Charge Pump Currents
     */
    typedef enum{
        CHARGE_PUMP_CURRENT_0_32MA,
        CHARGE_PUMP_CURRENT_0_64MA,
        CHARGE_PUMP_CURRENT_0_96MA,
        CHARGE_PUMP_CURRENT_1_28MA,
        CHARGE_PUMP_CURRENT_1_60MA,
        CHARGE_PUMP_CURRENT_1_92MA,
        CHARGE_PUMP_CURRENT_2_24MA,
        CHARGE_PUMP_CURRENT_2_56MA,
        CHARGE_PUMP_CURRENT_2_88MA,
        CHARGE_PUMP_CURRENT_3_20MA,
        CHARGE_PUMP_CURRENT_3_52MA,
        CHARGE_PUMP_CURRENT_3_84MA,
        CHARGE_PUMP_CURRENT_4_16MA,
        CHARGE_PUMP_CURRENT_4_48MA,
        CHARGE_PUMP_CURRENT_4_80MA,
        CHARGE_PUMP_CURRENT_5_12MA
    } charge_pump_current_t;

    /**
     * Output Powers
     */
    typedef enum{
        OUTPUT_POWER_M4DBM,
        OUTPUT_POWER_M1DBM,
        OUTPUT_POWER_2DBM,
        OUTPUT_POWER_5DBM
    } output_power_t;

    typedef enum {
        LOW_NOISE_AND_SPUR_LOW_NOISE,
        LOW_NOISE_AND_SPUR_LOW_SPUR_1,
        LOW_NOISE_AND_SPUR_LOW_SPUR_2
    } low_noise_and_spur_t;

    typedef enum {
        CLOCK_DIV_MODE_CLOCK_DIVIDER_OFF,
        CLOCK_DIV_MODE_FAST_LOCK,
        CLOCK_DIV_MODE_PHASE
    } clock_divider_mode_t;

    /**
     * Make a synthesizer
     * @param write write function
     * @return shared pointer to object
     */
    template <typename max287X_t> static sptr make(write_fn write)
    {
        return sptr(new max287X_t(write));
    }

    /**
     * Destructor
     */
    virtual ~max287x_iface() {};

    /**
     * Power up the synthesizer
     */
    virtual void power_up(void) = 0;

    /**
     * Shut down the synthesizer
     */
    virtual void shutdown(void) = 0;

    /**
     * Check if the synthesizer is shut down
     */
    virtual bool is_shutdown(void) = 0;

    /**
     * Set frequency
     * @param target_freq target frequency
     * @param ref_freq reference frequency
     * @param target_pfd_freq target phase detector frequency
     * @param is_int_n enable integer-N tuning
     * @return actual frequency
     */
    virtual double set_frequency(
                double target_freq,
                double ref_freq,
                double target_pfd_freq,
                bool is_int_n) = 0;

    /**
     * Set output power
     * @param power output power
     */
    virtual void set_output_power(output_power_t power) = 0;

    /**
     * Set lock detect pin mode
     * @param mode lock detect pin mode
     */
    virtual void set_ld_pin_mode(ld_pin_mode_t mode) = 0;

    /**
     * Set muxout pin mode
     * @param mode muxout mode
     */
    virtual void set_muxout_mode(muxout_mode_t mode) = 0;

    /**
     * Set charge pump current
     * @param cp_current charge pump current
     */
    virtual void set_charge_pump_current(charge_pump_current_t cp_current) = 0;

    /**
     * Enable or disable auto retune
     * @param enabled enable auto retune
     */
    virtual void set_auto_retune(bool enabled) = 0;

    /**
     * Set clock divider mode
     * @param mode clock divider mode
     */
    virtual void set_clock_divider_mode(clock_divider_mode_t mode) = 0;

    /**
     * Enable or disable cycle slip mode
     * @param enabled enable cycle slip mode
     */
    virtual void set_cycle_slip_mode(bool enabled) = 0;

    /**
     * Set low noise and spur mode
     * @param mode low noise and spur mode
     */
    virtual void set_low_noise_and_spur(low_noise_and_spur_t mode) = 0;

    /**
     * Set phase
     * @param phase the phase offset
     */
    virtual void set_phase(uint16_t phase) = 0;

    /**
     * Write values configured by the set_* functions.
     */
    virtual void commit(void) = 0;

    /**
     * Check whether this is in a state where it can be synchronized
     */
    virtual bool can_sync(void) = 0;

    /**
     * Configure synthesizer for phase synchronization
     */
    virtual void config_for_sync(bool enable) = 0;
};

/**
 * MAX287x
 * Base class for all MAX287x synthesizers
 */
template <typename max287x_regs_t>
class max287x : public max287x_iface
{
public:
    max287x(write_fn func);
    virtual ~max287x();
    virtual void power_up(void);
    virtual void shutdown(void);
    virtual bool is_shutdown(void);
    virtual double set_frequency(
        double target_freq,
        double ref_freq,
        double target_pfd_freq,
        bool is_int_n);
    virtual void set_output_power(output_power_t power);
    virtual void set_ld_pin_mode(ld_pin_mode_t mode);
    virtual void set_muxout_mode(muxout_mode_t mode);
    virtual void set_charge_pump_current(charge_pump_current_t cp_current);
    virtual void set_auto_retune(bool enabled);
    virtual void set_clock_divider_mode(clock_divider_mode_t mode);
    virtual void set_cycle_slip_mode(bool enabled);
    virtual void set_low_noise_and_spur(low_noise_and_spur_t mode);
    virtual void set_phase(uint16_t phase);
    virtual void commit();
    virtual bool can_sync();
    virtual void config_for_sync(bool enable);

protected:
    max287x_regs_t _regs;
    bool _can_sync;
    bool _config_for_sync;
    bool _write_all_regs;

private:
    write_fn _write;
    bool _delay_after_write;
};

/**
 * MAX2870
 */
class max2870 : public max287x<max2870_regs_t>
{
public:
    max2870(write_fn func) : max287x<max2870_regs_t>(func) {}
    ~max2870() {}
    double set_frequency(
        double target_freq,
        double ref_freq,
        double target_pfd_freq,
        bool is_int_n)
    {
        _regs.cpoc = is_int_n ? max2870_regs_t::CPOC_ENABLED : max2870_regs_t::CPOC_DISABLED;
        _regs.feedback_select = target_freq >= 3.0e9 ?
            max2870_regs_t::FEEDBACK_SELECT_DIVIDED :
            max2870_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;

        return max287x<max2870_regs_t>::set_frequency(target_freq, ref_freq, target_pfd_freq, is_int_n);
    }
    void commit(void)
    {
        // For MAX2870, we always need to write all registers.
        _write_all_regs = true;
        max287x<max2870_regs_t>::commit();
    }
};

/**
 * MAX2871
 */
// Table of frequency ranges for each VCO value.
// The values were derived from sampling multiple
// units over a temperature range of -10 to 40 deg C.
typedef std::map<uint8_t,uhd::range_t> vco_map_t;
static const vco_map_t max2871_vco_map =
    boost::assign::map_list_of
    (0,uhd::range_t(2767776024.0,2838472816.0))
    (1,uhd::range_t(2838472816.0,2879070053.0))
    (1,uhd::range_t(2879070053.0,2921202504.0))
    (3,uhd::range_t(2921202504.0,2960407579.0))
    (4,uhd::range_t(2960407579.0,3001687422.0))
    (5,uhd::range_t(3001687422.0,3048662562.0))
    (6,uhd::range_t(3048662562.0,3097511550.0))
    (7,uhd::range_t(3097511550.0,3145085864.0))
    (8,uhd::range_t(3145085864.0,3201050835.0))
    (9,uhd::range_t(3201050835.0,3259581909.0))
    (10,uhd::range_t(3259581909.0,3321408729.0))
    (11,uhd::range_t(3321408729.0,3375217285.0))
    (12,uhd::range_t(3375217285.0,3432807972.0))
    (13,uhd::range_t(3432807972.0,3503759088.0))
    (14,uhd::range_t(3503759088.0,3579011283.0))
    (15,uhd::range_t(3579011283.0,3683570865.0))
    (20,uhd::range_t(3683570865.0,3711845712.0))
    (21,uhd::range_t(3711845712.0,3762188221.0))
    (22,uhd::range_t(3762188221.0,3814209551.0))
    (23,uhd::range_t(3814209551.0,3865820020.0))
    (24,uhd::range_t(3865820020.0,3922520021.0))
    (25,uhd::range_t(3922520021.0,3981682709.0))
    (26,uhd::range_t(3981682709.0,4043154280.0))
    (27,uhd::range_t(4043154280.0,4100400020.0))
    (28,uhd::range_t(4100400020.0,4159647583.0))
    (29,uhd::range_t(4159647583.0,4228164842.0))
    (30,uhd::range_t(4228164842.0,4299359879.0))
    (31,uhd::range_t(4299359879.0,4395947962.0))
    (33,uhd::range_t(4395947962.0,4426512061.0))
    (34,uhd::range_t(4426512061.0,4480333656.0))
    (35,uhd::range_t(4480333656.0,4526297331.0))
    (36,uhd::range_t(4526297331.0,4574689510.0))
    (37,uhd::range_t(4574689510.0,4633102021.0))
    (38,uhd::range_t(4633102021.0,4693755616.0))
    (39,uhd::range_t(4693755616.0,4745624435.0))
    (40,uhd::range_t(4745624435.0,4803922123.0))
    (41,uhd::range_t(4803922123.0,4871523881.0))
    (42,uhd::range_t(4871523881.0,4942111286.0))
    (43,uhd::range_t(4942111286.0,5000192446.0))
    (44,uhd::range_t(5000192446.0,5059567510.0))
    (45,uhd::range_t(5059567510.0,5136258187.0))
    (46,uhd::range_t(5136258187.0,5215827295.0))
    (47,uhd::range_t(5215827295.0,5341282949.0))
    (49,uhd::range_t(5341282949.0,5389819310.0))
    (50,uhd::range_t(5389819310.0,5444868434.0))
    (51,uhd::range_t(5444868434.0,5500079705.0))
    (52,uhd::range_t(5500079705.0,5555329630.0))
    (53,uhd::range_t(5555329630.0,5615049833.0))
    (54,uhd::range_t(5615049833.0,5676098527.0))
    (55,uhd::range_t(5676098527.0,5744191577.0))
    (56,uhd::range_t(5744191577.0,5810869917.0))
    (57,uhd::range_t(5810869917.0,5879176194.0))
    (58,uhd::range_t(5879176194.0,5952430629.0))
    (59,uhd::range_t(5952430629.0,6016743964.0))
    (60,uhd::range_t(6016743964.0,6090658690.0))
    (61,uhd::range_t(6090658690.0,6128133570.0));

class max2871 : public max287x<max2871_regs_t>
{
public:
    max2871(write_fn func) : max287x<max2871_regs_t>(func) {}
    ~max2871() {};
    void set_muxout_mode(muxout_mode_t mode)
    {
        switch(mode)
        {
        case MUXOUT_SYNC:
            _regs.muxout = max2871_regs_t::MUXOUT_SYNC;
            break;
        case MUXOUT_SPI:
            _regs.muxout = max2871_regs_t::MUXOUT_SPI;
            break;
        default:
            max287x<max2871_regs_t>::set_muxout_mode(mode);
        }
    }

    double set_frequency(
        double target_freq,
        double ref_freq,
        double target_pfd_freq,
        bool is_int_n)
    {
        _regs.feedback_select = max2871_regs_t::FEEDBACK_SELECT_DIVIDED;
        double freq = max287x<max2871_regs_t>::set_frequency(target_freq, ref_freq, target_pfd_freq, is_int_n);

        // To support phase synchronization on MAX2871, the same VCO
        // subband must be manually programmed on all synthesizers and
        // several registers must be set to specific values.
        if (_config_for_sync)
        {
            // Need to manually program VCO value
            static const double MIN_VCO_FREQ = 3e9;
            double vco_freq = target_freq;
            while (vco_freq < MIN_VCO_FREQ)
                vco_freq *=2;
            uint8_t vco_index = 0xFF;
            for(const vco_map_t::value_type &vco:  max2871_vco_map)
            {
                if (uhd::math::fp_compare::fp_compare_epsilon<double>(vco_freq) < vco.second.stop())
                {
                    vco_index = vco.first;
                    break;
                }
            }
            if (vco_index == 0xFF)
                throw uhd::index_error("Invalid VCO frequency");

            // Settings required for phase synchronization as per MAX2871 datasheet
            _regs.shutdown_vas = max2871_regs_t::SHUTDOWN_VAS_DISABLED;
            _regs.vco = vco_index;
            _regs.low_noise_and_spur = max2871_regs_t::LOW_NOISE_AND_SPUR_LOW_NOISE;
            _regs.f01 = max2871_regs_t::F01_FRAC_N;
            _regs.aux_output_select = max2871_regs_t::AUX_OUTPUT_SELECT_DIVIDED;
        }
        else
        {
            // Reset values to defaults
            _regs.shutdown_vas = max2871_regs_t::SHUTDOWN_VAS_ENABLED;  // turn VCO auto selection on
            _regs.low_noise_and_spur = max2871_regs_t::LOW_NOISE_AND_SPUR_LOW_SPUR_2;
            _regs.f01 = max2871_regs_t::F01_AUTO;
            _regs.aux_output_select = max2871_regs_t::AUX_OUTPUT_SELECT_FUNDAMENTAL;
        }

        return freq;
    }

    void commit()
    {
        max287x<max2871_regs_t>::commit();

        // According to Maxim support, the following factors must be true to allow for phase synchronization
        if (_regs.int_n_mode == max2871_regs_t::INT_N_MODE_FRAC_N and
            _regs.feedback_select == max2871_regs_t::FEEDBACK_SELECT_DIVIDED and
            _regs.aux_output_select == max2871_regs_t::AUX_OUTPUT_SELECT_DIVIDED and
            _regs.rf_divider_select <= max2871_regs_t::RF_DIVIDER_SELECT_DIV16 and
            _regs.low_noise_and_spur == max2871_regs_t::LOW_NOISE_AND_SPUR_LOW_NOISE and
            _regs.f01 == max2871_regs_t::F01_FRAC_N and
            _regs.reference_doubler == max2871_regs_t::REFERENCE_DOUBLER_DISABLED and
            _regs.reference_divide_by_2 == max2871_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED and
            _regs.r_counter_10_bit == 1)
        {
            _can_sync = true;
        } else {
            _can_sync = false;
        }
    }
};


// Implementation of max287x template class
// To avoid linker errors, it was either include
// it here or put it in a .cpp file and include
// that file in this header file.  Decided to just
// include it here.

template <typename max287x_regs_t>
max287x<max287x_regs_t>::max287x(write_fn func) :
        _can_sync(false),
        _config_for_sync(false),
        _write_all_regs(true),
        _write(func),
        _delay_after_write(true)
{
    power_up();
}

template <typename max287x_regs_t>
max287x<max287x_regs_t>::~max287x()
{
    UHD_SAFE_CALL
    (
        shutdown();
    )
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::power_up(void)
{
    _regs.power_down = max287x_regs_t::POWER_DOWN_NORMAL;
    _regs.double_buffer = max287x_regs_t::DOUBLE_BUFFER_ENABLED;

    // According to MAX287x data sheets:
    // "Upon power-up, the registers should be programmed twice with at
    // least a 20ms pause between writes.  The first write ensures that
    // the device is enabled, and the second write starts the VCO
    // selection process."
    // The first write and the 20ms wait are done here.  The second write
    // is done when any other function that does a write to the registers
    // is called (such as tuning).
    _write_all_regs = true;
    _delay_after_write = true;
    commit();
    _write_all_regs = true; // Next call to commit() writes all regs
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::shutdown(void)
{
    _regs.rf_output_enable = max287x_regs_t::RF_OUTPUT_ENABLE_DISABLED;
    _regs.aux_output_enable = max287x_regs_t::AUX_OUTPUT_ENABLE_DISABLED;
    _regs.power_down = max287x_regs_t::POWER_DOWN_SHUTDOWN;
    commit();
}

template <typename max287x_regs_t>
bool max287x<max287x_regs_t>::is_shutdown(void)
{
    return (_regs.power_down == max287x_regs_t::POWER_DOWN_SHUTDOWN);
}

template <typename max287x_regs_t>
double max287x<max287x_regs_t>::set_frequency(
    double target_freq,
    double ref_freq,
    double target_pfd_freq,
    bool is_int_n)
{
    //map rf divider select output dividers to enums
    static const uhd::dict<int, typename max287x_regs_t::rf_divider_select_t> rfdivsel_to_enum =
        boost::assign::map_list_of
        (1,   max287x_regs_t::RF_DIVIDER_SELECT_DIV1)
        (2,   max287x_regs_t::RF_DIVIDER_SELECT_DIV2)
        (4,   max287x_regs_t::RF_DIVIDER_SELECT_DIV4)
        (8,   max287x_regs_t::RF_DIVIDER_SELECT_DIV8)
        (16,  max287x_regs_t::RF_DIVIDER_SELECT_DIV16)
        (32,  max287x_regs_t::RF_DIVIDER_SELECT_DIV32)
        (64,  max287x_regs_t::RF_DIVIDER_SELECT_DIV64)
        (128, max287x_regs_t::RF_DIVIDER_SELECT_DIV128);

    //map mode setting to valid integer divider (N) values
    static const uhd::range_t int_n_mode_div_range(16,65535,1);
    static const uhd::range_t frac_n_mode_div_range(19,4091,1);

    //other ranges and constants from MAX287X datasheets
    static const uhd::range_t clock_div_range(1,4095,1);
    static const uhd::range_t r_range(1,1023,1);
    static const double MIN_VCO_FREQ = 3e9;
    static const double BS_FREQ = 50e3;
    static const int MAX_BS_VALUE = 1023;

    int T = 0;
    int D = ref_freq <= 10.0e6 ? 1 : 0;
    int R = 0;
    int BS = 0;
    int N = 0;
    int FRAC = 0;
    int MOD = 4095;
    int RFdiv = 1;
    double pfd_freq = target_pfd_freq;
    bool feedback_divided = (_regs.feedback_select == max287x_regs_t::FEEDBACK_SELECT_DIVIDED);

    //increase RF divider until acceptable VCO frequency (MIN freq for MAX287x VCO is 3GHz)
    UHD_ASSERT_THROW(target_freq > 0);
    double vco_freq = target_freq;
    while (vco_freq < MIN_VCO_FREQ)
    {
        vco_freq *= 2;
        RFdiv *= 2;
    }

    // The feedback frequency can be the fundamental VCO frequency or
    // divided frequency.  The output divider for MAX287x is actually
    // 2 dividers, but only the first (1/2/4/8/16) is included in the
    // feedback loop.
    int fb_divisor = feedback_divided ? (RFdiv > 16 ? 16 : RFdiv) : 1;

    /*
     * The goal here is to loop though possible R dividers,
     * band select clock dividers, N (int) dividers, and FRAC
     * (frac) dividers.
     *
     * Calculate the N and F dividers for each set of values.
     * The loop exits when it meets all of the constraints.
     * The resulting loop values are loaded into the registers.
     *
     * f_pfd = f_ref*(1+D)/(R*(1+T))
     * f_vco = (N + (FRAC/MOD))*f_pfd
     *     N = f_vco/f_pfd - FRAC/MOD = f_vco*((R*(T+1))/(f_ref*(1+D))) - FRAC/MOD
     * f_rf  = f_vco/RFdiv
     */
    for(R = int(ref_freq*(1+D)/(target_pfd_freq*(1+T))); R <= r_range.stop(); R++)
    {
        //PFD input frequency = f_ref/R ... ignoring Reference doubler/divide-by-2 (D & T)
        pfd_freq = ref_freq*(1+D)/(R*(1+T));

        //keep the PFD frequency at or below target
        if (pfd_freq > target_pfd_freq)
            continue;

        //ignore fractional part of tuning
        N = int((vco_freq/pfd_freq)/fb_divisor);

        //Fractional-N calculation
        FRAC = int(boost::math::round(((vco_freq/pfd_freq)/fb_divisor - N)*MOD));

        if(is_int_n)
        {
            if (FRAC > (MOD / 2)) //Round integer such that actual freq is closest to target
                N++;
            FRAC = 0;
        }

        //keep N within int divider requirements
        if(is_int_n)
        {
            if(N <= int_n_mode_div_range.start()) continue;
            if(N >= int_n_mode_div_range.stop()) continue;
        }
        else
        {
            if(N <= frac_n_mode_div_range.start()) continue;
            if(N >= frac_n_mode_div_range.stop()) continue;
        }

        //keep pfd freq low enough to achieve 50kHz BS clock
        BS = std::ceil(pfd_freq / BS_FREQ);
        if(BS <= MAX_BS_VALUE) break;
    }
    UHD_ASSERT_THROW(R <= r_range.stop());

    //Reference divide-by-2 for 50% duty cycle
    // if R even, move one divide by 2 to to regs.reference_divide_by_2
    if(R % 2 == 0)
    {
        T = 1;
        R /= 2;
    }

    //actual frequency calculation
    double actual_freq = double((N + (double(FRAC)/double(MOD)))*ref_freq*(1+int(D))/(R*(1+int(T)))) * fb_divisor / RFdiv;

    UHD_LOGGER_TRACE("MAX287X")
        << boost::format("MAX287x: Intermediates: ref=%0.2f, outdiv=%f, fbdiv=%f")
            % ref_freq % double(RFdiv*2) % double(N + double(FRAC)/double(MOD))
        << boost::format("MAX287x: tune: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d, type=%s")
            % R % BS % N % FRAC % MOD % T % D % RFdiv % ((is_int_n) ? "Integer-N" : "Fractional")
        << boost::format("MAX287x: Frequencies (MHz): REQ=%0.2f, ACT=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f")
            % (target_freq/1e6) % (actual_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6)
    ;

    //load the register values
    _regs.rf_output_enable = max287x_regs_t::RF_OUTPUT_ENABLE_ENABLED;

    if(is_int_n) {
        _regs.cpl = max287x_regs_t::CPL_DISABLED;
        _regs.ldf = max287x_regs_t::LDF_INT_N;
        _regs.int_n_mode = max287x_regs_t::INT_N_MODE_INT_N;
    } else {
        _regs.cpl = max287x_regs_t::CPL_ENABLED;
        _regs.ldf = max287x_regs_t::LDF_FRAC_N;
        _regs.int_n_mode = max287x_regs_t::INT_N_MODE_FRAC_N;
    }

    _regs.lds = pfd_freq <= 32e6 ? max287x_regs_t::LDS_SLOW : max287x_regs_t::LDS_FAST;

    _regs.frac_12_bit = FRAC;
    _regs.int_16_bit = N;
    _regs.mod_12_bit = MOD;
    _regs.clock_divider_12_bit = std::max(int(clock_div_range.start()), int(std::ceil(400e-6*pfd_freq/MOD)));
    UHD_ASSERT_THROW(_regs.clock_divider_12_bit <= clock_div_range.stop());
    _regs.r_counter_10_bit = R;
    _regs.reference_divide_by_2 = T ?
        max287x_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED :
        max287x_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
    _regs.reference_doubler = D ?
        max287x_regs_t::REFERENCE_DOUBLER_ENABLED :
        max287x_regs_t::REFERENCE_DOUBLER_DISABLED;
    _regs.band_select_clock_div = BS & 0xFF;
    _regs.bs_msb = (BS & 0x300) >> 8;
    UHD_ASSERT_THROW(rfdivsel_to_enum.has_key(RFdiv));
    _regs.rf_divider_select = rfdivsel_to_enum[RFdiv];

    if (_regs.clock_div_mode == max287x_regs_t::CLOCK_DIV_MODE_FAST_LOCK)
    {
        // Charge pump current needs to be set to lowest value in fast lock mode
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_0_32MA;
        // Make sure the register containing the charge pump current is written
        _write_all_regs = true;
    }

    return actual_freq;
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::set_output_power(output_power_t power)
{
    switch (power)
    {
    case OUTPUT_POWER_M4DBM:
        _regs.output_power = max287x_regs_t::OUTPUT_POWER_M4DBM;
        break;
    case OUTPUT_POWER_M1DBM:
        _regs.output_power = max287x_regs_t::OUTPUT_POWER_M1DBM;
        break;
    case OUTPUT_POWER_2DBM:
        _regs.output_power = max287x_regs_t::OUTPUT_POWER_2DBM;
        break;
    case OUTPUT_POWER_5DBM:
        _regs.output_power = max287x_regs_t::OUTPUT_POWER_5DBM;
        break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::set_ld_pin_mode(ld_pin_mode_t mode)
{
    switch(mode)
    {
    case LD_PIN_MODE_LOW:
        _regs.ld_pin_mode = max287x_regs_t::LD_PIN_MODE_LOW;
        break;
    case LD_PIN_MODE_DLD:
        _regs.ld_pin_mode = max287x_regs_t::LD_PIN_MODE_DLD;
        break;
    case LD_PIN_MODE_ALD:
        _regs.ld_pin_mode = max287x_regs_t::LD_PIN_MODE_ALD;
        break;
    case LD_PIN_MODE_HIGH:
        _regs.ld_pin_mode = max287x_regs_t::LD_PIN_MODE_HIGH;
        break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::set_muxout_mode(muxout_mode_t mode)
{
    switch(mode)
    {
    case MUXOUT_TRI_STATE:
        _regs.muxout = max287x_regs_t::MUXOUT_TRI_STATE;
        break;
    case MUXOUT_HIGH:
        _regs.muxout = max287x_regs_t::MUXOUT_HIGH;
        break;
    case MUXOUT_LOW:
        _regs.muxout = max287x_regs_t::MUXOUT_LOW;
        break;
    case MUXOUT_RDIV:
        _regs.muxout = max287x_regs_t::MUXOUT_RDIV;
        break;
    case MUXOUT_NDIV:
        _regs.muxout = max287x_regs_t::MUXOUT_NDIV;
        break;
    case MUXOUT_ALD:
        _regs.muxout = max287x_regs_t::MUXOUT_ALD;
        break;
    case MUXOUT_DLD:
        _regs.muxout = max287x_regs_t::MUXOUT_DLD;
        break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::set_charge_pump_current(charge_pump_current_t cp_current)
{
    switch(cp_current)
    {
    case CHARGE_PUMP_CURRENT_0_32MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_0_32MA;
        break;
    case CHARGE_PUMP_CURRENT_0_64MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_0_64MA;
        break;
    case CHARGE_PUMP_CURRENT_0_96MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_0_96MA;
        break;
    case CHARGE_PUMP_CURRENT_1_28MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_1_28MA;
        break;
    case CHARGE_PUMP_CURRENT_1_60MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_1_60MA;
        break;
    case CHARGE_PUMP_CURRENT_1_92MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_1_92MA;
        break;
    case CHARGE_PUMP_CURRENT_2_24MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_2_24MA;
        break;
    case CHARGE_PUMP_CURRENT_2_56MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_2_56MA;
        break;
    case CHARGE_PUMP_CURRENT_2_88MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_2_88MA;
        break;
    case CHARGE_PUMP_CURRENT_3_20MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_3_20MA;
        break;
    case CHARGE_PUMP_CURRENT_3_52MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_3_52MA;
        break;
    case CHARGE_PUMP_CURRENT_3_84MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_3_84MA;
        break;
    case CHARGE_PUMP_CURRENT_4_16MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_4_16MA;
        break;
    case CHARGE_PUMP_CURRENT_4_48MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_4_48MA;
        break;
    case CHARGE_PUMP_CURRENT_4_80MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_4_80MA;
        break;
    case CHARGE_PUMP_CURRENT_5_12MA:
        _regs.charge_pump_current = max287x_regs_t::CHARGE_PUMP_CURRENT_5_12MA;
        break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::set_auto_retune(bool enabled)
{
    _regs.retune = enabled ? max287x_regs_t::RETUNE_ENABLED : max287x_regs_t::RETUNE_DISABLED;
}

template <>
inline void max287x<max2871_regs_t>::set_auto_retune(bool enabled)
{
    _regs.retune = enabled ? max2871_regs_t::RETUNE_ENABLED : max2871_regs_t::RETUNE_DISABLED;
    _regs.vas_dly = enabled ? max2871_regs_t::VAS_DLY_ENABLED : max2871_regs_t::VAS_DLY_DISABLED;
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::set_clock_divider_mode(clock_divider_mode_t mode)
{
    switch(mode)
    {
    case CLOCK_DIV_MODE_CLOCK_DIVIDER_OFF:
        _regs.clock_div_mode = max287x_regs_t::CLOCK_DIV_MODE_CLOCK_DIVIDER_OFF;
        break;
    case CLOCK_DIV_MODE_FAST_LOCK:
        _regs.clock_div_mode = max287x_regs_t::CLOCK_DIV_MODE_FAST_LOCK;
        break;
    case CLOCK_DIV_MODE_PHASE:
        _regs.clock_div_mode = max287x_regs_t::CLOCK_DIV_MODE_PHASE;
        break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::set_cycle_slip_mode(bool enabled)
{
    if (enabled)
        throw uhd::runtime_error("Cycle slip mode not supported on this MAX287x synthesizer.");
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::set_low_noise_and_spur(low_noise_and_spur_t mode)
{
    switch(mode)
    {
    case LOW_NOISE_AND_SPUR_LOW_NOISE:
        _regs.low_noise_and_spur = max287x_regs_t::LOW_NOISE_AND_SPUR_LOW_NOISE;
        break;
    case LOW_NOISE_AND_SPUR_LOW_SPUR_1:
        _regs.low_noise_and_spur = max287x_regs_t::LOW_NOISE_AND_SPUR_LOW_SPUR_1;
        break;
    case LOW_NOISE_AND_SPUR_LOW_SPUR_2:
        _regs.low_noise_and_spur = max287x_regs_t::LOW_NOISE_AND_SPUR_LOW_SPUR_2;
        break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::set_phase(uint16_t phase)
{
    _regs.phase_12_bit = phase & 0xFFF;
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::commit()
{
    std::vector<uint32_t> regs;
    std::set<uint32_t> changed_regs;

    // Get only regs with changes
    if (_write_all_regs)
    {
        for (int addr = 5; addr >= 0; addr--)
            regs.push_back(_regs.get_reg(uint32_t(addr)));
    } else {
        try {
            changed_regs = _regs.template get_changed_addrs<uint32_t> ();
            // register 0 must be written to apply double buffered fields
            if (changed_regs.size() > 0)
            {
                changed_regs.insert(0);
            }

            for (int addr = 5; addr >= 0; addr--)
            {
                if (changed_regs.find(uint32_t(addr)) != changed_regs.end())
                    regs.push_back(_regs.get_reg(uint32_t(addr)));
            }
        } catch (uhd::runtime_error&) {
            // No saved state - write all regs
            for (int addr = 5; addr >= 0; addr--)
                regs.push_back(_regs.get_reg(uint32_t(addr)));
        }
    }

    _write(regs);
    _regs.save_state();
    _write_all_regs = false;

    if (_delay_after_write)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        _delay_after_write = false;
    }
}

template <typename max287x_regs_t>
bool max287x<max287x_regs_t>::can_sync(void)
{
    return _can_sync;
}

template <typename max287x_regs_t>
void max287x<max287x_regs_t>::config_for_sync(bool enable)
{
    _config_for_sync = enable;
}

#endif // MAX287X_HPP_INCLUDED
