//
// Copyright 2010-2012,2015,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "clock_ctrl.hpp"
#include "usrp2_regs.hpp" //wishbone address constants
#include "usrp2_fifo_ctrl.hpp"
#include "ad7922_regs.hpp" //aux adc
#include "ad5623_regs.hpp" //aux dac
#include <uhdlib/usrp/cores/gpio_core_200.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/asio.hpp> //htonl and ntohl
#include <boost/math/special_functions/round.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

class usrp2_dboard_iface : public dboard_iface{
public:
    usrp2_dboard_iface(
        timed_wb_iface::sptr wb_iface,
        uhd::i2c_iface::sptr i2c_iface,
        uhd::spi_iface::sptr spi_iface,
        usrp2_clock_ctrl::sptr clock_ctrl
    );
    ~usrp2_dboard_iface(void);

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

    void set_clock_rate(unit_t, double);
    double get_clock_rate(unit_t);
    std::vector<double> get_clock_rates(unit_t);
    void set_clock_enabled(unit_t, bool);
    double get_codec_rate(unit_t);
    void set_fe_connection(unit_t unit, const std::string&, const fe_connection_t& fe_conn);

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

private:
    timed_wb_iface::sptr _wb_iface;
    uhd::i2c_iface::sptr _i2c_iface;
    uhd::spi_iface::sptr _spi_iface;
    usrp2_clock_ctrl::sptr _clock_ctrl;
    gpio_core_200::sptr _gpio;

    uhd::dict<unit_t, ad5623_regs_t> _dac_regs;
    uhd::dict<unit_t, double> _clock_rates;
    void _write_aux_dac(unit_t);
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_iface::sptr make_usrp2_dboard_iface(
    timed_wb_iface::sptr wb_iface,
    uhd::i2c_iface::sptr i2c_iface,
    uhd::spi_iface::sptr spi_iface,
    usrp2_clock_ctrl::sptr clock_ctrl
){
    return dboard_iface::sptr(new usrp2_dboard_iface(wb_iface, i2c_iface, spi_iface, clock_ctrl));
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_dboard_iface::usrp2_dboard_iface(
    timed_wb_iface::sptr wb_iface,
    uhd::i2c_iface::sptr i2c_iface,
    uhd::spi_iface::sptr spi_iface,
    usrp2_clock_ctrl::sptr clock_ctrl
):
    _wb_iface(wb_iface),
    _i2c_iface(i2c_iface),
    _spi_iface(spi_iface),
    _clock_ctrl(clock_ctrl)
{
    _gpio = gpio_core_200::make(wb_iface, U2_REG_SR_ADDR(SR_GPIO), U2_REG_GPIO_RB);

    //reset the aux dacs
    _dac_regs[UNIT_RX] = ad5623_regs_t();
    _dac_regs[UNIT_TX] = ad5623_regs_t();
    for(unit_t unit:  _dac_regs.keys()){
        _dac_regs[unit].data = 1;
        _dac_regs[unit].addr = ad5623_regs_t::ADDR_ALL;
        _dac_regs[unit].cmd  = ad5623_regs_t::CMD_RESET;
        this->_write_aux_dac(unit);
    }

    //init the clock rate shadows with max rate clock
    this->set_clock_rate(UNIT_RX, sorted(this->get_clock_rates(UNIT_RX)).back());
    this->set_clock_rate(UNIT_TX, sorted(this->get_clock_rates(UNIT_TX)).back());
}

usrp2_dboard_iface::~usrp2_dboard_iface(void){
    /* NOP */
}

/***********************************************************************
 * Clocks
 **********************************************************************/
void usrp2_dboard_iface::set_clock_rate(unit_t unit, double rate){
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    _clock_rates[unit] = rate; //set to shadow
    switch(unit){
    case UNIT_RX: _clock_ctrl->set_rate_rx_dboard_clock(rate); return;
    case UNIT_TX: _clock_ctrl->set_rate_tx_dboard_clock(rate); return;
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

double usrp2_dboard_iface::get_clock_rate(unit_t unit){
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    return _clock_rates[unit]; //get from shadow
}

std::vector<double> usrp2_dboard_iface::get_clock_rates(unit_t unit){
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    switch(unit){
    case UNIT_RX: return _clock_ctrl->get_rates_rx_dboard_clock();
    case UNIT_TX: return _clock_ctrl->get_rates_tx_dboard_clock();
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

void usrp2_dboard_iface::set_clock_enabled(unit_t unit, bool enb){
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    switch(unit){
    case UNIT_RX:   _clock_ctrl->enable_rx_dboard_clock(enb); return;
    case UNIT_TX:   _clock_ctrl->enable_tx_dboard_clock(enb); return;
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

double usrp2_dboard_iface::get_codec_rate(unit_t unit){
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    return _clock_ctrl->get_master_clock_rate();
}

/***********************************************************************
 * GPIO
 **********************************************************************/
void usrp2_dboard_iface::set_pin_ctrl(unit_t unit, uint32_t value, uint32_t mask){
    _gpio->set_pin_ctrl(unit, static_cast<uint16_t>(value), static_cast<uint16_t>(mask));
}

uint32_t usrp2_dboard_iface::get_pin_ctrl(unit_t unit){
    return static_cast<uint32_t>(_gpio->get_pin_ctrl(unit));
}

void usrp2_dboard_iface::set_atr_reg(unit_t unit, atr_reg_t reg, uint32_t value, uint32_t mask){
    _gpio->set_atr_reg(unit, reg, static_cast<uint16_t>(value), static_cast<uint16_t>(mask));
}

uint32_t usrp2_dboard_iface::get_atr_reg(unit_t unit, atr_reg_t reg){
    return static_cast<uint32_t>(_gpio->get_atr_reg(unit, reg));
}

void usrp2_dboard_iface::set_gpio_ddr(unit_t unit, uint32_t value, uint32_t mask){
    _gpio->set_gpio_ddr(unit, static_cast<uint16_t>(value), static_cast<uint16_t>(mask));
}

uint32_t usrp2_dboard_iface::get_gpio_ddr(unit_t unit){
    return static_cast<uint32_t>(_gpio->get_gpio_ddr(unit));
}

void usrp2_dboard_iface::set_gpio_out(unit_t unit, uint32_t value, uint32_t mask){
    _gpio->set_gpio_out(unit, static_cast<uint16_t>(value), static_cast<uint16_t>(mask));
}

uint32_t usrp2_dboard_iface::get_gpio_out(unit_t unit){
    return static_cast<uint32_t>(_gpio->get_gpio_out(unit));
}

uint32_t usrp2_dboard_iface::read_gpio(unit_t unit){
    return _gpio->read_gpio(unit);
}

/***********************************************************************
 * SPI
 **********************************************************************/
static const uhd::dict<dboard_iface::unit_t, int> unit_to_spi_dev = map_list_of
    (dboard_iface::UNIT_TX, SPI_SS_TX_DB)
    (dboard_iface::UNIT_RX, SPI_SS_RX_DB)
;

void usrp2_dboard_iface::write_spi(
    unit_t unit,
    const spi_config_t &config,
    uint32_t data,
    size_t num_bits
){
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    _spi_iface->write_spi(unit_to_spi_dev[unit], config, data, num_bits);
}

uint32_t usrp2_dboard_iface::read_write_spi(
    unit_t unit,
    const spi_config_t &config,
    uint32_t data,
    size_t num_bits
){
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    return _spi_iface->read_spi(unit_to_spi_dev[unit], config, data, num_bits);
}

/***********************************************************************
 * I2C
 **********************************************************************/
void usrp2_dboard_iface::write_i2c(uint16_t addr, const byte_vector_t &bytes){
    return _i2c_iface->write_i2c(addr, bytes);
}

byte_vector_t usrp2_dboard_iface::read_i2c(uint16_t addr, size_t num_bytes){
    return _i2c_iface->read_i2c(addr, num_bytes);
}

/***********************************************************************
 * Aux DAX/ADC
 **********************************************************************/
void usrp2_dboard_iface::_write_aux_dac(unit_t unit){
    static const uhd::dict<unit_t, int> unit_to_spi_dac = map_list_of
        (UNIT_RX, SPI_SS_RX_DAC)
        (UNIT_TX, SPI_SS_TX_DAC)
    ;
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    _spi_iface->write_spi(
        unit_to_spi_dac[unit], spi_config_t::EDGE_FALL,
        _dac_regs[unit].get_reg(), 24
    );
}

void usrp2_dboard_iface::write_aux_dac(unit_t unit, aux_dac_t which, double value){
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");

    _dac_regs[unit].data = boost::math::iround(4095*value/3.3);
    _dac_regs[unit].cmd = ad5623_regs_t::CMD_WR_UP_DAC_CHAN_N;

    typedef uhd::dict<aux_dac_t, ad5623_regs_t::addr_t> aux_dac_to_addr;
    static const uhd::dict<unit_t, aux_dac_to_addr> unit_to_which_to_addr = map_list_of
        (UNIT_RX, map_list_of
            (AUX_DAC_A, ad5623_regs_t::ADDR_DAC_B)
            (AUX_DAC_B, ad5623_regs_t::ADDR_DAC_A)
            (AUX_DAC_C, ad5623_regs_t::ADDR_DAC_A)
            (AUX_DAC_D, ad5623_regs_t::ADDR_DAC_B)
        )
        (UNIT_TX, map_list_of
            (AUX_DAC_A, ad5623_regs_t::ADDR_DAC_A)
            (AUX_DAC_B, ad5623_regs_t::ADDR_DAC_B)
            (AUX_DAC_C, ad5623_regs_t::ADDR_DAC_B)
            (AUX_DAC_D, ad5623_regs_t::ADDR_DAC_A)
        )
    ;
    _dac_regs[unit].addr = unit_to_which_to_addr[unit][which];
    this->_write_aux_dac(unit);
}

double usrp2_dboard_iface::read_aux_adc(unit_t unit, aux_adc_t which){
    static const uhd::dict<unit_t, int> unit_to_spi_adc = map_list_of
        (UNIT_RX, SPI_SS_RX_ADC)
        (UNIT_TX, SPI_SS_TX_ADC)
    ;

    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");

    //setup spi config args
    spi_config_t config;
    config.mosi_edge = spi_config_t::EDGE_FALL;
    config.miso_edge = spi_config_t::EDGE_RISE;

    //setup the spi registers
    ad7922_regs_t ad7922_regs;
    switch(which){
    case AUX_ADC_A: ad7922_regs.mod = 0; break;
    case AUX_ADC_B: ad7922_regs.mod = 1; break;
    } ad7922_regs.chn = ad7922_regs.mod; //normal mode: mod == chn

    //write and read spi
    _spi_iface->write_spi(
        unit_to_spi_adc[unit], config,
        ad7922_regs.get_reg(), 16
    );
    ad7922_regs.set_reg(uint16_t(_spi_iface->read_spi(
        unit_to_spi_adc[unit], config,
        ad7922_regs.get_reg(), 16
    )));

    //convert to voltage and return
    return 3.3*ad7922_regs.result/4095;
}

uhd::time_spec_t usrp2_dboard_iface::get_command_time()
{
    return _wb_iface->get_time();
}

void usrp2_dboard_iface::set_command_time(const uhd::time_spec_t& t)
{
    _wb_iface->set_time(t);
}

void usrp2_dboard_iface::set_fe_connection(unit_t, const std::string&, const fe_connection_t&)
{
    throw uhd::not_implemented_error("fe connection configuration support not implemented");
}
