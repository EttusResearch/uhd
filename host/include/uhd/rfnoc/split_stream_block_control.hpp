//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

/*! Split Stream Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The Split Stream Block is an RFNoC block that takes in a single CHDR
 * stream and duplicates it, creating a number of output streams for each
 * input stream. The number of streams is defined by the NUM_PORTS parameter
 * used to instantiate the RFNoC block in the image, while the number of
 * output branches is defined by the NUM_BRANCH parameter. In software, this
 * corresponds to an RFNoC block having NUM_PORTS input ports and
 * NUM_PORTS * NUM_BRANCH output ports.
 *
 * Given the following example of a split stream RFNoC block with
 * NUM_PORTS = 2 and NUM_BRANCH = 3, the input streams map to output branches
 * and streams as follows. The number located at each input and output
 * indicates the port number corresponding to that stream and branch:
 *
 *                  +----------+
 *     Stream A --->|0        0|---> Stream A } Branch 0
 *     Stream B --->|1        1|---> Stream B
 *                  |         2|---> Stream A } Branch 1
 *                  |         3|---> Stream B
 *                  |         4|---> Stream A } Branch 2
 *                  |         5|---> Stream B
 *                  +----------+
 *
 * In other words, the port number corresponding to stream S of branch B is
 * given by B * (num_input_ports) + S.
 *
 * \section ss_fwd_behavior Property Propagation and Action Forwarding Behavior
 *
 * The default behavior of the split stream block controller is to propagate
 * properties received on a particular stream to all branches of that stream.
 * For example, if a property is received at branch 0, stream B in the example
 * above, that property will be propagated to stream B in branches 1 and 2
 * AND to stream B in the input. The property will not propagate across
 * streams.
 *
 * For actions, an action received on a particular stream is forwarded to
 * that stream on *all opposite* branches. If in the example above, an action
 * is received at branch 2, stream A, it will be forwarded to stream A on the
 * input side. Similarly, if an action is received on stream B of the input,
 * it will be forwarded to stream B on branches 0, 1, AND 2.
 */
class UHD_API split_stream_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(split_stream_block_control)
};

}} // namespace uhd::rfnoc
