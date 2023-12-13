//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/switchboard_block_control.hpp>

using namespace uhd::rfnoc;

// Register offsets
const uint32_t switchboard_block_control::REG_BLOCK_SIZE        = 8;
const uint32_t switchboard_block_control::REG_DEMUX_SELECT_ADDR = 0;
const uint32_t switchboard_block_control::REG_MUX_SELECT_ADDR   = 4;

// User properties
const char* const PROP_KEY_INPUT_SELECT  = "input_select";
const char* const PROP_KEY_OUTPUT_SELECT = "output_select";

class switchboard_block_control_impl : public switchboard_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(switchboard_block_control),
        _num_input_ports(get_num_input_ports()),
        _num_output_ports(get_num_output_ports()), _connect_call(false),
        _switchboard_reg_iface(*this, 0, REG_BLOCK_SIZE)
    {
        UHD_ASSERT_THROW(_num_input_ports > 0 && _num_output_ports > 0);

        _register_props();

        // Configure property propagation and action forwarding behavior.
        set_prop_forwarding_policy(forwarding_policy_t::USE_MAP);
        set_action_forwarding_policy(forwarding_policy_t::USE_MAP);
        // MTU forwarding doesn't allow for USE_MAP, but we assume that packets
        // coming in may potentially go to any output port. We thus fan out the
        // MTU propagation.
        set_mtu_forwarding_policy(forwarding_policy_t::ONE_TO_FAN);

        _update_forwarding_map();
    }

    void connect(const size_t input, const size_t output) override
    {
        // Let the property resolvers know they don't need to call
        // _update_forwarding_map() themselves, connect() will call it after both
        // properties are set.
        _connect_call = true;
        try {
            set_property<int>(PROP_KEY_INPUT_SELECT, static_cast<int>(input), output);
            set_property<int>(PROP_KEY_OUTPUT_SELECT, static_cast<int>(output), input);
        } catch (...) {
            _connect_call = false;
            throw;
        }
        _update_forwarding_map();
        _connect_call = false;
    }

private:
    const size_t _num_input_ports;
    const size_t _num_output_ports;
    bool _connect_call;

    void _register_props()
    {
        _input_select.reserve(_num_output_ports);
        _output_select.reserve(_num_input_ports);

        // Register _input_select properties
        for (size_t output_port = 0; output_port < _num_output_ports; output_port++) {
            _input_select.emplace_back(property_t<int>(
                PROP_KEY_INPUT_SELECT, 0, {res_source_info::USER, output_port}));

            register_property(&_input_select.back(), [this, output_port]() {
                int select_val = _input_select.at(output_port).get();
                if (select_val < 0
                    || static_cast<unsigned int>(select_val) >= _num_input_ports)
                    throw uhd::value_error("Index out of bounds");
                _switchboard_reg_iface.poke32(
                    REG_MUX_SELECT_ADDR, select_val, output_port);
                if (!_connect_call)
                    _update_forwarding_map();
            });
        }

        // Register _output_select properties
        for (size_t input_port = 0; input_port < _num_input_ports; input_port++) {
            _output_select.emplace_back(property_t<int>(
                PROP_KEY_OUTPUT_SELECT, 0, {res_source_info::USER, input_port}));

            register_property(&_output_select.back(), [this, input_port]() {
                int select_val = _output_select.at(input_port).get();
                if (select_val < 0
                    || static_cast<unsigned int>(select_val) >= _num_output_ports)
                    throw uhd::value_error("Index out of bounds");
                _switchboard_reg_iface.poke32(
                    REG_DEMUX_SELECT_ADDR, select_val, input_port);
                if (!_connect_call)
                    _update_forwarding_map();
            });
        }
    }

    void _update_forwarding_map()
    {
        node_t::forwarding_map_t prop_fwd_map;
        node_t::forwarding_map_t action_fwd_map;

        // Property propagation scheme:
        //   Connected inputs and outputs will propagate to each other.
        //   Unconnected inputs and outputs do not propagate.
        for (size_t input_port = 0; input_port < _num_input_ports; input_port++) {
            size_t linked_output_port = _output_select.at(input_port).get();
            size_t linked_input_port  = _input_select.at(linked_output_port).get();
            if (linked_input_port == input_port) {
                prop_fwd_map.insert({{res_source_info::INPUT_EDGE, linked_input_port},
                    {{res_source_info::OUTPUT_EDGE, linked_output_port}}});
                prop_fwd_map.insert({{res_source_info::OUTPUT_EDGE, linked_output_port},
                    {{res_source_info::INPUT_EDGE, linked_input_port}}});
                action_fwd_map.insert({{res_source_info::INPUT_EDGE, linked_input_port},
                    {{res_source_info::OUTPUT_EDGE, linked_output_port}}});
                action_fwd_map.insert({{res_source_info::OUTPUT_EDGE, linked_output_port},
                    {{res_source_info::INPUT_EDGE, linked_input_port}}});
            }
        }
        set_prop_forwarding_map(prop_fwd_map);
        set_action_forwarding_map(action_fwd_map);
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    std::vector<property_t<int>> _input_select;
    std::vector<property_t<int>> _output_select;

    /**************************************************************************
     * Register Interface
     *************************************************************************/
    multichan_register_iface _switchboard_reg_iface;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(switchboard_block_control,
    SWITCHBOARD_BLOCK,
    "Switchboard",
    CLOCK_KEY_GRAPH,
    "bus_clk")
