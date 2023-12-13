//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <uhdlib/rfnoc/factory.hpp>

using namespace uhd::rfnoc;

uhd::rfnoc::mock_block_container uhd::rfnoc::get_mock_block(const noc_id_t noc_id,
    const size_t num_inputs,
    const size_t num_outputs,
    const device_addr_t& args,
    const size_t mtu,
    const device_type_t device_id,
    std::shared_ptr<mock_reg_iface_t> client_reg_iface,
    mb_controller::sptr mbc)
{
    block_factory_info_t fac_info = factory::get_block_factory(noc_id, device_id);

    mock_block_container ret_val;
    ret_val.factory   = fac_info.factory_fn;
    ret_val.reg_iface = (client_reg_iface) ? client_reg_iface
                                           : std::make_shared<mock_reg_iface_t>();
    ret_val.tree      = uhd::property_tree::make();
    // Create make args
    ret_val.make_args                   = std::make_unique<noc_block_base::make_args_t>();
    ret_val.make_args->noc_id           = noc_id;
    ret_val.make_args->block_id         = block_id_t(fac_info.block_name);
    ret_val.make_args->num_input_ports  = num_inputs;
    ret_val.make_args->num_output_ports = num_outputs;
    ret_val.make_args->mtu              = mtu;
    ret_val.make_args->chdr_w           = CHDR_W_64;
    ret_val.make_args->reg_iface        = ret_val.reg_iface;
    ret_val.make_args->tree             = ret_val.tree;
    ret_val.make_args->args             = args;
    ret_val.make_args->tb_clk_iface =
        std::make_shared<clock_iface>(fac_info.timebase_clk);
    ret_val.make_args->ctrlport_clk_iface =
        std::make_shared<clock_iface>(fac_info.ctrlport_clk);
    ret_val.make_args->mb_control = mbc;
    if (fac_info.mb_access && !mbc) {
        UHD_LOG_WARNING("MOCK",
            "Mock block controllers has request for MB controller, but none was given.");
    }

    // Make block and return
    return ret_val;
}
