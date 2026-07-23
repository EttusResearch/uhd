// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.h>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/b300/b300_pcie_fifo.hpp>
#include <uhdlib/usrp/b300/b300_pcie_session.hpp>
#include <algorithm>

// Forward declaration for extended FIFO factory
namespace uhd { namespace usrp { namespace b300 {
b300_pcie_fifo::sptr b300_pcie_fifo_make_with_session(uint32_t fifo_num,
    const std::string& device,
    bool is_write,
    b300_pcie_session* session);
}}} // namespace uhd::usrp::b300
#include "inchworm/user/libnifpga-usrp/src/DeviceFile_win32.h"
#include "inchworm/user/libnifpga-usrp/src/Exception.h"
#include "inchworm/user/libnifpga-usrp/src/tInterfaceIoctl_B310.h"
#include <cfgmgr32.h>
#include <fcntl.h>
#include <setupapi.h>
#include <system_error>
#include <windows.h>
#include <cerrno>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>

using namespace uhd::usrp::b300;

// B310 kernel driver interface GUID.
// {EED3AE96-96B3-4180-8B13-CA88C146FF00}
static const GUID B310_WIN_DRIVER_GUID = {
    0xEED3AE96, 0x96B3, 0x4180, {0x8B, 0x13, 0xCA, 0x88, 0xC1, 0x46, 0xFF, 0x00}};

namespace {
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

std::string find_device_path(const std::string& pci_location)
{
    SP_DEVICE_INTERFACE_DATA device_interface_data;
    DWORD required_size     = 0;
    std::string device_path = "";

    // Create a unique_ptr for HDEVINFO with custom deleter to ensure cleanup
    std::unique_ptr<void, void (*)(HDEVINFO)> device_info_set(
        SetupDiGetClassDevsA(&B310_WIN_DRIVER_GUID,
            nullptr,
            nullptr,
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE),
        [](HDEVINFO h) {
            if (h != INVALID_HANDLE_VALUE)
                SetupDiDestroyDeviceInfoList(h);
        });

    if (device_info_set.get() == INVALID_HANDLE_VALUE) {
        UHD_LOG_DEBUG("B300", "SetupDiGetClassDevs failed, error: " << GetLastError());
        return "";
    }

    // Enumerate device interfaces
    device_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD device_index = 0;; device_index++) {
        SP_DEVINFO_DATA device_info_data;
        device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
        device_path             = "";

        if (!SetupDiEnumDeviceInterfaces(device_info_set.get(),
                nullptr,
                &B310_WIN_DRIVER_GUID,
                device_index,
                &device_interface_data)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                break; // No more devices
            }
            continue; // Try next device
        }

        // Get the required size for device interface detail
        SetupDiGetDeviceInterfaceDetailA(device_info_set.get(),
            &device_interface_data,
            nullptr,
            0,
            &required_size,
            nullptr);

        // Allocate memory using unique_ptr with custom deleter
        auto device_interface_detail =
            std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA_A, decltype(&free)>(
                static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_A>(malloc(required_size)),
                &free);

        if (!device_interface_detail) {
            continue;
        }

        device_interface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

        // Get device interface detail
        if (SetupDiGetDeviceInterfaceDetailA(device_info_set.get(),
                &device_interface_data,
                device_interface_detail.get(),
                required_size,
                nullptr,
                &device_info_data)) {
            // Found a device interface, extract the path
            device_path = std::string(device_interface_detail->DevicePath);
            UHD_LOG_TRACE("B300", "Found device interface: " << device_path);

            // Get the PCI location of this device
            DEVINST dev_inst   = device_info_data.DevInst;
            ULONG bus_number   = 0;
            ULONG bus_num_size = sizeof(bus_number);
            std::string dev_pci_location;

            // Get bus number
            if (CM_Get_DevNode_Registry_PropertyA(
                    dev_inst, CM_DRP_BUSNUMBER, nullptr, &bus_number, &bus_num_size, 0)
                == CR_SUCCESS) {
                // Get address (device.function)
                ULONG address      = 0;
                ULONG address_size = sizeof(address);

                if (CM_Get_DevNode_Registry_PropertyA(
                        dev_inst, CM_DRP_ADDRESS, nullptr, &address, &address_size, 0)
                    == CR_SUCCESS) {
                    // Extract device and function from address
                    uint8_t device   = static_cast<uint8_t>((address >> 16) & 0x1F);
                    uint8_t function = static_cast<uint8_t>(address & 0x7);

                    // Format as decimal PCIe identifier: bus:device:function
                    dev_pci_location = std::to_string(bus_number) + ":"
                                       + std::to_string(device) + ":"
                                       + std::to_string(function);

                    // Check if this device matches the requested PCI location
                    if (dev_pci_location == pci_location) {
                        UHD_LOG_TRACE(
                            "B300", "Matched device at PCI location: " << pci_location);
                        break;
                    } else {
                        UHD_LOG_DEBUG("B300",
                            "Found Device PCI location: " << dev_pci_location
                                                          << ", but requested: "
                                                          << pci_location);
                    }
                }
            }
        }
    }
    return device_path;
}

} // namespace

class b300_pcie_session_impl : public b300_pcie_session
{
public:
    b300_pcie_session_impl(const std::string& device)
        : _device(device), _device_path(), _closing(false)
    {
        UHD_LOG_TRACE("B300", "Creating B310 PCIe session for device: " << device);

        // Extract PCI location from device string (e.g., "101:0:0" from device discovery)
        std::string pci_location = device;
        if (device.find("b300_pcie") == 0 && device.length() > 9) {
            // If device string is like "b300_pcie101:0:0", extract the PCI location
            pci_location = device.substr(9); // Skip "b300_pcie" prefix
        }

        UHD_LOG_TRACE("B300",
            "Looking for B310 device interface for PCI location: " << pci_location);

        try {
            // Find the device interface path
            _device_path = find_device_path(pci_location);

            if (_device_path.empty()) {
                throw uhd::runtime_error("No B310 device interface found. "
                                         "Verify that the b310k driver is "
                                         "installed and the B310 device is detected.");
            }

            UHD_LOG_TRACE("B300", "B310 device path: " << _device_path);

            // Open the device using the B310 device path
            // Note: FILE_FLAG_OVERLAPPED is required for proper B310 IOCTL operations
            // (matches Python implementation)
            HANDLE deviceHandle = CreateFileA(_device_path.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                nullptr);

            if (deviceHandle == INVALID_HANDLE_VALUE) {
                DWORD error           = GetLastError();
                std::string error_msg = "Failed to open device interface: " + _device_path
                                        + ". Windows error: " + std::to_string(error);
                if (error == ERROR_FILE_NOT_FOUND) {
                    error_msg += " (Device interface not accessible)";
                } else if (error == ERROR_ACCESS_DENIED) {
                    error_msg += " (Access denied - try running as administrator)";
                }
                throw uhd::runtime_error(error_msg);
            }

            // Create DeviceFile wrapper around the Windows handle
            _devfile = std::make_unique<nirio::DeviceFile>(
                deviceHandle, nirio::DeviceFile::ReadWrite);

            UHD_LOG_INFO("B300", "Successfully opened device: " << _device_path);
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR("B300",
                "Failed to open B310 PCIe device " << device << ": " << ex.what());
            throw uhd::runtime_error(
                "Failed to open B310 PCIe device: " + std::string(ex.what()));
        }
    }

    ~b300_pcie_session_impl()
    {
        close(false);
    }

    void close(bool UHD_UNUSED(reset_if_last_session) = false) final
    {
        // Move FIFO objects out while holding the lock, then destroy them after
        // releasing the lock to avoid re-entering _mutex from FIFO destructors.
        std::unordered_map<uint32_t, b300_pcie_fifo::sptr> fifos_to_destroy;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_closing) {
                UHD_LOG_DEBUG("B300", "B310 session is already closing.");
                return;
            }
            _fifos.swap(fifos_to_destroy);
            _closing = true; // Mark session as closing to prevent new FIFO creation
        }

        // Destructors can call back into session->fifo_ioctl(), which acquires _mutex.
        // Keep this outside the lock to prevent self-deadlock.
        fifos_to_destroy.clear();

        // Close the device file after FIFO shutdown has completed.
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_devfile) {
                UHD_LOG_TRACE("B300", "Closing B310 PCIe device: " << _device_path);
                _devfile.reset();
            }
        }

        UHD_LOG_TRACE("B300", "B310 PCIe session closed for device: " << _device);
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

        // Convert fifo_infos to the expected format
        for (const auto& fifo : fifo_infos) {
            result.emplace_back(fifo.number, std::make_pair(fifo.name, fifo.is_write));
        }

        return result;
    }

    uint32_t peek32(uint32_t addr) final
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_devfile) {
            throw uhd::runtime_error("B310 device not initialized");
        }

        try {
            struct
            {
                tIn_B310_read32 input;
                tOut_B310_read32 output;
            } ioctl_buffer;

            ioctl_buffer.input.offset = addr;
            ioctl_buffer.output.value = 0; // Clear output

            _devfile->ioctl(B310_WIN_IOC_READ32, &ioctl_buffer);

            uint32_t result = ioctl_buffer.output.value;

            return result;
        } catch (const nirio::HardwareFaultException& ex) {
            UHD_LOG_ERROR("B300",
                "Hardware fault during peek32 at address 0x"
                    << std::hex << addr << " (code: " << ex.getCode() << ")");
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error(
                "B310 hardware fault during register read at " + addr_str.str());
        } catch (const nirio::SoftwareFaultException& ex) {
            UHD_LOG_ERROR("B300",
                "Software fault during peek32 at address 0x"
                    << std::hex << addr << " (code: " << ex.getCode() << ")");
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error(
                "B310 software fault during register read at " + addr_str.str());
        } catch (const nirio::InvalidParameterException& ex) {
            UHD_LOG_ERROR("B300",
                "Invalid parameter during peek32 at address 0x"
                    << std::hex << addr << " (code: " << ex.getCode() << ")");
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error(
                "B310 invalid parameter during register read at " + addr_str.str());
        } catch (const nirio::CommunicationTimeoutException& ex) {
            UHD_LOG_ERROR("B300",
                "Communication timeout during peek32 at address 0x"
                    << std::hex << addr << " (code: " << ex.getCode() << ")");
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error(
                "B310 communication timeout during register read at " + addr_str.str());
        } catch (const nirio::Exception<-50400>& ex) { // Generic NIRIO exception base
            UHD_LOG_ERROR("B300",
                "NIRIO exception during peek32 at address 0x"
                    << std::hex << addr << " (code: " << ex.getCode() << ")");
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error("B310 NIRIO error during register read at "
                                     + addr_str.str()
                                     + " (code: " + std::to_string(ex.getCode()) + ")");
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR("B300",
                "Standard exception during peek32 at address 0x" << std::hex << addr
                                                                 << ": " << ex.what());
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error("B310 register read failed at " + addr_str.str()
                                     + ": " + std::string(ex.what()));
        } catch (...) {
            UHD_LOG_ERROR("B300",
                "Unknown exception during peek32 at address 0x" << std::hex << addr);
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error(
                "B310 register read failed at " + addr_str.str() + ": Unknown exception");
        }
    }

    void poke32(uint32_t addr, uint32_t data) final
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_devfile) {
            throw uhd::runtime_error("B310 device not initialized");
        }

        try {
            tIn_B310_write32 input_data;
            input_data.offset = addr;
            input_data.value  = data;

            _devfile->ioctl(B310_WIN_IOC_WRITE32, &input_data);

        } catch (const nirio::ExceptionBase& ex) {
            if (ex.getCode() == -50 || ex.getCode() == 50 || ex.getCode() == 0x32) {
                UHD_LOG_WARNING("B300",
                    "poke32 not supported by B310 driver at address 0x"
                        << std::hex << addr
                        << " - this may be a read-only device. Continuing...");
                return; // Don't fail initialization for unsupported writes
            }
            UHD_LOG_ERROR("B300",
                "poke32 failed at address 0x" << std::hex << addr << " with data 0x"
                                              << data << ": NIRIO exception code "
                                              << ex.getCode());
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error("B310 register write failed at " + addr_str.str()
                                     + ": NIRIO error " + std::to_string(ex.getCode()));
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR("B300",
                "poke32 failed at address 0x" << std::hex << addr << " with data 0x"
                                              << data << ": " << ex.what());
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error("B310 register write failed at " + addr_str.str()
                                     + ": " + std::string(ex.what()));
        } catch (...) {
            UHD_LOG_ERROR("B300",
                "poke32 failed at address 0x" << std::hex << addr << " with data 0x"
                                              << data << ": Unknown exception type");
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << addr;
            throw uhd::runtime_error("B310 register write failed at " + addr_str.str()
                                     + ": Unknown exception type");
        }
    }

    uint64_t peek64(uint32_t addr) final
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_devfile) {
            throw uhd::runtime_error("B310 device not initialized");
        }

        try {
            tIn_B310_read64 input_data;
            input_data.offset = addr;

            struct
            {
                tIn_B310_read64 input;
                tOut_B310_read64 output;
            } ioctl_buffer;

            ioctl_buffer.input = input_data;

            _devfile->ioctl(B310_WIN_IOC_READ64, &ioctl_buffer);

            // Return the data read
            return ioctl_buffer.output.value;

        } catch (const std::system_error& ex) {
            UHD_LOG_ERROR("B300",
                "peek64 failed at address 0x" << std::hex << addr << ": " << ex.what());
            throw uhd::runtime_error(
                "B310 register read failed: " + std::string(ex.what()));
        }
    }

    void poke64(uint32_t addr, uint64_t data) final
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_devfile) {
            throw uhd::runtime_error("B310 device not initialized");
        }

        try {
            // Prepare B310 write64 structures
            tIn_B310_write64 input_data;
            input_data.offset = addr;
            input_data.value  = data;

            // Perform IOCTL call using proper B310 write64 constant
            _devfile->ioctl(B310_WIN_IOC_WRITE64, &input_data);

        } catch (const std::system_error& ex) {
            UHD_LOG_ERROR("B300",
                "poke64 failed at address 0x" << std::hex << addr << " with data 0x"
                                              << data << ": " << ex.what());
            throw uhd::runtime_error(
                "B310 register write failed: " + std::string(ex.what()));
        }
    }

    b300_pcie_fifo::sptr create_fifo(uint32_t fifo_num, bool is_write)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_closing) {
            throw uhd::runtime_error("B310 session is closing, cannot create new FIFO");
        }

        // Check if the FIFO already exists
        auto it = _fifos.find(fifo_num);
        if (it != _fifos.end()) {
            return it->second;
        }

        // Validate FIFO number and direction
        auto fifo_it = std::find_if(fifo_infos.begin(),
            fifo_infos.end(),
            [fifo_num, is_write](const fifo_info_t& fifo) {
                return fifo.number == fifo_num && fifo.is_write == is_write;
            });

        if (fifo_it == fifo_infos.end()) {
            UHD_LOG_THROW(uhd::value_error,
                "B300",
                "Invalid FIFO number: " + std::to_string(fifo_num));
        }

        // Create the FIFO
        try {
            // Use extended factory to pass session reference for B310 IOCTL access
            auto fifo =
                b300_pcie_fifo_make_with_session(fifo_num, _device, is_write, this);
            _fifos[fifo_num] = fifo;
            UHD_LOG_TRACE("B300",
                "Successfully created " << (is_write ? "TX" : "RX") << " FIFO "
                                        << fifo_num << " (" << fifo_it->name
                                        << ") with B310 session");
            return fifo;
        } catch (const std::exception& ex) {
            UHD_LOG_THROW(uhd::runtime_error,
                "B300",
                std::string("Failed to create FIFO ") + std::to_string(fifo_num) + ": "
                    + ex.what());
        }
    }

    // Public method to allow FIFOs to perform IOCTLs
    void fifo_ioctl(uint32_t control_code, void* buffer)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_devfile) {
            throw uhd::runtime_error("B310 device not initialized for FIFO IOCTL");
        }

        _devfile->ioctl(control_code, buffer);
    }

    uint32_t get_session_count() final
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_devfile) {
            throw uhd::runtime_error("B310 device not initialized for session count");
        }

        try {
            tOut_B310_get_session_count output{};

            _devfile->ioctl(B310_WIN_IOC_GET_SESSION_COUNT, &output);

            return output.count;
        } catch (const std::exception& ex) {
            UHD_LOG_ERROR("B300", "Failed to get session count: " << ex.what());
            throw uhd::runtime_error(
                "B310 get_session_count failed: " + std::string(ex.what()));
        }
    }

private:
    std::string _device;
    std::string _device_path;
    std::unique_ptr<nirio::DeviceFile> _devfile;
    std::mutex _mutex;
    bool _closing;

    // Store FIFO objects
    std::unordered_map<uint32_t, b300_pcie_fifo::sptr> _fifos;
};

//
// Factory functions
//
b300_pcie_session::sptr b300_pcie_session::make(const std::string& device)
{
    return std::make_shared<b300_pcie_session_impl>(device);
}
