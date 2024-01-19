//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/split_stream_block_control.hpp>
#include <string>

using namespace uhd::rfnoc;


class split_stream_block_control_impl : public split_stream_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(split_stream_block_control)
    {
        // Ensure that the block is configured correctly, i.e., that the
        // number of output ports is an integer multiple of the number of
        // input ports, and that there are at least two output branches.
        const size_t num_input_ports  = get_num_input_ports();
        const size_t num_output_ports = get_num_output_ports();
        const size_t num_branches     = num_output_ports / num_input_ports;
        UHD_ASSERT_THROW((num_output_ports % num_input_ports == 0) && (num_branches > 1));

        //! Little helper to calculate the output port number given the branch
        // (0..num_branches) and stream (0..num_input_ports()) numbers.
        auto calculate_output_port = [num_input_ports, num_branches](
                                         size_t branch, size_t stream) -> size_t {
            UHD_ASSERT_THROW(branch < num_branches);
            UHD_ASSERT_THROW(stream < num_input_ports);
            return branch * num_input_ports + stream;
        };

        // Configure property propagation and action forwarding behavior for
        // the split stream block.
        set_prop_forwarding_policy(forwarding_policy_t::USE_MAP);
        set_action_forwarding_policy(forwarding_policy_t::USE_MAP);
        // MTU forwarding doesn't allow for USE_MAP, but we assume that packets
        // coming in may potentially go to any output port. We thus fan out the
        // MTU propagation.
        set_mtu_forwarding_policy(forwarding_policy_t::ONE_TO_FAN);

        // Property propagation scheme (X --> Y means 'Properties received on
        // X propagate to Y'):
        //   Input stream S --> {output branch Bo, stream S} for all
        //      S in streams and Bo in branches
        //   Output branch Bo, stream S --> input stream S for all
        //      S in streams and Bo in branches
        //   Output branch Bo, stream S --> {output branch Bp, stream S}
        //      for all S in stream and Bo, Bp in branches (Bo != Bp)
        node_t::forwarding_map_t prop_fwd_map;
        for (size_t stream = 0; stream < num_input_ports; stream++) {
            std::vector<res_source_info> dest_ports;
            for (size_t branch = 0; branch < num_branches; branch++) {
                size_t output_port = calculate_output_port(branch, stream);
                dest_ports.push_back({res_source_info::OUTPUT_EDGE, output_port});
            }
            // Input stream S --> {all output branches, stream S}
            prop_fwd_map.insert({{res_source_info::INPUT_EDGE, stream}, dest_ports});

            for (size_t branch_a = 0; branch_a < num_branches; branch_a++) {
                size_t output_port_a = calculate_output_port(branch_a, stream);
                // The first entry in the back propagation vector is
                // output branch A, stream S --> input stream S
                std::vector<res_source_info> dest_ports_back{
                    {res_source_info::INPUT_EDGE, stream}};

                for (size_t branch_b = 0; branch_b < num_branches; branch_b++) {
                    if (branch_a == branch_b) {
                        continue;
                    }
                    size_t output_port_b = calculate_output_port(branch_b, stream);
                    // Add all output branches that are not the 'source'
                    // output port
                    dest_ports_back.push_back(
                        {res_source_info::OUTPUT_EDGE, output_port_b});
                }
                prop_fwd_map.insert(
                    {{res_source_info::OUTPUT_EDGE, output_port_a}, dest_ports_back});
            }
        }
        set_prop_forwarding_map(prop_fwd_map);

        // Action forwarding scheme (X --> Y means 'Actions received on
        // X forward to Y'):
        //   Input stream S --> {output branch Bo, stream S} for all
        //      S in streams and Bo in branches
        //   Output branch Bo, stream S --> input stream S for all
        //      S in streams and Bo in branches
        node_t::forwarding_map_t action_fwd_map;
        for (size_t stream = 0; stream < num_input_ports; stream++) {
            std::vector<res_source_info> dest_ports;
            for (size_t branch = 0; branch < num_branches; branch++) {
                size_t output_port = calculate_output_port(branch, stream);
                dest_ports.push_back({res_source_info::OUTPUT_EDGE, output_port});

                action_fwd_map.insert({{res_source_info::OUTPUT_EDGE, output_port},
                    {{res_source_info::INPUT_EDGE, stream}}});
            }
            action_fwd_map.insert({{res_source_info::INPUT_EDGE, stream}, dest_ports});
        }
        set_action_forwarding_map(action_fwd_map);
    }
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(split_stream_block_control,
    SPLIT_STREAM_BLOCK,
    "SplitStream",
    CLOCK_KEY_GRAPH,
    "bus_clk")
