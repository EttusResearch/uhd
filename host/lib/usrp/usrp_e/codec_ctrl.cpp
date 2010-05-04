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

#include "codec_ctrl.hpp"
#include "ad9862_regs.hpp"
#include <boost/cstdint.hpp>
#include "usrp_e_regs.hpp" //spi slave constants
#include <boost/assign/list_of.hpp>
//#include <boost/foreach.hpp>
//#include <utility>
#include <iostream>

    //test out codec ls dac/adc
    //ad9862_regs_t ad9862_regs;
    //ad9862_regs.select_a = ad9862_regs_t::SELECT_A_AUX_ADC1;
    //ad9862_regs.aux_dac_a = 0xff/2;
    //_iface->transact_spi(
    //    UE_SPI_SS_AD9862,
    //    spi_config_t::EDGE_RISE,
    //    ad9862_regs.get_write_reg(34), 16, false /*no rb*/
    //);
    //_iface->transact_spi(
    //    UE_SPI_SS_AD9862,
    //    spi_config_t::EDGE_RISE,
    //    ad9862_regs.get_write_reg(36), 16, false /*no rb*/
    //);
    //boost::uint32_t val = _iface->transact_spi(
    //    UE_SPI_SS_AD9862,
    //    spi_config_t::EDGE_RISE,
    //    ad9862_regs.get_read_reg(29), 16, true
    //);
    //std::cout << "value: " << std::hex << val << std::endl;

using namespace uhd;

/***********************************************************************
 * Codec Control Implementation
 **********************************************************************/
class codec_ctrl_impl : public codec_ctrl{
public:
    //structors
    codec_ctrl_impl(usrp_e_iface::sptr iface);
    ~codec_ctrl_impl(void);

    //aux adc and dac control
    float read_aux_adc(aux_adc_t which);
    void read_aux_adc(aux_dac_t which, float volts);

private:
    usrp_e_iface::sptr _iface;
    ad9862_regs_t _ad9862_regs;
    void send_reg(boost::uint8_t addr);
};

/***********************************************************************
 * Codec Control Methods
 **********************************************************************/
codec_ctrl_impl::codec_ctrl_impl(usrp_e_iface::sptr iface){
    _iface = iface;

    //soft reset
    _ad9862_regs.soft_reset = 1;
    this->send_reg(0);

    //initialize the codec register settings
    _ad9862_regs.sdio_bidir = ad9862_regs_t::SDIO_BIDIR_SDIO_SDO;
    _ad9862_regs.lsb_first = ad9862_regs_t::LSB_FIRST_MSB;
    _ad9862_regs.soft_reset = 0;

    //write the register settings to the codec
    for (uint8_t addr = 0; addr <= 50; addr++){
        this->send_reg(addr);
    }
}

codec_ctrl_impl::~codec_ctrl_impl(void){
    _ad9862_regs.all_rx_pd = 1;
    this->send_reg(1);
    _ad9862_regs.tx_digital_pd = 1;
    _ad9862_regs.tx_analog_pd = ad9862_regs_t::TX_ANALOG_PD_BOTH;
    this->send_reg(8);
}

float codec_ctrl_impl::read_aux_adc(aux_adc_t which){
    return 0;

}

void codec_ctrl_impl::read_aux_adc(aux_dac_t which, float volts){
    
}

void codec_ctrl_impl::send_reg(boost::uint8_t addr){
    boost::uint32_t reg = _ad9862_regs.get_write_reg(addr);
    //std::cout << "codec control write reg: " << std::hex << reg << std::endl;
    _iface->transact_spi(
        UE_SPI_SS_AD9862,
        spi_config_t::EDGE_RISE,
        reg, 24, false /*no rb*/
    );
}

/***********************************************************************
 * Codec Control Make
 **********************************************************************/
codec_ctrl::sptr codec_ctrl::make(usrp_e_iface::sptr iface){
    return sptr(new codec_ctrl_impl(iface));
}
