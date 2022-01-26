//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/addsub_block_control.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <string>

using namespace uhd::rfnoc;


class addsub_block_control_impl : public addsub_block_control
{
public:
    constexpr static size_t ARG_A_INPUT_PORT = 0;
    constexpr static size_t ARG_B_INPUT_PORT = 1;
    constexpr static size_t SUM_OUTPUT_PORT  = 0;
    constexpr static size_t DIFF_OUTPUT_PORT = 1;

    RFNOC_BLOCK_CONSTRUCTOR(addsub_block_control)
    {
        // Ensure that the block is configured correctly, i.e., that there
        // are exactly two input ports for the input data and two output
        // ports, one for the sum and one for the difference data.
        const size_t num_input_ports  = get_num_input_ports();
        const size_t num_output_ports = get_num_output_ports();
        UHD_ASSERT_THROW(num_input_ports == 2);
        UHD_ASSERT_THROW(num_output_ports == 2);

        // Actions and properties are fanned out to all ports on the
        // opposite side from which they are received.
        set_prop_forwarding_policy(forwarding_policy_t::ONE_TO_FAN);
        set_action_forwarding_policy(forwarding_policy_t::ONE_TO_FAN);
        // MTU is also fanned out the same way as we produce output packets the
        // same size as input packets.
        set_mtu_forwarding_policy(forwarding_policy_t::ONE_TO_FAN);

        // Register the block's edge type properties.
        register_property(&_prop_type_in_a);
        register_property(&_prop_type_in_b);
        register_property(&_prop_type_out_sum);
        register_property(&_prop_type_out_diff);

        // Add resolvers for edge port types to keep them at sc16, which is
        // the only data type that this add/sub block can handle.
        add_property_resolver({&_prop_type_in_a}, {&_prop_type_in_a}, [this]() {
            _prop_type_in_a.set(IO_TYPE_SC16);
        });
        add_property_resolver({&_prop_type_in_b}, {&_prop_type_in_b}, [this]() {
            _prop_type_in_b.set(IO_TYPE_SC16);
        });
        add_property_resolver({&_prop_type_out_sum}, {&_prop_type_out_sum}, [this]() {
            _prop_type_out_sum.set(IO_TYPE_SC16);
        });
        add_property_resolver({&_prop_type_out_diff}, {&_prop_type_out_diff}, [this]() {
            _prop_type_out_diff.set(IO_TYPE_SC16);
        });
    }

private:
    /**************************************************************************
     * Attributes
     *************************************************************************/
    property_t<std::string> _prop_type_in_a = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE, ARG_A_INPUT_PORT}};
    property_t<std::string> _prop_type_in_b = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE, ARG_B_INPUT_PORT}};
    property_t<std::string> _prop_type_out_sum = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE, SUM_OUTPUT_PORT}};
    property_t<std::string> _prop_type_out_diff = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE, DIFF_OUTPUT_PORT}};
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    addsub_block_control, ADDSUB_BLOCK, "AddSub", CLOCK_KEY_GRAPH, "bus_clk")
