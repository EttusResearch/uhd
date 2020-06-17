//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/logpwr_block_control.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <string>

using namespace uhd::rfnoc;


class logpwr_block_control_impl : public logpwr_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(logpwr_block_control)
    {
        const size_t num_input_ports  = get_num_input_ports();
        const size_t num_output_ports = get_num_output_ports();
        UHD_ASSERT_THROW(num_output_ports == num_input_ports);

        _prop_type_in.reserve(num_input_ports);
        _prop_type_out.reserve(num_output_ports);

        for (size_t chan = 0; chan < num_input_ports; chan++) {
            // register edge properties
            _prop_type_in.emplace_back(property_t<std::string>{
                PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE, chan}});
            _prop_type_out.emplace_back(property_t<std::string>{
                PROP_KEY_TYPE, IO_TYPE_S16, {res_source_info::OUTPUT_EDGE, chan}});
            register_property(&_prop_type_in.back());
            register_property(&_prop_type_out.back());

            // add resolvers for type
            add_property_resolver({&_prop_type_in.back()},
                {&_prop_type_in.back()},
                [this, chan]() { _prop_type_in.at(chan).set(IO_TYPE_SC16); });
            add_property_resolver({&_prop_type_out.back()},
                {&_prop_type_out.back()},
                [this, chan]() { _prop_type_out.at(chan).set(IO_TYPE_S16); });
        }
    }

private:
    /**************************************************************************
     * Attributes
     *************************************************************************/
    std::vector<property_t<std::string>> _prop_type_in;
    std::vector<property_t<std::string>> _prop_type_out;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    logpwr_block_control, LOGPWR_BLOCK, "LogPwr", CLOCK_KEY_GRAPH, "bus_clk")
