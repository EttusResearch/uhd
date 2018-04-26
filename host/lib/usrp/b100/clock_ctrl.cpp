//
// Copyright 2011,2014,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "clock_ctrl.hpp"
#include "ad9522_regs.hpp"
#include <uhd/utils/log.hpp>

#include <uhd/exception.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/safe_call.hpp>
#include <stdint.h>
#include "b100_regs.hpp" //spi slave constants
#include <boost/format.hpp>
#include <boost/math/common_factor_rt.hpp> //gcd
#include <algorithm>
#include <utility>
#include <chrono>
#include <thread>

using namespace uhd;

/***********************************************************************
 * Constants
 **********************************************************************/
static const bool ENABLE_THE_TEST_OUT = true;
static const double REFERENCE_INPUT_RATE = 10e6;

/***********************************************************************
 * Helpers
 **********************************************************************/
template <typename div_type, typename bypass_type> static void set_clock_divider(
    size_t divider, div_type &low, div_type &high, bypass_type &bypass
){
    high = divider/2 - 1;
    low = divider - high - 2;
    bypass = (divider == 1)? 1 : 0;
}

/***********************************************************************
 * Clock rate calculation stuff:
 *   Using the internal VCO between 1400 and 1800 MHz
 **********************************************************************/
struct clock_settings_type{
    size_t ref_clock_doubler, r_counter, a_counter, b_counter, prescaler, vco_divider, chan_divider;
    size_t get_n_counter(void) const{return prescaler * b_counter + a_counter;}
    double get_ref_rate(void) const{return REFERENCE_INPUT_RATE * ref_clock_doubler;}
    double get_vco_rate(void) const{return get_ref_rate()/r_counter * get_n_counter();}
    double get_chan_rate(void) const{return get_vco_rate()/vco_divider;}
    double get_out_rate(void) const{return get_chan_rate()/chan_divider;}
    std::string to_pp_string(void) const{
        return str(boost::format(
            "  r_counter: %d\n"
            "  a_counter: %d\n"
            "  b_counter: %d\n"
            "  prescaler: %d\n"
            "  vco_divider: %d\n"
            "  chan_divider: %d\n"
            "  vco_rate: %fMHz\n"
            "  chan_rate: %fMHz\n"
            "  out_rate: %fMHz\n"
            )
            % r_counter
            % a_counter
            % b_counter
            % prescaler
            % vco_divider
            % chan_divider
            % (get_vco_rate()/1e6)
            % (get_chan_rate()/1e6)
            % (get_out_rate()/1e6)
        );
    }
};

//! gives the greatest divisor of num between 1 and max inclusive
template<typename T> static inline T greatest_divisor(T num, T max){
    for (T i = max; i > 1; i--){
        if (num%i == 0){
            return i;
        }
    }
    return 1;
}

//! gives the least divisor of num between min and num exclusive
template<typename T> static inline T least_divisor(T num, T min){
    for (T i = min; i < num; i++){
        if (num%i == 0){
            return i;
        }
    }
    return 1;
}

static clock_settings_type get_clock_settings(double rate){
    clock_settings_type cs;
    cs.ref_clock_doubler = 2; //always doubling
    cs.prescaler = 8; //set to 8 when input is under 2400 MHz

    //basic formulas used below:
    //out_rate*X = ref_rate*Y
    //X = i*ref_rate/gcd
    //Y = i*out_rate/gcd
    //X = chan_div * vco_div * R
    //Y = P*B + A

    const uint64_t out_rate = uint64_t(rate);
    const uint64_t ref_rate = uint64_t(cs.get_ref_rate());
    const size_t gcd = size_t(boost::math::gcd(ref_rate, out_rate));

    for (size_t i = 1; i <= 100; i++){
        const size_t X = size_t(i*ref_rate/gcd);
        const size_t Y = size_t(i*out_rate/gcd);

        //determine A and B (P is fixed)
        cs.b_counter = Y/cs.prescaler;
        cs.a_counter = Y - cs.b_counter*cs.prescaler;

        static const double vco_bound_pad = 100e6;
        for ( //calculate an r divider that fits into the bounds of the vco
            cs.r_counter  = size_t(cs.get_n_counter()*cs.get_ref_rate()/(1800e6 - vco_bound_pad));
            cs.r_counter <= size_t(cs.get_n_counter()*cs.get_ref_rate()/(1400e6 + vco_bound_pad))
            and cs.r_counter > 0; cs.r_counter++
        ){

            //determine chan_div and vco_div
            //and fill in that order of preference
            cs.chan_divider = greatest_divisor<size_t>(X/cs.r_counter, 32);
            cs.vco_divider = greatest_divisor<size_t>(X/cs.chan_divider/cs.r_counter, 6);

            //avoid a vco divider of 1 (if possible)
            if (cs.vco_divider == 1){
                cs.vco_divider = least_divisor<size_t>(cs.chan_divider, 2);
                cs.chan_divider /= cs.vco_divider;
            }

            UHD_LOGGER_DEBUG("B100")
                << "gcd: " << gcd
                << " X: " << X
                << " Y: " << Y
                << cs.to_pp_string()
            ;

            //filter limits on the counters
            if (cs.vco_divider == 1) continue;
            if (cs.r_counter >= (1<<14)) continue;
            if (cs.b_counter == 2) continue;
            if (cs.b_counter == 1 and cs.a_counter != 0) continue;
            if (cs.b_counter >= (1<<13)) continue;
            if (cs.a_counter >= (1<<6)) continue;
            if (cs.get_vco_rate() > 1800e6 - vco_bound_pad) continue;
            if (cs.get_vco_rate() < 1400e6 + vco_bound_pad) continue;
            if (cs.get_out_rate() != rate) continue;

            UHD_LOGGER_INFO("B100") << "USRP-B100 clock control: " << i  << cs.to_pp_string() ;
            return cs;
        }
    }

    throw uhd::value_error(str(boost::format(
        "USRP-B100 clock control: could not calculate settings for clock rate %fMHz"
    ) % (rate/1e6)));
}

b100_clock_ctrl::~b100_clock_ctrl(void) {
    /* NOP */
}

/***********************************************************************
 * Clock Control Implementation
 **********************************************************************/
class b100_clock_ctrl_impl : public b100_clock_ctrl{
public:
    b100_clock_ctrl_impl(i2c_iface::sptr iface, double master_clock_rate){
        _iface = iface;
        _chan_rate = 0.0;
        _out_rate = 0.0;

        //perform soft-reset
        _ad9522_regs.soft_reset = 1;
        this->send_reg(0x000);
        this->latch_regs();
        _ad9522_regs.soft_reset = 0;

        //init the clock gen registers
        _ad9522_regs.sdo_active = ad9522_regs_t::SDO_ACTIVE_SDO_SDIO;
        _ad9522_regs.enb_stat_eeprom_at_stat_pin = 0; //use status pin
        _ad9522_regs.status_pin_control = 0x1; //n divider
        _ad9522_regs.ld_pin_control = 0x00; //dld
        _ad9522_regs.refmon_pin_control = 0x12; //show ref2
        _ad9522_regs.lock_detect_counter = ad9522_regs_t::LOCK_DETECT_COUNTER_16CYC;

        this->use_internal_ref();

        this->set_fpga_clock_rate(master_clock_rate); //initialize to something

        this->enable_fpga_clock(true);
        this->enable_test_clock(ENABLE_THE_TEST_OUT);
        this->enable_rx_dboard_clock(false);
        this->enable_tx_dboard_clock(false);
    }

    ~b100_clock_ctrl_impl(void){
        UHD_SAFE_CALL(
            this->enable_test_clock(ENABLE_THE_TEST_OUT);
            this->enable_rx_dboard_clock(false);
            this->enable_tx_dboard_clock(false);
            //this->enable_fpga_clock(false); //FIXME
        )
    }

    /***********************************************************************
     * Clock rate control:
     *  - set clock rate w/ internal VCO
     *  - set clock rate w/ external VCXO
     **********************************************************************/
    void set_clock_settings_with_internal_vco(double rate){
        const clock_settings_type cs = get_clock_settings(rate);

        //set the rates to private variables so the implementation knows!
        _chan_rate = cs.get_chan_rate();
        _out_rate = cs.get_out_rate();

        _ad9522_regs.enable_clock_doubler = (cs.ref_clock_doubler == 2)? 1 : 0;

        _ad9522_regs.set_r_counter(cs.r_counter);
        _ad9522_regs.a_counter = cs.a_counter;
        _ad9522_regs.set_b_counter(cs.b_counter);
        UHD_ASSERT_THROW(cs.prescaler == 8); //assumes this below:
        _ad9522_regs.prescaler_p = ad9522_regs_t::PRESCALER_P_DIV8_9;

        _ad9522_regs.pll_power_down = ad9522_regs_t::PLL_POWER_DOWN_NORMAL;
        _ad9522_regs.cp_current = ad9522_regs_t::CP_CURRENT_1_2MA;

        _ad9522_regs.bypass_vco_divider = 0;
        switch(cs.vco_divider){
        case 1: _ad9522_regs.vco_divider = ad9522_regs_t::VCO_DIVIDER_DIV1; break;
        case 2: _ad9522_regs.vco_divider = ad9522_regs_t::VCO_DIVIDER_DIV2; break;
        case 3: _ad9522_regs.vco_divider = ad9522_regs_t::VCO_DIVIDER_DIV3; break;
        case 4: _ad9522_regs.vco_divider = ad9522_regs_t::VCO_DIVIDER_DIV4; break;
        case 5: _ad9522_regs.vco_divider = ad9522_regs_t::VCO_DIVIDER_DIV5; break;
        case 6: _ad9522_regs.vco_divider = ad9522_regs_t::VCO_DIVIDER_DIV6; break;
        }
        _ad9522_regs.select_vco_or_clock = ad9522_regs_t::SELECT_VCO_OR_CLOCK_VCO;

        //setup fpga master clock
        _ad9522_regs.out0_format = ad9522_regs_t::OUT0_FORMAT_LVDS;
        set_clock_divider(cs.chan_divider,
            _ad9522_regs.divider0_low_cycles,
            _ad9522_regs.divider0_high_cycles,
            _ad9522_regs.divider0_bypass
        );

        //setup codec clock
        _ad9522_regs.out3_format = ad9522_regs_t::OUT3_FORMAT_LVDS;
        set_clock_divider(cs.chan_divider,
            _ad9522_regs.divider1_low_cycles,
            _ad9522_regs.divider1_high_cycles,
            _ad9522_regs.divider1_bypass
        );

        this->send_all_regs();
        calibrate_now();
    }

    void set_clock_settings_with_external_vcxo(double rate){
        //set the rates to private variables so the implementation knows!
        _chan_rate = rate;
        _out_rate = rate;

        _ad9522_regs.enable_clock_doubler = 1; //doubler always on
        const double ref_rate = REFERENCE_INPUT_RATE*2;

        //bypass prescaler such that N = B
        long gcd = boost::math::gcd(long(ref_rate), long(rate));
        _ad9522_regs.set_r_counter(int(ref_rate/gcd));
        _ad9522_regs.a_counter = 0;
        _ad9522_regs.set_b_counter(int(rate/gcd));
        _ad9522_regs.prescaler_p = ad9522_regs_t::PRESCALER_P_DIV1;

        //setup external vcxo
        _ad9522_regs.pll_power_down = ad9522_regs_t::PLL_POWER_DOWN_NORMAL;
        _ad9522_regs.cp_current = ad9522_regs_t::CP_CURRENT_1_2MA;
        _ad9522_regs.bypass_vco_divider = 1;
        _ad9522_regs.select_vco_or_clock = ad9522_regs_t::SELECT_VCO_OR_CLOCK_EXTERNAL;

        //setup fpga master clock
        _ad9522_regs.out0_format = ad9522_regs_t::OUT0_FORMAT_LVDS;
        _ad9522_regs.divider0_bypass = 1;

        //setup codec clock
        _ad9522_regs.out3_format = ad9522_regs_t::OUT3_FORMAT_LVDS;
        _ad9522_regs.divider1_bypass = 1;

        this->send_all_regs();
    }

    void set_fpga_clock_rate(double rate){
        if (_out_rate == rate) return;
        if (rate == 61.44e6) set_clock_settings_with_external_vcxo(rate);
        else                 set_clock_settings_with_internal_vco(rate);
        //clock rate changed! update dboard clocks and FPGA ticks per second
        set_rx_dboard_clock_rate(rate);
        set_tx_dboard_clock_rate(rate);
    }

    double get_fpga_clock_rate(void){
        return this->_out_rate;
    }

    /***********************************************************************
     * FPGA clock enable
     **********************************************************************/
    void enable_fpga_clock(bool enb){
        _ad9522_regs.out0_format = ad9522_regs_t::OUT0_FORMAT_LVDS;
        _ad9522_regs.out0_lvds_power_down = !enb;
        this->send_reg(0x0F0);
        this->latch_regs();
    }

    /***********************************************************************
     * Special test clock output
     **********************************************************************/
    void enable_test_clock(bool enb){
        //setup test clock (same divider as codec clock)
        _ad9522_regs.out4_format = ad9522_regs_t::OUT4_FORMAT_CMOS;
        _ad9522_regs.out4_cmos_configuration = (enb)?
            ad9522_regs_t::OUT4_CMOS_CONFIGURATION_A_ON :
            ad9522_regs_t::OUT4_CMOS_CONFIGURATION_OFF;
        this->send_reg(0x0F4);
        this->latch_regs();
    }

    /***********************************************************************
     * RX Dboard Clock Control (output 9, divider 3)
     **********************************************************************/
    void enable_rx_dboard_clock(bool enb){
        _ad9522_regs.out9_format = ad9522_regs_t::OUT9_FORMAT_LVDS;
        _ad9522_regs.out9_lvds_power_down = !enb;
        this->send_reg(0x0F9);
        this->latch_regs();
    }

    std::vector<double> get_rx_dboard_clock_rates(void){
        std::vector<double> rates;
        for(size_t div = 1; div <= 16+16; div++)
            rates.push_back(this->_chan_rate/div);
        return rates;
    }

    void set_rx_dboard_clock_rate(double rate){
        assert_has(get_rx_dboard_clock_rates(), rate, "rx dboard clock rate");
        _rx_clock_rate = rate;
        size_t divider = size_t(this->_chan_rate/rate);
        //set the divider registers
        set_clock_divider(divider,
            _ad9522_regs.divider3_low_cycles,
            _ad9522_regs.divider3_high_cycles,
            _ad9522_regs.divider3_bypass
        );
        this->send_reg(0x199);
        this->send_reg(0x19a);
        this->soft_sync();
    }

    double get_rx_clock_rate(void){
        return _rx_clock_rate;
    }

    /***********************************************************************
     * TX Dboard Clock Control (output 6, divider 2)
     **********************************************************************/
    void enable_tx_dboard_clock(bool enb){
        _ad9522_regs.out6_format = ad9522_regs_t::OUT6_FORMAT_LVDS;
        _ad9522_regs.out6_lvds_power_down = !enb;
        this->send_reg(0x0F6);
        this->latch_regs();
    }

    std::vector<double> get_tx_dboard_clock_rates(void){
        return get_rx_dboard_clock_rates(); //same master clock, same dividers...
    }

    void set_tx_dboard_clock_rate(double rate){
        assert_has(get_tx_dboard_clock_rates(), rate, "tx dboard clock rate");
        _tx_clock_rate = rate;
        size_t divider = size_t(this->_chan_rate/rate);
        //set the divider registers
        set_clock_divider(divider,
            _ad9522_regs.divider2_low_cycles,
            _ad9522_regs.divider2_high_cycles,
            _ad9522_regs.divider2_bypass
        );
        this->send_reg(0x196);
        this->send_reg(0x197);
        this->soft_sync();
    }

    double get_tx_clock_rate(void){
        return _tx_clock_rate;
    }

    /***********************************************************************
     * Clock reference control
     **********************************************************************/
    void use_internal_ref(void) {
        _ad9522_regs.enable_ref2 = 1;
        _ad9522_regs.enable_ref1 = 0;
        _ad9522_regs.select_ref = ad9522_regs_t::SELECT_REF_REF2;
        _ad9522_regs.enb_auto_ref_switchover = ad9522_regs_t::ENB_AUTO_REF_SWITCHOVER_MANUAL;
        this->send_reg(0x01C);
        this->latch_regs();
    }

    void use_external_ref(void) {
        _ad9522_regs.enable_ref2 = 0;
        _ad9522_regs.enable_ref1 = 1;
        _ad9522_regs.select_ref = ad9522_regs_t::SELECT_REF_REF1;
        _ad9522_regs.enb_auto_ref_switchover = ad9522_regs_t::ENB_AUTO_REF_SWITCHOVER_MANUAL;
        this->send_reg(0x01C);
        this->latch_regs();
    }

    void use_auto_ref(void) {
        _ad9522_regs.enable_ref2 = 1;
        _ad9522_regs.enable_ref1 = 1;
        _ad9522_regs.select_ref = ad9522_regs_t::SELECT_REF_REF1;
        _ad9522_regs.enb_auto_ref_switchover = ad9522_regs_t::ENB_AUTO_REF_SWITCHOVER_AUTO;
        this->send_reg(0x01C);
        this->latch_regs();
    }

    bool get_locked(void){
        static const uint8_t addr = 0x01F;
        uint32_t reg = this->read_reg(addr);
        _ad9522_regs.set_reg(addr, reg);
        return _ad9522_regs.digital_lock_detect != 0;
    }

private:
    i2c_iface::sptr _iface;
    ad9522_regs_t _ad9522_regs;
    double _out_rate; //rate at the fpga and codec
    double _chan_rate; //rate before final dividers
    double _rx_clock_rate, _tx_clock_rate;

    void latch_regs(void){
        _ad9522_regs.io_update = 1;
        this->send_reg(0x232);
    }

    void send_reg(uint16_t addr){
        uint32_t reg = _ad9522_regs.get_write_reg(addr);
        UHD_LOGGER_TRACE("B100") << "clock control write reg: " << std::hex << reg ;
        byte_vector_t buf;
        buf.push_back(uint8_t(reg >> 16));
        buf.push_back(uint8_t(reg >> 8));
        buf.push_back(uint8_t(reg & 0xff));

        _iface->write_i2c(0x5C, buf);
    }

    uint8_t read_reg(uint16_t addr){
        byte_vector_t buf;
        buf.push_back(uint8_t(addr >> 8));
        buf.push_back(uint8_t(addr & 0xff));
        _iface->write_i2c(0x5C, buf);

        buf = _iface->read_i2c(0x5C, 1);

        return uint32_t(buf[0] & 0xFF);
    }

    void calibrate_now(void){
        //vco calibration routine:
        _ad9522_regs.vco_calibration_now = 0;
        this->send_reg(0x18);
        this->latch_regs();
        _ad9522_regs.vco_calibration_now = 1;
        this->send_reg(0x18);
        this->latch_regs();
        //wait for calibration done:
        static const uint8_t addr = 0x01F;
        for (size_t ms10 = 0; ms10 < 100; ms10++){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            uint32_t reg = read_reg(addr);
            _ad9522_regs.set_reg(addr, reg);
            if (_ad9522_regs.vco_calibration_finished) goto wait_for_ld;
        }
        UHD_LOGGER_ERROR("B100") << "USRP-B100 clock control: VCO calibration timeout";
        wait_for_ld:
        //wait for digital lock detect:
        for (size_t ms10 = 0; ms10 < 100; ms10++){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            uint32_t reg = read_reg(addr);
            _ad9522_regs.set_reg(addr, reg);
            if (_ad9522_regs.digital_lock_detect) return;
        }
        UHD_LOGGER_ERROR("B100") << "USRP-B100 clock control: lock detection timeout";
    }

    void soft_sync(void){
        _ad9522_regs.soft_sync = 1;
        this->send_reg(0x230);
        this->latch_regs();
        _ad9522_regs.soft_sync = 0;
        this->send_reg(0x230);
        this->latch_regs();
    }

    void send_all_regs(void){
        //setup a list of register ranges to write
        typedef std::pair<uint16_t, uint16_t> range_t;
        static const std::vector<range_t> ranges{
            range_t(0x000, 0x000),
            range_t(0x010, 0x01F),
            range_t(0x0F0, 0x0FD),
            range_t(0x190, 0x19B),
            range_t(0x1E0, 0x1E1),
            range_t(0x230, 0x230)
        };

        //write initial register values and latch/update
        for(const range_t &range:  ranges){
            for(uint16_t addr = range.first; addr <= range.second; addr++){
                this->send_reg(addr);
            }
        }
        this->latch_regs();
    }
};

/***********************************************************************
 * Clock Control Make
 **********************************************************************/
b100_clock_ctrl::sptr b100_clock_ctrl::make(i2c_iface::sptr iface, double master_clock_rate){
    return sptr(new b100_clock_ctrl_impl(iface, master_clock_rate));
}
