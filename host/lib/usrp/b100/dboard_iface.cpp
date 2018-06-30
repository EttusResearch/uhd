//
// Copyright 2011,2015,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b100_regs.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include <uhd/types/serial.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <uhdlib/usrp/cores/gpio_core_200.hpp>
#include <boost/assign/list_of.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

class b100_dboard_iface : public dboard_iface{
public:

    b100_dboard_iface(
        timed_wb_iface::sptr wb_iface,
        i2c_iface::sptr i2c_iface,
        spi_iface::sptr spi_iface,
        b100_clock_ctrl::sptr clock,
        b100_codec_ctrl::sptr codec
    ){
        _wb_iface = wb_iface;
        _i2c_iface = i2c_iface;
        _spi_iface = spi_iface;
        _clock = clock;
        _codec = codec;
        _gpio = gpio_core_200::make(_wb_iface, TOREG(SR_GPIO), REG_RB_GPIO);

        //init the clock rate shadows
        this->set_clock_rate(UNIT_RX, _clock->get_fpga_clock_rate());
        this->set_clock_rate(UNIT_TX, _clock->get_fpga_clock_rate());
    }

    ~b100_dboard_iface(void){
        /* NOP */
    }

    special_props_t get_special_props(void){
        special_props_t props;
        props.soft_clock_divider = false;
        props.mangle_i2c_addrs = false;
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

    void set_command_time(const uhd::time_spec_t& t);
    uhd::time_spec_t get_command_time(void);

    void write_i2c(uint16_t, const byte_vector_t &);
    byte_vector_t read_i2c(uint16_t, size_t);

    void write_spi(
        unit_t unit,
        const spi_config_t &config,
        uint32_t data,
        size_t num_bits
    );

    uint32_t read_write_spi(
        unit_t unit,
        const spi_config_t &config,
        uint32_t data,
        size_t num_bits
    );

    void set_clock_rate(unit_t, double);
    std::vector<double> get_clock_rates(unit_t);
    double get_clock_rate(unit_t);
    void set_clock_enabled(unit_t, bool);
    double get_codec_rate(unit_t);
    void set_fe_connection(unit_t unit, const std::string&, const fe_connection_t& fe_conn);

private:
    timed_wb_iface::sptr _wb_iface;
    i2c_iface::sptr _i2c_iface;
    spi_iface::sptr _spi_iface;
    b100_clock_ctrl::sptr _clock;
    b100_codec_ctrl::sptr _codec;
    gpio_core_200::sptr _gpio;
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_iface::sptr make_b100_dboard_iface(
    timed_wb_iface::sptr wb_iface,
    i2c_iface::sptr i2c_iface,
    spi_iface::sptr spi_iface,
    b100_clock_ctrl::sptr clock,
    b100_codec_ctrl::sptr codec
){
    return dboard_iface::sptr(new b100_dboard_iface(wb_iface, i2c_iface, spi_iface, clock, codec));
}

/***********************************************************************
 * Clock Rates
 **********************************************************************/
void b100_dboard_iface::set_clock_rate(unit_t unit, double rate){
    switch(unit){
    case UNIT_RX: return _clock->set_rx_dboard_clock_rate(rate);
    case UNIT_TX: return _clock->set_tx_dboard_clock_rate(rate);
    case UNIT_BOTH: set_clock_rate(UNIT_RX, rate); set_clock_rate(UNIT_TX, rate); return;
    }
}

std::vector<double> b100_dboard_iface::get_clock_rates(unit_t unit){
    switch(unit){
    case UNIT_RX: return _clock->get_rx_dboard_clock_rates();
    case UNIT_TX: return _clock->get_tx_dboard_clock_rates();
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

double b100_dboard_iface::get_clock_rate(unit_t unit){
    switch(unit){
    case UNIT_RX: return _clock->get_rx_clock_rate();
    case UNIT_TX: return _clock->get_tx_clock_rate();
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

void b100_dboard_iface::set_clock_enabled(unit_t unit, bool enb){
    switch(unit){
    case UNIT_RX: return _clock->enable_rx_dboard_clock(enb);
    case UNIT_TX: return _clock->enable_tx_dboard_clock(enb);
    case UNIT_BOTH: set_clock_enabled(UNIT_RX, enb); set_clock_enabled(UNIT_TX, enb); return;
    }
}

double b100_dboard_iface::get_codec_rate(unit_t){
    return _clock->get_fpga_clock_rate();
}

/***********************************************************************
 * GPIO
 **********************************************************************/
void b100_dboard_iface::set_pin_ctrl(unit_t unit, uint32_t value, uint32_t mask){
    _gpio->set_pin_ctrl(unit, static_cast<uint16_t>(value), static_cast<uint16_t>(mask));
}

uint32_t b100_dboard_iface::get_pin_ctrl(unit_t unit){
    return static_cast<uint32_t>(_gpio->get_pin_ctrl(unit));
}

void b100_dboard_iface::set_atr_reg(unit_t unit, atr_reg_t reg, uint32_t value, uint32_t mask){
    _gpio->set_atr_reg(unit, reg, static_cast<uint16_t>(value), static_cast<uint16_t>(mask));
}

uint32_t b100_dboard_iface::get_atr_reg(unit_t unit, atr_reg_t reg){
    return static_cast<uint32_t>(_gpio->get_atr_reg(unit, reg));
}

void b100_dboard_iface::set_gpio_ddr(unit_t unit, uint32_t value, uint32_t mask){
    _gpio->set_gpio_ddr(unit, static_cast<uint16_t>(value), static_cast<uint16_t>(mask));
}

uint32_t b100_dboard_iface::get_gpio_ddr(unit_t unit){
    return static_cast<uint32_t>(_gpio->get_gpio_ddr(unit));
}

void b100_dboard_iface::set_gpio_out(unit_t unit, uint32_t value, uint32_t mask){
    _gpio->set_gpio_out(unit, static_cast<uint16_t>(value), static_cast<uint16_t>(mask));
}

uint32_t b100_dboard_iface::get_gpio_out(unit_t unit){
    return static_cast<uint32_t>(_gpio->get_gpio_out(unit));
}

uint32_t b100_dboard_iface::read_gpio(unit_t unit){
    return _gpio->read_gpio(unit);
}

/***********************************************************************
 * SPI
 **********************************************************************/
/*!
 * Static function to convert a unit type to a spi slave device number.
 * \param unit the dboard interface unit type enum
 * \return the slave device number
 */
static uint32_t unit_to_otw_spi_dev(dboard_iface::unit_t unit){
    switch(unit){
    case dboard_iface::UNIT_TX: return B100_SPI_SS_TX_DB;
    case dboard_iface::UNIT_RX: return B100_SPI_SS_RX_DB;
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

void b100_dboard_iface::write_spi(
    unit_t unit,
    const spi_config_t &config,
    uint32_t data,
    size_t num_bits
){
    _spi_iface->write_spi(unit_to_otw_spi_dev(unit), config, data, num_bits);
}

uint32_t b100_dboard_iface::read_write_spi(
    unit_t unit,
    const spi_config_t &config,
    uint32_t data,
    size_t num_bits
){
    return _spi_iface->read_spi(unit_to_otw_spi_dev(unit), config, data, num_bits);
}

/***********************************************************************
 * I2C
 **********************************************************************/
void b100_dboard_iface::write_i2c(uint16_t addr, const byte_vector_t &bytes){
    return _i2c_iface->write_i2c(addr, bytes);
}

byte_vector_t b100_dboard_iface::read_i2c(uint16_t addr, size_t num_bytes){
    return _i2c_iface->read_i2c(addr, num_bytes);
}

/***********************************************************************
 * Aux DAX/ADC
 **********************************************************************/
void b100_dboard_iface::write_aux_dac(dboard_iface::unit_t, aux_dac_t which, double value){
    //same aux dacs for each unit
    static const uhd::dict<aux_dac_t, b100_codec_ctrl::aux_dac_t> which_to_aux_dac = map_list_of
        (AUX_DAC_A, b100_codec_ctrl::AUX_DAC_A)
        (AUX_DAC_B, b100_codec_ctrl::AUX_DAC_B)
        (AUX_DAC_C, b100_codec_ctrl::AUX_DAC_C)
        (AUX_DAC_D, b100_codec_ctrl::AUX_DAC_D)
    ;
    _codec->write_aux_dac(which_to_aux_dac[which], value);
}

double b100_dboard_iface::read_aux_adc(dboard_iface::unit_t unit, aux_adc_t which){
    static const uhd::dict<
        unit_t, uhd::dict<aux_adc_t, b100_codec_ctrl::aux_adc_t>
    > unit_to_which_to_aux_adc = map_list_of
        (UNIT_RX, map_list_of
            (AUX_ADC_A, b100_codec_ctrl::AUX_ADC_A1)
            (AUX_ADC_B, b100_codec_ctrl::AUX_ADC_B1)
        )
        (UNIT_TX, map_list_of
            (AUX_ADC_A, b100_codec_ctrl::AUX_ADC_A2)
            (AUX_ADC_B, b100_codec_ctrl::AUX_ADC_B2)
        )
    ;
    return _codec->read_aux_adc(unit_to_which_to_aux_adc[unit][which]);
}

void b100_dboard_iface::set_command_time(const uhd::time_spec_t& t)
{
    _wb_iface->set_time(t);
}

uhd::time_spec_t b100_dboard_iface::get_command_time(void)
{
    return _wb_iface->get_time();
}

void b100_dboard_iface::set_fe_connection(unit_t, const std::string&, const fe_connection_t&)
{
    throw uhd::not_implemented_error("fe connection configuration support not implemented");
}
