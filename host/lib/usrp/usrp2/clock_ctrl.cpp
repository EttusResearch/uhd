//
// Copyright 2010 Ettus Research LLC
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

#include "clock_ctrl.hpp"
#include "ad9510_regs.hpp"
#include "usrp2_regs.hpp" //spi slave constants
#include <uhd/utils/assert.hpp>
#include <boost/cstdint.hpp>
#include <iostream>

using namespace uhd;

/*!
 * A usrp2 clock control specific to the ad9510 ic.
 */
class usrp2_clock_ctrl_impl : public usrp2_clock_ctrl{
public:
    usrp2_clock_ctrl_impl(usrp2_iface::sptr iface){
        _iface = iface;

        _ad9510_regs.cp_current_setting = ad9510_regs_t::CP_CURRENT_SETTING_3_0MA;
        this->write_reg(0x09);

        // Setup the clock registers to 100MHz:
        //  This was already done by the firmware (or the host couldnt communicate).
        //  We could remove this part, and just leave it to the firmware.
        //  But why not leave it in for those who want to mess with clock settings?
        //  100mhz = 10mhz/R * (P*B + A)

        _ad9510_regs.pll_power_down = ad9510_regs_t::PLL_POWER_DOWN_NORMAL;
        _ad9510_regs.prescaler_value = ad9510_regs_t::PRESCALER_VALUE_DIV2;
        this->write_reg(0x0A);

        _ad9510_regs.acounter = 0;
        this->write_reg(0x04);

        _ad9510_regs.bcounter_msb = 0;
        _ad9510_regs.bcounter_lsb = 5;
        this->write_reg(0x05);
        this->write_reg(0x06);

        _ad9510_regs.ref_counter_msb = 0;
        _ad9510_regs.ref_counter_lsb = 1; // r divider = 1
        this->write_reg(0x0B);
        this->write_reg(0x0C);

        /* regs will be updated in commands below */

        this->enable_external_ref(false);
        this->enable_rx_dboard_clock(false);
        this->enable_tx_dboard_clock(false);

        /* private clock enables, must be set here */
        this->enable_dac_clock(true);
        this->enable_adc_clock(true);

        /* always driving the mimo reference */
        this->enable_mimo_clock_out(true);
    }

    ~usrp2_clock_ctrl_impl(void){
        //power down clock outputs
        this->enable_external_ref(false);
        this->enable_rx_dboard_clock(false);
        this->enable_tx_dboard_clock(false);
        this->enable_dac_clock(false);
        this->enable_adc_clock(false);
        this->enable_mimo_clock_out(false);
    }

    void enable_mimo_clock_out(bool enb){
        //FIXME TODO put this revision read in a common place
        boost::uint8_t rev_hi = _iface->read_eeprom(USRP2_I2C_ADDR_MBOARD, USRP2_EE_MBOARD_REV_MSB, 1).at(0);

        //calculate the low and high dividers
        size_t divider = size_t(this->get_master_clock_rate()/10e6);
        size_t high = divider/2;
        size_t low = divider - high;

        switch(rev_hi){
        case 3: //clock 2
            _ad9510_regs.power_down_lvpecl_out2 = enb?
                ad9510_regs_t::POWER_DOWN_LVPECL_OUT2_NORMAL :
                ad9510_regs_t::POWER_DOWN_LVPECL_OUT2_SAFE_PD;
            _ad9510_regs.output_level_lvpecl_out2 = ad9510_regs_t::OUTPUT_LEVEL_LVPECL_OUT2_810MV;
            //set the registers (divider - 1)
            _ad9510_regs.divider_low_cycles_out2 = low - 1;
            _ad9510_regs.divider_high_cycles_out2 = high - 1;
            _ad9510_regs.bypass_divider_out2 = 0;
            this->write_reg(0x3e);
            this->write_reg(0x4c);
            break;

        case 4: //clock 5
            _ad9510_regs.power_down_lvds_cmos_out5 = enb? 0 : 1;
            _ad9510_regs.lvds_cmos_select_out5 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT5_LVDS;
            _ad9510_regs.output_level_lvds_out5 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT5_1_75MA;
            //set the registers (divider - 1)
            _ad9510_regs.divider_low_cycles_out5 = low - 1;
            _ad9510_regs.divider_high_cycles_out5 = high - 1;
            _ad9510_regs.bypass_divider_out5 = 0;
            this->write_reg(0x41);
            this->write_reg(0x52);
            break;

        //TODO FIXME do i want to throw, what about uninitialized boards?
        //default: throw std::runtime_error("unknown rev hi in mboard eeprom");
        default: std::cerr << "unknown rev hi: " << rev_hi << std::endl;
        }
        this->update_regs();
    }

    //uses output clock 7 (cmos)
    void enable_rx_dboard_clock(bool enb){
        _ad9510_regs.power_down_lvds_cmos_out7 = enb? 0 : 1;
        _ad9510_regs.lvds_cmos_select_out7 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT7_CMOS;
        _ad9510_regs.output_level_lvds_out7 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT7_1_75MA;
        this->write_reg(0x43);
        this->update_regs();
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
        this->write_reg(0x56);
        this->write_reg(0x57);
        this->update_regs();
    }

    std::vector<double> get_rates_rx_dboard_clock(void){
        std::vector<double> rates;
        for (size_t i = 1; i <= 16+16; i++) rates.push_back(get_master_clock_rate()/i);
        return rates;
    }

    //uses output clock 6 (cmos)
    void enable_tx_dboard_clock(bool enb){
        _ad9510_regs.power_down_lvds_cmos_out6 = enb? 0 : 1;
        _ad9510_regs.lvds_cmos_select_out6 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT6_CMOS;
        _ad9510_regs.output_level_lvds_out6 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT6_1_75MA;
        this->write_reg(0x42);
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
        //set the registers (divider - 1)
        _ad9510_regs.divider_low_cycles_out6 = low - 1;
        _ad9510_regs.divider_high_cycles_out6 = high - 1;
        //write the registers
        this->write_reg(0x54);
        this->write_reg(0x55);
        this->update_regs();
    }

    std::vector<double> get_rates_tx_dboard_clock(void){
        return get_rates_rx_dboard_clock(); //same master clock, same dividers...
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
        this->write_reg(0x08);
        this->update_regs();
    }

    double get_master_clock_rate(void){
        return 100e6;
    }

private:
    /*!
     * Write a single register to the spi regs.
     * \param addr the address to write
     */
    void write_reg(boost::uint8_t addr){
        boost::uint32_t data = _ad9510_regs.get_write_reg(addr);
        _iface->transact_spi(SPI_SS_AD9510, spi_config_t::EDGE_RISE, data, 24, false /*no rb*/);
    }

    /*!
     * Tells the ad9510 to latch the settings into the operational registers.
     */
    void update_regs(void){
        _ad9510_regs.update_registers = 1;
        this->write_reg(0x5a);
    }

    //uses output clock 3 (pecl)
    void enable_dac_clock(bool enb){
        _ad9510_regs.power_down_lvpecl_out3 = (enb)?
            ad9510_regs_t::POWER_DOWN_LVPECL_OUT3_NORMAL :
            ad9510_regs_t::POWER_DOWN_LVPECL_OUT3_SAFE_PD;
        _ad9510_regs.output_level_lvpecl_out3 = ad9510_regs_t::OUTPUT_LEVEL_LVPECL_OUT3_810MV;
        _ad9510_regs.bypass_divider_out3 = 1;
        this->write_reg(0x3F);
        this->write_reg(0x4F);
        this->update_regs();
    }

    //uses output clock 4 (lvds)
    void enable_adc_clock(bool enb){
        _ad9510_regs.power_down_lvds_cmos_out4 = enb? 0 : 1;
        _ad9510_regs.lvds_cmos_select_out4 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT4_LVDS;
        _ad9510_regs.output_level_lvds_out4 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT4_1_75MA;
        _ad9510_regs.bypass_divider_out4 = 1;
        this->write_reg(0x40);
        this->write_reg(0x51);
        this->update_regs();
    }

    usrp2_iface::sptr _iface;
    ad9510_regs_t _ad9510_regs;
};

/***********************************************************************
 * Public make function for the ad9510 clock control
 **********************************************************************/
usrp2_clock_ctrl::sptr usrp2_clock_ctrl::make(usrp2_iface::sptr iface){
    return sptr(new usrp2_clock_ctrl_impl(iface));
}
