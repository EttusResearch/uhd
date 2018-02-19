//
// Copyright 2010-2012,2015,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "usrp1_iface.hpp"
#include "usrp1_impl.hpp"
#include "codec_ctrl.hpp"
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert_has.hpp>
#include <boost/assign/list_of.hpp>
#include <iostream>

#define FR_OE_0        5
#define FR_OE_1        6
#define FR_OE_2        7
#define FR_OE_3        8

#define FR_ATR_MASK_0  20
#define FR_ATR_TXVAL_0 21
#define FR_ATR_RXVAL_0 22

#define FR_ATR_MASK_1  23
#define FR_ATR_TXVAL_1 24
#define FR_ATR_RXVAL_1 25

#define FR_ATR_MASK_2  26
#define FR_ATR_TXVAL_2 27
#define FR_ATR_RXVAL_2 28

#define FR_ATR_MASK_3  29
#define FR_ATR_TXVAL_3 30
#define FR_ATR_RXVAL_3 31

#define FR_RX_A_REFCLK 41
#define FR_RX_B_REFCLK 43

// i/o registers for pins that go to daughterboards.
// top 16 is a mask, low 16 is value

#define FR_IO_0          9  // slot 0
#define FR_IO_1         10
#define FR_IO_2         11
#define FR_IO_3         12
#define SPI_ENABLE_TX_A     0x10    // select d'board TX A
#define SPI_ENABLE_RX_A     0x20    // select d'board RX A
#define SPI_ENABLE_TX_B     0x40    // select d'board TX B
#define SPI_ENABLE_RX_B     0x80    // select d'board RX B


using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::usrp::gpio_atr;
using namespace boost::assign;

static const dboard_id_t tvrx_id(0x0040);

class usrp1_dboard_iface : public dboard_iface {
public:

    usrp1_dboard_iface(usrp1_iface::sptr iface,
                       usrp1_codec_ctrl::sptr codec,
                       usrp1_impl::dboard_slot_t dboard_slot,
                       const double &master_clock_rate,
                       const dboard_id_t &rx_dboard_id
    ):
        _dboard_slot(dboard_slot),
        _master_clock_rate(master_clock_rate),
        _rx_dboard_id(rx_dboard_id)
    {
        _iface = iface;
        _codec = codec;

        _dbsrx_classic_div = 1;

        //yes this is evil but it's necessary for TVRX to work on USRP1
        if(_rx_dboard_id == tvrx_id) _codec->bypass_adc_buffers(false);
        //else _codec->bypass_adc_buffers(false); //don't think this is necessary
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

    void write_aux_dac(unit_t, aux_dac_t, double);
    double read_aux_adc(unit_t, aux_adc_t);

    void set_pin_ctrl(unit_t unit, uint32_t value, uint32_t mask = 0xffffffff);
    uint32_t get_pin_ctrl(unit_t unit);
    void set_atr_reg(unit_t unit, atr_reg_t reg, uint32_t value, uint32_t mask = 0xffffffff);
    uint32_t get_atr_reg(unit_t unit, atr_reg_t reg);
    void set_gpio_ddr(unit_t unit, uint32_t value, uint32_t mask = 0xffffffff);
    uint32_t get_gpio_ddr(unit_t unit);
    void set_gpio_out(unit_t unit, uint32_t value, uint32_t mask = 0xffffffff);
    uint32_t get_gpio_out(unit_t unit);
    uint32_t read_gpio(unit_t unit);

    void _set_pin_ctrl(unit_t, uint16_t);
    void _set_atr_reg(unit_t, atr_reg_t, uint16_t);
    void _set_gpio_ddr(unit_t, uint16_t);
    void _set_gpio_out(unit_t, uint16_t);

    void set_command_time(const uhd::time_spec_t& t);
    uhd::time_spec_t get_command_time(void);

    void write_i2c(uint16_t, const byte_vector_t &);
    byte_vector_t read_i2c(uint16_t, size_t);

    void write_spi(unit_t unit,
                   const spi_config_t &config,
                   uint32_t data,
                   size_t num_bits);

    uint32_t read_write_spi(unit_t unit,
                                   const spi_config_t &config,
                                   uint32_t data,
                                   size_t num_bits);

    void set_clock_rate(unit_t, double);
    std::vector<double> get_clock_rates(unit_t);
    double get_clock_rate(unit_t);
    void set_clock_enabled(unit_t, bool);
    double get_codec_rate(unit_t);
    void set_fe_connection(unit_t unit, const std::string&, const fe_connection_t& fe_conn);

private:
    usrp1_iface::sptr _iface;
    usrp1_codec_ctrl::sptr _codec;
    unsigned _dbsrx_classic_div;
    const usrp1_impl::dboard_slot_t _dboard_slot;
    const double &_master_clock_rate;
    const dboard_id_t _rx_dboard_id;
    uhd::dict<unit_t, uint16_t> _pin_ctrl, _gpio_out, _gpio_ddr;
    uhd::dict<unit_t, uhd::dict<atr_reg_t, uint16_t> > _atr_regs;
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_iface::sptr usrp1_impl::make_dboard_iface(usrp1_iface::sptr iface,
                                           usrp1_codec_ctrl::sptr codec,
                                           usrp1_impl::dboard_slot_t dboard_slot,
                                           const double &master_clock_rate,
                                           const dboard_id_t &rx_dboard_id
){
    return dboard_iface::sptr(new usrp1_dboard_iface(
        iface, codec, dboard_slot, master_clock_rate, rx_dboard_id
    ));
}

/***********************************************************************
 * Clock Rates
 **********************************************************************/
static const dboard_id_t dbsrx_classic_id(0x0002);

/*
 * Daughterboard reference clock register
 *
 * Bit  7    - 1 turns on refclk, 0 allows IO use
 * Bits 6:0  - Divider value
 */
void usrp1_dboard_iface::set_clock_rate(unit_t unit, double rate)
{
    assert_has(this->get_clock_rates(unit), rate, "dboard clock rate");

    if (unit == UNIT_RX && _rx_dboard_id == dbsrx_classic_id){
        _dbsrx_classic_div = size_t(_master_clock_rate/rate);
        switch(_dboard_slot){
        case usrp1_impl::DBOARD_SLOT_A:
            _iface->poke32(FR_RX_A_REFCLK, (_dbsrx_classic_div & 0x7f) | 0x80);
            break;

        case usrp1_impl::DBOARD_SLOT_B:
            _iface->poke32(FR_RX_B_REFCLK, (_dbsrx_classic_div & 0x7f) | 0x80);
            break;
        }
    }
}

std::vector<double> usrp1_dboard_iface::get_clock_rates(unit_t unit)
{
    std::vector<double> rates;
    if (unit == UNIT_RX && _rx_dboard_id == dbsrx_classic_id){
        for (size_t div = 1; div <= 127; div++)
            rates.push_back(_master_clock_rate / div);
    }
    else{
        rates.push_back(_master_clock_rate);
    }
    return rates;
}

double usrp1_dboard_iface::get_clock_rate(unit_t unit)
{
    if (unit == UNIT_RX && _rx_dboard_id == dbsrx_classic_id){
        return _master_clock_rate/_dbsrx_classic_div;
    }
    return _master_clock_rate;
}

void usrp1_dboard_iface::set_clock_enabled(unit_t, bool)
{
    //TODO we can only enable for special case anyway...
}

double usrp1_dboard_iface::get_codec_rate(unit_t){
    return _master_clock_rate;
}

/***********************************************************************
 * GPIO
 **********************************************************************/
template <typename T>
static T shadow_it(T &shadow, const T &value, const T &mask){
    shadow = (shadow & ~mask) | (value & mask);
    return shadow;
}

void usrp1_dboard_iface::set_pin_ctrl(unit_t unit, uint32_t value, uint32_t mask){
    _set_pin_ctrl(unit, shadow_it(_pin_ctrl[unit], static_cast<uint16_t>(value), static_cast<uint16_t>(mask)));
}

uint32_t usrp1_dboard_iface::get_pin_ctrl(unit_t unit){
    return _pin_ctrl[unit];
}

void usrp1_dboard_iface::set_atr_reg(unit_t unit, atr_reg_t reg, uint32_t value, uint32_t mask){
    _set_atr_reg(unit, reg, shadow_it(_atr_regs[unit][reg], static_cast<uint16_t>(value), static_cast<uint16_t>(mask)));
}

uint32_t usrp1_dboard_iface::get_atr_reg(unit_t unit, atr_reg_t reg){
    return _atr_regs[unit][reg];
}

void usrp1_dboard_iface::set_gpio_ddr(unit_t unit, uint32_t value, uint32_t mask){
    _set_gpio_ddr(unit, shadow_it(_gpio_ddr[unit], static_cast<uint16_t>(value), static_cast<uint16_t>(mask)));
}

uint32_t usrp1_dboard_iface::get_gpio_ddr(unit_t unit){
    return _gpio_ddr[unit];
}

void usrp1_dboard_iface::set_gpio_out(unit_t unit, uint32_t value, uint32_t mask){
    _set_gpio_out(unit, shadow_it(_gpio_out[unit], static_cast<uint16_t>(value), static_cast<uint16_t>(mask)));
}

uint32_t usrp1_dboard_iface::get_gpio_out(unit_t unit){
    return _gpio_out[unit];
}

uint32_t usrp1_dboard_iface::read_gpio(unit_t unit)
{
    uint32_t out_value;

    if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
        out_value = _iface->peek32(1);
    else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
        out_value = _iface->peek32(2);
    else
        UHD_THROW_INVALID_CODE_PATH();

    switch(unit) {
    case UNIT_RX:
        return (uint32_t)((out_value >> 16) & 0x0000ffff);
    case UNIT_TX:
        return (uint32_t)((out_value >>  0) & 0x0000ffff);
    default: UHD_THROW_INVALID_CODE_PATH();
    }
    UHD_ASSERT_THROW(false);
}

void usrp1_dboard_iface::_set_pin_ctrl(unit_t unit, uint16_t value)
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
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

void usrp1_dboard_iface::_set_gpio_ddr(unit_t unit, uint16_t value)
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
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

void usrp1_dboard_iface::_set_gpio_out(unit_t unit, uint16_t value)
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
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

void usrp1_dboard_iface::_set_atr_reg(unit_t unit,
                                     atr_reg_t atr, uint16_t value)
{
    // Ignore unsupported states
    if ((atr == ATR_REG_IDLE) || (atr == ATR_REG_TX_ONLY))
        return;
    if(atr == ATR_REG_RX_ONLY) {
        switch(unit) {
        case UNIT_RX:
            if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
                _iface->poke32(FR_ATR_RXVAL_1, value);
            else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
                _iface->poke32(FR_ATR_RXVAL_3, value);
            break;
        case UNIT_TX:
            if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
                _iface->poke32(FR_ATR_RXVAL_0, value);
            else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
                _iface->poke32(FR_ATR_RXVAL_2, value);
            break;
        default: UHD_THROW_INVALID_CODE_PATH();
        }
    } else if (atr == ATR_REG_FULL_DUPLEX) {
        switch(unit) {
        case UNIT_RX:
            if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
                _iface->poke32(FR_ATR_TXVAL_1, value);
            else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
                _iface->poke32(FR_ATR_TXVAL_3, value);
            break;
        case UNIT_TX:
            if (_dboard_slot == usrp1_impl::DBOARD_SLOT_A)
                _iface->poke32(FR_ATR_TXVAL_0, value);
            else if (_dboard_slot == usrp1_impl::DBOARD_SLOT_B)
                _iface->poke32(FR_ATR_TXVAL_2, value);
            break;
        default: UHD_THROW_INVALID_CODE_PATH();
        }
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
static uint32_t unit_to_otw_spi_dev(dboard_iface::unit_t unit,
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
    default:
        break;
    }
    UHD_THROW_INVALID_CODE_PATH();
}

void usrp1_dboard_iface::write_spi(unit_t unit,
                                   const spi_config_t &config,
                                   uint32_t data,
                                   size_t num_bits)
{
    _iface->write_spi(unit_to_otw_spi_dev(unit, _dboard_slot),
                         config, data, num_bits);
}

uint32_t usrp1_dboard_iface::read_write_spi(unit_t unit,
                                                   const spi_config_t &config,
                                                   uint32_t data,
                                                   size_t num_bits)
{
    return _iface->read_spi(unit_to_otw_spi_dev(unit, _dboard_slot),
                                config, data, num_bits);
}

/***********************************************************************
 * I2C
 **********************************************************************/
void usrp1_dboard_iface::write_i2c(uint16_t addr,
                                   const byte_vector_t &bytes)
{
    return _iface->write_i2c(addr, bytes);
}

byte_vector_t usrp1_dboard_iface::read_i2c(uint16_t addr,
                                           size_t num_bytes)
{
    return _iface->read_i2c(addr, num_bytes);
}

/***********************************************************************
 * Aux DAX/ADC
 **********************************************************************/
void usrp1_dboard_iface::write_aux_dac(dboard_iface::unit_t,
                                       aux_dac_t which, double value)
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

double usrp1_dboard_iface::read_aux_adc(dboard_iface::unit_t unit,
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

/***********************************************************************
 * Unsupported
 **********************************************************************/

void usrp1_dboard_iface::set_command_time(const uhd::time_spec_t&)
{
    throw uhd::not_implemented_error("timed command support not implemented");
}

uhd::time_spec_t usrp1_dboard_iface::get_command_time()
{
    throw uhd::not_implemented_error("timed command support not implemented");
}

void usrp1_dboard_iface::set_fe_connection(unit_t, const std::string&, const fe_connection_t&)
{
    throw uhd::not_implemented_error("fe connection configuration support not implemented");
}

