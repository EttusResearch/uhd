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

#include "usrp2_iface.hpp"
#include "clock_control.hpp"
#include "usrp2_regs.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/asio.hpp> //htonl and ntohl
#include <boost/math/special_functions/round.hpp>
#include <algorithm>

using namespace uhd::usrp;

class usrp2_dboard_iface : public dboard_iface{
public:
    usrp2_dboard_iface(usrp2_iface::sptr iface, clock_control::sptr clk_ctrl);
    ~usrp2_dboard_iface(void);

    void write_aux_dac(unit_t, int, float);
    float read_aux_adc(unit_t, int);

    void set_atr_reg(unit_t, atr_reg_t, boost::uint16_t);
    void set_gpio_ddr(unit_t, boost::uint16_t);
    boost::uint16_t read_gpio(unit_t);

    void write_i2c(int, const byte_vector_t &);
    byte_vector_t read_i2c(int, size_t);

    double get_clock_rate(unit_t);
    void set_clock_enabled(unit_t, bool);
    bool get_clock_enabled(unit_t);

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

private:
    usrp2_iface::sptr _iface;
    clock_control::sptr _clk_ctrl;
    boost::uint32_t _ddr_shadow;
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_iface::sptr make_usrp2_dboard_iface(
    usrp2_iface::sptr iface,
    clock_control::sptr clk_ctrl
){
    return dboard_iface::sptr(new usrp2_dboard_iface(iface, clk_ctrl));
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_dboard_iface::usrp2_dboard_iface(usrp2_iface::sptr iface, clock_control::sptr clk_ctrl){
    _iface = iface;
    _clk_ctrl = clk_ctrl;
    _ddr_shadow = 0;

    //set the selection mux to use atr
    boost::uint32_t new_sels = 0x0;
    for(size_t i = 0; i < 16; i++){
        new_sels |= FRF_GPIO_SEL_ATR << (i*2);
    }
    _iface->poke32(FR_GPIO_TX_SEL, new_sels);
    _iface->poke32(FR_GPIO_RX_SEL, new_sels);
}

usrp2_dboard_iface::~usrp2_dboard_iface(void){
    /* NOP */
}

/***********************************************************************
 * Clocks
 **********************************************************************/
double usrp2_dboard_iface::get_clock_rate(unit_t){
    return _iface->get_master_clock_freq();
}

void usrp2_dboard_iface::set_clock_enabled(unit_t unit, bool enb){
    switch(unit){
    case UNIT_RX:
        _clk_ctrl->enable_rx_dboard_clock(enb);
        return;
    case UNIT_TX:
        _clk_ctrl->enable_tx_dboard_clock(enb);
        return;
    }
}

/***********************************************************************
 * GPIO
 **********************************************************************/
static int unit_to_shift(dboard_iface::unit_t unit){
    switch(unit){
    case dboard_iface::UNIT_RX: return 0;
    case dboard_iface::UNIT_TX: return 16;
    }
    throw std::runtime_error("unknown unit type");
}

void usrp2_dboard_iface::set_gpio_ddr(unit_t unit, boost::uint16_t value){
    _ddr_shadow = \
        (_ddr_shadow & ~(0xffff << unit_to_shift(unit))) |
        (boost::uint32_t(value) << unit_to_shift(unit));
    _iface->poke32(FR_GPIO_DDR, _ddr_shadow);
}

boost::uint16_t usrp2_dboard_iface::read_gpio(unit_t unit){
    return boost::uint16_t(_iface->peek32(FR_GPIO_IO) >> unit_to_shift(unit));
}

void usrp2_dboard_iface::set_atr_reg(unit_t unit, atr_reg_t atr, boost::uint16_t value){
    //define mapping of unit to atr regs to register address
    static const uhd::dict<
        unit_t, uhd::dict<atr_reg_t, boost::uint32_t>
    > unit_to_atr_to_addr = boost::assign::map_list_of
        (UNIT_RX, boost::assign::map_list_of
            (ATR_REG_IDLE,        FR_ATR_IDLE_RXSIDE)
            (ATR_REG_TX_ONLY,     FR_ATR_INTX_RXSIDE)
            (ATR_REG_RX_ONLY,     FR_ATR_INRX_RXSIDE)
            (ATR_REG_FULL_DUPLEX, FR_ATR_FULL_RXSIDE)
        )
        (UNIT_TX, boost::assign::map_list_of
            (ATR_REG_IDLE,        FR_ATR_IDLE_TXSIDE)
            (ATR_REG_TX_ONLY,     FR_ATR_INTX_TXSIDE)
            (ATR_REG_RX_ONLY,     FR_ATR_INRX_TXSIDE)
            (ATR_REG_FULL_DUPLEX, FR_ATR_FULL_TXSIDE)
        )
    ;
    _iface->poke16(unit_to_atr_to_addr[unit][atr], value);
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
static boost::uint8_t unit_to_otw_spi_dev(dboard_iface::unit_t unit){
    switch(unit){
    case dboard_iface::UNIT_TX: return SPI_SS_TX_DB;
    case dboard_iface::UNIT_RX: return SPI_SS_RX_DB;
    }
    throw std::invalid_argument("unknown unit type");
}

void usrp2_dboard_iface::write_spi(
    unit_t unit,
    const spi_config_t &config,
    boost::uint32_t data,
    size_t num_bits
){
    _iface->transact_spi(unit_to_otw_spi_dev(unit), config, data, num_bits, false /*no rb*/);
}

boost::uint32_t usrp2_dboard_iface::read_write_spi(
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
void usrp2_dboard_iface::write_i2c(int i2c_addr, const byte_vector_t &buf){
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
    usrp2_ctrl_data_t in_data = _iface->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE);
}

dboard_iface::byte_vector_t usrp2_dboard_iface::read_i2c(int i2c_addr, size_t num_bytes){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO);
    out_data.data.i2c_args.addr = i2c_addr;
    out_data.data.i2c_args.bytes = num_bytes;

    //limitation of i2c transaction size
    ASSERT_THROW(num_bytes <= sizeof(out_data.data.i2c_args.data));

    //send and recv
    usrp2_ctrl_data_t in_data = _iface->ctrl_send_and_recv(out_data);
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
static boost::uint8_t unit_to_otw(dboard_iface::unit_t unit){
    switch(unit){
    case dboard_iface::UNIT_TX: return USRP2_DIR_TX;
    case dboard_iface::UNIT_RX: return USRP2_DIR_RX;
    }
    throw std::invalid_argument("unknown unit type");
}

void usrp2_dboard_iface::write_aux_dac(unit_t unit, int which, float value){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_WRITE_THIS_TO_THE_AUX_DAC_BRO);
    out_data.data.aux_args.dir = unit_to_otw(unit);
    out_data.data.aux_args.which = which;
    out_data.data.aux_args.value = htonl(boost::math::iround(4095*value/3.3));

    //send and recv
    usrp2_ctrl_data_t in_data = _iface->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_DONE_WITH_THAT_AUX_DAC_DUDE);
}

float usrp2_dboard_iface::read_aux_adc(unit_t unit, int which){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_READ_FROM_THIS_AUX_ADC_BRO);
    out_data.data.aux_args.dir = unit_to_otw(unit);
    out_data.data.aux_args.which = which;

    //send and recv
    usrp2_ctrl_data_t in_data = _iface->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_DONE_WITH_THAT_AUX_ADC_DUDE);
    return float(3.3*ntohl(in_data.data.aux_args.value)/4095);
}
