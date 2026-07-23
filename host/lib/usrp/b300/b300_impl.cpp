//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_impl.hpp"
#include "b300_bar0_spi_core.hpp"
#include "b300_clock_ctrl.hpp"
#include "b300_gps_control.hpp"
#include "b300_mb_controller.hpp"
#include "b300_mb_eeprom.hpp"
#include "b300_regs.hpp"
#include <uhd/device.hpp>
#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/rfnoc/device_id.hpp>
#include <uhdlib/usrp/cores/i2c_core_100_wb32.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>

// clang will put windows.h below others and cause compile errors.
// clang-format off
#ifdef _WIN32
#    include <windows.h>
#    include <cfgmgr32.h>
#    include <setupapi.h>
#elif defined(__linux__)
#    include "inchworm/user/libnifpga-usrp/src/SysfsFile.h"
#endif
// clang-format on

namespace uhd { namespace usrp { namespace b300 {
device_addrs_t b300_find(const device_addr_t& hint)
{
    device_addrs_t found_devices;

    if ((hint.has_key("type") and hint["type"] != "b3xx") || hint.has_key("addr")) {
        return found_devices;
    }

#ifdef __linux__
    namespace fs = boost::filesystem;
    const fs::path nirio_driver_dir("/sys/bus/pci/drivers/nib310rio");

    if (fs::exists(nirio_driver_dir) && fs::is_directory(nirio_driver_dir)) {
        for (const auto& entry : fs::directory_iterator(nirio_driver_dir)) {
            if (!is_directory(entry.path()))
                continue;
            // Skip known non-device entries
            std::string dirname = entry.path().filename().string();
            if (dirname == "bind" || dirname == "unbind" || dirname == "module"
                || dirname == "uevent")
                continue;

            fs::path nirio_subdir = entry.path() / "nib310rio";
            if (fs::exists(nirio_subdir) && fs::is_directory(nirio_subdir)) {
                // There should only be one subentry, which is the name of the device
                for (const auto& subentry : fs::directory_iterator(nirio_subdir)) {
                    if (is_directory(subentry.path())) {
                        device_addr_t new_addr;
                        new_addr["type"]     = "b3xx";
                        new_addr["resource"] = subentry.path().filename().string();
                        // Make a dummy tree so that we can make a pcie manager to read
                        // the EEPROM
                        uhd::property_tree::sptr dummy_tree;
                        wb_iface::sptr pcie_manager = std::make_shared<b300_pcie_manager>(
                            new_addr["resource"], dummy_tree, "");

                        auto session_count =
                            nirio::SysfsFile(new_addr["resource"], "session_count")
                                .readU32();
                        new_addr["claimed"] = (session_count > 1) ? "True" : "False";

                        auto tb_i2c = i2c_core_100_wb32::make(
                            pcie_manager, BAR0_TB_I2C_ADDR_BASE, true);
                        tb_i2c->set_clock_rate(B300_BUS_CLOCK_RATE, 100000);
                        auto eeprom = get_mb_eeprom(tb_i2c);
                        try {
                            new_addr["product"] = map_pid_to_product_name(
                                static_cast<uint32_t>(std::stoul(eeprom["product"])));
                        } catch (const std::exception& e) {
                            UHD_LOG_WARNING("B300",
                                "Failed to parse product ID from EEPROM: " << e.what());
                            new_addr["product"] = "UNKNOWN";
                        }
                        new_addr["name"]   = eeprom["name"];
                        new_addr["serial"] = eeprom["serial"];
                        if ((not hint.has_key("resource")
                                or hint["resource"] == new_addr["resource"])
                            and (not hint.has_key("name")
                                 or hint["name"] == new_addr["name"])
                            and (not hint.has_key("serial")
                                 or hint["serial"] == new_addr["serial"])
                            and (not hint.has_key("product")
                                 or hint["product"] == new_addr["product"])) {
                            found_devices.push_back(new_addr);
                        }
                    }
                }
            }
        }
    }
#elif defined(_WIN32)
    // Windows B310 device discovery using Windows Device Management APIs
    UHD_LOG_DEBUG("B300", "Searching for B3xx PCIe devices on Windows...");

    // We'll look for the b310k driver devices rather than raw PCI VID/DID
    const std::string b310_driver_name = "b310k";

    // Enumerate all present devices
    HDEVINFO deviceInfoSet =
        SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);

    if (deviceInfoSet != INVALID_HANDLE_VALUE) {
        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
            // Get device driver/service name
            char serviceName[MAX_PATH];
            if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet,
                    &deviceInfoData,
                    SPDRP_SERVICE,
                    nullptr,
                    (PBYTE)serviceName,
                    sizeof(serviceName),
                    nullptr)) {
                std::string service(serviceName);

                // Check if this is a B310 driver
                const bool isB310Driver = service.find(b310_driver_name)
                                          != std::string::npos;

                if (isB310Driver) {
                    // Get device instance ID for PCIe location information
                    char instanceId[MAX_PATH];
                    if (SetupDiGetDeviceInstanceIdA(deviceInfoSet,
                            &deviceInfoData,
                            instanceId,
                            MAX_PATH,
                            nullptr)) {
                        device_addr_t new_addr;
                        new_addr["type"] = "b3xx";

                        std::string pcie_location;

                        // Get actual PCIe location information using Configuration
                        // Manager
                        DEVINST devInst     = deviceInfoData.DevInst;
                        ULONG busNumber     = 0;
                        ULONG busNumberSize = sizeof(busNumber);

                        // Try to get the bus number from device properties
                        if (CM_Get_DevNode_Registry_PropertyA(devInst,
                                CM_DRP_BUSNUMBER,
                                nullptr,
                                &busNumber,
                                &busNumberSize,
                                0)
                            == CR_SUCCESS) {
                            // Get address (device.function) from device properties
                            ULONG address     = 0;
                            ULONG addressSize = sizeof(address);

                            if (CM_Get_DevNode_Registry_PropertyA(devInst,
                                    CM_DRP_ADDRESS,
                                    nullptr,
                                    &address,
                                    &addressSize,
                                    0)
                                == CR_SUCCESS) {
                                // Extract device and function from address
                                // Address format: (device << 16) | function
                                uint8_t device   = (address >> 16) & 0xFFFF;
                                uint8_t function = address & 0xFFFF;

                                // Format as decimal PCIe identifier: bus:device:function
                                // (Windows format)
                                char pcie_buf[32];
                                snprintf(pcie_buf,
                                    sizeof(pcie_buf),
                                    "%u:%u:%u",
                                    busNumber,
                                    device,
                                    function);
                                pcie_location = pcie_buf;
                            }
                        }

                        // Fallback to instance ID parsing if CM APIs fail
                        if (pcie_location.empty()) {
                            std::string instance_str(instanceId);

                            // Try to extract PCIe location from the instance ID
                            size_t last_backslash = instance_str.find_last_of('\\');
                            if (last_backslash != std::string::npos) {
                                std::string location_part =
                                    instance_str.substr(last_backslash + 1);

                                // Parse the location part - typically something like
                                // "4&1234ABCD&0&0000"
                                size_t final_amp = location_part.find_last_of('&');
                                if (final_amp != std::string::npos
                                    && final_amp + 1 < location_part.length()) {
                                    std::string bdf_hex =
                                        location_part.substr(final_amp + 1);

                                    // Convert hex string to integer, then extract
                                    // bus:device.function
                                    try {
                                        uint32_t bdf_value =
                                            std::stoul(bdf_hex, nullptr, 16);
                                        uint8_t bus      = (bdf_value >> 8) & 0xFF;
                                        uint8_t device   = (bdf_value >> 3) & 0x1F;
                                        uint8_t function = bdf_value & 0x07;

                                        // Format as standard PCIe identifier:
                                        // bus:device.function
                                        char pcie_buf[32];
                                        snprintf(pcie_buf,
                                            sizeof(pcie_buf),
                                            "%02x:%02x.%x",
                                            bus,
                                            device,
                                            function);
                                        pcie_location = pcie_buf;
                                    } catch (const std::exception&) {
                                        // Final fallback to service name
                                        pcie_location = service;
                                    }
                                }
                            }
                        }

                        new_addr["resource"] = pcie_location;
                        // Make a dummy tree so that we can make a pcie manager to read
                        // the EEPROM
                        uhd::property_tree::sptr dummy_tree;
                        b300_pcie_manager::sptr pcie_manager =
                            std::make_shared<b300_pcie_manager>(
                                new_addr["resource"], dummy_tree, "");
                        uint32_t session_count = pcie_manager->get_session_count();
                        new_addr["claimed"]    = (session_count > 1) ? "True" : "False";

                        auto tb_i2c = i2c_core_100_wb32::make(
                            pcie_manager, BAR0_TB_I2C_ADDR_BASE, true);
                        tb_i2c->set_clock_rate(B300_BUS_CLOCK_RATE, 100000);
                        auto eeprom = get_mb_eeprom(tb_i2c);
                        try {
                            new_addr["product"] = map_pid_to_product_name(
                                static_cast<uint32_t>(std::stoul(eeprom["product"])));
                        } catch (const std::exception& e) {
                            UHD_LOG_WARNING("B300",
                                "Failed to parse product ID from EEPROM: " << e.what());
                            new_addr["product"] = "UNKNOWN";
                        }
                        new_addr["name"]   = eeprom["name"];
                        new_addr["serial"] = eeprom["serial"];

                        // Check if device matches hint criteria
                        if ((not hint.has_key("resource")
                                or hint["resource"] == new_addr["resource"])
                            and (not hint.has_key("name")
                                 or hint["name"] == new_addr["name"])
                            and (not hint.has_key("serial")
                                 or hint["serial"] == new_addr["serial"])
                            and (not hint.has_key("product")
                                 or hint["product"] == new_addr["product"])) {
                            UHD_LOG_INFO("B300",
                                "Found B3xx device: "
                                    << pcie_location << " (driver: " << service
                                    << ", instance: " << instanceId << ")");
                            found_devices.push_back(new_addr);
                        }
                    }
                }
            }
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);
    } else {
        UHD_LOG_WARNING("B300", "Failed to enumerate Windows devices for B310 discovery");
    }
#endif

    return found_devices;
}

static device::sptr b300_make(const device_addr_t& device_addr)
{
    return device::sptr(new b300_impl(device_addr));
}

UHD_STATIC_BLOCK(register_b300_device)
{
    device::register_device(&b300_find, &b300_make, device::USRP);
}

b300_impl::b300_impl(const uhd::device_addr_t& dev_args)
    : rfnoc_device(), _device_args(dev_args)
{
    UHD_LOG_INFO("B300", "B300 initialization sequence...");

    const device_addrs_t device_args = separate_device_addr(dev_args);

    for (size_t i = 0; i < device_args.size(); ++i) {
        if (device_args[i]["claimed"] == "True") {
            throw uhd::runtime_error("B300 device " + device_args[i]["resource"]
                                     + " is currently claimed by another process.");
        }
        this->setup_mb(i, device_args[i]);
    }

    if (device_args.size() > 1) {
        bool matching_sync_sources      = true;
        uhd::device_addr_t sync_sources = _mb_controllers[0]->get_sync_source();
        for (size_t i = 1; i < device_args.size(); ++i) {
            uhd::device_addr_t compare_sync_sources =
                _mb_controllers[i]->get_sync_source();
            if (sync_sources != compare_sync_sources) {
                matching_sync_sources = false;
                break;
            }
        }

        // If we have a multi-device session where the time source and clock source
        // combination is either sync,sync or sync_gpsdo,sync_gpsdo or external,external.
        if (matching_sync_sources
            && ((sync_sources["clock_source"] == "sync"
                    && sync_sources["time_source"] == "sync")
                || (sync_sources["clock_source"] == "sync_gpsdo"
                    && sync_sources["time_source"] == "sync_gpsdo")
                || (sync_sources["clock_source"] == "external"
                    && sync_sources["time_source"] == "external"))) {
            UHD_LOG_INFO("B300", "Running Multi-Device Sync...");
            for (size_t i = 0; i < device_args.size(); ++i) {
                _mb_controllers[i]->setup_multi_device_sync();
            }
            for (size_t i = 0; i < device_args.size(); ++i) {
                _mb_controllers[i]->configure_lmk_for_sync();
            }
            for (size_t i = 0; i < device_args.size(); ++i) {
                _mb_controllers[i]->finish_multi_device_sync();
            }
        }
    }
}

void b300_impl::setup_mb(const size_t mb_idx, const uhd::device_addr_t& dev_args)
{
    const fs_path mb_path = fs_path("/mboards") / mb_idx;
    UHD_ASSERT_THROW(dev_args.has_key("resource"));

    std::string device_identifier = dev_args["resource"];
    double mcr =
        dev_args.cast<double>("master_clock_rate", B300_DEFAULT_MASTER_CLOCK_RATE);

    _tree->create<double>(mb_path / "tick_rate")
        .set_coercer([mcr](const double rate) {
            // The contract of multi_usrp::set_master_clock_rate() is to coerce
            // and not throw, so we'll follow that behaviour here.
            if (!uhd::math::frequencies_are_equal(rate, mcr)) {
                UHD_LOG_WARNING("B300",
                    "Cannot update master clock rate! B310 does not "
                    "allow changing the clock rate during runtime.");
            }
            return mcr;
        })
        .set(mcr);

    auto pcie_mgr =
        std::make_shared<b300_pcie_manager>(device_identifier, _tree, mb_path);
    uhd::compat_num32 fpga_compat = check_fpga_compat(mb_path, pcie_mgr);

    uint32_t rfnoc_info = pcie_mgr->peek32(RFNOC_INFO_REG);
    auto chdr_width     = uhd::rfnoc::bits_to_chdr_w(
        (rfnoc_info >> 16) & 0xFFFF); // RFNoC CHDR Width is in the upper 16 bits
    auto rfnoc_proto_ver = rfnoc_info
                           & 0xFFFF; // RFNoC protocol version is in the lower 16 bits

    _mb_ifaces.insert({mb_idx,
        b300_mb_iface(pcie_mgr,
            fpga_compat,
            mcr,
            chdr_width,
            rfnoc_proto_ver,
            uhd::rfnoc::allocate_device_id())});

    // Create the SPI Core that hangs off BAR 0.
    auto spi = b300_bar0_spi_core::make(pcie_mgr, BAR0_SR_SPI, BAR0_RB_SPI);

    auto mboard_i2c = i2c_core_100_wb32::make(pcie_mgr, BAR0_MB_I2C_ADDR_BASE);
    mboard_i2c->set_clock_rate(B300_BUS_CLOCK_RATE);

    auto tb_i2c = i2c_core_100_wb32::make(pcie_mgr, BAR0_TB_I2C_ADDR_BASE, true);
    // TB i2c core needs to run at a non-standard 100kHz clock rate for the eeprom.
    tb_i2c->set_clock_rate(B300_BUS_CLOCK_RATE, B300_TB_I2C_DATA_RATE);

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    const mboard_eeprom_t mb_eeprom = get_mb_eeprom(tb_i2c);
    _tree
        ->create<mboard_eeprom_t>(mb_path / "eeprom")
        // Initialize the property with a current copy of the EEPROM contents
        .set(mb_eeprom)
        // Whenever this property is written, update the chip
        .add_coerced_subscriber([tb_i2c](const mboard_eeprom_t& mb_eeprom) {
            set_mb_eeprom(tb_i2c, mb_eeprom);
        });

    uint16_t board_rev = get_and_check_hw_rev(mb_eeprom);

    auto bar0_regmap = std::make_shared<bar0_regmap_t>();
    bar0_regmap->initialize(*pcie_mgr, true);

    // If the clock source is sync or sync_gpsdo, then that means it is the inputting the
    // high frequency reference output of the sync cable. The rate of this clock will be
    // the master clock rate.
    double ext_clk_rate =
        (dev_args.get("clock_source", B300_DEFAULT_CLOCK_SOURCE) == "sync"
            || dev_args.get("clock_source", B300_DEFAULT_CLOCK_SOURCE) == "sync_gpsdo")
            ? mcr
            : dev_args.cast<double>("system_ref_rate", B300_DEFAULT_SYSTEM_REF_RATE);

    auto clock_ctrl = b300_clock_ctrl::make(bar0_regmap, spi, ext_clk_rate, board_rev);

    auto gps_ctrl = b300_gps_control::make(
        [pcie_mgr](uint32_t addr, uint32_t value) {
            pcie_mgr->poke32(GPS_UART_BASE + addr, value);
        },
        [pcie_mgr](uint32_t addr) { return pcie_mgr->peek32(GPS_UART_BASE + addr); },
        [pcie_mgr](
            bool power_on) { pcie_mgr->poke32(GPS_CTRL_REG, power_on ? 0x3 : 0x0); });

    auto mb_ctrl = std::make_shared<uhd::rfnoc::b300_mb_controller>(clock_ctrl,
        bar0_regmap,
        gps_ctrl,
        mboard_i2c,
        dev_args,
        mcr,
        board_rev,
        mb_eeprom);
    register_mb_controller(mb_idx, mb_ctrl);
    _mb_controllers.insert({mb_idx, mb_ctrl});

    for (const std::string& sensor_name : mb_ctrl->get_sensor_names()) {
        _tree->create<sensor_value_t>(mb_path / "sensors" / sensor_name)
            .set_publisher(
                [mb_ctrl, sensor_name]() { return mb_ctrl->get_sensor(sensor_name); });
    }

    _tree->create<std::string>(mb_path / "time_source" / "value")
        .set(mb_ctrl->get_time_source())
        .add_coerced_subscriber([mb_ctrl](const std::string& time_source) {
            mb_ctrl->set_time_source(time_source);
        });
    _tree->create<std::vector<std::string>>(mb_path / "time_source" / "options")
        .set(mb_ctrl->get_time_sources());

    _tree->create<std::string>(mb_path / "clock_source" / "value")
        .set(mb_ctrl->get_clock_source())
        .add_coerced_subscriber([mb_ctrl](const std::string& clock_source) {
            mb_ctrl->set_clock_source(clock_source);
        })
        .set_publisher([mb_ctrl]() { return mb_ctrl->get_clock_source(); });
    _tree->create<std::vector<std::string>>(mb_path / "clock_source" / "options")
        .set(mb_ctrl->get_clock_sources());

    try {
        _tree->create<std::string>("/name").set("B3xx");
    } catch (const uhd::runtime_error&) {
        // property_tree lacks an atomic check to only create a new node if it
        // doesn't exist, so we simply try and create it and when it fails, we
        // assume that another device has already created this node and we move
        // on. If we did "if exists" before creating, there's a non-zero chance
        // that a concurrent device init would still throw.
    }

    _tree->create<std::string>(mb_path / "name").set(mb_ctrl->get_mboard_name());
}

uhd::rfnoc::mb_iface& b300_impl::get_mb_iface(const size_t mb_idx)
{
    if (mb_idx >= _mb_ifaces.size()) {
        throw uhd::index_error(
            std::string("Cannot get mb_iface, invalid motherboard index: ")
            + std::to_string(mb_idx));
    }
    return _mb_ifaces.at(mb_idx);
}

b300_impl::~b300_impl(void)
{
    // Remove all mboard nodes from the property tree since many hold a reference to the
    // pcie manager and we want to ensure that the pcie manager detructs upon device
    // destruction so that the device frees up for other sessions to use. As long as the
    // pcie manager stays alive, the session count remains incremented in the kernel and
    // will not allow other device sessions to open.
    const device_addrs_t device_args = separate_device_addr(_device_args);
    for (size_t i = 0; i < device_args.size(); ++i) {
        const fs_path mb_path = fs_path("/mboards") / i;
        for (const auto& node : _tree->list(mb_path)) {
            _tree->remove(mb_path / node);
        }
    }
}

uhd::compat_num32 b300_impl::check_fpga_compat(
    const uhd::fs_path& mb_path, b300_pcie_manager::sptr pcie_mgr)
{
    // Check that the FPGA is a recent enough version to work with with this version of
    // UHD.
    uint32_t fpga_revision = pcie_mgr->peek32(CORE_REVISION_REG);
    uint8_t fpga_rev_major = (fpga_revision >> 24) & 0xff;
    uint8_t fpga_rev_minor = (fpga_revision >> 16) & 0xff;
    if (fpga_rev_major < B300_FPGA_COMPAT_NUM_MAJOR
        || (fpga_rev_major == B300_FPGA_COMPAT_NUM_MAJOR
            && fpga_rev_minor < B300_FPGA_COMPAT_NUM_MINOR)) {
        throw uhd::runtime_error("FPGA compatibility number mismatch. Expected: "
                                 + std::to_string(B300_FPGA_COMPAT_NUM_MAJOR) + "."
                                 + std::to_string(B300_FPGA_COMPAT_NUM_MINOR)
                                 + " Actual: " + std::to_string(fpga_rev_major) + "."
                                 + std::to_string(fpga_rev_minor)
                                 + ". Please update FPGA image.");
    }

    // Check that this version of UHD is recent enough to work with the programmed FPGA.
    uint32_t fpga_oldest_compat_revision = pcie_mgr->peek32(CORE_OLDEST_REVISION_REG);
    uint8_t fpga_oldest_rev_major        = (fpga_oldest_compat_revision >> 24) & 0xff;
    uint8_t fpga_oldest_rev_minor        = (fpga_oldest_compat_revision >> 16) & 0xff;
    if (fpga_oldest_rev_major > B300_FPGA_COMPAT_NUM_MAJOR
        || (fpga_oldest_rev_major == B300_FPGA_COMPAT_NUM_MAJOR
            && fpga_oldest_rev_minor > B300_FPGA_COMPAT_NUM_MINOR)) {
        throw uhd::runtime_error(
            "FPGA compatibility number mismatch. Expected: "
            + std::to_string(B300_FPGA_COMPAT_NUM_MAJOR) + "."
            + std::to_string(B300_FPGA_COMPAT_NUM_MINOR)
            + " FPGA Oldest Compatible Version: " + std::to_string(fpga_oldest_rev_major)
            + "." + std::to_string(fpga_oldest_rev_minor) + ". Please update UHD.");
    }

    _tree->create<std::string>(mb_path / "fpga_version")
        .set(std::to_string(fpga_rev_major) + "." + std::to_string(fpga_rev_minor));

    const uint32_t git_hash = pcie_mgr->peek32(GIT_HASH_REG);
    std::stringstream git_hash_stream;
    git_hash_stream << std::hex << std::setw(7) << std::setfill('0')
                    << (git_hash & 0x0FFFFFFF);
    const std::string git_hash_str =
        git_hash_stream.str() + ((git_hash & 0xF0000000) ? "-dirty" : "");
    _tree->create<std::string>(mb_path / "fpga_version_hash").set(git_hash_str);

    return {static_cast<uint16_t>(fpga_rev_major), static_cast<uint16_t>(fpga_rev_minor)};
}

uint16_t b300_impl::get_and_check_hw_rev(const mboard_eeprom_t& mb_eeprom)
{
    uint16_t hw_rev = 0;
    if (mb_eeprom.has_key("revision") and not mb_eeprom["revision"].empty()) {
        try {
            hw_rev = uhd::cast::from_str<uint16_t>(mb_eeprom["revision"]);
        } catch (...) {
            UHD_LOG_WARNING("B300",
                "Revision in EEPROM is invalid! Please reprogram your EEPROM! Falling "
                "back to revision C.");
            hw_rev = B300_FALLBACK_REVISION;
        }
    } else {
        UHD_LOG_WARNING("B300",
            "No revision detected! Please reprogram your EEPROM! Falling "
            "back to revision C.");
        hw_rev = B300_FALLBACK_REVISION;
    }

    uint16_t hw_rev_compat = 0;
    if (mb_eeprom.has_key("revision_compat")
        and not mb_eeprom["revision_compat"].empty()) {
        try {
            hw_rev_compat = uhd::cast::from_str<uint16_t>(mb_eeprom["revision_compat"]);
        } catch (...) {
            UHD_LOG_WARNING("B300",
                "Revision compat in EEPROM is invalid! Please reprogram your EEPROM! "
                "Falling back to revision C.");
            hw_rev_compat = B300_FALLBACK_REVISION;
        }
    } else {
        UHD_LOG_WARNING("B300",
            "No revision compat detected! Please reprogram your EEPROM! Falling "
            "back to revision C.");
        hw_rev_compat = B300_FALLBACK_REVISION;
    }

    if (hw_rev_compat > B300_REVISION_COMPAT) {
        throw uhd::runtime_error(
            std::string("Hardware is too new for this software. Please upgrade to "
                        "a driver that supports hardware revision ")
            + std::to_string(hw_rev));
    } else if (hw_rev < B300_REVISION_MIN) {
        throw uhd::runtime_error(
            std::string("Software is too new for this hardware. Please downgrade "
                        "to a driver that supports hardware revision ")
            + std::to_string(hw_rev));
    }

    return hw_rev;
}

}}} // namespace uhd::usrp::b300
