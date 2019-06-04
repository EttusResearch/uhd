//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rfnoc_mock_reg_iface.hpp"
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

namespace {

noc_block_base::make_args_ptr make_make_args(noc_block_base::noc_id_t noc_id,
    const std::string& block_id,
    const size_t n_inputs,
    const size_t n_outputs)
{
    auto make_args              = std::make_unique<noc_block_base::make_args_t>();
    make_args->noc_id           = noc_id;
    make_args->num_input_ports  = n_inputs;
    make_args->num_output_ports = n_outputs;
    make_args->reg_iface        = std::make_shared<mock_reg_iface_t>();
    make_args->block_id         = block_id;
    make_args->clk_iface        = std::make_shared<clock_iface>("MOCK_CLOCK");
    make_args->tree             = uhd::property_tree::make();
    return make_args;
}

} // namespace

#define MOCK_REGISTER(BLOCK_NAME)                       \
    uhd::rfnoc::noc_block_base::sptr BLOCK_NAME##_make( \
        uhd::rfnoc::noc_block_base::make_args_ptr make_args);

MOCK_REGISTER(ddc_block_control)

BOOST_AUTO_TEST_CASE(test_ddc_block)
{
    node_accessor_t node_accessor{};
    constexpr uint32_t num_hb                      = 2;
    constexpr uint32_t max_cic                     = 128;
    constexpr size_t num_chans                     = 4;
    constexpr noc_block_base::noc_id_t mock_noc_id = 0x7E57DDC0;

    auto ddc_make_args = make_make_args(mock_noc_id, "0/DDC#0", num_chans, num_chans);
    ddc_make_args->args = uhd::device_addr_t("foo=bar");
    auto ddc_reg_iface = std::dynamic_pointer_cast<mock_reg_iface_t>(ddc_make_args->reg_iface);
    ddc_reg_iface->read_memory[ddc_block_control::RB_COMPAT_NUM] =
        (ddc_block_control::MAJOR_COMPAT << 16) | ddc_block_control::MINOR_COMPAT;
    ddc_reg_iface->read_memory[ddc_block_control::RB_NUM_HB]        = num_hb;
    ddc_reg_iface->read_memory[ddc_block_control::RB_CIC_MAX_DECIM] = max_cic;
    auto test_ddc = ddc_block_control_make(std::move(ddc_make_args));
    BOOST_CHECK_EQUAL(test_ddc->get_block_args().get("foo"), "bar");

    node_accessor.init_props(test_ddc.get());
    UHD_LOG_DEBUG("TEST", "Init done.");
    test_ddc->set_property<int>("decim", 4, 0);

    BOOST_REQUIRE(ddc_reg_iface->write_memory.count(ddc_block_control::SR_DECIM_ADDR));
    BOOST_CHECK_EQUAL(
        ddc_reg_iface->write_memory.at(ddc_block_control::SR_DECIM_ADDR), 2 << 8 | 1);
}

