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

#include "usrp1_iface.hpp"
#include "usrp1_impl.hpp"
#include "fpga_regs_common.h"
#include "usrp_spi_defs.h"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

class usrp1_dboard_iface : public dboard_iface {
public:

    usrp1_dboard_iface(usrp1_iface::sptr iface,
                       usrp1_clock_ctrl::sptr clock,
                       usrp1_codec_ctrl::sptr codec,
                       usrp1_impl::dboard_slot_t dboard_slot
    ){
        _iface = iface;
        _clock = clock;
        _codec = codec;
        _dboard_slot = dboard_slot;

        //init the clock rate shadows
        this->set_clock_rate(UNIT_RX, _clock->get_master_clock_freq());
        this->set_clock_rate(UNIT_TX, _clock->get_master_clock_freq());
    }

    ~usrp1_dboard_iface()
    {
        /* NOP */
    }

    special_props_t get_special_props()
    {
        special_props_t props;
        props.soft_clock_divider = true;
        props.mangle_i2c_addrs = (_dboard_slot == usrp1_impl::DBOARD_SLOT_B);
        return props;
    }

    void write_aux_dac(unit_t, aux_dac_t, float);
    float read_aux_adc(unit_t, aux_adc_t);

    void set_pin_ctrl(unit_t, boost::uint16_t);
    void set_atr_reg(unit_t, atr_reg_t, boost::uint16_t);
    void set_gpio_ddr(unit_t, boost::uint16_t);
    void write_gpio(unit_t, boost::uint16_t);
    boost::uint16_t read_gpio(unit_t);

    void write_i2c(boost::uint8_t, const byte_vector_t &);
    byte_vector_t read_i2c(boost::uint8_t, size_t);

    void write_spi(unit_t unit,
                   const spi_config_t &config,
                   boost::uint32_t data,
                   size_t num_bits);

    boost::uint32_t read_write_spi(unit_t unit,
                                   const spi_config_t &config,
                                   boost::uint32_t data,
                                   size_t num_bits);

    void set_clock_rate(unit_t, double);
    std::vector<double> get_clock_rates(unit_t);
    double get_clock_rate(unit_t);
    void set_clock_enabled(unit_t, bool);

private:
    usrp1_iface::sptr _iface;
    usrp1_clock_ctrl::sptr _clock;
    usrp1_codec_ctrl::sptr _codec;
    uhd::dict<unit_t, double> _clock_rates;
    usrp1_impl::dboard_slot_t _dboard_slot;
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_iface::sptr usrp1_impl::make_dboard_iface(usrp1_iface::sptr iface,
                                           usrp1_clock_ctrl::sptr clock,
                                           usrp1_codec_ctrl::sptr codec,
                                           dboard_slot_t dboard_slot
){
    return dboard_iface::sptr(new usrp1_dboard_iface(iface, clock, codec, dboard_slot));
}

/***********************************************************************
 * Clock Rates
 **********************************************************************/
void usrp1_dboard_iface::set_clock_rate(unit_t unit, double rate)
{
    _clock_rates[unit] = rate;
    switch(unit) {
    case UNIT_RX: return _clock->set_rx_dboard_clock_rate(rate);    
    case UNIT_TX: return _clock->set_tx_dboard_clock_rate(rate);    
    }
}

/*
 * TODO: if this is a dbsrx return the rate of 4MHZ and set FPGA magic
 */
std::vector<double> usrp1_dboard_iface::get_clock_rates(unit_t unit)
{
    switch(unit) {
    case UNIT_RX: return _clock->get_rx_dboard_clock_rates();
    case UNIT_TX: return _clock->get_tx_dboard_clock_rates();
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

double usrp1_dboard_iface::get_clock_rate(unit_t unit)
{
    return _clock_rates[unit];
}

void usrp1_dboard_iface::set_clock_enabled(unit_t unit, bool enb)
{
    switch(unit) {
    case UNIT_RX: return _clock->enable_rx_dboard_clock(enb);
    case UNIT_TX: return _clock->enable_tx_dboard_clock(enb);
    }
}

/***********************************************************************
 * GPIO
 **********************************************************************/
void usrp1_dboard_iface::set_pin_ctrl(unit_t unit, boost::uint16_t value)
{
    switch(unit) {
    case UNIT_RX:
        if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
             _iface->poke32(FR_ATR_MASK_1, value);
        else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
             _iface->poke32(FR_ATR_MASK_3, value);
        break;
    case UNIT_TX:
        if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
            _iface->poke32(FR_ATR_MASK_0, value);
        else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
            _iface->poke32(FR_ATR_MASK_2, value);
        break;
    }
}

void usrp1_dboard_iface::set_gpio_ddr(unit_t unit, boost::uint16_t value)
{
    switch(unit) {
    case UNIT_RX:
        if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
            _iface->poke32(FR_OE_1, 0xffff0000 | value);
        else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
            _iface->poke32(FR_OE_3, 0xffff0000 | value);
        break;
    case UNIT_TX:
        if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
            _iface->poke32(FR_OE_0, 0xffff0000 | value);
        else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
            _iface->poke32(FR_OE_2, 0xffff0000 | value);
        break;
    }
}

void usrp1_dboard_iface::write_gpio(unit_t unit, boost::uint16_t value)
{
    switch(unit) {
    case UNIT_RX:
        if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
            _iface->poke32(FR_IO_1, 0xffff0000 | value);
        else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
            _iface->poke32(FR_IO_3, 0xffff0000 | value);
        break;
    case UNIT_TX:
        if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
            _iface->poke32(FR_IO_0, 0xffff0000 | value);
        else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
            _iface->poke32(FR_IO_2, 0xffff0000 | value);
        break;
    }
}

boost::uint16_t usrp1_dboard_iface::read_gpio(unit_t unit)
{
    boost::uint32_t out_value;

    if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
        out_value = _iface->peek32(1);
    else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
        out_value = _iface->peek32(2);
    else
        UHD_THROW_INVALID_CODE_PATH();

    switch(unit) {
    case UNIT_RX:
        return (boost::uint16_t)((out_value >> 16) & 0x0000ffff);
    case UNIT_TX:
        return (boost::uint16_t)((out_value >>  0) & 0x0000ffff);
    }
    UHD_ASSERT_THROW(false);
}

void usrp1_dboard_iface::set_atr_reg(unit_t unit,
                                     atr_reg_t atr, boost::uint16_t value)
{
    if ((atr == ATR_REG_IDLE) || (atr == ATR_REG_FULL_DUPLEX)) {
        //TODO probably just ignore these two atr settings because all dboards will try to set them
        std::cerr << "error: set_atr_reg(): unsupported state" << std::endl;
        return;
    }

    switch(unit) {
    case UNIT_RX:
        if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
            _iface->poke32(FR_ATR_RXVAL_1, value);
        else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
            _iface->poke32(FR_ATR_RXVAL_3, value);
        break; 
    case UNIT_TX:
        if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
            _iface->poke32(FR_ATR_TXVAL_0, value);
        else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
            _iface->poke32(FR_ATR_TXVAL_2, value);
        break;
    }
}
/***********************************************************************
 * SPI
 **********************************************************************/
/*!
 * Static function to convert a unit type to a spi slave device number.
 * \param unit the dboard interface unit type enum
 * \param slot the side (A or B) the dboard is attached
 * \return the slave device number
 */
static boost::uint32_t unit_to_otw_spi_dev(dboard_iface::unit_t unit,
                                           usrp1_impl::dboard_slot_t slot)
{
    switch(unit) {
    case dboard_iface::UNIT_TX:
        if (slot == usrp1_impl::DBOARD_SLOT_A)
            return SPI_ENABLE_TX_A; 
        else if (slot == usrp1_impl::DBOARD_SLOT_B)
            return SPI_ENABLE_TX_B;
        else
            break;
    case dboard_iface::UNIT_RX:
        if (slot == usrp1_impl::DBOARD_SLOT_A)
            return SPI_ENABLE_RX_A; 
        else if (slot == usrp1_impl::DBOARD_SLOT_B)
            return SPI_ENABLE_RX_B;
        else
            break;
    }
    throw std::invalid_argument("unknown unit type");
}

void usrp1_dboard_iface::write_spi(unit_t unit,
                                   const spi_config_t &config,
                                   boost::uint32_t data,
                                   size_t num_bits)
{
    _iface->transact_spi(unit_to_otw_spi_dev(unit, _dboard_slot),
                         config, data, num_bits, false);
}

boost::uint32_t usrp1_dboard_iface::read_write_spi(unit_t unit,
                                                   const spi_config_t &config,
                                                   boost::uint32_t data,
                                                   size_t num_bits)
{
    return _iface->transact_spi(unit_to_otw_spi_dev(unit, _dboard_slot),
                                config, data, num_bits, true);
}

/***********************************************************************
 * I2C
 **********************************************************************/
void usrp1_dboard_iface::write_i2c(boost::uint8_t addr,
                                   const byte_vector_t &bytes)
{
    return _iface->write_i2c(addr, bytes);
}

byte_vector_t usrp1_dboard_iface::read_i2c(boost::uint8_t addr,
                                           size_t num_bytes)
{
    return _iface->read_i2c(addr, num_bytes);
}

/***********************************************************************
 * Aux DAX/ADC
 **********************************************************************/
void usrp1_dboard_iface::write_aux_dac(dboard_iface::unit_t,
                                       aux_dac_t which, float value)
{
    //same aux dacs for each unit
    static const uhd::dict<aux_dac_t, usrp1_codec_ctrl::aux_dac_t>
        which_to_aux_dac = map_list_of
                                     (AUX_DAC_A, usrp1_codec_ctrl::AUX_DAC_A)
                                     (AUX_DAC_B, usrp1_codec_ctrl::AUX_DAC_B)
                                     (AUX_DAC_C, usrp1_codec_ctrl::AUX_DAC_C)
                                     (AUX_DAC_D, usrp1_codec_ctrl::AUX_DAC_D);

    _codec->write_aux_dac(which_to_aux_dac[which], value);
}

float usrp1_dboard_iface::read_aux_adc(dboard_iface::unit_t unit,
                                       aux_adc_t which)
{
    static const
    uhd::dict<unit_t, uhd::dict<aux_adc_t, usrp1_codec_ctrl::aux_adc_t> >
        unit_to_which_to_aux_adc = map_list_of(UNIT_RX, map_list_of
                                    (AUX_ADC_A, usrp1_codec_ctrl::AUX_ADC_A1)
                                    (AUX_ADC_B, usrp1_codec_ctrl::AUX_ADC_B1))
                                              (UNIT_TX, map_list_of
                                    (AUX_ADC_A, usrp1_codec_ctrl::AUX_ADC_A2)
                                    (AUX_ADC_B, usrp1_codec_ctrl::AUX_ADC_B2));

    return _codec->read_aux_adc(unit_to_which_to_aux_adc[unit][which]);
}
