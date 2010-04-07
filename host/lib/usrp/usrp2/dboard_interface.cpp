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
#include "usrp2_regs.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <algorithm>

using namespace uhd::usrp;

class usrp2_dboard_interface : public dboard_interface{
public:
    usrp2_dboard_interface(usrp2_impl *impl);
    ~usrp2_dboard_interface(void);

    void write_aux_dac(unit_type_t, int, int);
    int read_aux_adc(unit_type_t, int);

    void set_atr_reg(gpio_bank_t, atr_reg_t, boost::uint16_t);
    void set_gpio_ddr(gpio_bank_t, boost::uint16_t);
    boost::uint16_t read_gpio(gpio_bank_t);

    void write_i2c(int, const byte_vector_t &);
    byte_vector_t read_i2c(int, size_t);

    double get_rx_clock_rate(void);
    double get_tx_clock_rate(void);

    void write_spi(
        unit_type_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    ){
        transact_spi(unit, config, data, num_bits, false /*no rb*/);
    }

    boost::uint32_t read_spi(
        unit_type_t unit,
        const spi_config_t &config,
        size_t num_bits
    ){
        return transact_spi(unit, config, 0, num_bits, true /*rb*/);
    }

    boost::uint32_t read_write_spi(
        unit_type_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    ){
        return transact_spi(unit, config, data, num_bits, true /*rb*/);
    }

private:
    boost::uint32_t transact_spi(
        unit_type_t, const spi_config_t &, boost::uint32_t, size_t, bool
    );

    usrp2_impl *_impl;
    boost::uint32_t _ddr_shadow;
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_interface::sptr make_usrp2_dboard_interface(usrp2_impl *impl){
    return dboard_interface::sptr(new usrp2_dboard_interface(impl));
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_dboard_interface::usrp2_dboard_interface(usrp2_impl *impl){
    _impl = impl;
    _ddr_shadow = 0;

    //set the selection mux to use atr
    boost::uint32_t new_sels = 0x0;
    for(size_t i = 0; i < 16; i++){
        new_sels |= FRF_GPIO_SEL_ATR << (i*2);
    }
    _impl->poke32(FR_GPIO_TX_SEL, new_sels);
    _impl->poke32(FR_GPIO_RX_SEL, new_sels);
}

usrp2_dboard_interface::~usrp2_dboard_interface(void){
    /* NOP */
}

/***********************************************************************
 * Clock Rates
 **********************************************************************/
double usrp2_dboard_interface::get_rx_clock_rate(void){
    return _impl->get_master_clock_freq();
}

double usrp2_dboard_interface::get_tx_clock_rate(void){
    return _impl->get_master_clock_freq();
}

/***********************************************************************
 * GPIO
 **********************************************************************/
static int bank_to_shift(dboard_interface::gpio_bank_t bank){
    switch(bank){
    case dboard_interface::GPIO_BANK_RX: return 0;
    case dboard_interface::GPIO_BANK_TX: return 16;
    }
    throw std::runtime_error("unknown gpio bank type");
}

void usrp2_dboard_interface::set_gpio_ddr(gpio_bank_t bank, boost::uint16_t value){
    _ddr_shadow = \
        (_ddr_shadow & ~(0xffff << bank_to_shift(bank))) |
        (boost::uint32_t(value) << bank_to_shift(bank));
    _impl->poke32(FR_GPIO_DDR, _ddr_shadow);
}

boost::uint16_t usrp2_dboard_interface::read_gpio(gpio_bank_t bank){
    return boost::uint16_t(_impl->peek32(FR_GPIO_IO) >> bank_to_shift(bank));
}

void usrp2_dboard_interface::set_atr_reg(gpio_bank_t bank, atr_reg_t atr, boost::uint16_t value){
    //define mapping of bank to atr regs to register address
    static const uhd::dict<
        gpio_bank_t, uhd::dict<atr_reg_t, boost::uint32_t>
    > bank_to_atr_to_addr = boost::assign::map_list_of
        (GPIO_BANK_RX, boost::assign::map_list_of
            (ATR_REG_IDLE,        FR_ATR_IDLE_RXSIDE)
            (ATR_REG_TX_ONLY,     FR_ATR_INTX_RXSIDE)
            (ATR_REG_RX_ONLY,     FR_ATR_INRX_RXSIDE)
            (ATR_REG_FULL_DUPLEX, FR_ATR_FULL_RXSIDE)
        )
        (GPIO_BANK_TX, boost::assign::map_list_of
            (ATR_REG_IDLE,        FR_ATR_IDLE_TXSIDE)
            (ATR_REG_TX_ONLY,     FR_ATR_INTX_TXSIDE)
            (ATR_REG_RX_ONLY,     FR_ATR_INRX_TXSIDE)
            (ATR_REG_FULL_DUPLEX, FR_ATR_FULL_TXSIDE)
        )
    ;
    _impl->poke16(bank_to_atr_to_addr[bank][atr], value);
}

/***********************************************************************
 * SPI
 **********************************************************************/
/*!
 * Static function to convert a unit type enum
 * to an over-the-wire value for the spi device.
 * \param unit the dboard interface unit type enum
 * \return an over the wire representation
 */
static boost::uint8_t unit_to_otw_dev(dboard_interface::unit_type_t unit){
    switch(unit){
    case dboard_interface::UNIT_TYPE_TX: return SPI_SS_TX_DB;
    case dboard_interface::UNIT_TYPE_RX: return SPI_SS_RX_DB;
    }
    throw std::invalid_argument("unknown unit type type");
}

/*!
 * Static function to convert a spi edge enum
 * to an over-the-wire value for the usrp2 control.
 * \param edge the dboard interface spi edge enum
 * \return an over the wire representation
 */
static boost::uint8_t spi_edge_to_otw(dboard_interface::spi_config_t::edge_t edge){
    switch(edge){
    case dboard_interface::spi_config_t::EDGE_RISE: return USRP2_CLK_EDGE_RISE;
    case dboard_interface::spi_config_t::EDGE_FALL: return USRP2_CLK_EDGE_FALL;
    }
    throw std::invalid_argument("unknown spi edge type");
}

boost::uint32_t usrp2_dboard_interface::transact_spi(
    unit_type_t unit,
    const spi_config_t &config,
    boost::uint32_t data,
    size_t num_bits,
    bool readback
){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_TRANSACT_ME_SOME_SPI_BRO);
    out_data.data.spi_args.dev = unit_to_otw_dev(unit);
    out_data.data.spi_args.miso_edge = spi_edge_to_otw(config.miso_edge);
    out_data.data.spi_args.mosi_edge = spi_edge_to_otw(config.mosi_edge);
    out_data.data.spi_args.readback = (readback)? 1 : 0;
    out_data.data.spi_args.num_bits = num_bits;
    out_data.data.spi_args.data = htonl(data);

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_OMG_TRANSACTED_SPI_DUDE);

    return ntohl(out_data.data.spi_args.data);
}

/***********************************************************************
 * I2C
 **********************************************************************/
void usrp2_dboard_interface::write_i2c(int i2c_addr, const byte_vector_t &buf){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_WRITE_THESE_I2C_VALUES_BRO);
    out_data.data.i2c_args.addr = i2c_addr;
    out_data.data.i2c_args.bytes = buf.size();

    //limitation of i2c transaction size
    ASSERT_THROW(buf.size() <= sizeof(out_data.data.i2c_args.data));

    //copy in the data
    std::copy(buf.begin(), buf.end(), out_data.data.i2c_args.data);

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE);
}

dboard_interface::byte_vector_t usrp2_dboard_interface::read_i2c(int i2c_addr, size_t num_bytes){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO);
    out_data.data.i2c_args.addr = i2c_addr;
    out_data.data.i2c_args.bytes = num_bytes;

    //limitation of i2c transaction size
    ASSERT_THROW(num_bytes <= sizeof(out_data.data.i2c_args.data));

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_HERES_THE_I2C_DATA_DUDE);
    ASSERT_THROW(in_data.data.i2c_args.addr = num_bytes);

    //copy out the data
    byte_vector_t result(num_bytes);
    std::copy(in_data.data.i2c_args.data, in_data.data.i2c_args.data + num_bytes, result.begin());
    return result;
}

/***********************************************************************
 * Aux DAX/ADC
 **********************************************************************/
/*!
 * Static function to convert a unit type enum
 * to an over-the-wire value for the usrp2 control.
 * \param unit the dboard interface unit type enum
 * \return an over the wire representation
 */
static boost::uint8_t unit_to_otw(dboard_interface::unit_type_t unit){
    switch(unit){
    case dboard_interface::UNIT_TYPE_TX: return USRP2_DIR_TX;
    case dboard_interface::UNIT_TYPE_RX: return USRP2_DIR_RX;
    }
    throw std::invalid_argument("unknown unit type type");
}

void usrp2_dboard_interface::write_aux_dac(unit_type_t unit, int which, int value){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_WRITE_THIS_TO_THE_AUX_DAC_BRO);
    out_data.data.aux_args.dir = unit_to_otw(unit);
    out_data.data.aux_args.which = which;
    out_data.data.aux_args.value = htonl(value);

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_DONE_WITH_THAT_AUX_DAC_DUDE);
}

int usrp2_dboard_interface::read_aux_adc(unit_type_t unit, int which){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_READ_FROM_THIS_AUX_ADC_BRO);
    out_data.data.aux_args.dir = unit_to_otw(unit);
    out_data.data.aux_args.which = which;

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_DONE_WITH_THAT_AUX_ADC_DUDE);
    return ntohl(in_data.data.aux_args.value);
}
