//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_DBOARD_IFACE_PYTHON_HPP
#define INCLUDED_UHD_USRP_DBOARD_IFACE_PYTHON_HPP

#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include "../include/uhdlib/usrp/gpio_defs.hpp"

void export_dboard_iface()
{
    using dboard_iface    = uhd::usrp::dboard_iface;
    using special_props_t = uhd::usrp::dboard_iface_special_props_t;
    using unit_t          = dboard_iface::unit_t;

    using aux_dac_t       = dboard_iface::aux_dac_t;
    using aux_adc_t       = dboard_iface::aux_adc_t;

    using gpio_atr_reg_t  = uhd::usrp::gpio_atr::gpio_atr_reg_t;
    using gpio_atr_mode_t = uhd::usrp::gpio_atr::gpio_atr_mode_t;

    bp::enum_<gpio_atr_reg_t>("gpio_atr_reg")
        .value("ATR_REG_IDLE"       , gpio_atr_reg_t::ATR_REG_IDLE       )
        .value("ATR_REG_TX_ONLY"    , gpio_atr_reg_t::ATR_REG_TX_ONLY    )
        .value("ATR_REG_RX_ONLY"    , gpio_atr_reg_t::ATR_REG_RX_ONLY    )
        .value("ATR_REG_FULL_DUPLEX", gpio_atr_reg_t::ATR_REG_FULL_DUPLEX)
        ;

    bp::enum_<gpio_atr_mode_t>("gpio_atr_mode")
        .value("MODE_ATR" , gpio_atr_mode_t::MODE_ATR )
        .value("MODE_GPIO", gpio_atr_mode_t::MODE_GPIO)
        ;

    bp::enum_<unit_t>("unit")
        .value("UNIT_RX"  , unit_t::UNIT_RX  )
        .value("UNIT_TX"  , unit_t::UNIT_TX  )
        .value("UNIT_BOTH", unit_t::UNIT_BOTH)
        ;

    bp::enum_<aux_dac_t>("aux_dac")
        .value("AUX_DAC_A", aux_dac_t::AUX_DAC_A)
        .value("AUX_DAC_B", aux_dac_t::AUX_DAC_B)
        .value("AUX_DAC_C", aux_dac_t::AUX_DAC_C)
        .value("AUX_DAC_D", aux_dac_t::AUX_DAC_D)
        ;

    bp::enum_<aux_adc_t>("aux_adc")
        .value("AUX_ADC_A", aux_adc_t::AUX_ADC_A)
        .value("AUX_ADC_B", aux_adc_t::AUX_ADC_B)
        ;

    bp::class_<special_props_t>("special_props")

        // Properties
        .add_property("soft_clock_divider", &special_props_t::soft_clock_divider)
        .add_property("mangle_i2c_addrs"  , &special_props_t::mangle_i2c_addrs  )
        ;

    bp::class_<
        dboard_iface,
        boost::shared_ptr<dboard_iface>,
        boost::noncopyable>("dboard_iface", bp::no_init)

        // Methods
        .def("get_special_props", &dboard_iface::get_special_props)
        .def("write_aux_dac"    , &dboard_iface::write_aux_dac    )
        .def("read_aux_adc"     , &dboard_iface::read_aux_adc     )
        .def("set_pin_ctrl"     , &dboard_iface::set_pin_ctrl     )
        .def("get_pin_ctrl"     , &dboard_iface::get_pin_ctrl     )
        .def("set_atr_reg"      , &dboard_iface::set_atr_reg      )
        .def("get_atr_reg"      , &dboard_iface::get_atr_reg      )
        .def("set_gpio_ddr"     , &dboard_iface::set_gpio_ddr     )
        .def("get_gpio_ddr"     , &dboard_iface::get_gpio_ddr     )
        .def("get_gpio_out"     , &dboard_iface::get_gpio_out     )
        .def("set_gpio_out"     , &dboard_iface::set_gpio_out     )
        .def("read_gpio"        , &dboard_iface::read_gpio        )
        .def("write_spi"        , &dboard_iface::write_spi        )
        .def("read_write_spi"   , &dboard_iface::read_write_spi   )
        .def("set_clock_rate"   , &dboard_iface::set_clock_rate   )
        .def("get_clock_rate"   , &dboard_iface::get_clock_rate   )
        .def("get_clock_rates"  , &dboard_iface::get_clock_rates  )
        .def("set_clock_enabled", &dboard_iface::set_clock_enabled)
        .def("get_codec_rate"   , &dboard_iface::get_codec_rate   )
        .def("set_fe_connection", &dboard_iface::set_fe_connection)
        .def("get_command_time" , &dboard_iface::get_command_time )
        .def("set_command_time" , &dboard_iface::set_command_time )
        .def("sleep"            , &dboard_iface::sleep            )
        ;
}

#endif /* INCLUDED_UHD_USRP_DBOARD_IFACE_PYTHON_HPP */
