//
// Copyright 2013,2015,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_dboard_iface.hpp"
#include "x300_regs.hpp"
#include <uhd/utils/safe_call.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * Structors
 **********************************************************************/
x300_dboard_iface::x300_dboard_iface(const x300_dboard_iface_config_t &config):
    _config(config)
{
    //reset the aux dacs
    _dac_regs[UNIT_RX] = ad5623_regs_t();
    _dac_regs[UNIT_TX] = ad5623_regs_t();
    for(unit_t unit:  _dac_regs.keys())
    {
        _dac_regs[unit].data = 1;
        _dac_regs[unit].addr = ad5623_regs_t::ADDR_ALL;
        _dac_regs[unit].cmd  = ad5623_regs_t::CMD_RESET;
        this->_write_aux_dac(unit);
    }

    _clock_rates[UNIT_RX] = _config.clock->get_dboard_rate(_config.which_rx_clk);
    _clock_rates[UNIT_TX] = _config.clock->get_dboard_rate(_config.which_tx_clk);

    this->set_clock_enabled(UNIT_RX, false);
    this->set_clock_enabled(UNIT_TX, false);
}

x300_dboard_iface::~x300_dboard_iface(void)
{
    UHD_SAFE_CALL
    (
        this->set_clock_enabled(UNIT_RX, false);
        this->set_clock_enabled(UNIT_TX, false);
    )
}

/***********************************************************************
 * Clocks
 **********************************************************************/
void x300_dboard_iface::set_clock_rate(unit_t unit, double rate)
{
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");

    // Just return if the requested rate is already set
    if (std::fabs(_clock_rates[unit] - rate) < std::numeric_limits<double>::epsilon())
        return;

    switch(unit)
    {
        case UNIT_RX:
            _config.clock->set_dboard_rate(_config.which_rx_clk, rate);
            break;
        case UNIT_TX:
            _config.clock->set_dboard_rate(_config.which_tx_clk, rate);
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
    _clock_rates[unit] = rate; //set to shadow
}

double x300_dboard_iface::get_clock_rate(unit_t unit)
{
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    return _clock_rates[unit]; //get from shadow
}

std::vector<double> x300_dboard_iface::get_clock_rates(unit_t unit)
{
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    switch(unit)
    {
        case UNIT_RX:
            return _config.clock->get_dboard_rates(_config.which_rx_clk);
        case UNIT_TX:
            return _config.clock->get_dboard_rates(_config.which_tx_clk);
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void x300_dboard_iface::set_clock_enabled(unit_t unit, bool enb)
{
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    switch(unit)
    {
        case UNIT_RX:
            return _config.clock->enable_dboard_clock(_config.which_rx_clk, enb);
        case UNIT_TX:
            return _config.clock->enable_dboard_clock(_config.which_tx_clk, enb);
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

double x300_dboard_iface::get_codec_rate(unit_t unit)
{
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    return _config.clock->get_master_clock_rate();
}

/***********************************************************************
 * GPIO
 **********************************************************************/
void x300_dboard_iface::set_pin_ctrl(unit_t unit, uint32_t value, uint32_t mask)
{
    _config.gpio->set_pin_ctrl(unit, value, mask);
}

uint32_t x300_dboard_iface::get_pin_ctrl(unit_t unit)
{
    return _config.gpio->get_pin_ctrl(unit);
}

void x300_dboard_iface::set_atr_reg(unit_t unit, atr_reg_t reg, uint32_t value, uint32_t mask)
{
    _config.gpio->set_atr_reg(unit, reg, value, mask);
}

uint32_t x300_dboard_iface::get_atr_reg(unit_t unit, atr_reg_t reg)
{
    return _config.gpio->get_atr_reg(unit, reg);
}

void x300_dboard_iface::set_gpio_ddr(unit_t unit, uint32_t value, uint32_t mask)
{
    _config.gpio->set_gpio_ddr(unit, value, mask);
}

uint32_t x300_dboard_iface::get_gpio_ddr(unit_t unit)
{
    return _config.gpio->get_gpio_ddr(unit);
}

void x300_dboard_iface::set_gpio_out(unit_t unit, uint32_t value, uint32_t mask)
{
    _config.gpio->set_gpio_out(unit, value, mask);
}

uint32_t x300_dboard_iface::get_gpio_out(unit_t unit)
{
    return _config.gpio->get_gpio_out(unit);
}

uint32_t x300_dboard_iface::read_gpio(unit_t unit)
{
    return _config.gpio->read_gpio(unit);
}

/***********************************************************************
 * SPI
 **********************************************************************/
void x300_dboard_iface::write_spi(
    unit_t unit,
    const spi_config_t &config,
    uint32_t data,
    size_t num_bits
){
    uint32_t slave = 0;
    if (unit == UNIT_TX) slave |= _config.tx_spi_slaveno;
    if (unit == UNIT_RX) slave |= _config.rx_spi_slaveno;

    _config.spi->write_spi(int(slave), config, data, num_bits);
}

uint32_t x300_dboard_iface::read_write_spi(
    unit_t unit,
    const spi_config_t &config,
    uint32_t data,
    size_t num_bits
){
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    return _config.spi->read_spi(
        (unit==dboard_iface::UNIT_TX)?_config.tx_spi_slaveno:_config.rx_spi_slaveno,
        config, data, num_bits);
}

/***********************************************************************
 * I2C
 **********************************************************************/
void x300_dboard_iface::write_i2c(uint16_t addr, const byte_vector_t &bytes)
{
    return _config.i2c->write_i2c(addr, bytes);
}

byte_vector_t x300_dboard_iface::read_i2c(uint16_t addr, size_t num_bytes)
{
    return _config.i2c->read_i2c(addr, num_bytes);
}

/***********************************************************************
 * Aux DAX/ADC
 **********************************************************************/
void x300_dboard_iface::_write_aux_dac(unit_t unit)
{
    static const uhd::dict<unit_t, int> unit_to_spi_dac = map_list_of
        (UNIT_RX, DB_RX_LSDAC_SEN)
        (UNIT_TX, DB_TX_LSDAC_SEN)
    ;
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");
    _config.spi->write_spi(
        unit_to_spi_dac[unit], spi_config_t::EDGE_FALL,
        _dac_regs[unit].get_reg(), 24
    );
}

void x300_dboard_iface::write_aux_dac(unit_t unit, aux_dac_t which, double value)
{
    if (unit == UNIT_BOTH) throw uhd::runtime_error("UNIT_BOTH not supported.");

    _dac_regs[unit].data = boost::math::iround(4095*value/3.3);
    _dac_regs[unit].cmd = ad5623_regs_t::CMD_WR_UP_DAC_CHAN_N;

    typedef uhd::dict<aux_dac_t, ad5623_regs_t::addr_t> aux_dac_to_addr;
    static const uhd::dict<unit_t, aux_dac_to_addr> unit_to_which_to_addr = map_list_of
        (UNIT_RX, map_list_of
            (AUX_DAC_A, ad5623_regs_t::ADDR_DAC_A)
            (AUX_DAC_B, ad5623_regs_t::ADDR_DAC_B)
            (AUX_DAC_C, ad5623_regs_t::ADDR_DAC_B)
            (AUX_DAC_D, ad5623_regs_t::ADDR_DAC_A)
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

double x300_dboard_iface::read_aux_adc(unit_t unit, aux_adc_t which)
{
    static const uhd::dict<unit_t, int> unit_to_spi_adc = map_list_of
        (UNIT_RX, DB_RX_LSADC_SEN)
        (UNIT_TX, DB_TX_LSADC_SEN)
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
    _config.spi->write_spi(
        unit_to_spi_adc[unit], config,
        ad7922_regs.get_reg(), 16
    );
    ad7922_regs.set_reg(uint16_t(_config.spi->read_spi(
        unit_to_spi_adc[unit], config,
        ad7922_regs.get_reg(), 16
    )));

    //convert to voltage and return
    return 3.3*ad7922_regs.result/4095;
}

uhd::time_spec_t x300_dboard_iface::get_command_time()
{
    return _config.cmd_time_ctrl->get_time();
}

void x300_dboard_iface::set_command_time(const uhd::time_spec_t& t)
{
    _config.cmd_time_ctrl->set_time(t);
}

void x300_dboard_iface::add_rx_fe(
    const std::string& fe_name,
    rx_frontend_core_3000::sptr fe_core)
{
    _rx_fes[fe_name] = fe_core;
}

void x300_dboard_iface::set_fe_connection(
    unit_t unit, const std::string& fe_name,
    const fe_connection_t& fe_conn)
{
    if (unit == UNIT_RX) {
        if (_rx_fes.has_key(fe_name)) {
            _rx_fes[fe_name]->set_fe_connection(fe_conn);
        } else {
            throw uhd::assertion_error("front-end name was not registered: " + fe_name);
        }
    } else {
        throw uhd::not_implemented_error("frontend connection not configurable for TX");
    }
}
