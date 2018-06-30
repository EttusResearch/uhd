//
// Copyright 2010-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_DBOARD_IFACE_HPP
#define INCLUDED_X300_DBOARD_IFACE_HPP

#include "x300_clock_ctrl.hpp"
#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <uhdlib/usrp/cores/i2c_core_100_wb32.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <uhdlib/usrp/cores/rx_frontend_core_3000.hpp>
#include "ad7922_regs.hpp" //aux adc
#include "ad5623_regs.hpp" //aux dac
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/types/dict.hpp>

struct x300_dboard_iface_config_t
{
    uhd::usrp::gpio_atr::db_gpio_atr_3000::sptr gpio;
    spi_core_3000::sptr                         spi;
    size_t                                      rx_spi_slaveno;
    size_t                                      tx_spi_slaveno;
    uhd::i2c_iface::sptr                        i2c;
    x300_clock_ctrl::sptr                       clock;
    x300_clock_which_t                          which_rx_clk;
    x300_clock_which_t                          which_tx_clk;
    uint8_t                              dboard_slot;
    uhd::timed_wb_iface::sptr                   cmd_time_ctrl;
};

class x300_dboard_iface : public uhd::usrp::dboard_iface
{
public:
    x300_dboard_iface(const x300_dboard_iface_config_t &config);
    ~x300_dboard_iface(void);

    inline special_props_t get_special_props(void)
    {
        special_props_t props;
        props.soft_clock_divider = false;
        props.mangle_i2c_addrs = (_config.dboard_slot == 1);
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

    void write_i2c(uint16_t, const uhd::byte_vector_t &);
    uhd::byte_vector_t read_i2c(uint16_t, size_t);

    void set_clock_rate(unit_t, double);
    double get_clock_rate(unit_t);
    std::vector<double> get_clock_rates(unit_t);
    void set_clock_enabled(unit_t, bool);
    double get_codec_rate(unit_t);

    void write_spi(
        unit_t unit,
        const uhd::spi_config_t &config,
        uint32_t data,
        size_t num_bits
    );

    uint32_t read_write_spi(
        unit_t unit,
        const uhd::spi_config_t &config,
        uint32_t data,
        size_t num_bits
    );
    void set_fe_connection(
        unit_t unit,  const std::string& name,
        const uhd::usrp::fe_connection_t& fe_conn);

    // X300 can set the FE connection on the RX side
    bool has_set_fe_connection(const unit_t unit) override
    {
        return unit == UNIT_RX;
    }

    void add_rx_fe(
        const std::string& fe_name,
        rx_frontend_core_3000::sptr fe_core);

private:
    const x300_dboard_iface_config_t _config;
    uhd::dict<unit_t, ad5623_regs_t> _dac_regs;
    uhd::dict<unit_t, double> _clock_rates;
    uhd::dict<std::string, rx_frontend_core_3000::sptr> _rx_fes;
    void _write_aux_dac(unit_t);
};



#endif /* INCLUDED_X300_DBOARD_IFACE_HPP */
