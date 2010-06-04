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

#include "usrp_e_iface.hpp"
#include "usrp_e_regs.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <linux/usrp_e.h> //i2c and spi constants

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

class usrp_e_dboard_iface : public dboard_iface{
public:

    usrp_e_dboard_iface(
        usrp_e_iface::sptr iface,
        usrp_e_clock_ctrl::sptr clock,
        usrp_e_codec_ctrl::sptr codec
    ){
        _iface = iface;
        _clock = clock;
        _codec = codec;
    }

    ~usrp_e_dboard_iface(void){
        /* NOP */
    }

    void write_aux_dac(unit_t, int, float);
    float read_aux_adc(unit_t, int);

    void set_pin_ctrl(unit_t, boost::uint16_t);
    void set_atr_reg(unit_t, atr_reg_t, boost::uint16_t);
    void set_gpio_ddr(unit_t, boost::uint16_t);
    void write_gpio(unit_t, boost::uint16_t);
    boost::uint16_t read_gpio(unit_t);

    void write_i2c(boost::uint8_t, const byte_vector_t &);
    byte_vector_t read_i2c(boost::uint8_t, size_t);

    void write_spi(
        unit_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    );

    boost::uint32_t read_write_spi(
        unit_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    );

    double get_clock_rate(unit_t);
    void set_clock_enabled(unit_t, bool);

private:
    usrp_e_iface::sptr _iface;
    usrp_e_clock_ctrl::sptr _clock;
    usrp_e_codec_ctrl::sptr _codec;
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_iface::sptr make_usrp_e_dboard_iface(
    usrp_e_iface::sptr iface,
    usrp_e_clock_ctrl::sptr clock,
    usrp_e_codec_ctrl::sptr codec
){
    return dboard_iface::sptr(new usrp_e_dboard_iface(iface, clock, codec));
}

/***********************************************************************
 * Clock Rates
 **********************************************************************/
double usrp_e_dboard_iface::get_clock_rate(unit_t unit){
    switch(unit){
    case UNIT_RX: return _clock->get_rx_dboard_clock_rate();
    case UNIT_TX: return _clock->get_tx_dboard_clock_rate();
    }
    UHD_ASSERT_THROW(false);
}

void usrp_e_dboard_iface::set_clock_enabled(unit_t unit, bool enb){
    switch(unit){
    case UNIT_RX: return _clock->enable_rx_dboard_clock(enb);
    case UNIT_TX: return _clock->enable_tx_dboard_clock(enb);
    }
}

/***********************************************************************
 * GPIO
 **********************************************************************/
void usrp_e_dboard_iface::set_pin_ctrl(unit_t unit, boost::uint16_t value){
    UHD_ASSERT_THROW(GPIO_SEL_ATR == 1); //make this assumption
    switch(unit){
    case UNIT_RX: _iface->poke16(UE_REG_GPIO_RX_SEL, value); return;
    case UNIT_TX: _iface->poke16(UE_REG_GPIO_TX_SEL, value); return;
    }
}

void usrp_e_dboard_iface::set_gpio_ddr(unit_t unit, boost::uint16_t value){
    switch(unit){
    case UNIT_RX: _iface->poke16(UE_REG_GPIO_RX_DDR, value); return;
    case UNIT_TX: _iface->poke16(UE_REG_GPIO_TX_DDR, value); return;
    }
}

void usrp_e_dboard_iface::write_gpio(unit_t unit, boost::uint16_t value){
    switch(unit){
    case UNIT_RX: _iface->poke16(UE_REG_GPIO_RX_IO, value); return;
    case UNIT_TX: _iface->poke16(UE_REG_GPIO_TX_IO, value); return;
    }
}

boost::uint16_t usrp_e_dboard_iface::read_gpio(unit_t unit){
    switch(unit){
    case UNIT_RX: return _iface->peek16(UE_REG_GPIO_RX_IO);
    case UNIT_TX: return _iface->peek16(UE_REG_GPIO_TX_IO);
    }
    UHD_ASSERT_THROW(false);
}

void usrp_e_dboard_iface::set_atr_reg(unit_t unit, atr_reg_t atr, boost::uint16_t value){
    //define mapping of unit to atr regs to register address
    static const uhd::dict<
        unit_t, uhd::dict<atr_reg_t, boost::uint32_t>
    > unit_to_atr_to_addr = map_list_of
        (UNIT_RX, map_list_of
            (ATR_REG_IDLE,        UE_REG_ATR_IDLE_RXSIDE)
            (ATR_REG_TX_ONLY,     UE_REG_ATR_INTX_RXSIDE)
            (ATR_REG_RX_ONLY,     UE_REG_ATR_INRX_RXSIDE)
            (ATR_REG_FULL_DUPLEX, UE_REG_ATR_FULL_RXSIDE)
        )
        (UNIT_TX, map_list_of
            (ATR_REG_IDLE,        UE_REG_ATR_IDLE_TXSIDE)
            (ATR_REG_TX_ONLY,     UE_REG_ATR_INTX_TXSIDE)
            (ATR_REG_RX_ONLY,     UE_REG_ATR_INRX_TXSIDE)
            (ATR_REG_FULL_DUPLEX, UE_REG_ATR_FULL_TXSIDE)
        )
    ;
    _iface->poke16(unit_to_atr_to_addr[unit][atr], value);
}

/***********************************************************************
 * SPI
 **********************************************************************/
/*!
 * Static function to convert a unit type to a spi slave device number.
 * \param unit the dboard interface unit type enum
 * \return the slave device number
 */
static boost::uint32_t unit_to_otw_spi_dev(dboard_iface::unit_t unit){
    switch(unit){
    case dboard_iface::UNIT_TX: return UE_SPI_SS_TX_DB;
    case dboard_iface::UNIT_RX: return UE_SPI_SS_RX_DB;
    }
    throw std::invalid_argument("unknown unit type");
}

void usrp_e_dboard_iface::write_spi(
    unit_t unit,
    const spi_config_t &config,
    boost::uint32_t data,
    size_t num_bits
){
    _iface->transact_spi(unit_to_otw_spi_dev(unit), config, data, num_bits, false /*no rb*/);
}

boost::uint32_t usrp_e_dboard_iface::read_write_spi(
    unit_t unit,
    const spi_config_t &config,
    boost::uint32_t data,
    size_t num_bits
){
    return _iface->transact_spi(unit_to_otw_spi_dev(unit), config, data, num_bits, true /*rb*/);
}

/***********************************************************************
 * I2C
 **********************************************************************/
void usrp_e_dboard_iface::write_i2c(boost::uint8_t addr, const byte_vector_t &bytes){
    return _iface->write_i2c(addr, bytes);
}

byte_vector_t usrp_e_dboard_iface::read_i2c(boost::uint8_t addr, size_t num_bytes){
    return _iface->read_i2c(addr, num_bytes);
}

/***********************************************************************
 * Aux DAX/ADC
 **********************************************************************/
void usrp_e_dboard_iface::write_aux_dac(dboard_iface::unit_t, int which, float value){
    //same aux dacs for each unit
    static const uhd::dict<int, usrp_e_codec_ctrl::aux_dac_t> which_to_aux_dac = map_list_of
        (0, usrp_e_codec_ctrl::AUX_DAC_A) (1, usrp_e_codec_ctrl::AUX_DAC_B)
        (2, usrp_e_codec_ctrl::AUX_DAC_C) (3, usrp_e_codec_ctrl::AUX_DAC_D)
    ;
    _codec->write_aux_dac(which_to_aux_dac[which], value);
}

float usrp_e_dboard_iface::read_aux_adc(dboard_iface::unit_t unit, int which){
    static const uhd::dict<
        unit_t, uhd::dict<int, usrp_e_codec_ctrl::aux_adc_t>
    > unit_to_which_to_aux_adc = map_list_of
        (UNIT_RX, map_list_of
            (0, usrp_e_codec_ctrl::AUX_ADC_A1)
            (1, usrp_e_codec_ctrl::AUX_ADC_B1)
        )
        (UNIT_TX, map_list_of
            (0, usrp_e_codec_ctrl::AUX_ADC_A2)
            (1, usrp_e_codec_ctrl::AUX_ADC_B2)
        )
    ;
    return _codec->read_aux_adc(unit_to_which_to_aux_adc[unit][which]);
}
