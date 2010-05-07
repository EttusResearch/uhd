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
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/cstdint.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/math/special_functions/round.hpp>
#include "usrp_e_regs.hpp" //spi slave constants
#include <boost/assign/list_of.hpp>
#include <iostream>

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
    void write_aux_dac(aux_dac_t which, float volts);

private:
    usrp_e_iface::sptr _iface;
    ad9862_regs_t _ad9862_regs;
    aux_adc_t _last_aux_adc_a, _last_aux_adc_b;
    void send_reg(boost::uint8_t addr);
    void recv_reg(boost::uint8_t addr);
};

/***********************************************************************
 * Codec Control Structors
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
    //set aux dacs to zero
    this->write_aux_dac(AUX_DAC_A, 0);
    this->write_aux_dac(AUX_DAC_B, 0);
    this->write_aux_dac(AUX_DAC_C, 0);
    this->write_aux_dac(AUX_DAC_D, 0);

    //power down
    _ad9862_regs.all_rx_pd = 1;
    this->send_reg(1);
    _ad9862_regs.tx_digital_pd = 1;
    _ad9862_regs.tx_analog_pd = ad9862_regs_t::TX_ANALOG_PD_BOTH;
    this->send_reg(8);
}

/***********************************************************************
 * Codec Control AUX ADC Methods
 **********************************************************************/
static float aux_adc_to_volts(boost::uint8_t high, boost::uint8_t low){
    return float((boost::uint16_t(high) << 2) | low)*3.3/0x3ff;
}

float codec_ctrl_impl::read_aux_adc(aux_adc_t which){
    //check to see if the switch needs to be set
    bool write_switch = false;
    switch(which){

    case AUX_ADC_A1:
    case AUX_ADC_A2:
        if (which != _last_aux_adc_a){
            _ad9862_regs.select_a = (which == AUX_ADC_A1)?
                ad9862_regs_t::SELECT_A_AUX_ADC1: ad9862_regs_t::SELECT_A_AUX_ADC2;
            _last_aux_adc_a = which;
            write_switch = true;
        }
        break;

    case AUX_ADC_B1:
    case AUX_ADC_B2:
        if (which != _last_aux_adc_b){
            _ad9862_regs.select_b = (which == AUX_ADC_B1)?
                ad9862_regs_t::SELECT_B_AUX_ADC1: ad9862_regs_t::SELECT_B_AUX_ADC2;
            _last_aux_adc_b = which;
            write_switch = true;
        }
        break;

    }

    //write the switch if it changed
    if(write_switch) this->send_reg(34);

    //map aux adcs to register values to read
    static const uhd::dict<aux_adc_t, boost::uint8_t> aux_dac_to_addr = boost::assign::map_list_of
        (AUX_ADC_A2, 26) (AUX_ADC_A1, 28)
        (AUX_ADC_B2, 30) (AUX_ADC_B1, 32)
    ;

    //read the value
    this->recv_reg(aux_dac_to_addr[which]+0);
    this->recv_reg(aux_dac_to_addr[which]+1);

    //return the value scaled to volts
    switch(which){
    case AUX_ADC_A1: return aux_adc_to_volts(_ad9862_regs.aux_adc_a1_9_2, _ad9862_regs.aux_adc_a1_1_0);
    case AUX_ADC_A2: return aux_adc_to_volts(_ad9862_regs.aux_adc_a2_9_2, _ad9862_regs.aux_adc_a2_1_0);
    case AUX_ADC_B1: return aux_adc_to_volts(_ad9862_regs.aux_adc_b1_9_2, _ad9862_regs.aux_adc_b1_1_0);
    case AUX_ADC_B2: return aux_adc_to_volts(_ad9862_regs.aux_adc_b2_9_2, _ad9862_regs.aux_adc_b2_1_0);
    }
    UHD_ASSERT_THROW(false);
}

/***********************************************************************
 * Codec Control AUX DAC Methods
 **********************************************************************/
void codec_ctrl_impl::write_aux_dac(aux_dac_t which, float volts){
    //special case for aux dac d (aka sigma delta word)
    if (which == AUX_DAC_D){
        boost::uint16_t dac_word = std::clip(boost::math::iround(volts*0xfff/3.3), 0, 0xfff);
        _ad9862_regs.sig_delt_11_4 = boost::uint8_t(dac_word >> 4);
        _ad9862_regs.sig_delt_3_0 = boost::uint8_t(dac_word & 0xf);
        this->send_reg(42);
        this->send_reg(43);
        return;
    }

    //calculate the dac word for aux dac a, b, c
    boost::uint8_t dac_word = std::clip(boost::math::iround(volts*0xff/3.3), 0, 0xff);

    //setup a lookup table for the aux dac params (reg ref, reg addr)
    typedef boost::tuple<boost::uint8_t*, boost::uint8_t> dac_params_t;
    uhd::dict<aux_dac_t, dac_params_t> aux_dac_to_params = boost::assign::map_list_of
        (AUX_DAC_A, dac_params_t(&_ad9862_regs.aux_dac_a, 36))
        (AUX_DAC_B, dac_params_t(&_ad9862_regs.aux_dac_b, 37))
        (AUX_DAC_C, dac_params_t(&_ad9862_regs.aux_dac_c, 38))
    ;

    //set the aux dac register
    UHD_ASSERT_THROW(aux_dac_to_params.has_key(which));
    boost::uint8_t *reg_ref, reg_addr;
    boost::tie(reg_ref, reg_addr) = aux_dac_to_params[which];
    *reg_ref = dac_word;
    this->send_reg(reg_addr);
}

/***********************************************************************
 * Codec Control SPI Methods
 **********************************************************************/
void codec_ctrl_impl::send_reg(boost::uint8_t addr){
    boost::uint32_t reg = _ad9862_regs.get_write_reg(addr);
    //std::cout << "codec control write reg: " << std::hex << reg << std::endl;
    _iface->transact_spi(
        UE_SPI_SS_AD9862,
        spi_config_t::EDGE_RISE,
        reg, 16, false /*no rb*/
    );
}

void codec_ctrl_impl::recv_reg(boost::uint8_t addr){
    boost::uint32_t reg = _ad9862_regs.get_read_reg(addr);
    //std::cout << "codec control read reg: " << std::hex << reg << std::endl;
    boost::uint32_t ret = _iface->transact_spi(
        UE_SPI_SS_AD9862,
        spi_config_t::EDGE_RISE,
        reg, 16, true /*rb*/
    );
    //std::cout << "codec control read ret: " << std::hex << ret << std::endl;
    _ad9862_regs.set_reg(addr, boost::uint16_t(ret));
}

/***********************************************************************
 * Codec Control Make
 **********************************************************************/
codec_ctrl::sptr codec_ctrl::make(usrp_e_iface::sptr iface){
    return sptr(new codec_ctrl_impl(iface));
}
