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

#include "usrp2_impl.hpp"
#include "clock_control.hpp"
#include "ad9510_regs.hpp"
#include "usrp2_regs.hpp" //spi slave constants
#include <boost/cstdint.hpp>

using namespace uhd;
using namespace uhd::usrp;

/*!
 * A usrp2 clock control specific to the ad9510 ic.
 */
class clock_control_ad9510 : public clock_control{
public:
    clock_control_ad9510(usrp2_iface::sptr iface){
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

    }

    ~clock_control_ad9510(void){
        /* private clock enables, must be set here */
        this->enable_dac_clock(false);
        this->enable_adc_clock(false);
    }

    //uses output clock 7 (cmos)
    void enable_rx_dboard_clock(bool enb){
        _ad9510_regs.power_down_lvds_cmos_out7 = enb? 0 : 1;
        _ad9510_regs.lvds_cmos_select_out7 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT7_CMOS;
        _ad9510_regs.output_level_lvds_out7 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT7_1_75MA;
        this->write_reg(0x43);
        this->update_regs();
    }

    //uses output clock 6 (cmos)
    void enable_tx_dboard_clock(bool enb){
        _ad9510_regs.power_down_lvds_cmos_out6 = enb? 0 : 1;
        _ad9510_regs.lvds_cmos_select_out6 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT6_CMOS;
        _ad9510_regs.output_level_lvds_out6 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT6_1_75MA;
        this->write_reg(0x42);
        this->update_regs();
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
        this->write_reg(0x3F);
        this->update_regs();
    }

    //uses output clock 4 (lvds)
    void enable_adc_clock(bool enb){
        _ad9510_regs.power_down_lvds_cmos_out4 = enb? 0 : 1;
        _ad9510_regs.lvds_cmos_select_out4 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT4_LVDS;
        _ad9510_regs.output_level_lvds_out4 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT4_1_75MA;
        this->write_reg(0x40);
        this->update_regs();
    }

    usrp2_iface::sptr _iface;
    ad9510_regs_t _ad9510_regs;
};

/***********************************************************************
 * Public make function for the ad9510 clock control
 **********************************************************************/
clock_control::sptr clock_control::make_ad9510(usrp2_iface::sptr iface){
    return clock_control::sptr(new clock_control_ad9510(iface));
}
