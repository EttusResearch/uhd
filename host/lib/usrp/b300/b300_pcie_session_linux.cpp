// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "./inchworm/user/libnifpga-usrp/src/DeviceFile.h"
#include "./inchworm/user/libnifpga-usrp/src/Exception.h"
#include "./inchworm/user/libnifpga-usrp/src/SysfsFile.h"
#include "./inchworm/user/libnifpga-usrp/src/Timer.h"
#include "./inchworm/user/libnifpga-usrp/src/nib310rio.h"
#include "b300_regs.hpp"
#include <uhd/config.h>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/b300/b300_pcie_fifo.hpp>
#include <uhdlib/usrp/b300/b300_pcie_session.hpp>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <memory>
#include <mutex>

using namespace uhd::usrp::b300;

namespace {
// Error map for B300 PCIe session, similar to alreadyErrnoMap in Session.cpp
const class : public nirio::ErrnoMap
{
public:
    void throwErrno(const int error) const final
    {
        // Add any B300-specific error handling here if needed
        nirio::ErrnoMap::throwErrno(error);
    }
} b300ErrnoMap;

// FIFO info structure
struct fifo_info_t
{
    std::string name;
    uint32_t number;
    bool is_write;
};

// Define the available FIFOs (based on the kernel driver definition)
const std::vector<fifo_info_t> fifo_infos = {
    {"input_rfnoc_control", 0, false},
    {"input_streamer1", 1, false},
    {"input_streamer2", 2, false},
    {"input_streamer3", 3, false},
    {"input_streamer4", 4, false},
    {"output_rfnoc_control", 5, true},
    {"output_streamer1", 6, true},
    {"output_streamer2", 7, true},
    {"output_streamer3", 8, true},
    {"output_streamer4", 9, true},
};
} // namespace

class b300_pcie_session_impl : public b300_pcie_session
{
public:
    b300_pcie_session_impl(const std::string& device)
        : _device(device), _device_path(nirio::DeviceFile::getCdevPath(device))
    {
        _devfile = std::make_unique<nirio::DeviceFile>(
            _device_path, nirio::DeviceFile::ReadWrite, b300ErrnoMap);
        _devfile->mapMemory(B300_BAR0_MAP_SIZE);
    }

    ~b300_pcie_session_impl()
    {
        close(false);
    }

    void close(bool UHD_UNUSED(reset_if_last_session) = false) final
    {
        std::lock_guard<std::mutex> lock(_mutex);
        UHD_LOG_TRACE("B300", "Closing PCIe session");
        _devfile.reset();
        // Release all FIFOs
        for (auto& fifo_pair : _fifos) {
            try {
                fifo_pair.second->stop();
            } catch (const std::exception& ex) {
                UHD_LOG_WARNING("B300",
                    "Error stopping FIFO " << fifo_pair.first << ": " << ex.what());
            }
        }
        _fifos.clear();
    }

    b300_pcie_fifo::sptr create_rx_fifo(uint32_t fifo_num) final
    {
        return create_fifo(fifo_num, false);
    }

    b300_pcie_fifo::sptr create_tx_fifo(uint32_t fifo_num) final
    {
        return create_fifo(fifo_num, true);
    }

    std::string get_resource() const final
    {
        return _device_path;
    }

    std::vector<std::pair<uint32_t, std::pair<std::string, bool>>>
    get_fifo_info() const final
    {
        std::vector<std::pair<uint32_t, std::pair<std::string, bool>>> result;
        for (const auto& info : fifo_infos) {
            result.push_back({info.number, {info.name, info.is_write}});
        }
        return result;
    }

    uint32_t peek32(uint32_t addr) final
    {
        struct ioctl_nib310rio_reg32 xfer = {addr, 0};
        _devfile->ioctl(NIB310RIO_IOC_PEEK32, &xfer);
        return xfer.value;
    }

    void poke32(uint32_t addr, uint32_t value) final
    {
        struct ioctl_nib310rio_reg32 xfer = {addr, value};
        _devfile->ioctl(NIB310RIO_IOC_POKE32, &xfer);
    }

    uint64_t peek64(uint32_t addr) final
    {
        struct ioctl_nib310rio_reg64 xfer = {addr, 0};
        _devfile->ioctl(NIB310RIO_IOC_PEEK64, &xfer);
        return xfer.value;
    }

    void poke64(uint32_t addr, uint64_t value) final
    {
        struct ioctl_nib310rio_reg64 xfer = {addr, value};
        _devfile->ioctl(NIB310RIO_IOC_POKE64, &xfer);
    }

    void fifo_ioctl(UHD_UNUSED(uint32_t control_code), UHD_UNUSED(void* buffer)) final
    {
        // Linux implementation stub - not used since Linux uses nirio::Fifo directly
        UHD_LOG_WARNING(
            "B300", "fifo_ioctl called on Linux - this is a Windows-only method");
    }

    uint32_t get_session_count() final
    {
        auto session_count = nirio::SysfsFile(_device, "session_count").readU32();
        return session_count;
    }

private:
    b300_pcie_fifo::sptr create_fifo(uint32_t fifo_num, bool is_write)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // Check if FIFO already exists
        auto fifo_it = _fifos.find(fifo_num);
        if (fifo_it != _fifos.end()) {
            return fifo_it->second;
        }
        // Validate FIFO number and direction
        bool found = false;
        for (const auto& info : fifo_infos) {
            if (info.number == fifo_num) {
                if (info.is_write != is_write) {
                    UHD_LOG_THROW(uhd::runtime_error,
                        "B300",
                        "FIFO " + std::to_string(fifo_num)
                            + " direction mismatch: requested " + (is_write ? "TX" : "RX")
                            + " but FIFO is " + (info.is_write ? "TX" : "RX"));
                }
                found = true;
                break;
            }
        }
        if (!found) {
            UHD_LOG_THROW(uhd::runtime_error,
                "B300",
                "Invalid FIFO number: " + std::to_string(fifo_num));
        }
        // Create the FIFO
        try {
            auto fifo        = b300_pcie_fifo::make(fifo_num, _device, is_write);
            _fifos[fifo_num] = fifo;
            return fifo;
        } catch (const std::exception& ex) {
            UHD_LOG_THROW(uhd::runtime_error,
                "B300",
                std::string("Failed to create FIFO ") + std::to_string(fifo_num) + ": "
                    + ex.what());
        }
    }

    const std::string _device;
    std::string _device_path;
    std::unique_ptr<nirio::DeviceFile> _devfile;
    std::unordered_map<uint32_t, b300_pcie_fifo::sptr> _fifos;
    std::mutex _mutex;
};

b300_pcie_session::sptr b300_pcie_session::make(const std::string& device)
{
    return std::make_shared<b300_pcie_session_impl>(device);
}
