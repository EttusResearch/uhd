//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <functional>
#include <memory>

class dma_fifo_core_3000 : uhd::noncopyable
{
public:
    using sptr        = std::shared_ptr<dma_fifo_core_3000>;
    using poke32_fn_t = std::function<void(uint32_t, uint32_t)>;
    using peek32_fn_t = std::function<uint32_t(uint32_t)>;

    virtual ~dma_fifo_core_3000(void) = 0;

    //! Create a DMA FIFO controller for a specific channel
    static sptr make(
        poke32_fn_t&& poke_fn, peek32_fn_t&& peek_fn, const size_t fifo_index);

    /**************************************************************************
     * API
     *************************************************************************/
    virtual bool has_bist() const = 0;

    //! Return the fullness of the FIFO in bytes
    virtual uint64_t get_fifo_fullness() = 0;

    //! Get the transfer timeout value for the transfer in memory interface
    // clock cycles
    virtual uint16_t get_fifo_timeout() = 0;

    //! Set the transfer timeout value for the transfer in memory interface
    // clock cycles
    virtual void set_fifo_timeout(const uint16_t timeout_cycles) = 0;

    /*!
     * Run the built-in-self-test routine for the DMA FIFO
     *
     * Returns an approximation of the RAM throughput.
     */
    virtual double run_bist(const uint64_t num_bytes, const double timeout_s) = 0;

    //! Return the number of packats that have been transferred
    virtual uint32_t get_packet_count() = 0;
};
