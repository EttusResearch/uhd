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
#include <cstddef>

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

private:
    byte_vector_t transact_spi(
        spi_dev_t, spi_edge_t, const byte_vector_t &, bool
    );

    usrp2_impl *_impl;

    //shadows
    boost::uint32_t _ddr_shadow;
    uhd::dict<atr_reg_t, uint32_t> _atr_reg_shadows;

    //utilities
    static int bank_to_shift(gpio_bank_t bank){
        static const uhd::dict<gpio_bank_t, int> _bank_to_shift = \
        boost::assign::map_list_of(GPIO_BANK_RX, 0)(GPIO_BANK_TX, 16);
        return _bank_to_shift[bank];
    }
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
void usrp2_dboard_interface::set_gpio_ddr(gpio_bank_t bank, boost::uint16_t value){
    //calculate the new 32 bit ddr value
    int shift = bank_to_shift(bank);
    boost::uint32_t new_ddr_val =
        (_ddr_shadow & ~(boost::uint32_t(0xffff) << shift)) //zero out new bits
        | (boost::uint32_t(value) << shift); //or'ed in the new bits

    //poke in the value and shadow
    _impl->poke(offsetof(gpio_regs_t, ddr) + 0xC800, new_ddr_val);
    _ddr_shadow = new_ddr_val;
}

boost::uint16_t usrp2_dboard_interface::read_gpio(gpio_bank_t bank){
    boost::uint32_t data = _impl->peek(offsetof(gpio_regs_t, io) + 0xC800);
    return boost::uint16_t(data >> bank_to_shift(bank));
}

void usrp2_dboard_interface::set_atr_reg(gpio_bank_t bank, atr_reg_t reg, boost::uint16_t value){
    //map the atr reg to an offset in register space
    static const uhd::dict<atr_reg_t, int> reg_to_offset = boost::assign::map_list_of
        (ATR_REG_IDLE, ATR_IDLE) (ATR_REG_TXONLY, ATR_TX)
        (ATR_REG_RXONLY, ATR_RX) (ATR_REG_BOTH, ATR_FULL)
    ;
    int offset = reg_to_offset[reg];

    //ensure a value exists in the shadow
    if (not _atr_reg_shadows.has_key(reg)) _atr_reg_shadows[reg] = 0;

    //calculate the new 32 bit atr value
    int shift = bank_to_shift(bank);
    boost::uint32_t new_atr_val =
        (_atr_reg_shadows[reg] & ~(boost::uint32_t(0xffff) << shift)) //zero out new bits
        | (boost::uint32_t(value) << shift); //or'ed in the new bits

    //poke in the value and shadow
    _impl->poke(offsetof(atr_regs_t, v) + 0xE400 + offset, new_atr_val);
    _atr_reg_shadows[reg] = new_atr_val;
}

/***********************************************************************
 * SPI
 **********************************************************************/
/*!
 * Static function to convert a spi dev enum
 * to an over-the-wire value for the usrp2 control.
 * \param dev the dboard interface spi dev enum
 * \return an over the wire representation
 */
static boost::uint8_t spi_dev_to_otw(dboard_interface::spi_dev_t dev){
    switch(dev){
    case uhd::usrp::dboard_interface::SPI_DEV_TX: return USRP2_DIR_TX;
    case uhd::usrp::dboard_interface::SPI_DEV_RX: return USRP2_DIR_RX;
    }
    throw std::invalid_argument("unknown spi device type");
}

/*!
 * Static function to convert a spi edge enum
 * to an over-the-wire value for the usrp2 control.
 * \param edge the dboard interface spi edge enum
 * \return an over the wire representation
 */
static boost::uint8_t spi_edge_to_otw(dboard_interface::spi_edge_t edge){
    switch(edge){
    case uhd::usrp::dboard_interface::SPI_EDGE_RISE: return USRP2_CLK_EDGE_RISE;
    case uhd::usrp::dboard_interface::SPI_EDGE_FALL: return USRP2_CLK_EDGE_FALL;
    }
    throw std::invalid_argument("unknown spi edge type");
}

dboard_interface::byte_vector_t usrp2_dboard_interface::transact_spi(
    spi_dev_t dev,
    spi_edge_t edge,
    const byte_vector_t &buf,
    bool readback
){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_TRANSACT_ME_SOME_SPI_BRO);
    out_data.data.spi_args.dev = spi_dev_to_otw(dev);
    out_data.data.spi_args.edge = spi_edge_to_otw(edge);
    out_data.data.spi_args.readback = (readback)? 1 : 0;
    out_data.data.spi_args.bytes = buf.size();

    //limitation of spi transaction size
    ASSERT_THROW(buf.size() <= sizeof(out_data.data.spi_args.data));

    //copy in the data
    for (size_t i = 0; i < buf.size(); i++){
        out_data.data.spi_args.data[i] = buf[i];
    }

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_OMG_TRANSACTED_SPI_DUDE);
    ASSERT_THROW(in_data.data.spi_args.bytes == buf.size());

    //copy out the data
    byte_vector_t result;
    for (size_t i = 0; i < buf.size(); i++){
        result.push_back(in_data.data.spi_args.data[i]);
    }
    return result;
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
    for (size_t i = 0; i < buf.size(); i++){
        out_data.data.i2c_args.data[i] = buf[i];
    }

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
    byte_vector_t result;
    for (size_t i = 0; i < num_bytes; i++){
        result.push_back(in_data.data.i2c_args.data[i]);
    }
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
static boost::uint8_t spi_dev_to_otw(dboard_interface::unit_type_t unit){
    switch(unit){
    case uhd::usrp::dboard_interface::UNIT_TYPE_TX: return USRP2_DIR_TX;
    case uhd::usrp::dboard_interface::UNIT_TYPE_RX: return USRP2_DIR_RX;
    }
    throw std::invalid_argument("unknown unit type type");
}

void usrp2_dboard_interface::write_aux_dac(dboard_interface::unit_type_t unit, int which, int value){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_WRITE_THIS_TO_THE_AUX_DAC_BRO);
    out_data.data.aux_args.dir = spi_dev_to_otw(unit);
    out_data.data.aux_args.which = which;
    out_data.data.aux_args.value = htonl(value);

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_DONE_WITH_THAT_AUX_DAC_DUDE);
}

int usrp2_dboard_interface::read_aux_adc(dboard_interface::unit_type_t unit, int which){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_READ_FROM_THIS_AUX_ADC_BRO);
    out_data.data.aux_args.dir = spi_dev_to_otw(unit);
    out_data.data.aux_args.which = which;

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_DONE_WITH_THAT_AUX_ADC_DUDE);
    return ntohl(in_data.data.aux_args.value);
}
