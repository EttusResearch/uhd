//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_REPLAY_BLOCK_HPP
#define INCLUDED_LIBUHD_RFNOC_REPLAY_BLOCK_HPP

#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/rfnoc/source_block_ctrl_base.hpp>

namespace uhd { namespace rfnoc {

/*! \brief Replay block controller
 *
 * The replay block has the following features:
 * - One input and one output
 * - The ability to record and playback data
 * - Configurable base addresses and buffer sizes
 * - Independent record and playback controls
 * - Radio-like playback interface
 * - The storage for the replay data can be any
 *   memory, usually an off-chip DRAM.
 *
 */
class UHD_RFNOC_API replay_block_ctrl : public source_block_ctrl_base,
                                        public sink_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(replay_block_ctrl)

    //! Configure the base address and size of the record buffer region (in bytes).
    virtual void config_record(
        const uint32_t base_addr, const uint32_t size, const size_t chan) = 0;

    //! Configure the base address and size of the playback buffer region (in bytes).
    virtual void config_play(
        const uint32_t base_addr, const uint32_t size, const size_t chan) = 0;

    //! Restarts recording at the beginning of the record buffer
    virtual void record_restart(const size_t chan) = 0;

    //! Returns the base address of the record buffer (in bytes).
    virtual uint32_t get_record_addr(const size_t chan) = 0;

    //! Returns the base address of the playback buffer (in bytes).
    virtual uint32_t get_play_addr(const size_t chan) = 0;

    //! Returns the size of the record buffer (in bytes).
    virtual uint32_t get_record_size(const size_t chan) = 0;

    //! Returns the current fullness of the record buffer (in bytes).
    virtual uint32_t get_record_fullness(const size_t chan) = 0;

    //! Returns the size of the playback buffer (in bytes).
    virtual uint32_t get_play_size(const size_t chan) = 0;

    //! Sets the size of the packets played by the Replay block (in 64-bit words)
    virtual void set_words_per_packet(const uint32_t num_words, const size_t chan) = 0;

    //! Returns the size of the packets played by the Replay block (in 64-bit words)
    virtual uint32_t get_words_per_packet(const size_t chan) = 0;

    //! Halts playback and clears the playback command FIFO
    virtual void play_halt(const size_t chan) = 0;

}; /* class replay_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_REPLAY_BLOCK_HPP */
