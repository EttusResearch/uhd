//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/ranges.hpp>

namespace uhd { namespace rfnoc {

/*! Keep One in N Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The Keep One in N block has two modes: sample mode and packet mode.
 * In sample mode, the first sample is kept and then N-1 samples are dropped.
 * Packet mode is similar to sample mode, except a packet of samples is kept
 * and then N-1 packets are dropped. The packet size is determined automatically.
 */
class UHD_API keep_one_in_n_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(keep_one_in_n_block_control)

    enum class mode { SAMPLE_MODE, PACKET_MODE };

    // Block registers
    static const uint32_t REG_N_OFFSET;
    static const uint32_t REG_MODE_OFFSET;
    static const uint32_t REG_WIDTH_N_OFFSET;

    /*! Get the maximum supported value for N
     *
     * Get the maximum supported value for N for all channels
     *
     * \returns The maximum supported value for N
     */
    virtual size_t get_max_n() const = 0;

    /*! Get the current value of N
     *
     * Get the current value of N
     *
     * \param chan The block channel
     * \returns The current value of N
     */
    virtual size_t get_n(const size_t chan = 0) const = 0;

    /*! Set the value of N
     *
     * Set the value of N.
     * See set_mode() for how the value of N is interpreted
     * depending on the mode.
     *
     * \param n The number of samples or packets to drop (minus one)
     * \param chan The block channel
     */
    virtual void set_n(const size_t n, const size_t chan = 0) = 0;

    /*! Get the current mode
     *
     * Get the current mode (sample or packet mode, see mode enum)
     *
     * \param chan The block channel
     * \returns The current mode
     */
    virtual mode get_mode(const size_t chan = 0) const = 0;

    /*! Set the mode
     *
     * Set the mode.
     * There are two modes, sample mode (0) and packet mode (1).
     * In sample mode, the block will keep 1 value and then drop N-1 values.
     * In packet mode, the block will keep 1 packet and then drop N-1 packets.
     *
     * \param mode The mode of the block
     * \param chan The block channel
     */
    virtual void set_mode(const mode mode, const size_t chan = 0) = 0;
};

}} // namespace uhd::rfnoc
