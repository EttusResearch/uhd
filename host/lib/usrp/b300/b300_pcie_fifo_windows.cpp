// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "inchworm/user/libnifpga-usrp/src/DmaBuf_win32.h"
#include "inchworm/user/libnifpga-usrp/src/tInterfaceIoctl_B310.h"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/b300/b300_pcie_fifo.hpp>
#include <uhdlib/usrp/b300/b300_pcie_session.hpp>
#include <algorithm>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace uhd::usrp::b300;

namespace {
// Define a mapping from Fifo indices to human-readable names
std::string get_fifo_name(uint32_t fifo_num, bool is_write)
{
    static const std::vector<std::string> fifo_names = {
        "input_rfnoc_control", // 0
        "input_streamer1", // 1
        "input_streamer2", // 2
        "input_streamer3", // 3
        "input_streamer4", // 4
        "output_rfnoc_control", // 5
        "output_streamer1", // 6
        "output_streamer2", // 7
        "output_streamer3", // 8
        "output_streamer4", // 9
    };

    const size_t index = fifo_num;
    if (index < fifo_names.size()) {
        return fifo_names[index];
    } else {
        return (is_write ? "output_" : "input_") + std::to_string(fifo_num);
    }
}
} // namespace

class b300_pcie_fifo_impl : public b300_pcie_fifo
{
public:
    b300_pcie_fifo_impl(uint32_t fifo_num,
        const std::string& device,
        bool is_write,
        b300_pcie_session* session = nullptr)
        : _fifo_num(fifo_num)
        , _device(device)
        , _is_write(is_write)
        , _stopped(true)
        , _fifo_name(get_fifo_name(fifo_num, is_write))
        , _configured_depth(0)
        , _dma_ptr(nullptr)
        , _acquired(0)
        , _next(0)
        , _session(session)
    {
        UHD_LOG_TRACE("B300",
            "Creating " << (_is_write ? "TX" : "RX") << " FIFO " << _fifo_num << " ("
                        << _fifo_name << ")");
    }

    ~b300_pcie_fifo_impl() final
    {
        UHD_LOG_TRACE(
            "B300", "Destroying FIFO " << _fifo_num << " (" << _fifo_name << ")");
        try {
            stop();
            UHD_LOG_TRACE("B300",
                "Successfully called stop() in destructor for FIFO " << _fifo_num);
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR("B300",
                "Exception in destructor stop() for FIFO " << _fifo_num << ": "
                                                           << ex.what());
        }
    }

    size_t configure(size_t requested_depth) final
    {
        const std::lock_guard<std::recursive_mutex> lock(_mutex);
        try {
            // Allocate DMA buffer for FIFO operations
            size_t buffer_size =
                requested_depth * sizeof(uint64_t); // Assume 8-byte elements

            // Validate buffer size fits in uint32_t to prevent truncation
            if (buffer_size > std::numeric_limits<uint32_t>::max()) {
                throw uhd::runtime_error(
                    "Buffer size (" + std::to_string(buffer_size)
                    + ") exceeds maximum supported size ("
                    + std::to_string(std::numeric_limits<uint32_t>::max()) + ")");
            }

            // Validate that buffer size is page-aligned (4096 bytes) for kernel
            // compatibility
            if (buffer_size % 4096 != 0) {
                // Round up to next page boundary
                buffer_size = ((buffer_size + 4095) / 4096) * 4096;
                UHD_LOG_DEBUG("B300",
                    "Rounded FIFO buffer size up to page boundary: "
                        << buffer_size << " bytes (" << (buffer_size / 8)
                        << " elements) for FIFO " << _fifo_num);
            }

            _dma_buffer =
                std::unique_ptr<nirio::DmaBuf>(nirio::DmaBuf::allocate(buffer_size));

            // Use B310 FIFO setBuf IOCTL to attach the DMA buffer to the FIFO
            struct
            {
                tIn_b310Win_fifoSetBuf input;
                tOut_b310Win_fifoSetBuf output;
            } ioctl_buffer = {};

            ioctl_buffer.input.channel = _fifo_num;
            ioctl_buffer.input.buffer  = reinterpret_cast<tAlignedU64>(
                const_cast<void*>(_dma_buffer->getPointer()));
            ioctl_buffer.input.fifoSizeBytes = static_cast<uint32_t>(buffer_size);
            ioctl_buffer.input.status        = 0;

            _session->fifo_ioctl(B310_WIN_IOC_FIFO_SET_BUF, &ioctl_buffer);

            // Check output status
            if (ioctl_buffer.output.status != 0) {
                UHD_LOG_ERROR("B300",
                    "FIFO setBuf IOCTL failed with status: "
                        << ioctl_buffer.output.status << " for FIFO " << _fifo_num << " ("
                        << _fifo_name << ")");
                throw uhd::runtime_error("FIFO setBuf IOCTL failed with status: "
                                         + std::to_string(ioctl_buffer.output.status));
            }

            UHD_LOG_TRACE("B300",
                "Attached DMA buffer (" << buffer_size << " bytes) to FIFO " << _fifo_num
                                        << " via B310 IOCTL");

            // Set direct pointer to DMA buffer for element access
            _dma_ptr = const_cast<void*>(_dma_buffer->getPointer());

            // Calculate actual configured depth based on allocated buffer size
            _configured_depth = buffer_size / sizeof(uint64_t);
            // Reset bookkeeping when reconfiguring
            _acquired = 0;
            _next     = 0;
            UHD_LOG_DEBUG("B300",
                "Configured FIFO " << _fifo_num << " with depth " << _configured_depth
                                   << " (requested: " << requested_depth
                                   << ", allocated: " << buffer_size << " bytes)");
            return _configured_depth;
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to configure FIFO " << _fifo_num << ": " << ex.what());
            throw uhd::runtime_error(
                "Failed to configure FIFO: " + std::string(ex.what()));
        }
    }

    void start() final
    {
        const std::lock_guard<std::recursive_mutex> lock(_mutex);
        try {
            UHD_LOG_TRACE("B300",
                "Starting FIFO " << _fifo_num << " (" << _fifo_name << ") - "
                                 << (_is_write ? "TX" : "RX") << " direction");

            // Create combined input+output buffer for B310 FIFO start IOCTL
            struct
            {
                tIn_B310_fifoStart input;
                tOut_B310_fifoStart output;
            } ioctl_buffer = {};

            ioctl_buffer.input.channel = _fifo_num;
            ioctl_buffer.input.status  = 0;

            UHD_LOG_TRACE("B300", "Calling fifoStart IOCTL for channel " << _fifo_num);
            _session->fifo_ioctl(B310_WIN_IOC_FIFO_START, &ioctl_buffer);

            // Check output status and provide more detailed error information
            if (ioctl_buffer.output.status != 0) {
                UHD_LOG_ERROR("B300",
                    "FIFO start IOCTL failed for FIFO "
                        << _fifo_num << " (" << _fifo_name
                        << ") with status: " << ioctl_buffer.output.status << " (0x"
                        << std::hex << ioctl_buffer.output.status << std::dec << ")");
                throw uhd::runtime_error("FIFO start IOCTL failed with status: "
                                         + std::to_string(ioctl_buffer.output.status));
            }

            UHD_LOG_TRACE("B300",
                "Started FIFO " << _fifo_num << " (" << _fifo_name << ") via IOCTL");
            _stopped = false;

            // Reset bookkeeping when starting FIFO
            _acquired = 0;
            _next     = 0;

        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to start FIFO " << _fifo_num << ": " << ex.what());
            throw uhd::runtime_error("Failed to start FIFO: " + std::string(ex.what()));
        }
    }

    void stop() final
    {
        const std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (_stopped) {
            UHD_LOG_DEBUG("B300",
                "FIFO " << _fifo_num << " (" << _fifo_name << ") is already stopped.");
            return;
        }
        try {
            // Create combined input+output buffer for B310 FIFO stop IOCTL
            struct
            {
                tIn_b310Win_fifoStop input;
                tOut_b310Win_fifoStop output;
            } ioctl_buffer = {};

            ioctl_buffer.input.channel = _fifo_num;
            ioctl_buffer.input.status  = 0;

            _session->fifo_ioctl(B310_WIN_IOC_FIFO_STOP, &ioctl_buffer);

            // Check output status (but don't throw in stop - just log)
            if (ioctl_buffer.output.status != 0) {
                UHD_LOG_WARNING("B300",
                    "FIFO stop IOCTL failed with status: " << ioctl_buffer.output.status);
            } else {
                UHD_LOG_TRACE("B300",
                    "Stopped FIFO " << _fifo_num << " (" << _fifo_name << ") via IOCTL");
                _stopped = true;
            }

            // Reset bookkeeping when stopping FIFO
            _acquired = 0;
            _next     = 0;

        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to stop FIFO " << _fifo_num << ": " << ex.what());
            // Don't throw in destructor path - just log the error
        }
    }

    fifo_acquire_result acquire(
        uint64_t*& elements, size_t elements_requested, uint32_t timeout_ms) final
    {
        const std::lock_guard<std::recursive_mutex> lock(_mutex);
        UHD_LOG_TRACE("B300",
            "Acquiring from FIFO " << _fifo_num << ": requested " << elements_requested
                                   << " elements with timeout " << timeout_ms << " ms");

        fifo_acquire_result result{0, 0};

        try {
            if (elements_requested > 0) {
                // Validate we don't exceed FIFO depth or overrun existing acquisitions
                if (elements_requested > _configured_depth) {
                    throw uhd::runtime_error("Requested elements ("
                                             + std::to_string(elements_requested)
                                             + ") exceeds FIFO depth ("
                                             + std::to_string(_configured_depth) + ")");
                }

                // Handle circular buffer wraparound - limit request to contiguous space
                size_t contiguous_space = (_next + elements_requested > _configured_depth)
                                              ? (_configured_depth - _next)
                                              : elements_requested;

                // Check that we don't exceed available space due to unreleased elements
                if (contiguous_space + _acquired > _configured_depth) {
                    throw uhd::runtime_error("Cannot acquire "
                                             + std::to_string(contiguous_space)
                                             + " elements: " + std::to_string(_acquired)
                                             + " elements already acquired");
                }

                struct
                {
                    tIn_b310Win_fifoAcquire input;
                    tOut_b310Win_fifoAcquire output;
                } ioctl_buffer;

                ioctl_buffer.input.channel   = _fifo_num;
                ioctl_buffer.input.timeoutMs = timeout_ms;
                ioctl_buffer.input.elements  = elements_requested;
                ioctl_buffer.input.available = 0;
                ioctl_buffer.input.timedOut  = 0;
                ioctl_buffer.input.status    = 0;

                // Clear output
                ioctl_buffer.output.available = 0;
                ioctl_buffer.output.timedOut  = 0;
                ioctl_buffer.output.status    = 0;

                _session->fifo_ioctl(B310_WIN_IOC_FIFO_ACQUIRE, &ioctl_buffer);

                UHD_LOG_TRACE("B300",
                    "IOCTL results: "
                        << "available " << ioctl_buffer.output.available << ", timed out "
                        << ioctl_buffer.output.timedOut << ", status "
                        << ioctl_buffer.output.status);

                // Process IOCTL results
                size_t available = static_cast<size_t>(ioctl_buffer.output.available);
                size_t actually_acquired = std::min(contiguous_space, elements_requested);
                // If the acquisition timed out then actually_acquired should be 0
                if (ioctl_buffer.output.timedOut) {
                    actually_acquired = 0;
                    // Throw niiro timeout warning here
                    UHD_LOG_TRACE(
                        "B300", "FIFO " << _fifo_num << " acquisition timed out");
                }

                UHD_LOG_TRACE("B300",
                    "IOCTL processed results: "
                        << "available " << available << ", actually acquired "
                        << actually_acquired);

                if (actually_acquired > 0 && _dma_ptr) {
                    // Return direct pointer into DMA buffer (Linux NIRIO pattern)
                    elements = &static_cast<uint64_t*>(_dma_ptr)[_next];

                    // Update bookkeeping
                    _acquired += actually_acquired;
                    _next += actually_acquired;
                    if (_next >= _configured_depth) {
                        _next = 0; // Circular wraparound
                    }
                } else {
                    // No elements available - return valid DMA buffer pointer but with 0
                    // elements This prevents null pointer crashes in CHDR packet writer
                    if (_dma_ptr) {
                        elements = &static_cast<uint64_t*>(_dma_ptr)[_next];
                    } else {
                        elements = nullptr;
                        UHD_LOG_WARNING("B300",
                            "FIFO " << _fifo_num << " has no DMA buffer configured");
                    }
                }

                result.elements_acquired  = actually_acquired;
                result.elements_remaining = available;

                UHD_LOG_TRACE("B300",
                    "FIFO acquire: acquired "
                        << result.elements_acquired << ", available "
                        << result.elements_remaining << ", timed out "
                        << ioctl_buffer.output.timedOut
                        << ", bookkeeping: acquired=" << _acquired << ", next=" << _next);
            }
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to acquire from FIFO " << _fifo_num << ": " << ex.what());
            throw uhd::runtime_error(
                "Failed to acquire from FIFO: " + std::string(ex.what()));
        }

        return result;
    }

    void release(size_t elements) final
    {
        const std::lock_guard<std::recursive_mutex> lock(_mutex);
        try {
            // Validate release parameters
            if (elements > _acquired) {
                UHD_LOG_WARNING("B300",
                    "Attempting to release " << elements << " elements but only "
                                             << _acquired << " are acquired");
                elements = _acquired; // Clamp to what's actually acquired
            }

            struct
            {
                tIn_b310Win_fifoRelease input;
                tOut_b310Win_fifoRelease output;
            } ioctl_buffer = {};

            ioctl_buffer.input.channel  = _fifo_num;
            ioctl_buffer.input.elements = elements;
            ioctl_buffer.input.status   = 0;

            // Clear output structure
            ioctl_buffer.output.status = 0;

            _session->fifo_ioctl(B310_WIN_IOC_FIFO_RELEASE, &ioctl_buffer);

            // Check output status for errors
            if (ioctl_buffer.output.status != 0) {
                UHD_LOG_ERROR("B300",
                    "FIFO release IOCTL failed for FIFO "
                        << _fifo_num << " with status: " << ioctl_buffer.output.status);
                throw uhd::runtime_error("FIFO release IOCTL failed with status: "
                                         + std::to_string(ioctl_buffer.output.status));
            }

            // Update bookkeeping
            _acquired -= elements;

            UHD_LOG_TRACE("B300",
                "Released " << elements << " elements from FIFO " << _fifo_num
                            << " via IOCTL (acquired=" << _acquired << ")");
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to release FIFO " << _fifo_num << ": " << ex.what());
            // Don't throw in release - just log the error
        }
    }

    uint32_t get_fifo_num() const final
    {
        return _fifo_num;
    }

    bool is_write_fifo() const final
    {
        return _is_write;
    }

    std::string get_name() const final
    {
        return _fifo_name;
    }

private:
    uint32_t _fifo_num;
    std::string _device;
    bool _is_write;
    bool _stopped;
    std::string _fifo_name;
    size_t _configured_depth;

    // DMA buffer management (matching Linux NIRIO pattern)
    std::unique_ptr<nirio::DmaBuf> _dma_buffer; // DMA buffer for FIFO operations
    void* _dma_ptr; // Direct pointer to DMA buffer memory

    // FIFO element bookkeeping (matching Linux NIRIO pattern)
    size_t _acquired; // Currently acquired elements
    size_t _next; // Next element offset (circular buffer)

    // Thread synchronization (matching Linux NIRIO pattern)
    mutable std::recursive_mutex _mutex; // Lock to serialize access

    b300_pcie_session* _session; // Session for B310 IOCTL access
};

b300_pcie_fifo::sptr b300_pcie_fifo::make(
    uint32_t fifo_num, const std::string& device, bool is_write)
{
    return std::make_shared<b300_pcie_fifo_impl>(fifo_num, device, is_write);
}

// Extended factory function for Windows B310 integration
namespace uhd { namespace usrp { namespace b300 {
b300_pcie_fifo::sptr b300_pcie_fifo_make_with_session(uint32_t fifo_num,
    const std::string& device,
    bool is_write,
    b300_pcie_session* session)
{
    return std::make_shared<b300_pcie_fifo_impl>(fifo_num, device, is_write, session);
}
}}} // namespace uhd::usrp::b300
