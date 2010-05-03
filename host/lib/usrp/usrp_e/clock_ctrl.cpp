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
#include "ad9522_regs.hpp"
#include <boost/cstdint.hpp>
#include "usrp_e_regs.hpp" //spi slave constants
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <utility>
#include <iostream>

using namespace uhd;

/***********************************************************************
 * Clock Control Implementation
 **********************************************************************/
class clock_ctrl_impl : public clock_ctrl{
public:
    //structors
    clock_ctrl_impl(usrp_e_iface::sptr iface);
    ~clock_ctrl_impl(void);

    void enable_rx_dboard_clock(bool enb);
    void enable_tx_dboard_clock(bool enb);

private:
    usrp_e_iface::sptr _iface;
    ad9522_regs_t _ad9522_regs;

    void latch_regs(void){
        _ad9522_regs.io_update = 1;
        this->send_reg(0x232);
    }
    void send_reg(boost::uint16_t addr);
};

/***********************************************************************
 * Clock Control Methods
 **********************************************************************/
clock_ctrl_impl::clock_ctrl_impl(usrp_e_iface::sptr iface){
    _iface = iface;

    //init the clock gen registers
    //Note: out0 should already be clocking the FPGA or this isnt going to work
    _ad9522_regs.sdo_active = ad9522_regs_t::SDO_ACTIVE_SDO_SDIO;
    _ad9522_regs.status_pin_control = 0x1; //n divider
    _ad9522_regs.ld_pin_control = 0x32; //show ref2
    _ad9522_regs.refmon_pin_control = 0x12; //show ref2

    _ad9522_regs.enable_ref2 = 1;
    _ad9522_regs.select_ref = ad9522_regs_t::SELECT_REF_REF2;

    _ad9522_regs.r_counter_lsb = 1;
    _ad9522_regs.r_counter_msb = 0;
    _ad9522_regs.a_counter = 0;
    _ad9522_regs.b_counter_lsb = 20;
    _ad9522_regs.b_counter_msb = 0;
    _ad9522_regs.prescaler_p = ad9522_regs_t::PRESCALER_P_DIV8_9;

    _ad9522_regs.pll_power_down = ad9522_regs_t::PLL_POWER_DOWN_NORMAL;
    _ad9522_regs.cp_current = ad9522_regs_t::CP_CURRENT_3_0MA;

    _ad9522_regs.vco_calibration_now = 1; //calibrate it!
    _ad9522_regs.vco_divider = ad9522_regs_t::VCO_DIVIDER_DIV5;
    _ad9522_regs.select_vco_or_clock = ad9522_regs_t::SELECT_VCO_OR_CLOCK_VCO;

    _ad9522_regs.out0_format = ad9522_regs_t::OUT0_FORMAT_LVDS;
    _ad9522_regs.divider0_low_cycles = 2; //3 low
    _ad9522_regs.divider0_high_cycles = 1; //2 high

    //setup a list of register ranges to write
    typedef std::pair<boost::uint16_t, boost::uint16_t> range_t;
    static const std::vector<range_t> ranges = boost::assign::list_of
        (range_t(0x000, 0x000)) (range_t(0x010, 0x01F))
        (range_t(0x0F0, 0x0FD)) (range_t(0x190, 0x19B))
        (range_t(0x1E0, 0x1E1)) (range_t(0x230, 0x230))
    ;

    //write initial register values and latch/update
    BOOST_FOREACH(const range_t &range, ranges){
        for(boost::uint16_t addr = range.first; addr <= range.second; addr++){
            this->send_reg(addr);
        }
    }
    this->latch_regs();
    //test read:
    //boost::uint32_t reg = _ad9522_regs.get_read_reg(0x01b);
    //boost::uint32_t result = _iface->transact_spi(
    //    UE_SPI_SS_AD9522,
    //    spi_config_t::EDGE_RISE,
    //    reg, 24, true /*no*/
    //);
    //std::cout << "result " << std::hex << result << std::endl;
}

clock_ctrl_impl::~clock_ctrl_impl(void){
    this->enable_rx_dboard_clock(false);
    this->enable_tx_dboard_clock(false);
}

void clock_ctrl_impl::enable_rx_dboard_clock(bool enb){
    
}

void clock_ctrl_impl::enable_tx_dboard_clock(bool enb){
    
}

void clock_ctrl_impl::send_reg(boost::uint16_t addr){
    boost::uint32_t reg = _ad9522_regs.get_write_reg(addr);
    std::cout << "clock control write reg: " << std::hex << reg << std::endl;
    _iface->transact_spi(
        UE_SPI_SS_AD9522,
        spi_config_t::EDGE_RISE,
        reg, 24, false /*no rb*/
    );
}

/***********************************************************************
 * Clock Control Make
 **********************************************************************/
clock_ctrl::sptr clock_ctrl::make(usrp_e_iface::sptr iface){
    return sptr(new clock_ctrl_impl(iface));
}
