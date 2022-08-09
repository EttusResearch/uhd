//
// Copyright 2010-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "codec_ctrl.hpp"
#include "ad9777_regs.hpp"

#include "ad9142a_regs.hpp"
#include <boost/thread/thread.hpp>

#include "ads62p44_regs.hpp"
#include "usrp2_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <stdint.h>

using namespace uhd;

usrp2_codec_ctrl::~usrp2_codec_ctrl(void) {
	/* NOP */
}

/*!
 * A usrp2 codec control specific to the ad9777 ic.
 */
class usrp2_codec_ctrl_impl: public usrp2_codec_ctrl {
public:
	usrp2_codec_ctrl_impl(usrp2_iface::sptr iface,
			uhd::spi_iface::sptr spiface) {
		_iface = iface;
		_spiface = spiface;

		switch (_iface->get_rev()) {
		case usrp2_iface::USRP_N210_XK:
			setup_the_ad9142a_dac();
			break;
		default:
			setup_the_ad9777_dac();
		}

		// power-up adc
		switch (_iface->get_rev()) {
		case usrp2_iface::USRP2_REV3:
		case usrp2_iface::USRP2_REV4:
			_iface->poke32(U2_REG_MISC_CTRL_ADC, U2_FLAG_MISC_CTRL_ADC_ON);
			break;

		case usrp2_iface::USRP_N200:
		case usrp2_iface::USRP_N210:
			_ads62p44_regs.reset = 1;
			this->send_ads62p44_reg(0x00); // issue a reset to the ADC
			// everything else should be pretty much default, i think
			//_ads62p44_regs.decimation = DECIMATION_DECIMATE_1;
			_ads62p44_regs.power_down = ads62p44_regs_t::POWER_DOWN_NORMAL;
			this->send_ads62p44_reg(0x14);
			this->set_rx_analog_gain(1);
			break;

		case usrp2_iface::USRP_N200_R4:
		case usrp2_iface::USRP_N210_R4:
			_ads62p44_regs.reset = 1;
			this->send_ads62p44_reg(0x00); // issue a reset to the ADC
			// everything else should be pretty much default, i think
			//_ads62p44_regs.decimation = DECIMATION_DECIMATE_1;
			_ads62p44_regs.override = 1;
			this->send_ads62p44_reg(0x14);
			_ads62p44_regs.power_down = ads62p44_regs_t::POWER_DOWN_NORMAL;
			_ads62p44_regs.output_interface =
					ads62p44_regs_t::OUTPUT_INTERFACE_LVDS;
			_ads62p44_regs.lvds_current = ads62p44_regs_t::LVDS_CURRENT_2_5MA;
			_ads62p44_regs.lvds_data_term = ads62p44_regs_t::LVDS_DATA_TERM_100;
			this->send_ads62p44_reg(0x11);
			this->send_ads62p44_reg(0x12);
			this->send_ads62p44_reg(0x14);
			this->set_rx_analog_gain(1);
			break;

		case usrp2_iface::USRP_NXXX:
			break;
		}
	}

	~usrp2_codec_ctrl_impl(void) override
	{
		UHD_SAFE_CALL(
		// power-down dac
				_ad9777_regs.power_down_mode = 1; this->send_ad9777_reg(0);

		// power-down adc
				switch (_iface->get_rev()) { case usrp2_iface::USRP2_REV3: case usrp2_iface::USRP2_REV4: _iface->poke32(U2_REG_MISC_CTRL_ADC, U2_FLAG_MISC_CTRL_ADC_OFF); break;

				case usrp2_iface::USRP_N200: case usrp2_iface::USRP_N210: case usrp2_iface::USRP_N200_R4: case usrp2_iface::USRP_N210_R4:
				// send a global power-down to the ADC here... it will get lifted on
				// reset
				_ads62p44_regs.power_down = ads62p44_regs_t::POWER_DOWN_GLOBAL_PD; this->send_ads62p44_reg(0x14); break;

				case usrp2_iface::USRP_NXXX: break; })
	}

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
		//this->send_ad9142a_reg(0x18);
		//_ad9142a_regs.QDAC_FULLSCALE_ADJUST_LSB = 0;
		//this->send_ad9142a_reg(0x1A);

		//_ad9142a_regs.IDAC_FULLSCALE_ADJUST_MSB = 0;
		//this->send_ad9142a_reg(0x19);
		//_ad9142a_regs.QDAC_FULLSCALE_ADJUST_MSB = 0;
		//this->send_ad9142a_reg(0x1B);

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
		for (uint8_t addr = 0; addr <= 0xC; addr++) {
			this->send_ad9777_reg(addr);
		}
		set_tx_mod_mode(0);
	}

	void set_tx_mod_mode(int mod_mode) override
	{
		// set the sign of the frequency shift
		_ad9777_regs.modulation_form =
				(mod_mode > 0) ?
						ad9777_regs_t::MODULATION_FORM_E_PLUS_JWT :
						ad9777_regs_t::MODULATION_FORM_E_MINUS_JWT;

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

	void set_rx_digital_gain(double gain) override
	{ // fine digital gain
		switch (_iface->get_rev()) {
		case usrp2_iface::USRP_N200:
		case usrp2_iface::USRP_N210:
		case usrp2_iface::USRP_N200_R4:
		case usrp2_iface::USRP_N210_R4:
			_ads62p44_regs.fine_gain = int(gain / 0.5);
			this->send_ads62p44_reg(0x17);
			break;

		default:
			UHD_THROW_INVALID_CODE_PATH();
		}
	}

	void set_rx_digital_fine_gain(double gain) override
	{ // gain correction
		switch (_iface->get_rev()) {
		case usrp2_iface::USRP_N200:
		case usrp2_iface::USRP_N210:
		case usrp2_iface::USRP_N200_R4:
		case usrp2_iface::USRP_N210_R4:
			_ads62p44_regs.gain_correction = int(gain / 0.05);
			this->send_ads62p44_reg(0x1A);
			break;

		default:
			UHD_THROW_INVALID_CODE_PATH();
		}
	}

	void set_rx_analog_gain(bool /*gain*/) override
	{ // turns on/off analog 3.5dB preamp
		switch (_iface->get_rev()) {
		case usrp2_iface::USRP_N200:
		case usrp2_iface::USRP_N210:
		case usrp2_iface::USRP_N200_R4:
		case usrp2_iface::USRP_N210_R4:
			_ads62p44_regs.coarse_gain = ads62p44_regs_t::COARSE_GAIN_3_5DB; // gain ? ads62p44_regs_t::COARSE_GAIN_3_5DB :
																			 // ads62p44_regs_t::COARSE_GAIN_0DB;
			this->send_ads62p44_reg(0x14);
			break;

		default:
			UHD_THROW_INVALID_CODE_PATH();
		}
	}

	size_t get_tx_interpolation() const override
	{
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

	void send_ad9142a_reg(uint16_t addr) {
		uint32_t reg = _ad9142a_regs.get_write_reg(addr);
		UHD_LOG_TRACE("USRP2",
				"send_ad9142a_reg: " << std::hex << reg << std::endl);
		_spiface->write_spi(SPI_SS_AD9142A, spi_config_t::EDGE_RISE, reg, 24);
		//UHD_MSG(status) << boost::format("AD9142A: %x") %reg << std::endl;
	}

	void write_ad9142a_reg(uint16_t addr, uint8_t data) {
		uint32_t reg = (boost::uint16_t(addr) << 8) | data;
		_spiface->write_spi(SPI_SS_AD9142A, spi_config_t::EDGE_RISE, reg, 24);
		UHD_LOG_TRACE("USRP2",
				"send AD9142A reg: 0x": std::hex << reg << std::endl);
	}

	uint32_t read_ad9142a_reg(uint16_t addr) {
		uint32_t reg = _ad9142a_regs.get_read_reg(addr);
		uint32_t readback = _spiface->read_spi(SPI_SS_AD9142A,
				spi_config_t::EDGE_RISE, reg, 24);
		readback &= 0x7fffff;
		UHD_LOG_TRACE("USRP2",
				"read AD9142A reg: 0x": std::hex << readback << std::endl);
		return readback;
	}

	void send_ads62p44_reg(uint8_t addr) {
		uint16_t reg = _ads62p44_regs.get_write_reg(addr);
		_spiface->write_spi(SPI_SS_ADS62P44, spi_config_t::EDGE_FALL, reg, 16);
	}
};

/***********************************************************************
 * Public make function for the usrp2 codec control
 **********************************************************************/
usrp2_codec_ctrl::sptr usrp2_codec_ctrl::make(usrp2_iface::sptr iface,
		uhd::spi_iface::sptr spiface) {
	return sptr(new usrp2_codec_ctrl_impl(iface, spiface));
}
