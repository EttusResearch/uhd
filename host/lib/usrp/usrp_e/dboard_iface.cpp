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
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <linux/usrp_e.h> //i2c and spi constants

using namespace uhd;
using namespace uhd::usrp;

class usrp_e_dboard_iface : public dboard_iface{
public:
    usrp_e_dboard_iface(usrp_e_iface::sptr iface);
    ~usrp_e_dboard_iface(void);

    void write_aux_dac(unit_t, int, float);
    float read_aux_adc(unit_t, int);

    void set_atr_reg(unit_t, atr_reg_t, boost::uint16_t);
    void set_gpio_ddr(unit_t, boost::uint16_t);
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
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_iface::sptr make_usrp_e_dboard_iface(usrp_e_iface::sptr iface){
    return dboard_iface::sptr(new usrp_e_dboard_iface(iface));
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp_e_dboard_iface::usrp_e_dboard_iface(usrp_e_iface::sptr iface){
    _iface = iface;
}

usrp_e_dboard_iface::~usrp_e_dboard_iface(void){
    /* NOP */
}

/***********************************************************************
 * Clock Rates
 **********************************************************************/
double usrp_e_dboard_iface::get_clock_rate(unit_t){
    throw std::runtime_error("not implemented");
}

void usrp_e_dboard_iface::set_clock_enabled(unit_t, bool){
    throw std::runtime_error("not implemented");
}

/***********************************************************************
 * GPIO
 **********************************************************************/
void usrp_e_dboard_iface::set_gpio_ddr(unit_t bank, boost::uint16_t value){
    //define mapping of gpio bank to register address
    static const uhd::dict<unit_t, boost::uint32_t> bank_to_addr = boost::assign::map_list_of
        (UNIT_RX, UE_REG_GPIO_RX_DDR)
        (UNIT_TX, UE_REG_GPIO_TX_DDR)
    ;
    _iface->poke16(bank_to_addr[bank], value);
}

boost::uint16_t usrp_e_dboard_iface::read_gpio(unit_t bank){
    //define mapping of gpio bank to register address
    static const uhd::dict<unit_t, boost::uint32_t> bank_to_addr = boost::assign::map_list_of
        (UNIT_RX, UE_REG_GPIO_RX_IO)
        (UNIT_TX, UE_REG_GPIO_TX_IO)
    ;
    return _iface->peek16(bank_to_addr[bank]);
}

void usrp_e_dboard_iface::set_atr_reg(unit_t bank, atr_reg_t atr, boost::uint16_t value){
    //define mapping of bank to atr regs to register address
    static const uhd::dict<
        unit_t, uhd::dict<atr_reg_t, boost::uint32_t>
    > bank_to_atr_to_addr = boost::assign::map_list_of
        (UNIT_RX, boost::assign::map_list_of
            (ATR_REG_IDLE,        UE_REG_ATR_IDLE_RXSIDE)
            (ATR_REG_TX_ONLY,     UE_REG_ATR_INTX_RXSIDE)
            (ATR_REG_RX_ONLY,     UE_REG_ATR_INRX_RXSIDE)
            (ATR_REG_FULL_DUPLEX, UE_REG_ATR_FULL_RXSIDE)
        )
        (UNIT_TX, boost::assign::map_list_of
            (ATR_REG_IDLE,        UE_REG_ATR_IDLE_TXSIDE)
            (ATR_REG_TX_ONLY,     UE_REG_ATR_INTX_TXSIDE)
            (ATR_REG_RX_ONLY,     UE_REG_ATR_INRX_TXSIDE)
            (ATR_REG_FULL_DUPLEX, UE_REG_ATR_FULL_TXSIDE)
        )
    ;
    _iface->poke16(bank_to_atr_to_addr[bank][atr], value);
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
void usrp_e_dboard_iface::write_aux_dac(dboard_iface::unit_t unit, int which, float value){
    throw std::runtime_error("not implemented");
}

float usrp_e_dboard_iface::read_aux_adc(dboard_iface::unit_t unit, int which){
    throw std::runtime_error("not implemented");
}
