//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc_graph.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhdlib/rfnoc/rfnoc_tx_streamer.hpp>

namespace uhd { namespace rfnoc {

/*!
 * Extends the rfnoc_tx_streamer so it can use a Replay block to
 * buffer TX data.
 */
class rfnoc_tx_streamer_replay_buffered : public rfnoc_tx_streamer
{
public:
    struct replay_config_t
    {
        replay_block_control::sptr ctrl = nullptr;  // Replay block control
        size_t port                     = 0;        // Replay port to use
        uint64_t start_address          = 0;        // Start address in memory
        uint64_t mem_size               = 0;        // Size of memory block to use
    };

    struct replay_status_t {
        const replay_config_t config;
        uint64_t record_offset          = 0;
        uint64_t play_offset            = 0;
        uint64_t play_end               = 0;
    };

    /*! Constructor
     *
     * \param num_ports     The number of ports
     * \param stream_args   Arguments to aid the construction of the streamer
     * \param disconnect_cb Callback function to disconnect the streamer when
     *                      the object is destroyed
     * \param replay_chans  Vector of Replay configurations to use (one per channel)
     */
    rfnoc_tx_streamer_replay_buffered(
        const size_t num_ports,
        const uhd::stream_args_t stream_args,
        std::function<void(const std::string&)> disconnect_cb,
        std::vector<replay_config_t> replay_configs);

    /*! Destructor
     */
    ~rfnoc_tx_streamer_replay_buffered();

    /*! Send
     * 
     * Sends data by recording to the Replay block and playing it.
     *
     * \param buffs a vector of read-only memory containing samples
     * \param nsamps_per_buff the number of samples to send, per buffer
     * \param metadata data describing the buffer's contents
     * \param timeout the timeout in seconds to wait on a packet
     * \return the number of samples sent
     */
    size_t send(const buffs_type& buffs,
        const size_t nsamps_per_buff,
        const tx_metadata_t& metadata,
        const double timeout = 0.1) override;

private:
    // Size of item
    size_t _bytes_per_otw_item;

    // Status of Replay channels
    std::vector<replay_status_t> _replay_chans;
};

}} // namespace uhd::rfnoc
