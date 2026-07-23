//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/transport/links.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace uhd { namespace usrp { namespace b300 {

/**
 * B300 PCIe FIFO implementation, based on the inchworm/libnifpga-usrp Fifo
 * implementation. This class wraps the functionality from that library.
 */
class b300_pcie_fifo
{
public:
    using sptr = std::shared_ptr<b300_pcie_fifo>;

    /**
     * Create a new B300 PCIe FIFO object
     *
     * \param fifo_num FIFO index number
     * \param device_path Path to the PCIe device file
     * \param is_write Whether this is a write (host-to-target) FIFO
     */
    static sptr make(uint32_t fifo_num, const std::string& device_path, bool is_write);

    /**
     * Configure FIFO parameters
     *
     * \param requested_depth Requested buffer depth (in elements)
     * \return Actual buffer depth (in elements)
     */
    virtual size_t configure(size_t requested_depth) = 0;

    /**
     * Start the FIFO operation
     */
    virtual void start() = 0;

    /**
     * Stop the FIFO operation
     */
    virtual void stop() = 0;

    struct fifo_acquire_result
    {
        size_t elements_acquired;
        size_t elements_remaining;
    };

    /**
     * Acquire buffer from FIFO
     *
     * \param elements Pointer to buffer (set by this function)
     * \param elements_requested Number of elements requested
     * \param timeout_ms Timeout in milliseconds
     * \return Struct containing number of elements acquired and remaining
     */
    virtual fifo_acquire_result acquire(
        uint64_t*& elements, size_t elements_requested, uint32_t timeout_ms) = 0;

    /**
     * Release elements back to FIFO
     *
     * \param elements Number of elements to release
     */
    virtual void release(size_t elements) = 0;

    /**
     * Get FIFO number
     */
    virtual uint32_t get_fifo_num() const = 0;

    /**
     * Get whether this is a write (host-to-target) FIFO
     */
    virtual bool is_write_fifo() const = 0;

    /**
     * Get the FIFO name
     */
    virtual std::string get_name() const = 0;

    /**
     * Destructor
     */
    virtual ~b300_pcie_fifo() = default;
};

}}} // namespace uhd::usrp::b300
