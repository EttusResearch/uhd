//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

/*! Switchboard Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The Switchboard Block is an RFNoC block that routes any single input to any
 * single output. Routing is 1 to 1, that is, an input port can only be connected
 * to one output port, and vice versa.
 *
 * INIT: This block is initialized with only input port 0 connected to output
 *       port 0.
 *
 * NOTE: This block is not intended to switch during the transmission of packets.
 *       Data on disconnected inputs will stall.
 */
class UHD_API switchboard_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(switchboard_block_control)

    // Block registers
    static const uint32_t REG_BLOCK_SIZE;

    static const uint32_t REG_DEMUX_SELECT_ADDR;
    static const uint32_t REG_MUX_SELECT_ADDR;

    /*! Connects an input to an output
     *
     * Bridges an input to an output. Any existing connections on
     * either the input or output will be dropped.
     *
     * \param input Index of the input port.
     * \param output Index of the output port.
     */
    virtual void connect(const size_t input, const size_t output) = 0;
};

}} // namespace uhd::rfnoc
