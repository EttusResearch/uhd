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

using namespace uhd::usrp;

/*!
 * A usrp2 clock control specific to the ad9510 ic.
 */
class clock_control_ad9510 : public clock_control{
public:
    clock_control_ad9510(usrp2_impl *impl){
        _impl = impl;
        this->enable_rx_dboard_clock(false);
        this->enable_tx_dboard_clock(false);
    }

    ~clock_control_ad9510(void){
        /* NOP */
    }

    void enable_rx_dboard_clock(bool enb){
        _ad9510_regs.power_down_lvds_cmos_out7 = enb? 0 : 1;
        _ad9510_regs.lvds_cmos_select_out7 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT7_CMOS;
        _ad9510_regs.output_level_lvds_out7 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT7_1_75MA;
        this->write_reg(0x43);
        this->update_regs();
    }

    void enable_tx_dboard_clock(bool enb){
        _ad9510_regs.power_down_lvds_cmos_out6 = enb? 0 : 1;
        _ad9510_regs.lvds_cmos_select_out6 = ad9510_regs_t::LVDS_CMOS_SELECT_OUT6_CMOS;
        _ad9510_regs.output_level_lvds_out6 = ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT6_1_75MA;
        this->write_reg(0x42);
        this->update_regs();
    }

private:
    /*!
     * Write a single register to the spi regs.
     * \param addr the address to write
     */
    void write_reg(boost::uint8_t addr){
        boost::uint32_t data = _ad9510_regs.get_write_reg(addr);
        _impl->transact_spi(SPI_SS_AD9510, spi_config_t::EDGE_RISE, data, 24, false /*no rb*/);
    }

    /*!
     * Tells the ad9510 to latch the settings into the operational registers.
     */
    void update_regs(void){
        _ad9510_regs.update_registers = 1;
        this->write_reg(0x5a);
    }

    usrp2_impl *_impl;
    ad9510_regs_t _ad9510_regs;
};

/***********************************************************************
 * Public make function for the ad9510 clock control
 **********************************************************************/
clock_control::sptr clock_control::make_ad9510(usrp2_impl *impl){
    return clock_control::sptr(new clock_control_ad9510(impl));
}
