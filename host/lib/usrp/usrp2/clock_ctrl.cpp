//
// Copyright 2010-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "clock_ctrl.hpp"
#include "ad9510_regs.hpp"
#include "usrp2_regs.hpp" //spi slave constants
#include "usrp2_clk_regs.hpp"
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/assert_has.hpp>
#include <stdint.h>
#include <boost/math/special_functions/round.hpp>
#include <iostream>

using namespace uhd;

static const bool enb_test_clk = false;

usrp2_clock_ctrl::~usrp2_clock_ctrl(void){
    /* NOP */
}

/*!
 * A usrp2 clock control specific to the ad9510 ic.
 */
class usrp2_clock_ctrl_impl : public usrp2_clock_ctrl{
public:
    usrp2_clock_ctrl_impl(usrp2_iface::sptr iface, uhd::spi_iface::sptr spiface){
        _iface = iface;
        _spiface = spiface;
        clk_regs = usrp2_clk_regs_t(_iface->get_rev());

        _ad9510_regs.cp_current_setting = ad9510_regs_t::CP_CURRENT_SETTING_3_0MA;
        this->write_reg(clk_regs.pll_3);

        // Setup the clock registers to 100MHz:
        //  This was already done by the firmware (or the host couldnt communicate).
        //  We could remove this part, and just leave it to the firmware.
        //  But why not leave it in for those who want to mess with clock settings?
        //  100mhz = 10mhz/R * (P*B + A)

        _ad9510_regs.pll_power_down = ad9510_regs_t::PLL_POWER_DOWN_NORMAL;
        _ad9510_regs.prescaler_value = ad9510_regs_t::PRESCALER_VALUE_DIV2;
        this->write_reg(clk_regs.pll_4);

        _ad9510_regs.acounter = 0;
        this->write_reg(clk_regs.acounter);

        _ad9510_regs.bcounter_msb = 0;
        _ad9510_regs.bcounter_lsb = 5;
        this->write_reg(clk_regs.bcounter_msb);
        this->write_reg(clk_regs.bcounter_lsb);

        _ad9510_regs.ref_counter_msb = 0;
        _ad9510_regs.ref_counter_lsb = 1; // r divider = 1
        this->write_reg(clk_regs.ref_counter_msb);
        this->write_reg(clk_regs.ref_counter_lsb);

        /* regs will be updated in commands below */

        this->enable_external_ref(false);
        this->enable_rx_dboard_clock(false);
        this->enable_tx_dboard_clock(false);
        this->enable_mimo_clock_out(false);

        /* private clock enables, must be set here */
        this->enable_dac_clock(true);
        this->enable_adc_clock(true);
        this->enable_test_clock(enb_test_clk);
    }

    ~usrp2_clock_ctrl_impl(void){UHD_SAFE_CALL(
        //power down clock outputs
        this->enable_external_ref(false);
        this->enable_rx_dboard_clock(false);
        this->enable_tx_dboard_clock(false);
        this->enable_dac_clock(false);
        this->enable_adc_clock(false);
        this->enable_mimo_clock_out(false);
        this->enable_test_clock(false);
    )}

    void enable_mimo_clock_out(bool enb){
        //calculate the low and high dividers
        size_t divider = size_t(this->get_master_clock_rate()/10e6);
        size_t high = divider/2;
        size_t low = divider - high;

        switch(clk_regs.exp){
        case 2: //U2 rev 3
            _ad9510_regs.power_down_lvpecl_out2 = enb?
                ad9510_regs_t::POWER_DOWN_LVPECL_OUT2_NORMAL :
                ad9510_regs_t::POWER_DOWN_LVPECL_OUT2_SAFE_PD;
            _ad9510_regs.output_level_lvpecl_out2 = ad9510_regs_t::OUTPUT_LEVEL_LVPECL_OUT2_810MV;
            //set the registers (divider - 1)
            _ad9510_regs.divider_low_cycles_out2 = low - 1;
            _ad9510_regs.divider_high_cycles_out2 = high - 1;
            _ad9510_regs.bypass_divider_out2 = 0;
            break;

        case 5: //U2 rev 4
            _ad9510_regs.power_down_lvds_cmos_out5 = enb? 0 : 1;
            _ad9510_regs.lvds_cmos_select_out5 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT5_LVDS;
            _ad9510_regs.output_level_lvds_out5 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT5_1_75MA;
            //set the registers (divider - 1)
            _ad9510_regs.divider_low_cycles_out5 = low - 1;
            _ad9510_regs.divider_high_cycles_out5 = high - 1;
            _ad9510_regs.bypass_divider_out5 = 0;
            break;
            
        case 6: //U2+
            _ad9510_regs.power_down_lvds_cmos_out6 = enb? 0 : 1;
            _ad9510_regs.lvds_cmos_select_out6 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT6_LVDS;
            _ad9510_regs.output_level_lvds_out6 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT6_1_75MA;
            //set the registers (divider - 1)
            _ad9510_regs.divider_low_cycles_out6 = low - 1;
            _ad9510_regs.divider_high_cycles_out6 = high - 1;
            _ad9510_regs.bypass_divider_out5 = 0;
            break;

        default:
            break;
        }
        this->write_reg(clk_regs.output(clk_regs.exp));
        this->write_reg(clk_regs.div_lo(clk_regs.exp));
        this->update_regs();
    }

    //uses output clock 7 (cmos)
    void enable_rx_dboard_clock(bool enb){
        switch(_iface->get_rev()) {
            case usrp2_iface::USRP_N200_R4:
            case usrp2_iface::USRP_N210_R4:
                _ad9510_regs.power_down_lvds_cmos_out7 = enb? 0 : 1;
                _ad9510_regs.lvds_cmos_select_out7 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT7_LVDS;
                _ad9510_regs.output_level_lvds_out7 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT7_1_75MA;
                this->write_reg(clk_regs.output(clk_regs.rx_db));
                this->update_regs();
                break;
            default:
                _ad9510_regs.power_down_lvds_cmos_out7 = enb? 0 : 1;
                _ad9510_regs.lvds_cmos_select_out7 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT7_CMOS;
                _ad9510_regs.output_level_lvds_out7 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT7_1_75MA;
                this->write_reg(clk_regs.output(clk_regs.rx_db));
                this->update_regs();
                break;
        }
    }

    void set_rate_rx_dboard_clock(double rate){
        assert_has(get_rates_rx_dboard_clock(), rate, "rx dboard clock rate");
        size_t divider = size_t(get_master_clock_rate()/rate);
        //bypass when the divider ratio is one
        _ad9510_regs.bypass_divider_out7 = (divider == 1)? 1 : 0;
        //calculate the low and high dividers
        size_t high = divider/2;
        size_t low = divider - high;
        //set the registers (divider - 1)
        _ad9510_regs.divider_low_cycles_out7 = low - 1;
        _ad9510_regs.divider_high_cycles_out7 = high - 1;
        //write the registers
        this->write_reg(clk_regs.div_lo(clk_regs.rx_db));
        this->write_reg(clk_regs.div_hi(clk_regs.rx_db));
        this->update_regs();
    }

    std::vector<double> get_rates_rx_dboard_clock(void){
        std::vector<double> rates;
        for (size_t i = 1; i <= 16+16; i++) rates.push_back(get_master_clock_rate()/i);
        return rates;
    }

    //uses output clock 6 (cmos) on USRP2, output clock 5 (cmos) on N200/N210 r3,
    //and output clock 5 (lvds) on N200/N210 r4
    void enable_tx_dboard_clock(bool enb){
        switch(_iface->get_rev()) {
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
          _ad9510_regs.power_down_lvds_cmos_out5 = enb? 0 : 1;
          _ad9510_regs.lvds_cmos_select_out5 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT5_LVDS;
          _ad9510_regs.output_level_lvds_out5 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT5_1_75MA;
          break;
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
          _ad9510_regs.power_down_lvds_cmos_out5 = enb? 0 : 1;
          _ad9510_regs.lvds_cmos_select_out5 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT5_CMOS;
          _ad9510_regs.output_level_lvds_out5 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT5_1_75MA;
          break;
        case usrp2_iface::USRP2_REV3:
        case usrp2_iface::USRP2_REV4:
          _ad9510_regs.power_down_lvds_cmos_out6 = enb? 0 : 1;
          _ad9510_regs.lvds_cmos_select_out6 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT6_CMOS;
          _ad9510_regs.output_level_lvds_out6 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT6_1_75MA;
          break;

        default:
          //throw uhd::not_implemented_error("enable_tx_dboard_clock: unknown hardware version");
          break;
        }

        this->write_reg(clk_regs.output(clk_regs.tx_db));
        this->update_regs();
    }

    void set_rate_tx_dboard_clock(double rate){
        assert_has(get_rates_tx_dboard_clock(), rate, "tx dboard clock rate");
        size_t divider = size_t(get_master_clock_rate()/rate);
        //bypass when the divider ratio is one
        _ad9510_regs.bypass_divider_out6 = (divider == 1)? 1 : 0;
        //calculate the low and high dividers
        size_t high = divider/2;
        size_t low = divider - high;

        switch(clk_regs.tx_db) {
        case 5: //USRP2+
          _ad9510_regs.bypass_divider_out5 = (divider == 1)? 1 : 0;
          _ad9510_regs.divider_low_cycles_out5 = low - 1;
          _ad9510_regs.divider_high_cycles_out5 = high - 1;
          break;
        case 6: //USRP2
          //bypass when the divider ratio is one
          _ad9510_regs.bypass_divider_out6 = (divider == 1)? 1 : 0;
          //set the registers (divider - 1)
          _ad9510_regs.divider_low_cycles_out6 = low - 1;
          _ad9510_regs.divider_high_cycles_out6 = high - 1;
          break;
        }

        //write the registers
        this->write_reg(clk_regs.div_hi(clk_regs.tx_db));
        this->write_reg(clk_regs.div_lo(clk_regs.tx_db));
        this->update_regs();
    }

    std::vector<double> get_rates_tx_dboard_clock(void){
        return get_rates_rx_dboard_clock(); //same master clock, same dividers...
    }
    
    void enable_test_clock(bool enb) {
        _ad9510_regs.power_down_lvpecl_out0 = enb?
            ad9510_regs_t::POWER_DOWN_LVPECL_OUT0_NORMAL :
            ad9510_regs_t::POWER_DOWN_LVPECL_OUT0_SAFE_PD;
        _ad9510_regs.output_level_lvpecl_out0 = ad9510_regs_t::OUTPUT_LEVEL_LVPECL_OUT0_810MV;
        _ad9510_regs.divider_low_cycles_out0 = 0;
        _ad9510_regs.divider_high_cycles_out0 = 0;
        _ad9510_regs.bypass_divider_out0 = 1;
        this->write_reg(0x3c);
        this->write_reg(0x48);
        this->write_reg(0x49);
    }

    /*!
     * If we are to use an external reference, enable the charge pump.
     * \param enb true to enable the CP
     */
    void enable_external_ref(bool enb){
        _ad9510_regs.charge_pump_mode = (enb)?
            ad9510_regs_t::CHARGE_PUMP_MODE_NORMAL :
            ad9510_regs_t::CHARGE_PUMP_MODE_3STATE ;
        _ad9510_regs.pll_mux_control = ad9510_regs_t::PLL_MUX_CONTROL_DLD_HIGH;
        _ad9510_regs.pfd_polarity = ad9510_regs_t::PFD_POLARITY_POS;
        this->write_reg(clk_regs.pll_2);
        this->update_regs();
    }

    double get_master_clock_rate(void){
        return 100e6;
    }
    
    void set_mimo_clock_delay(double delay) {
        //delay_val is a 5-bit value (0-31) for fine control
        //the equations below determine delay for a given ramp current, # of caps and fine delay register
        //delay range:
        //range_ns = 200*((caps+3)/i_ramp_ua)*1.3286
        //offset (zero delay):
        //offset_ns = 0.34 + (1600 - i_ramp_ua)*1e-4 + ((caps-1)/ramp)*6
        //delay_ns = offset_ns + range_ns * delay / 31

        int delay_val = boost::math::iround(delay/9.744e-9*31);

        if(delay_val == 0) {
            switch(clk_regs.exp) {
            case 5:
                _ad9510_regs.delay_control_out5 = 1;
                break;
            case 6:
                _ad9510_regs.delay_control_out6 = 1;
                break;
            default:
                break; //delay not supported on U2 rev 3
            }
        } else {
            switch(clk_regs.exp) {
            case 5:
                _ad9510_regs.delay_control_out5 = 0;
                _ad9510_regs.ramp_current_out5 = ad9510_regs_t::RAMP_CURRENT_OUT5_200UA;
                _ad9510_regs.ramp_capacitor_out5 = ad9510_regs_t::RAMP_CAPACITOR_OUT5_4CAPS;
                _ad9510_regs.delay_fine_adjust_out5 = delay_val;
                this->write_reg(0x34);
                this->write_reg(0x35);
                this->write_reg(0x36);
                break;
            case 6:
                _ad9510_regs.delay_control_out6 = 0;
                _ad9510_regs.ramp_current_out6 = ad9510_regs_t::RAMP_CURRENT_OUT6_200UA;
                _ad9510_regs.ramp_capacitor_out6 = ad9510_regs_t::RAMP_CAPACITOR_OUT6_4CAPS;
                _ad9510_regs.delay_fine_adjust_out6 = delay_val;
                this->write_reg(0x38);
                this->write_reg(0x39);
                this->write_reg(0x3A);
                break;
            default:
                break;
            }
        }
    }

private:
    /*!
     * Write a single register to the spi regs.
     * \param addr the address to write
     */
    void write_reg(uint8_t addr){
        uint32_t data = _ad9510_regs.get_write_reg(addr);
        _spiface->write_spi(SPI_SS_AD9510, spi_config_t::EDGE_RISE, data, 24);
    }

    /*!
     * Tells the ad9510 to latch the settings into the operational registers.
     */
    void update_regs(void){
        _ad9510_regs.update_registers = 1;
        this->write_reg(clk_regs.update);
    }

    //uses output clock 3 (pecl)
    //this is the same between USRP2 and USRP2+ and doesn't get a switch statement
    void enable_dac_clock(bool enb){
        _ad9510_regs.power_down_lvpecl_out3 = (enb)?
            ad9510_regs_t::POWER_DOWN_LVPECL_OUT3_NORMAL :
            ad9510_regs_t::POWER_DOWN_LVPECL_OUT3_SAFE_PD;
        _ad9510_regs.output_level_lvpecl_out3 = ad9510_regs_t::OUTPUT_LEVEL_LVPECL_OUT3_810MV;
        _ad9510_regs.bypass_divider_out3 = 1;
        this->write_reg(clk_regs.output(clk_regs.dac));
        this->write_reg(clk_regs.div_hi(clk_regs.dac));
        this->update_regs();
    }

    //uses output clock 4 (lvds) on USRP2 and output clock 2 (lvpecl) on USRP2+
    void enable_adc_clock(bool enb){
        switch(clk_regs.adc) {
        case 2:
          _ad9510_regs.power_down_lvpecl_out2 = enb? ad9510_regs_t::POWER_DOWN_LVPECL_OUT2_NORMAL : ad9510_regs_t::POWER_DOWN_LVPECL_OUT2_SAFE_PD;
          _ad9510_regs.output_level_lvpecl_out2 = ad9510_regs_t::OUTPUT_LEVEL_LVPECL_OUT2_500MV;
          _ad9510_regs.bypass_divider_out2 = 1;
          break;
        case 4:
          _ad9510_regs.power_down_lvds_cmos_out4 = enb? 0 : 1;
          _ad9510_regs.lvds_cmos_select_out4 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT4_LVDS;
          _ad9510_regs.output_level_lvds_out4 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT4_1_75MA;
          _ad9510_regs.bypass_divider_out4 = 1;
          break;
        }

        this->write_reg(clk_regs.output(clk_regs.adc));
        this->write_reg(clk_regs.div_hi(clk_regs.adc));
        this->update_regs();
    }
    
    usrp2_iface::sptr _iface;
    uhd::spi_iface::sptr _spiface;
    usrp2_clk_regs_t clk_regs;
    ad9510_regs_t _ad9510_regs;
};

/***********************************************************************
 * Public make function for the ad9510 clock control
 **********************************************************************/
usrp2_clock_ctrl::sptr usrp2_clock_ctrl::make(usrp2_iface::sptr iface, uhd::spi_iface::sptr spiface){
    return sptr(new usrp2_clock_ctrl_impl(iface, spiface));
}
