// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "./inchworm/user/libnifpga-usrp/src/DeviceFile.h"
#include "./inchworm/user/libnifpga-usrp/src/ErrnoMap.h"
#include "./inchworm/user/libnifpga-usrp/src/Fifo.h"
#include "./inchworm/user/libnifpga-usrp/src/FifoInfo.h"
#include "./inchworm/user/libnifpga-usrp/src/SysfsFile.h"
#include "./inchworm/user/libnifpga-usrp/src/Type.h"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/b300/b300_pcie_fifo.hpp>

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
    b300_pcie_fifo_impl(uint32_t fifo_num, const std::string& device, bool is_write)
        : _fifo_num(fifo_num)
        , _device(device)
        , _is_write(is_write)
        , _fifo_name(get_fifo_name(fifo_num, is_write))
    {
        UHD_LOG_TRACE("B300",
            "Creating " << (_is_write ? "TX" : "RX") << " FIFO " << _fifo_num << " ("
                        << _fifo_name << ")");

        // Create FifoInfo object for the underlying Fifo implementation
        nirio::FifoInfo fifo_info(_fifo_name, // name
            nirio::U64(), // type (U64)
            _fifo_num, // FIFO number
            0, // offset (not used)
            _is_write, // is_write
            _fifo_name // run-time name
        );

        // Create the actual Fifo object
        try {
            _fifo = std::make_unique<nirio::Fifo>(fifo_info, _device);
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to create FIFO " << _fifo_num << ": " << ex.what());
            throw uhd::runtime_error(std::string("Failed to create FIFO: ") + ex.what());
        }
    }

    ~b300_pcie_fifo_impl() override
    {
        try {
            if (_fifo) {
                _fifo->stop();
            }
        } catch (const std::exception& ex) {
            UHD_LOG_WARNING(
                "B300", "Error stopping FIFO " << _fifo_num << ": " << ex.what());
        }
    }

    size_t configure(size_t requested_depth) override
    {
        size_t actual_depth = 0;
        try {
            _fifo->configure(requested_depth, &actual_depth);
            UHD_LOG_DEBUG("B300",
                "Configured FIFO " << _fifo_num << " with depth " << actual_depth);
        } catch (const nirio::ExceptionBase& ex) {
            UHD_LOG_ERROR("B300",
                "NIRIO exception configuring FIFO " << _fifo_num
                                                    << ": error code: " << ex.getCode());
            throw uhd::runtime_error(
                std::string("Failed to configure FIFO (NIRIO), code: ")
                + std::to_string(ex.getCode()));
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to configure FIFO " << _fifo_num << ": " << ex.what());
            throw uhd::runtime_error(
                std::string("Failed to configure FIFO: ") + ex.what());
        }
        return actual_depth;
    }

    void start() override
    {
        try {
            _fifo->start();
            UHD_LOG_TRACE("B300", "Started FIFO " << _fifo_num);
        } catch (const nirio::ExceptionBase& ex) {
            UHD_LOG_ERROR("B300",
                "NIRIO exception starting FIFO " << _fifo_num
                                                 << ": error code: " << ex.getCode());
            throw uhd::runtime_error(std::string("Failed to start FIFO (NIRIO), code: ")
                                     + std::to_string(ex.getCode()));
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to start FIFO " << _fifo_num << ": " << ex.what());
            throw uhd::runtime_error(std::string("Failed to start FIFO: ") + ex.what());
        }
    }

    void stop() override
    {
        try {
            _fifo->stop();
            UHD_LOG_TRACE("B300", "Stopped FIFO " << _fifo_num);
        } catch (const nirio::ExceptionBase& ex) {
            UHD_LOG_ERROR("B300",
                "NIRIO exception stopping FIFO " << _fifo_num
                                                 << ": error code: " << ex.getCode());
            throw uhd::runtime_error(std::string("Failed to stop FIFO (NIRIO), code: ")
                                     + std::to_string(ex.getCode()));
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to stop FIFO " << _fifo_num << ": " << ex.what());
            throw uhd::runtime_error(std::string("Failed to stop FIFO: ") + ex.what());
        }
    }

    fifo_acquire_result acquire(
        uint64_t*& elements, size_t elements_requested, uint32_t timeout_ms) override
    {
        fifo_acquire_result result{0, 0};
        try {
            if (_is_write) {
                _fifo->acquire<nirio::U64, true>(elements,
                    elements_requested,
                    timeout_ms,
                    result.elements_acquired,
                    &result.elements_remaining);
            } else {
                _fifo->acquire<nirio::U64, false>(elements,
                    elements_requested,
                    timeout_ms,
                    result.elements_acquired,
                    &result.elements_remaining);
            }
        } catch (const nirio::FifoTimeoutException&) {
            throw; // throw FIFO Timeout, so the caller function can catch it.
        } catch (const nirio::ExceptionBase& ex) {
            throw uhd::runtime_error(
                std::string("Failed to acquire from FIFO " + std::to_string(_fifo_num)
                            + " (NIRIO), code: " + std::to_string(ex.getCode())));
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to acquire from FIFO " << _fifo_num << ": " << ex.what());
            throw uhd::runtime_error(
                std::string("Failed to acquire from FIFO: ") + ex.what());
        }
        return result;
    }

    void release(size_t elements) override
    {
        try {
            _fifo->release(elements);
        } catch (const nirio::ExceptionBase& ex) {
            UHD_LOG_ERROR("B300",
                "NIRIO exception releasing FIFO " << _fifo_num
                                                  << ": error code: " << ex.getCode());
            throw uhd::runtime_error(std::string("Failed to release FIFO (NIRIO), code: ")
                                     + std::to_string(ex.getCode()));
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR(
                "B300", "Failed to release FIFO " << _fifo_num << ": " << ex.what());
            throw uhd::runtime_error(std::string("Failed to release FIFO: ") + ex.what());
        }
    }

    uint32_t get_fifo_num() const override
    {
        return _fifo_num;
    }

    bool is_write_fifo() const override
    {
        return _is_write;
    }

    std::string get_name() const override
    {
        return _fifo_name;
    }

private:
    uint32_t _fifo_num;
    std::string _device;
    bool _is_write;
    std::string _fifo_name;
    std::unique_ptr<nirio::Fifo> _fifo;
};

b300_pcie_fifo::sptr b300_pcie_fifo::make(
    uint32_t fifo_num, const std::string& device, bool is_write)
{
    return std::make_shared<b300_pcie_fifo_impl>(fifo_num, device, is_write);
}
