//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/**
 * Contains functions to facilitate sending and receiving samples
 * to/from a RFNoC replay block.
 */
#pragma once

#include <uhd/rfnoc_graph.hpp>

#include <uhd/rfnoc/replay_block_control.hpp>


namespace uhd::rfnoc::dram {

/*!
 * Send samples to a RFNoC replay block.
 *
 * This sends samples to a RFNoC replay block. It handles common data losses
 * that can occur when sending data over a network interface using UDP. The
 * stream buffer has to be a one dimensional array. If a block is connected
 * to the input port of the replay block it will be temporarily disconnected
 * and reconnected after transfer.
 *
 * \param graph RFNoC graph to operate on
 * \param buff buffer to be sent
 * \param nsamps number of samples in buff
 * \param cpu_fmt format of the data in buff, must match the data in buff
 * \param otw_fmt format of the data over the wire, there must be a matching
 *        converter between cpu_fmt and otw_fmt (default: "sc16")
 * \param replay Replay block to send the data to
 *        (default: nullptr, will use the first available)
 * \param port input port to be used on the replay block (default: 0)
 * \param offset offset in the replay where to store the data (default: 0)
 */
UHD_API void upload(rfnoc_graph::sptr graph,
    const void* buff,
    const size_t nsamps,
    const std::string& cpu_fmt,
    const std::string& otw_fmt        = "sc16",
    replay_block_control::sptr replay = nullptr,
    size_t port                       = 0,
    size_t offset                     = 0);

/*!
 * Receives data from an RFNoC replay block
 *
 * This receives samples from an RFNoC replay block. It handles common data losses
 * that can occur when receiving data over a network interface using UDP.
 * If a block is connected to the input port of the replay block it will be
 * temporarily disconnected and reconnected after transfer.
 *
 * \param graph RFNoc graph to operate on
 * \param buff buffer to received data into
 * \param nsamps number of samples in buff
 * \param cpu_fmt format of the data in buff, must match the data in buff
 * \param otw_fmt format of the data over the wire, there must be a matching
 *        converter between cpu_fmt and otw_fmt (default: "sc16")
 * \param replay Replay block to send the data to
 *        (default: nullptr, will use the first available)
 * \param port output port to be used on the replay block (default: 0)
 * \param offset offset in the replay where to read the data (default: 0)
 */
UHD_API void download(rfnoc_graph::sptr graph,
    const void* buff,
    const size_t nsamps,
    const std::string& cpu_fmt,
    const std::string& otw_fmt        = "sc16",
    replay_block_control::sptr replay = nullptr,
    size_t port                       = 0,
    size_t offset                     = 0);
} // namespace uhd::rfnoc::dram
