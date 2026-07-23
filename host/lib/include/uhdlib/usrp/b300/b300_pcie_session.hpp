//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/usrp/b300/b300_pcie_fifo.hpp>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace b300 {

/**
 * Session implementation for B300 PCIe device.
 * This class manages the device connection and provides access to FIFOs
 * based on the inchworm reference implementation.
 */
class b300_pcie_session
{
public:
    using sptr = std::shared_ptr<b300_pcie_session>;

    /**
     * Create a new B300 PCIe session
     *
     * \param device_path Path to the PCIe device file
     */
    static sptr make(const std::string& device_path);

    /**
     * Close the session
     *
     * \param reset_if_last_session Reset the device if this is the last session
     */
    virtual void close(bool reset_if_last_session = false) = 0;

    /**
     * Create a FIFO for reading from device (target-to-host)
     *
     * \param fifo_num FIFO index number
     * \return Shared pointer to the created FIFO (throws on error)
     */
    virtual b300_pcie_fifo::sptr create_rx_fifo(uint32_t fifo_num) = 0;

    /**
     * Create a FIFO for writing to device (host-to-target)
     *
     * \param fifo_num FIFO index number
     * \return Shared pointer to the created FIFO (throws on error)
     */
    virtual b300_pcie_fifo::sptr create_tx_fifo(uint32_t fifo_num) = 0;

    /**
     * Get the device path
     */
    virtual std::string get_resource() const = 0;

    /**
     * Get FIFO metadata for initialization
     * Returns a map of FIFO indices to {name, is_write} pairs
     */
    virtual std::vector<std::pair<uint32_t, std::pair<std::string, bool>>>
    get_fifo_info() const = 0;

    /**
     * Peek (read) a 32-bit register from the device
     * \param addr Register address
     * \return 32-bit value
     */
    virtual uint32_t peek32(uint32_t addr) = 0;

    /**
     * Poke (write) a 32-bit register to the device
     * \param addr Register address
     * \param value 32-bit value to write
     */
    virtual void poke32(uint32_t addr, uint32_t value) = 0;

    /**
     * Peek (read) two consecutive 32-bit registers from the device
     * \param addr Register address
     * \return 64-bit value
     */
    virtual uint64_t peek64(uint32_t addr) = 0;

    /**
     * Poke (write) two consecutive 32-bit registers to the device
     * \param addr Register address
     * \param value 64-bit value to write
     */
    virtual void poke64(uint32_t addr, uint64_t value) = 0;

    /**
     * Perform FIFO IOCTL operation (Windows-specific)
     * \param control_code IOCTL control code
     * \param buffer Pointer to IOCTL buffer
     */
    virtual void fifo_ioctl(uint32_t control_code, void* buffer) = 0;

    /**
     * Get the number of active sessions for this device
     * \return number of active sessions
     */
    virtual uint32_t get_session_count() = 0;

protected:
    // File descriptor for device access (implement in derived class)
    int _fd = -1;

    /**
     * Destructor
     */
    virtual ~b300_pcie_session() = default;
};

}}} // namespace uhd::usrp::b300
