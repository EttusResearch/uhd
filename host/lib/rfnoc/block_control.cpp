//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/block_control.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/registry.hpp>

using namespace uhd::rfnoc;

class block_control_impl : public block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(block_control) {}
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    block_control, DEFAULT_NOC_ID, DEFAULT_BLOCK_NAME, CLOCK_KEY_GRAPH, "bus_clk")
