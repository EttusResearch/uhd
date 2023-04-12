//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "codec_ctrl.hpp"
#include "ad9142a_regs.hpp"
#include "ad9777_regs.hpp"
#include "ads62p44_regs.hpp"
#include "kintex7sdr_iface.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <stdint.h>
#include <boost/thread/thread.hpp>

using namespace uhd;

/*!
 * A kintex7sdr codec control specific to the ad9777 or ad9142a ic.
 */
class kintex7sdr_codec_ctrl_impl : public usrp2_codec_ctrl {
public:
    kintex7sdr_codec_ctrl_impl(kintex7sdr_iface::sptr iface, uhd::spi_iface::sptr spiface) {
        _iface = iface;
        _spiface = spiface;

        switch (static_cast<kintex7sdr_iface::rev_type>(_iface->get_rev())) {
            case kintex7sdr_iface::USRP_N210_XK:
                setup_the_ad9142a_dac();
                break;
            case kintex7sdr_iface::USRP_NXXX:
                break;
            default:
                setup_the_ad9777_dac();
        }
    }

    ~kintex7sdr_codec_ctrl_impl(void) override { UHD_SAFE_CALL(
        // power-down dac
        _ad9777_regs.power_down_mode = 1;
        this->send_ad9777_reg(0);

        // power-down adc
        switch (static_cast<kintex7sdr_iface::rev_type>(_iface->get_rev())) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                //TODO: выключение устройства в деструкторе
            case kintex7sdr_iface::USRP_NXXX:
                break;
        }
    )}

    void setup_the_ad9142a_dac() {
        _ad9142a_regs.DEVICE_RESET = 1;
        this->send_ad9142a_reg(0x00);

        _ad9142a_regs.INTERRUPT_CONFIGURATION = 1;
        this->send_ad9142a_reg(0x20);

        _ad9142a_regs.DIGLOGIC_DIVIDER = 0;
        _ad9142a_regs.VCO_DIVIDER = 2;
        _ad9142a_regs.LOOP_DIVIDER = 1;
        this->send_ad9142a_reg(0x14);
        this->send_ad9142a_reg(0x15);

        _ad9142a_regs.PLL_ENABLE = 1;
        _ad9142a_regs.AUTO_MANUAL_SEL = 1;
        this->send_ad9142a_reg(0x12);

        _ad9142a_regs.AUTO_MANUAL_SEL = 0;
        this->send_ad9142a_reg(0x12);
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));

        _ad9142a_regs.DELAY_CELL0_ENABLE = 0xFE;
        _ad9142a_regs.DELAY_CELL1_ENABLE = 0x67;
        this->send_ad9142a_reg(0x5E);
        this->send_ad9142a_reg(0x5F);

        _ad9142a_regs.LOW_DCI_EN = 1;
        _ad9142a_regs.DC_COUPLE_LOW_EN = 1;
        _ad9142a_regs.DUTY_CORRECTION_ENABLE = 0;
        this->send_ad9142a_reg(0x0D);
        this->send_ad9142a_reg(0x0A);

        _ad9142a_regs.IQ_GAIN_ADJ_DCOFFSET_ENABLE = 1;
        this->send_ad9142a_reg(0x27);

        _ad9142a_regs.INTERPOLATION_MODE = 2;
        this->send_ad9142a_reg(0x28);

        _ad9142a_regs.FIFO_SPI_RESET_REQUEST = 1;
        this->send_ad9142a_reg(0x25);
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        _ad9142a_regs.FIFO_SPI_RESET_REQUEST = 0;
        this->send_ad9142a_reg(0x25);

        _ad9142a_regs.INVSINC_ENABLE = 1;
        this->send_ad9142a_reg(0x27);

        //_ad9142a_regs.IDAC_FULLSCALE_ADJUST_LSB = 0;
        // this->send_ad9142a_reg(0x18);
        //_ad9142a_regs.QDAC_FULLSCALE_ADJUST_LSB = 0;
        // this->send_ad9142a_reg(0x1A);

        //_ad9142a_regs.IDAC_FULLSCALE_ADJUST_MSB = 0;
        // this->send_ad9142a_reg(0x19);
        //_ad9142a_regs.QDAC_FULLSCALE_ADJUST_MSB = 0;
        // this->send_ad9142a_reg(0x1B);

        _ad9142a_regs.PD_IDAC = 0;
        _ad9142a_regs.PD_QDAC = 0;
        this->send_ad9142a_reg(0x01);

        /*    	this->read_ad9142a_reg(0x17);

        this->read_ad9142a_reg(0x06);
        this->read_ad9142a_reg(0x05);

        this->read_ad9142a_reg(0x42);
        this->read_ad9142a_reg(0x43);

        this->read_ad9142a_reg(0x0E);
        this->read_ad9142a_reg(0x01);*/
    }

    void setup_the_ad9777_dac() {
        // setup the ad9777 dac
        _ad9777_regs.x_1r_2r_mode = ad9777_regs_t::X_1R_2R_MODE_1R;
        _ad9777_regs.filter_interp_rate = ad9777_regs_t::FILTER_INTERP_RATE_4X;
        _ad9777_regs.mix_mode = ad9777_regs_t::MIX_MODE_COMPLEX;
        _ad9777_regs.pll_divide_ratio = ad9777_regs_t::PLL_DIVIDE_RATIO_DIV1;
        _ad9777_regs.pll_state = ad9777_regs_t::PLL_STATE_ON;
        _ad9777_regs.auto_cp_control = ad9777_regs_t::AUTO_CP_CONTROL_AUTO;
        // I dac values
        _ad9777_regs.idac_fine_gain_adjust = 0;
        _ad9777_regs.idac_coarse_gain_adjust = 0xf;
        _ad9777_regs.idac_offset_adjust_lsb = 0;
        _ad9777_regs.idac_offset_adjust_msb = 0;
        // Q dac values
        _ad9777_regs.qdac_fine_gain_adjust = 0;
        _ad9777_regs.qdac_coarse_gain_adjust = 0xf;
        _ad9777_regs.qdac_offset_adjust_lsb = 0;
        _ad9777_regs.qdac_offset_adjust_msb = 0;
        // write all regs
        for (uint8_t addr = 0; addr <= 0xC; addr++) {
            this->send_ad9777_reg(addr);
        }
        set_tx_mod_mode(0);
    }

    void set_tx_mod_mode(int mod_mode) override {
        // set the sign of the frequency shift
        _ad9777_regs.modulation_form = (mod_mode > 0) ? ad9777_regs_t::MODULATION_FORM_E_PLUS_JWT
                                                      : ad9777_regs_t::MODULATION_FORM_E_MINUS_JWT;

        // set the frequency shift
        switch (std::abs(mod_mode)) {
            case 0:
            case 1:
                _ad9777_regs.modulation_mode = ad9777_regs_t::MODULATION_MODE_NONE;
                break;
            case 2:
                _ad9777_regs.modulation_mode = ad9777_regs_t::MODULATION_MODE_FS_2;
                break;
            case 4:
                _ad9777_regs.modulation_mode = ad9777_regs_t::MODULATION_MODE_FS_4;
                break;
            case 8:
                _ad9777_regs.modulation_mode = ad9777_regs_t::MODULATION_MODE_FS_8;
                break;
            default:
                throw uhd::value_error("unknown modulation mode for ad9777");
        }

        this->send_ad9777_reg(0x01); // set the register
    }

    void set_rx_digital_gain(double gain) override { // fine digital gain
        switch (static_cast<kintex7sdr_iface::rev_type>(_iface->get_rev())) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                throw uhd::not_implemented_error("set_rx_digital_gain: unknown hardware version");
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_rx_digital_fine_gain(double gain) override { // gain correction
        switch (static_cast<kintex7sdr_iface::rev_type>(_iface->get_rev())) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                throw uhd::not_implemented_error("set_rx_digital_fine_gain: unknown hardware version");
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_rx_analog_gain(bool /*gain*/) override { // turns on/off analog 3.5dB preamp
        switch (static_cast<kintex7sdr_iface::rev_type>(_iface->get_rev())) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                throw uhd::not_implemented_error("set_rx_analog_gain: unknown hardware version");
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    size_t get_tx_interpolation() const override {
        return 4;
    }

private:
    ad9777_regs_t _ad9777_regs;
    ad9142a_regs_t _ad9142a_regs;
    ads62p44_regs_t _ads62p44_regs;
    usrp2_iface::sptr _iface;
    uhd::spi_iface::sptr _spiface;

    void send_ad9777_reg(uint8_t addr) {
        uint16_t reg = _ad9777_regs.get_write_reg(addr);
        UHD_LOG_TRACE("USRP2", "send_ad9777_reg: 0x" << std::hex << reg);
        _spiface->write_spi(SPI_SS_AD9777, spi_config_t::EDGE_RISE, reg, 16);
    }

    void send_ads62p44_reg(uint8_t addr) {
        uint16_t reg = _ads62p44_regs.get_write_reg(addr);
        _spiface->write_spi(SPI_SS_ADS62P44, spi_config_t::EDGE_FALL, reg, 16);
    }

    void send_ad9142a_reg(uint16_t addr) {
        uint32_t reg = _ad9142a_regs.get_write_reg(addr);
        UHD_LOG_TRACE("USRP2", "send_ad9142a_reg: " << std::hex << reg << std::endl);
        _spiface->write_spi(SPI_SS_AD9142A, spi_config_t::EDGE_RISE, reg, 24);
        // UHD_MSG(status) << boost::format("AD9142A: %x") %reg << std::endl;
    }

    void write_ad9142a_reg(uint16_t addr, uint8_t data) {
        uint32_t reg = (boost::uint16_t(addr) << 8) | data;
        _spiface->write_spi(SPI_SS_AD9142A, spi_config_t::EDGE_RISE, reg, 24);
        UHD_LOG_TRACE("USRP2", "send AD9142A reg: 0x" : std::hex << reg << std::endl);
    }

    uint32_t read_ad9142a_reg(uint16_t addr) {
        uint32_t reg = _ad9142a_regs.get_read_reg(addr);
        uint32_t readback =
                _spiface->read_spi(SPI_SS_AD9142A, spi_config_t::EDGE_RISE, reg, 24);
        readback &= 0x7fffff;
        UHD_LOG_TRACE("USRP2", "read AD9142A reg: 0x"
                : std::hex << readback << std::endl);
        return readback;
    }

};

/***********************************************************************
 * Public make function for the usrp2 codec control
 **********************************************************************/
kintex7sdr_codec_ctrl::sptr kintex7sdr_codec_ctrl::make(kintex7sdr_iface::sptr iface, uhd::spi_iface::sptr spiface) {
    return sptr(new kintex7sdr_codec_ctrl_impl(iface, spiface));
}
