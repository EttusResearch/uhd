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
#include "ad9777_regs.hpp"
#include "ads62p44_regs.hpp"
#include "usrp2_regs.hpp"
#include <boost/cstdint.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <uhd/utils/exception.hpp>

static const bool codec_ctrl_debug = false;

using namespace uhd;

/*!
 * A usrp2 codec control specific to the ad9777 ic.
 */
class usrp2_codec_ctrl_impl : public usrp2_codec_ctrl{
public:
    usrp2_codec_ctrl_impl(usrp2_iface::sptr iface){
        _iface = iface;

        //setup the ad9777 dac
        _ad9777_regs.x_1r_2r_mode = ad9777_regs_t::X_1R_2R_MODE_1R;
        _ad9777_regs.filter_interp_rate = ad9777_regs_t::FILTER_INTERP_RATE_4X;
        _ad9777_regs.mix_mode = ad9777_regs_t::MIX_MODE_REAL;
        _ad9777_regs.pll_divide_ratio = ad9777_regs_t::PLL_DIVIDE_RATIO_DIV1;
        _ad9777_regs.pll_state = ad9777_regs_t::PLL_STATE_ON;
        _ad9777_regs.auto_cp_control = ad9777_regs_t::AUTO_CP_CONTROL_AUTO;
        //I dac values
        _ad9777_regs.idac_fine_gain_adjust = 0;
        _ad9777_regs.idac_coarse_gain_adjust = 0xf;
        _ad9777_regs.idac_offset_adjust_lsb = 0;
        _ad9777_regs.idac_offset_adjust_msb = 0;
        //Q dac values
        _ad9777_regs.qdac_fine_gain_adjust = 0;
        _ad9777_regs.qdac_coarse_gain_adjust = 0xf;
        _ad9777_regs.qdac_offset_adjust_lsb = 0;
        _ad9777_regs.qdac_offset_adjust_msb = 0;
        //write all regs
        for(boost::uint8_t addr = 0; addr <= 0xC; addr++){
            this->send_ad9777_reg(addr);
        }

        //power-up adc
        if(!_iface->is_usrp2p()) { //if we're on a USRP2
          _iface->poke32(_iface->regs.misc_ctrl_adc, U2_FLAG_MISC_CTRL_ADC_ON);
        } else { //we're on a USRP2+
          _ads62p44_regs.reset = 1;
          this->send_ads62p44_reg(0x00); //issue a reset to the ADC
          //everything else should be pretty much default, i think
//          _ads62p44_regs.decimation = DECIMATION_DECIMATE_1;
          _ads62p44_regs.power_down = ads62p44_regs_t::POWER_DOWN_NORMAL;
          this->send_ads62p44_reg(0x14);
        }
    }

    ~usrp2_codec_ctrl_impl(void){
        //power-down dac
        _ad9777_regs.power_down_mode = 1;
        this->send_ad9777_reg(0);

        //power-down adc
        if(!_iface->is_usrp2p()) { //if we're on a USRP2
          _iface->poke32(_iface->regs.misc_ctrl_adc, U2_FLAG_MISC_CTRL_ADC_OFF);
        } else { //we're on a USRP2+
          //send a global power-down to the ADC here... it will get lifted on reset
          _ads62p44_regs.power_down = ads62p44_regs_t::POWER_DOWN_GLOBAL_PD;
          this->send_ads62p44_reg(0x14);
        }
    }

    void set_rx_digital_gain(float gain) {  //fine digital gain
      if(_iface->is_usrp2p()) {
        _ads62p44_regs.fine_gain = int(gain/0.5);
        this->send_ads62p44_reg(0x17);
      } else UHD_THROW_INVALID_CODE_PATH(); //should never have been called for USRP2
    }

    void set_rx_digital_fine_gain(float gain) { //gain correction      
      if(_iface->is_usrp2p()) {
        _ads62p44_regs.gain_correction = int(gain / 0.05);
        this->send_ads62p44_reg(0x1A);
      } else UHD_THROW_INVALID_CODE_PATH(); //should never have been called for USRP2
    }

    void set_rx_analog_gain(bool gain) { //turns on/off analog 3.5dB preamp
      if(_iface->is_usrp2p()) {
        _ads62p44_regs.coarse_gain = gain ? ads62p44_regs_t::COARSE_GAIN_3_5DB : ads62p44_regs_t::COARSE_GAIN_0DB;
        this->send_ads62p44_reg(0x14);
      } else UHD_THROW_INVALID_CODE_PATH();
    }

private:
    ad9777_regs_t _ad9777_regs;
    ads62p44_regs_t _ads62p44_regs;
    usrp2_iface::sptr _iface;

    void send_ad9777_reg(boost::uint8_t addr){
        boost::uint16_t reg = _ad9777_regs.get_write_reg(addr);
        if (codec_ctrl_debug) std::cout << "send_ad9777_reg: " << std::hex << reg << std::endl;
        _iface->transact_spi(
            SPI_SS_AD9777, spi_config_t::EDGE_RISE,
            reg, 16, false /*no rb*/
        );
    }

    void send_ads62p44_reg(boost::uint8_t addr) {
        boost::uint16_t reg = _ads62p44_regs.get_write_reg(addr);
        _iface->transact_spi(
            SPI_SS_ADS62P44, spi_config_t::EDGE_FALL,
            reg, 16, false /*no rb*/
        );
    }
};

/***********************************************************************
 * Public make function for the usrp2 codec control
 **********************************************************************/
usrp2_codec_ctrl::sptr usrp2_codec_ctrl::make(usrp2_iface::sptr iface){
    return sptr(new usrp2_codec_ctrl_impl(iface));
}
