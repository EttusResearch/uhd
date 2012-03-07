//
// Copyright 2010-2012 Ettus Research LLC
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
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/exception.hpp>
#include <boost/cstdint.hpp>
#include <boost/foreach.hpp>

using namespace uhd;

/*!
 * A usrp2 codec control specific to the ad9777 ic.
 */
class usrp2_codec_ctrl_impl : public usrp2_codec_ctrl{
public:
    usrp2_codec_ctrl_impl(usrp2_iface::sptr iface, uhd::spi_iface::sptr spiface){
        _iface = iface;
        _spiface = spiface;

        //setup the ad9777 dac
        _ad9777_regs.x_1r_2r_mode = ad9777_regs_t::X_1R_2R_MODE_1R;
        _ad9777_regs.filter_interp_rate = ad9777_regs_t::FILTER_INTERP_RATE_4X;
        _ad9777_regs.mix_mode = ad9777_regs_t::MIX_MODE_COMPLEX;
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
        set_tx_mod_mode(0);

        //power-up adc
        switch(_iface->get_rev()){
        case usrp2_iface::USRP2_REV3:
        case usrp2_iface::USRP2_REV4:
            _iface->poke32(U2_REG_MISC_CTRL_ADC, U2_FLAG_MISC_CTRL_ADC_ON);
            break;

        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
            _ads62p44_regs.reset = 1;
            this->send_ads62p44_reg(0x00); //issue a reset to the ADC
            //everything else should be pretty much default, i think
            //_ads62p44_regs.decimation = DECIMATION_DECIMATE_1;
            _ads62p44_regs.power_down = ads62p44_regs_t::POWER_DOWN_NORMAL;
            this->send_ads62p44_reg(0x14);
            this->set_rx_analog_gain(1);
            break;

        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
            _ads62p44_regs.reset = 1;
            this->send_ads62p44_reg(0x00); //issue a reset to the ADC
            //everything else should be pretty much default, i think
            //_ads62p44_regs.decimation = DECIMATION_DECIMATE_1;
            _ads62p44_regs.override = 1;
            this->send_ads62p44_reg(0x14);
            _ads62p44_regs.power_down = ads62p44_regs_t::POWER_DOWN_NORMAL;
            _ads62p44_regs.output_interface = ads62p44_regs_t::OUTPUT_INTERFACE_LVDS;
            _ads62p44_regs.lvds_current = ads62p44_regs_t::LVDS_CURRENT_2_5MA;
            _ads62p44_regs.lvds_data_term = ads62p44_regs_t::LVDS_DATA_TERM_100;
            this->send_ads62p44_reg(0x11);
            this->send_ads62p44_reg(0x12);
            this->send_ads62p44_reg(0x14);
            this->set_rx_analog_gain(1);
            break;

        case usrp2_iface::USRP_NXXX: break;
        }
    }

    ~usrp2_codec_ctrl_impl(void){UHD_SAFE_CALL(
        //power-down dac
        _ad9777_regs.power_down_mode = 1;
        this->send_ad9777_reg(0);

        //power-down adc
        switch(_iface->get_rev()){
        case usrp2_iface::USRP2_REV3:
        case usrp2_iface::USRP2_REV4:
            _iface->poke32(U2_REG_MISC_CTRL_ADC, U2_FLAG_MISC_CTRL_ADC_OFF);
            break;

        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
            //send a global power-down to the ADC here... it will get lifted on reset
            _ads62p44_regs.power_down = ads62p44_regs_t::POWER_DOWN_GLOBAL_PD;
            this->send_ads62p44_reg(0x14);
            break;

        case usrp2_iface::USRP_NXXX: break;
        }
    )}

    void set_tx_mod_mode(int mod_mode){
        //set the sign of the frequency shift
        _ad9777_regs.modulation_form = (mod_mode > 0)?
            ad9777_regs_t::MODULATION_FORM_E_PLUS_JWT:
            ad9777_regs_t::MODULATION_FORM_E_MINUS_JWT
        ;

        //set the frequency shift
        switch(std::abs(mod_mode)){
        case 0:
        case 1: _ad9777_regs.modulation_mode = ad9777_regs_t::MODULATION_MODE_NONE; break;
        case 2: _ad9777_regs.modulation_mode = ad9777_regs_t::MODULATION_MODE_FS_2; break;
        case 4: _ad9777_regs.modulation_mode = ad9777_regs_t::MODULATION_MODE_FS_4; break;
        case 8: _ad9777_regs.modulation_mode = ad9777_regs_t::MODULATION_MODE_FS_8; break;
        default: throw uhd::value_error("unknown modulation mode for ad9777");
        }

        this->send_ad9777_reg(0x01); //set the register
    }

    void set_rx_digital_gain(double gain) {  //fine digital gain
        switch(_iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
            _ads62p44_regs.fine_gain = int(gain/0.5);
            this->send_ads62p44_reg(0x17);
            break;

        default: UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_rx_digital_fine_gain(double gain) { //gain correction      
        switch(_iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
            _ads62p44_regs.gain_correction = int(gain / 0.05);
            this->send_ads62p44_reg(0x1A);
            break;

        default: UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_rx_analog_gain(bool /*gain*/) { //turns on/off analog 3.5dB preamp
        switch(_iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
        case usrp2_iface::USRP_N200_R4:
        case usrp2_iface::USRP_N210_R4:
            _ads62p44_regs.coarse_gain = ads62p44_regs_t::COARSE_GAIN_3_5DB;//gain ? ads62p44_regs_t::COARSE_GAIN_3_5DB : ads62p44_regs_t::COARSE_GAIN_0DB;
            this->send_ads62p44_reg(0x14);
            break;

        default: UHD_THROW_INVALID_CODE_PATH();
        }
    }

private:
    ad9777_regs_t _ad9777_regs;
    ads62p44_regs_t _ads62p44_regs;
    usrp2_iface::sptr _iface;
    uhd::spi_iface::sptr _spiface;

    void send_ad9777_reg(boost::uint8_t addr){
        boost::uint16_t reg = _ad9777_regs.get_write_reg(addr);
        UHD_LOGV(always) << "send_ad9777_reg: " << std::hex << reg << std::endl;
        _spiface->write_spi(
            SPI_SS_AD9777, spi_config_t::EDGE_RISE,
            reg, 16
        );
    }

    void send_ads62p44_reg(boost::uint8_t addr) {
        boost::uint16_t reg = _ads62p44_regs.get_write_reg(addr);
        _spiface->write_spi(
            SPI_SS_ADS62P44, spi_config_t::EDGE_FALL,
            reg, 16
        );
    }
};

/***********************************************************************
 * Public make function for the usrp2 codec control
 **********************************************************************/
usrp2_codec_ctrl::sptr usrp2_codec_ctrl::make(usrp2_iface::sptr iface, uhd::spi_iface::sptr spiface){
    return sptr(new usrp2_codec_ctrl_impl(iface, spiface));
}
