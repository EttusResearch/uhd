//
// Copyright 2013-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "lmk04816_regs.hpp"
#include "x300_clock_ctrl.hpp"
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/math.hpp>
#include <stdint.h>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <stdexcept>
#include <cmath>
#include <cstdlib>

static const double X300_REF_CLK_OUT_RATE  = 10e6;
static const uint16_t X300_MAX_CLKOUT_DIV = 1045;

struct x300_clk_delays {
    x300_clk_delays() :
        fpga_dly_ns(0.0),adc_dly_ns(0.0),dac_dly_ns(0.0),db_rx_dly_ns(0.0),db_tx_dly_ns(0.0)
    {}
    x300_clk_delays(double fpga, double adc, double dac, double db_rx, double db_tx) :
        fpga_dly_ns(fpga),adc_dly_ns(adc),dac_dly_ns(dac),db_rx_dly_ns(db_rx),db_tx_dly_ns(db_tx)
    {}

    double fpga_dly_ns;
    double adc_dly_ns;
    double dac_dly_ns;
    double db_rx_dly_ns;
    double db_tx_dly_ns;
};

// Tune the FPGA->ADC clock delay to ensure a safe ADC_SSCLK -> RADIO_CLK crossing.
// If the FPGA_CLK is delayed, we also need to delay the reference clocks going to the DAC
// because the data interface clock is generated from FPGA_CLK.
static const x300_clk_delays X300_REV0_6_CLK_DELAYS = x300_clk_delays(
    /*fpga=*/0.000, /*adc=*/2.200, /*dac=*/0.000, /*db_rx=*/0.000, /*db_tx=*/0.000);

static const x300_clk_delays X300_REV7_CLK_DELAYS = x300_clk_delays(
    /*fpga=*/0.000, /*adc=*/0.000, /*dac=*/0.000, /*db_rx=*/0.000, /*db_tx=*/0.000);

using namespace uhd;

x300_clock_ctrl::~x300_clock_ctrl(void){
    /* NOP */
}

class x300_clock_ctrl_impl : public x300_clock_ctrl    {

public:

    ~x300_clock_ctrl_impl(void) {}

    x300_clock_ctrl_impl(uhd::spi_iface::sptr spiface,
        const size_t slaveno,
        const size_t hw_rev,
        const double master_clock_rate,
        const double dboard_clock_rate,
        const double system_ref_rate):
        _spiface(spiface),
        _slaveno(slaveno),
        _hw_rev(hw_rev),
        _master_clock_rate(master_clock_rate),
        _dboard_clock_rate(dboard_clock_rate),
        _system_ref_rate(system_ref_rate)
    {
        init();
    }

    void reset_clocks() {
        _lmk04816_regs.RESET = lmk04816_regs_t::RESET_RESET;
        this->write_regs(0);
        _lmk04816_regs.RESET = lmk04816_regs_t::RESET_NO_RESET;
        for (size_t i = 0; i <= 16; ++i) {
            this->write_regs(i);
        }
        for (size_t i = 24; i <= 31; ++i) {
            this->write_regs(i);
        }
        sync_clocks();
    }

    void sync_clocks(void) {
        //soft sync:
        //put the sync IO into output mode - FPGA must be input
        //write low, then write high - this triggers a soft sync
        _lmk04816_regs.SYNC_POL_INV = lmk04816_regs_t::SYNC_POL_INV_SYNC_LOW;
        this->write_regs(11);
        _lmk04816_regs.SYNC_POL_INV = lmk04816_regs_t::SYNC_POL_INV_SYNC_HIGH;
        this->write_regs(11);
    }

    double get_master_clock_rate(void) {
        return _master_clock_rate;
    }

    double get_sysref_clock_rate(void) {
        return _system_ref_rate;
    }

    double get_refout_clock_rate(void) {
        //We support only one reference output rate
        return X300_REF_CLK_OUT_RATE;
    }

    void set_dboard_rate(const x300_clock_which_t which, double rate) {
        uint16_t div = uint16_t(_vco_freq / rate);
        uint16_t *reg = NULL;
        uint8_t addr = 0xFF;

        // Make sure requested rate is an even divisor of the VCO frequency
        if (not math::frequencies_are_equal(_vco_freq / div, rate))
            throw uhd::value_error("invalid dboard rate requested");

        switch (which)
        {
        case X300_CLOCK_WHICH_DB0_RX:
        case X300_CLOCK_WHICH_DB1_RX:
            reg = &_lmk04816_regs.CLKout2_3_DIV;
            addr = 1;
            break;
        case X300_CLOCK_WHICH_DB0_TX:
        case X300_CLOCK_WHICH_DB1_TX:
            reg = &_lmk04816_regs.CLKout4_5_DIV;
            addr = 2;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }

        if (*reg == div)
            return;

        // Since the clock rate on one daughter board cannot be changed without
        // affecting the other daughter board, don't allow it.
        throw uhd::not_implemented_error("x3xx set dboard clock rate does not support changing the clock rate");

        // This is open source code and users may need to enable this function
        // to support other daughterboards.  If so, comment out the line above
        // that throws the error and allow the program to reach the code below.

        // The LMK04816 datasheet says the register must be written twice if SYNC is enabled
        *reg = div;
        write_regs(addr);
        write_regs(addr);
        sync_clocks();
    }

    double get_dboard_rate(const x300_clock_which_t which)
    {
        double rate = 0.0;
        switch (which)
        {
        case X300_CLOCK_WHICH_DB0_RX:
        case X300_CLOCK_WHICH_DB1_RX:
            rate = _vco_freq / _lmk04816_regs.CLKout2_3_DIV;
            break;
        case X300_CLOCK_WHICH_DB0_TX:
        case X300_CLOCK_WHICH_DB1_TX:
            rate = _vco_freq / _lmk04816_regs.CLKout4_5_DIV;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }
        return rate;
    }

    std::vector<double> get_dboard_rates(const x300_clock_which_t)
    {
        std::vector<double> rates;
        for (size_t div = size_t(_vco_freq / _master_clock_rate); div <= X300_MAX_CLKOUT_DIV; div++)
            rates.push_back(_vco_freq / div);
        return rates;
    }

    void enable_dboard_clock(const x300_clock_which_t which, const bool enable)
    {
        switch (which)
        {
        case X300_CLOCK_WHICH_DB0_RX:
            if (enable != (_lmk04816_regs.CLKout2_TYPE == lmk04816_regs_t::CLKOUT2_TYPE_LVPECL_700MVPP))
            {
                _lmk04816_regs.CLKout2_TYPE = enable ? lmk04816_regs_t::CLKOUT2_TYPE_LVPECL_700MVPP : lmk04816_regs_t::CLKOUT2_TYPE_P_DOWN;
                write_regs(6);
            }
            break;
        case X300_CLOCK_WHICH_DB1_RX:
            if (enable != (_lmk04816_regs.CLKout3_TYPE == lmk04816_regs_t::CLKOUT3_TYPE_LVPECL_700MVPP))
            {
                _lmk04816_regs.CLKout3_TYPE = enable ? lmk04816_regs_t::CLKOUT3_TYPE_LVPECL_700MVPP : lmk04816_regs_t::CLKOUT3_TYPE_P_DOWN;
                write_regs(6);
            }
            break;
        case X300_CLOCK_WHICH_DB0_TX:
            if (enable != (_lmk04816_regs.CLKout5_TYPE == lmk04816_regs_t::CLKOUT5_TYPE_LVPECL_700MVPP))
            {
                _lmk04816_regs.CLKout5_TYPE = enable ? lmk04816_regs_t::CLKOUT5_TYPE_LVPECL_700MVPP : lmk04816_regs_t::CLKOUT5_TYPE_P_DOWN;
                write_regs(7);
            }
            break;
        case X300_CLOCK_WHICH_DB1_TX:
            if (enable != (_lmk04816_regs.CLKout4_TYPE == lmk04816_regs_t::CLKOUT4_TYPE_LVPECL_700MVPP))
            {
                _lmk04816_regs.CLKout4_TYPE = enable ? lmk04816_regs_t::CLKOUT4_TYPE_LVPECL_700MVPP : lmk04816_regs_t::CLKOUT4_TYPE_P_DOWN;
                write_regs(7);
            }
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_ref_out(const bool enable) {
        // TODO  Implement divider configuration to allow for configurable output
        // rates
        if (enable)
            _lmk04816_regs.CLKout10_TYPE = lmk04816_regs_t::CLKOUT10_TYPE_LVDS;
        else
            _lmk04816_regs.CLKout10_TYPE = lmk04816_regs_t::CLKOUT10_TYPE_P_DOWN;
        this->write_regs(8);
    }

    void write_regs(uint8_t addr) {
        uint32_t data = _lmk04816_regs.get_reg(addr);
        _spiface->write_spi(_slaveno, spi_config_t::EDGE_RISE, data,32);
    }

    double set_clock_delay(const x300_clock_which_t which, const double delay_ns, const bool resync = true) {
        //All dividers have are delayed by 5 taps by default. The delay
        //set by this function is relative to the 5 tap delay
        static const uint16_t DDLY_MIN_TAPS  = 5;
        static const uint16_t DDLY_MAX_TAPS  = 522;  //Extended mode

        //The resolution and range of the analog delay is fixed
        static const double ADLY_RES_NS = 0.025;
        static const double ADLY_MIN_NS = 0.500;
        static const double ADLY_MAX_NS = 0.975;

        //Each digital tap delays the clock by one VCO period
        double vco_period_ns = 1.0e9/_vco_freq;
        double half_vco_period_ns = vco_period_ns/2.0;

        //Implement as much of the requested delay using digital taps. Whatever is leftover
        //will be made up using the analog delay element and the half-cycle digital tap.
        //A caveat here is that the analog delay starts at ADLY_MIN_NS, so we need to back off
        //by that much when coming up with the digital taps so that the difference can be made
        //up using the analog delay.
        uint16_t ddly_taps = 0;
        if (delay_ns < ADLY_MIN_NS) {
            ddly_taps = static_cast<uint16_t>(std::floor((delay_ns)/vco_period_ns));
        } else {
            ddly_taps = static_cast<uint16_t>(std::floor((delay_ns-ADLY_MIN_NS)/vco_period_ns));
        }
        double leftover_delay = delay_ns - (vco_period_ns * ddly_taps);

        //Compute settings
        uint16_t ddly_value    = ddly_taps + DDLY_MIN_TAPS;
        bool            adly_en       = false;
        uint8_t  adly_value    = 0;
        uint8_t  half_shift_en = 0;

        if (ddly_value > DDLY_MAX_TAPS) {
            throw uhd::value_error("set_clock_delay: Requested delay is out of range.");
        }

        double coerced_delay = (vco_period_ns * ddly_taps);
        if (leftover_delay > ADLY_MAX_NS) {
            //The VCO is running too slowly for us to compensate the digital delay difference using
            //analog delay. Do the best we can.
            adly_en = true;
            adly_value = static_cast<uint8_t>(boost::math::round((ADLY_MAX_NS-ADLY_MIN_NS)/ADLY_RES_NS));
            coerced_delay += ADLY_MAX_NS;
        } else if (leftover_delay >= ADLY_MIN_NS && leftover_delay <= ADLY_MAX_NS) {
            //The leftover delay can be compensated by the analog delay up to the analog delay resolution
            adly_en = true;
            adly_value = static_cast<uint8_t>(boost::math::round((leftover_delay-ADLY_MIN_NS)/ADLY_RES_NS));
            coerced_delay += ADLY_MIN_NS+(ADLY_RES_NS*adly_value);
        } else if (leftover_delay >= (ADLY_MIN_NS - half_vco_period_ns) && leftover_delay < ADLY_MIN_NS) {
            //The leftover delay if less than the minimum supported analog delay but if we move the digital
            //delay back by half a VCO cycle then it will be in the range of the analog delay. So do that!
            adly_en = true;
            adly_value = static_cast<uint8_t>(boost::math::round((leftover_delay+half_vco_period_ns-ADLY_MIN_NS)/ADLY_RES_NS));
            half_shift_en = 1;
            coerced_delay += ADLY_MIN_NS+(ADLY_RES_NS*adly_value)-half_vco_period_ns;
        } else {
            //Even after moving the digital delay back by half a cycle, we cannot make up the difference
            //so give up on compensating for the difference from the digital delay tap.
            //If control reaches here then the value of leftover_delay is possible very small and will still
            //be close to what the client requested.
        }

        UHD_LOG_DEBUG("X300", boost::format("x300_clock_ctrl::set_clock_delay: Which=%d, Requested=%f, Digital Taps=%d, Half Shift=%d, Analog Delay=%d (%s), Coerced Delay=%fns"
                          ) % which % delay_ns % ddly_value % (half_shift_en?"ON":"OFF") % ((int)adly_value) % (adly_en?"ON":"OFF") % coerced_delay)

        //Apply settings
        switch (which)
        {
        case X300_CLOCK_WHICH_FPGA:
            _lmk04816_regs.CLKout0_1_DDLY = ddly_value;
            _lmk04816_regs.CLKout0_1_HS = half_shift_en;
            if (adly_en) {
                _lmk04816_regs.CLKout0_ADLY_SEL = lmk04816_regs_t::CLKOUT0_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout1_ADLY_SEL = lmk04816_regs_t::CLKOUT1_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout0_1_ADLY = adly_value;
            } else {
                _lmk04816_regs.CLKout0_ADLY_SEL = lmk04816_regs_t::CLKOUT0_ADLY_SEL_D_PD;
                _lmk04816_regs.CLKout1_ADLY_SEL = lmk04816_regs_t::CLKOUT1_ADLY_SEL_D_PD;
            }
            write_regs(0);
            write_regs(6);
            _delays.fpga_dly_ns = coerced_delay;
            break;
        case X300_CLOCK_WHICH_DB0_RX:
        case X300_CLOCK_WHICH_DB1_RX:
            _lmk04816_regs.CLKout2_3_DDLY = ddly_value;
            _lmk04816_regs.CLKout2_3_HS = half_shift_en;
            if (adly_en) {
                _lmk04816_regs.CLKout2_ADLY_SEL = lmk04816_regs_t::CLKOUT2_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout3_ADLY_SEL = lmk04816_regs_t::CLKOUT3_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout2_3_ADLY = adly_value;
            } else {
                _lmk04816_regs.CLKout2_ADLY_SEL = lmk04816_regs_t::CLKOUT2_ADLY_SEL_D_PD;
                _lmk04816_regs.CLKout3_ADLY_SEL = lmk04816_regs_t::CLKOUT3_ADLY_SEL_D_PD;
            }
            write_regs(1);
            write_regs(6);
            _delays.db_rx_dly_ns = coerced_delay;
            break;
        case X300_CLOCK_WHICH_DB0_TX:
        case X300_CLOCK_WHICH_DB1_TX:
            _lmk04816_regs.CLKout4_5_DDLY = ddly_value;
            _lmk04816_regs.CLKout4_5_HS = half_shift_en;
            if (adly_en) {
                _lmk04816_regs.CLKout4_ADLY_SEL = lmk04816_regs_t::CLKOUT4_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout5_ADLY_SEL = lmk04816_regs_t::CLKOUT5_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout4_5_ADLY = adly_value;
            } else {
                _lmk04816_regs.CLKout4_ADLY_SEL = lmk04816_regs_t::CLKOUT4_ADLY_SEL_D_PD;
                _lmk04816_regs.CLKout5_ADLY_SEL = lmk04816_regs_t::CLKOUT5_ADLY_SEL_D_PD;
            }
            write_regs(2);
            write_regs(7);
            _delays.db_tx_dly_ns = coerced_delay;
            break;
        case X300_CLOCK_WHICH_DAC0:
        case X300_CLOCK_WHICH_DAC1:
            _lmk04816_regs.CLKout6_7_DDLY = ddly_value;
            _lmk04816_regs.CLKout6_7_HS = half_shift_en;
            if (adly_en) {
                _lmk04816_regs.CLKout6_ADLY_SEL = lmk04816_regs_t::CLKOUT6_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout7_ADLY_SEL = lmk04816_regs_t::CLKOUT7_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout6_7_ADLY = adly_value;
            } else {
                _lmk04816_regs.CLKout6_ADLY_SEL = lmk04816_regs_t::CLKOUT6_ADLY_SEL_D_PD;
                _lmk04816_regs.CLKout7_ADLY_SEL = lmk04816_regs_t::CLKOUT7_ADLY_SEL_D_PD;
            }
            write_regs(3);
            write_regs(7);
            _delays.dac_dly_ns = coerced_delay;
            break;
        case X300_CLOCK_WHICH_ADC0:
        case X300_CLOCK_WHICH_ADC1:
            _lmk04816_regs.CLKout8_9_DDLY = ddly_value;
            _lmk04816_regs.CLKout8_9_HS = half_shift_en;
            if (adly_en) {
                _lmk04816_regs.CLKout8_ADLY_SEL = lmk04816_regs_t::CLKOUT8_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout9_ADLY_SEL = lmk04816_regs_t::CLKOUT9_ADLY_SEL_D_BOTH;
                _lmk04816_regs.CLKout8_9_ADLY = adly_value;
            } else {
                _lmk04816_regs.CLKout8_ADLY_SEL = lmk04816_regs_t::CLKOUT8_ADLY_SEL_D_PD;
                _lmk04816_regs.CLKout9_ADLY_SEL = lmk04816_regs_t::CLKOUT9_ADLY_SEL_D_PD;
            }
            write_regs(4);
            write_regs(8);
            _delays.adc_dly_ns = coerced_delay;
            break;
        default:
            throw uhd::value_error("set_clock_delay: Requested source is invalid.");
        }

        //Delays are applied only on a sync event
        if (resync) sync_clocks();

        return coerced_delay;
    }

    double get_clock_delay(const x300_clock_which_t which) {
        switch (which)
        {
        case X300_CLOCK_WHICH_FPGA:
            return _delays.fpga_dly_ns;
        case X300_CLOCK_WHICH_DB0_RX:
        case X300_CLOCK_WHICH_DB1_RX:
            return _delays.db_rx_dly_ns;
        case X300_CLOCK_WHICH_DB0_TX:
        case X300_CLOCK_WHICH_DB1_TX:
            return _delays.db_tx_dly_ns;
        case X300_CLOCK_WHICH_DAC0:
        case X300_CLOCK_WHICH_DAC1:
            return _delays.dac_dly_ns;
        case X300_CLOCK_WHICH_ADC0:
        case X300_CLOCK_WHICH_ADC1:
            return _delays.adc_dly_ns;
        default:
            throw uhd::value_error("get_clock_delay: Requested source is invalid.");
        }
    }

private:

    void init() {
        /* The X3xx has two primary rates. The first is the
         * _system_ref_rate, which is sourced from the "clock_source"/"value" field
         * of the property tree, and whose value can be 10e6, 30.72e6, or 200e6.
         * The _system_ref_rate is the input to the clocking system, and
         * what comes out is a disciplined master clock running at the
         * _master_clock_rate. As such, only certain combinations of
         * system reference rates and master clock rates are supported.
         * Additionally, a subset of these will operate in "zero delay" mode. */

        enum opmode_t { INVALID,
                        m10M_200M_NOZDEL,      // used for debug purposes only
                        m10M_200M_ZDEL,        // Normal mode
                        m30_72M_184_32M_ZDEL,  // LTE with external ref, aka CPRI Mode
                        m10M_184_32M_NOZDEL,   // LTE with 10 MHz ref
                        m10M_120M_ZDEL };       // NI USRP 120 MHz Clocking

        /* The default clocking mode is 10MHz reference generating a 200 MHz master
         * clock, in zero-delay mode. */
        opmode_t clocking_mode = INVALID;

        if (math::frequencies_are_equal(_system_ref_rate, 10e6)) {
            if (math::frequencies_are_equal(_master_clock_rate, 184.32e6)) {
                /* 10MHz reference, 184.32 MHz master clock out, NOT Zero Delay. */
                clocking_mode = m10M_184_32M_NOZDEL;
            } else if (math::frequencies_are_equal(_master_clock_rate, 200e6)) {
                /* 10MHz reference, 200 MHz master clock out, Zero Delay */
                clocking_mode = m10M_200M_ZDEL;
            } else if (math::frequencies_are_equal(_master_clock_rate, 120e6)) {
                /* 10MHz reference, 120 MHz master clock rate, Zero Delay */
                clocking_mode = m10M_120M_ZDEL;
            } else {
                throw uhd::runtime_error(str(
                    boost::format("Invalid master clock rate: %.2f MHz.\n"
                                  "Valid master clock rates when using a %f MHz reference clock are:\n"
                                  "120 MHz, 184.32 MHz and 200 MHz.")
                    % (_master_clock_rate / 1e6)  % (_system_ref_rate / 1e6)
                ));
            }
        } else if (math::frequencies_are_equal(_system_ref_rate, 30.72e6)) {
            if (math::frequencies_are_equal(_master_clock_rate, 184.32e6)) {
                /* 30.72MHz reference, 184.32 MHz master clock out, Zero Delay */
                clocking_mode = m30_72M_184_32M_ZDEL;
            } else {
                throw uhd::runtime_error(str(
                    boost::format("Invalid master clock rate: %.2f MHz.\n"
                                  "Valid master clock rate when using a %.2f MHz reference clock is: 184.32 MHz.")
                    % (_master_clock_rate / 1e6)  % (_system_ref_rate / 1e6)
                ));
            }
        } else {
            throw uhd::runtime_error(str(
                boost::format("Invalid system reference rate: %.2f MHz.\nValid reference frequencies are: 10 MHz, 30.72 MHz.")
                % (_system_ref_rate / 1e6)
            ));
        }
        UHD_ASSERT_THROW(clocking_mode != INVALID);

        // For 200 MHz output, the VCO is run at 2400 MHz
        // For the LTE/CPRI rate of 184.32 MHz, the VCO runs at 2580.48 MHz

        // Note: PLL2 N2 prescaler is enabled for all cases
        //       PLL2 reference doubler is enabled for all cases

        /* All LMK04816 settings are from the LMK datasheet for our clocking
         * architecture. Please refer to the datasheet for more information. */
        switch (clocking_mode) {
            case m10M_200M_NOZDEL:
                _vco_freq = 2400e6;
                _lmk04816_regs.MODE = lmk04816_regs_t::MODE_DUAL_INT;

                // PLL1 - 2 MHz compare frequency
                _lmk04816_regs.PLL1_N_28 = 48;
                _lmk04816_regs.PLL1_R_27 = 5;
                _lmk04816_regs.PLL1_CP_GAIN_27 = lmk04816_regs_t::PLL1_CP_GAIN_27_100UA;

                // PLL2 - 48 MHz compare frequency
                _lmk04816_regs.PLL2_N_30 = 25;
                _lmk04816_regs.PLL2_P_30 = lmk04816_regs_t::PLL2_P_30_DIV_2A;
                _lmk04816_regs.PLL2_R_28 = 4;
                _lmk04816_regs.PLL2_CP_GAIN_26 = lmk04816_regs_t::PLL2_CP_GAIN_26_3200UA;

                break;

            case m10M_200M_ZDEL:
                _vco_freq = 2400e6;
                _lmk04816_regs.MODE = lmk04816_regs_t::MODE_DUAL_INT_ZER_DELAY;

                // PLL1 - 2 MHz compare frequency
                _lmk04816_regs.PLL1_N_28 = 5;
                _lmk04816_regs.PLL1_R_27 = 5;
                _lmk04816_regs.PLL1_CP_GAIN_27 = lmk04816_regs_t::PLL1_CP_GAIN_27_1600UA;

                // PLL2 - 96 MHz compare frequency
                _lmk04816_regs.PLL2_N_30 = 5;
                _lmk04816_regs.PLL2_P_30 = lmk04816_regs_t::PLL2_P_30_DIV_5;
                _lmk04816_regs.PLL2_R_28 = 2;

                if(_hw_rev <= 4)
                    _lmk04816_regs.PLL2_CP_GAIN_26 = lmk04816_regs_t::PLL2_CP_GAIN_26_1600UA;
                else
                    _lmk04816_regs.PLL2_CP_GAIN_26 = lmk04816_regs_t::PLL2_CP_GAIN_26_400UA;

                break;

            case m30_72M_184_32M_ZDEL:
                _vco_freq = 2580.48e6;
                _lmk04816_regs.MODE = lmk04816_regs_t::MODE_DUAL_INT_ZER_DELAY;

                // PLL1 - 2.048 MHz compare frequency
                _lmk04816_regs.PLL1_N_28 = 15;
                _lmk04816_regs.PLL1_R_27 = 15;
                _lmk04816_regs.PLL1_CP_GAIN_27 = lmk04816_regs_t::PLL1_CP_GAIN_27_100UA;

                // PLL2 - 7.68 MHz compare frequency
                _lmk04816_regs.PLL2_N_30 = 168;
                _lmk04816_regs.PLL2_P_30 = lmk04816_regs_t::PLL2_P_30_DIV_2A;
                _lmk04816_regs.PLL2_R_28 = 25;
                _lmk04816_regs.PLL2_CP_GAIN_26 = lmk04816_regs_t::PLL2_CP_GAIN_26_3200UA;

                _lmk04816_regs.PLL2_R3_LF = lmk04816_regs_t::PLL2_R3_LF_1KILO_OHM;
                _lmk04816_regs.PLL2_C3_LF = lmk04816_regs_t::PLL2_C3_LF_39PF;

                _lmk04816_regs.PLL2_R4_LF = lmk04816_regs_t::PLL2_R4_LF_1KILO_OHM;
                _lmk04816_regs.PLL2_C4_LF = lmk04816_regs_t::PLL2_C4_LF_34PF;

                break;

            case m10M_184_32M_NOZDEL:
                _vco_freq = 2580.48e6;
                _lmk04816_regs.MODE = lmk04816_regs_t::MODE_DUAL_INT;

                // PLL1 - 2 MHz compare frequency
                _lmk04816_regs.PLL1_N_28 = 48;
                _lmk04816_regs.PLL1_R_27 = 5;
                _lmk04816_regs.PLL1_CP_GAIN_27 = lmk04816_regs_t::PLL1_CP_GAIN_27_100UA;

                // PLL2 - 7.68 MHz compare frequency
                _lmk04816_regs.PLL2_N_30 = 168;
                _lmk04816_regs.PLL2_P_30 = lmk04816_regs_t::PLL2_P_30_DIV_2A;
                _lmk04816_regs.PLL2_R_28 = 25;
                _lmk04816_regs.PLL2_CP_GAIN_26 = lmk04816_regs_t::PLL2_CP_GAIN_26_3200UA;

                _lmk04816_regs.PLL2_R3_LF = lmk04816_regs_t::PLL2_R3_LF_4KILO_OHM;
                _lmk04816_regs.PLL2_C3_LF = lmk04816_regs_t::PLL2_C3_LF_39PF;

                _lmk04816_regs.PLL2_R4_LF = lmk04816_regs_t::PLL2_R4_LF_1KILO_OHM;
                _lmk04816_regs.PLL2_C4_LF = lmk04816_regs_t::PLL2_C4_LF_71PF;

                break;

            case m10M_120M_ZDEL:
                _vco_freq = 2400e6;
                _lmk04816_regs.MODE = lmk04816_regs_t::MODE_DUAL_INT_ZER_DELAY;

                // PLL1 - 2 MHz compare frequency
                _lmk04816_regs.PLL1_N_28 = 5;
                _lmk04816_regs.PLL1_R_27 = 5;
                _lmk04816_regs.PLL1_CP_GAIN_27 = lmk04816_regs_t::PLL1_CP_GAIN_27_100UA;

                // PLL2 - 96 MHz compare frequency
                _lmk04816_regs.PLL2_N_30 = 5;
                _lmk04816_regs.PLL2_P_30 = lmk04816_regs_t::PLL2_P_30_DIV_5;
                _lmk04816_regs.PLL2_R_28 = 2;

                if(_hw_rev <= 4)
                    _lmk04816_regs.PLL2_CP_GAIN_26 = lmk04816_regs_t::PLL2_CP_GAIN_26_1600UA;
                else
                    _lmk04816_regs.PLL2_CP_GAIN_26 = lmk04816_regs_t::PLL2_CP_GAIN_26_400UA;

                break;

            default:
                UHD_THROW_INVALID_CODE_PATH();
                break;
        };

        uint16_t master_clock_div = static_cast<uint16_t>(
            std::ceil(_vco_freq / _master_clock_rate));

        uint16_t dboard_div = static_cast<uint16_t>(
            std::ceil(_vco_freq / _dboard_clock_rate));

        /* Reset the LMK clock controller. */
        _lmk04816_regs.RESET = lmk04816_regs_t::RESET_RESET;
        this->write_regs(0);
        _lmk04816_regs.RESET = lmk04816_regs_t::RESET_NO_RESET;
        this->write_regs(0);

        /* Initial power-up */
        _lmk04816_regs.CLKout0_1_PD = lmk04816_regs_t::CLKOUT0_1_PD_POWER_UP;
        this->write_regs(0);
        _lmk04816_regs.CLKout0_1_DIV = master_clock_div;
        this->write_regs(0);

        // Register 1
        _lmk04816_regs.CLKout2_3_PD = lmk04816_regs_t::CLKOUT2_3_PD_POWER_UP;
        _lmk04816_regs.CLKout2_3_DIV = dboard_div;
        // Register 2
        _lmk04816_regs.CLKout4_5_PD = lmk04816_regs_t::CLKOUT4_5_PD_POWER_UP;
        _lmk04816_regs.CLKout4_5_DIV = dboard_div;
        // Register 3
        _lmk04816_regs.CLKout6_7_DIV = master_clock_div;
        _lmk04816_regs.CLKout6_7_OSCin_Sel = lmk04816_regs_t::CLKOUT6_7_OSCIN_SEL_VCO;
        // Register 4
        _lmk04816_regs.CLKout8_9_DIV = master_clock_div;
        // Register 5
        _lmk04816_regs.CLKout10_11_PD = lmk04816_regs_t::CLKOUT10_11_PD_NORMAL;
        _lmk04816_regs.CLKout10_11_DIV =
            static_cast<uint16_t>(std::ceil(_vco_freq / _system_ref_rate));

        // Register 6
        _lmk04816_regs.CLKout0_TYPE = lmk04816_regs_t::CLKOUT0_TYPE_LVDS; //FPGA
        _lmk04816_regs.CLKout1_TYPE = lmk04816_regs_t::CLKOUT1_TYPE_P_DOWN; //CPRI feedback clock, use LVDS
        _lmk04816_regs.CLKout2_TYPE = lmk04816_regs_t::CLKOUT2_TYPE_LVPECL_700MVPP; //DB_0_RX
        _lmk04816_regs.CLKout3_TYPE = lmk04816_regs_t::CLKOUT3_TYPE_LVPECL_700MVPP; //DB_1_RX

        // Register 7
        _lmk04816_regs.CLKout4_TYPE = lmk04816_regs_t::CLKOUT4_TYPE_LVPECL_700MVPP; //DB_1_TX
        _lmk04816_regs.CLKout5_TYPE = lmk04816_regs_t::CLKOUT5_TYPE_LVPECL_700MVPP; //DB_0_TX
        _lmk04816_regs.CLKout6_TYPE = lmk04816_regs_t::CLKOUT6_TYPE_LVPECL_700MVPP; //DB0_DAC
        _lmk04816_regs.CLKout7_TYPE = lmk04816_regs_t::CLKOUT7_TYPE_LVPECL_700MVPP; //DB1_DAC
        _lmk04816_regs.CLKout8_TYPE = lmk04816_regs_t::CLKOUT8_TYPE_LVPECL_700MVPP; //DB0_ADC

        // Register 8
        _lmk04816_regs.CLKout9_TYPE = lmk04816_regs_t::CLKOUT9_TYPE_LVPECL_700MVPP; //DB1_ADC
        _lmk04816_regs.CLKout10_TYPE = lmk04816_regs_t::CLKOUT10_TYPE_LVDS; //REF_CLKOUT
        _lmk04816_regs.CLKout11_TYPE = lmk04816_regs_t::CLKOUT11_TYPE_P_DOWN; //Debug header, use LVPECL


        // Register 10
        _lmk04816_regs.EN_OSCout0 = lmk04816_regs_t::EN_OSCOUT0_DISABLED; //Debug header
        _lmk04816_regs.FEEDBACK_MUX = 5; //use output 10 (REF OUT) for feedback
        _lmk04816_regs.EN_FEEDBACK_MUX = lmk04816_regs_t::EN_FEEDBACK_MUX_ENABLED;

        // Register 11
        // MODE set in individual cases above
        _lmk04816_regs.SYNC_QUAL = lmk04816_regs_t::SYNC_QUAL_FB_MUX;
        _lmk04816_regs.EN_SYNC = lmk04816_regs_t::EN_SYNC_ENABLE;
        _lmk04816_regs.NO_SYNC_CLKout0_1 = lmk04816_regs_t::NO_SYNC_CLKOUT0_1_CLOCK_XY_SYNC;
        _lmk04816_regs.NO_SYNC_CLKout2_3 = lmk04816_regs_t::NO_SYNC_CLKOUT2_3_CLOCK_XY_SYNC;
        _lmk04816_regs.NO_SYNC_CLKout4_5 = lmk04816_regs_t::NO_SYNC_CLKOUT4_5_CLOCK_XY_SYNC;
        _lmk04816_regs.NO_SYNC_CLKout6_7 = lmk04816_regs_t::NO_SYNC_CLKOUT6_7_CLOCK_XY_SYNC;
        _lmk04816_regs.NO_SYNC_CLKout8_9 = lmk04816_regs_t::NO_SYNC_CLKOUT8_9_CLOCK_XY_SYNC;
        _lmk04816_regs.NO_SYNC_CLKout10_11 = lmk04816_regs_t::NO_SYNC_CLKOUT10_11_CLOCK_XY_SYNC;
        _lmk04816_regs.SYNC_TYPE = lmk04816_regs_t::SYNC_TYPE_INPUT;

        // Register 12
        _lmk04816_regs.LD_MUX = lmk04816_regs_t::LD_MUX_BOTH;

        /* Input Clock Configurations */
        // Register 13
        _lmk04816_regs.EN_CLKin0 = lmk04816_regs_t::EN_CLKIN0_NO_VALID_USE;  // This is not connected
        _lmk04816_regs.EN_CLKin2 = lmk04816_regs_t::EN_CLKIN2_NO_VALID_USE;  // Used only for CPRI
        _lmk04816_regs.Status_CLKin1_MUX = lmk04816_regs_t::STATUS_CLKIN1_MUX_UWIRE_RB;
        _lmk04816_regs.CLKin_Select_MODE = lmk04816_regs_t::CLKIN_SELECT_MODE_CLKIN1_MAN;
        _lmk04816_regs.HOLDOVER_MUX = lmk04816_regs_t::HOLDOVER_MUX_PLL1_R;
        // Register 14
        _lmk04816_regs.Status_CLKin1_TYPE = lmk04816_regs_t::STATUS_CLKIN1_TYPE_OUT_PUSH_PULL;
        _lmk04816_regs.Status_CLKin0_TYPE = lmk04816_regs_t::STATUS_CLKIN0_TYPE_OUT_PUSH_PULL;

        // Register 26
        // PLL2_CP_GAIN_26 set above in individual cases
        _lmk04816_regs.PLL2_CP_POL_26 = lmk04816_regs_t::PLL2_CP_POL_26_NEG_SLOPE;
        _lmk04816_regs.EN_PLL2_REF_2X = lmk04816_regs_t::EN_PLL2_REF_2X_DOUBLED_FREQ_REF;

        // Register 27
        // PLL1_CP_GAIN_27 set in individual cases above
        // PLL1_R_27 set in the individual cases above

        // Register 28
        // PLL1_N_28 and PLL2_R_28 are set in the individual cases above

        // Register 29
        _lmk04816_regs.PLL2_N_CAL_29 = _lmk04816_regs.PLL2_N_30;  // N_CAL should always match N
        _lmk04816_regs.OSCin_FREQ_29 = lmk04816_regs_t::OSCIN_FREQ_29_63_TO_127MHZ;

        // Register 30
        // PLL2_P_30 set in individual cases above
        // PLL2_N_30 set in individual cases above

        if (_hw_rev >= 7) {
            _delays = X300_REV7_CLK_DELAYS;
        } else {
            _delays = X300_REV0_6_CLK_DELAYS;
        }

        //Apply delay values
        set_clock_delay(X300_CLOCK_WHICH_FPGA,   _delays.fpga_dly_ns,  false);
        set_clock_delay(X300_CLOCK_WHICH_DB0_RX, _delays.db_rx_dly_ns, false);  //Sets both Ch0 and Ch1
        set_clock_delay(X300_CLOCK_WHICH_DB0_TX, _delays.db_tx_dly_ns, false);  //Sets both Ch0 and Ch1
        set_clock_delay(X300_CLOCK_WHICH_ADC0,   _delays.adc_dly_ns,   false);  //Sets both Ch0 and Ch1
        set_clock_delay(X300_CLOCK_WHICH_DAC0,   _delays.dac_dly_ns,   false);  //Sets both Ch0 and Ch1

        /* Write the configuration values into the LMK */
        for (size_t i = 1; i <= 16; ++i) {
            this->write_regs(i);
        }
        for (size_t i = 24; i <= 31; ++i) {
            this->write_regs(i);
        }

        this->sync_clocks();
    }

    const spi_iface::sptr   _spiface;
    const size_t            _slaveno;
    const size_t            _hw_rev;
    const double            _master_clock_rate;
    const double            _dboard_clock_rate;
    const double            _system_ref_rate;
    lmk04816_regs_t         _lmk04816_regs;
    double                  _vco_freq;
    x300_clk_delays         _delays;
};

x300_clock_ctrl::sptr x300_clock_ctrl::make(uhd::spi_iface::sptr spiface,
        const size_t slaveno,
        const size_t hw_rev,
        const double master_clock_rate,
        const double dboard_clock_rate,
        const double system_ref_rate) {
    return sptr(new x300_clock_ctrl_impl(spiface, slaveno, hw_rev,
                master_clock_rate, dboard_clock_rate, system_ref_rate));
}
