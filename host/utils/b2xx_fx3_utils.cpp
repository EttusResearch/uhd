//
// Copyright 2010-2014 Ettus Research LLC
// Copyright 2018-2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/transport/usb_control.hpp>
#include <uhd/transport/usb_device_handle.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/scope_exit.hpp>
#include <b200_iface.hpp>
#include <libusb.h>
#include <stdint.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace {
struct vid_pid_t
{
    uint16_t vid;
    uint16_t pid;
};
const vid_pid_t known_vid_pids[] = {{FX3_VID, FX3_DEFAULT_PID},
    {FX3_VID, FX3_REENUM_PID},
    {B200_VENDOR_ID, B200_PRODUCT_ID},
    {B200_VENDOR_ID, B200MINI_PRODUCT_ID},
    {B200_VENDOR_ID, B205MINI_PRODUCT_ID},
    {B200_VENDOR_ID, B206MINI_PRODUCT_ID},
    {B200_VENDOR_NI_ID, B200_PRODUCT_NI_ID},
    {B200_VENDOR_NI_ID, B210_PRODUCT_NI_ID}};
const std::vector<vid_pid_t> known_vid_pid_vector(known_vid_pids,
    known_vid_pids + (sizeof(known_vid_pids) / sizeof(known_vid_pids[0])));

const uhd::byte_vector_t OLD_EEPROM_SIGNATURE = {0x43, 0x59, 0x14, 0xB2};
const uhd::byte_vector_t NEW_EEPROM_SIGNATURE = {0x43, 0x59, 0x1A, 0xB0};

uhd::byte_vector_t construct_eeprom_init_value_vector(uint16_t vid, uint16_t pid)
{
    uhd::byte_vector_t init_values(OLD_EEPROM_SIGNATURE);
    init_values.push_back(static_cast<uint8_t>(pid & 0xff));
    init_values.push_back(static_cast<uint8_t>(pid >> 8));
    init_values.push_back(static_cast<uint8_t>(vid & 0xff));
    init_values.push_back(static_cast<uint8_t>(vid >> 8));
    return init_values;
}

constexpr size_t EEPROM_SIZE = 0x8000; // 32kB

constexpr uint8_t EEPROM_DATA_ADDR_HIGH_BYTE = 0x7F;
constexpr uint8_t EEPROM_DATA_HEADER_ADDR    = 0x00;
constexpr uint8_t EEPROM_DATA_VID_PID_ADDR   = 0x06;
constexpr uint8_t EEPROM_DATA_OLD_DATA_ADDR  = 0x0A;

constexpr uint8_t EEPROM_SERIAL_OFFSET = 0x1B;
constexpr size_t EEPROM_SERIAL_LENGTH  = 9;

const uhd::byte_vector_t EEPROM_DATA_HEADER = {
    0x00,
    0xB2, // magic
    0x01,
    0x00, // eeprom_revision
    0x01,
    0x00 // eeprom_compat
};

} // namespace

//! used with lexical cast to parse a hex string
template <class T>
struct to_hex
{
    T value;
    operator T() const
    {
        return value;
    }
    friend std::istream& operator>>(std::istream& in, to_hex& out)
    {
        in >> std::hex >> out.value;
        return in;
    }
};

//! parse hex-formatted ASCII text into an int
uint16_t atoh(const std::string& string)
{
    if (string.substr(0, 2) == "0x") {
        std::stringstream interpreter(string);
        to_hex<uint16_t> hh;
        interpreter >> hh;
        return hh.value;
    }
    return boost::lexical_cast<uint16_t>(string);
}

int reset_usb()
{
    /* Okay, first, we need to discover what the path is to the ehci and
     * xhci device files. */
    std::set<fs::path> path_list;
    path_list.insert("/sys/bus/pci/drivers/xhci-pci/");
    path_list.insert("/sys/bus/pci/drivers/ehci-pci/");
    path_list.insert("/sys/bus/pci/drivers/xhci_hcd/");
    path_list.insert("/sys/bus/pci/drivers/ehci_hcd/");

    /* Check each of the possible paths above to find which ones this system
     * uses. */
    for (std::set<fs::path>::iterator found = path_list.begin(); found != path_list.end();
         ++found) {
        if (fs::exists(*found)) {
            fs::path devpath = *found;

            std::set<fs::path> globbed;

            /* Now, glob all of the files in the directory. */
            fs::directory_iterator end_itr;
            for (fs::directory_iterator itr(devpath); itr != end_itr; ++itr) {
                globbed.insert((*itr).path());
            }

            /* Check each file path string to see if it is a device file. */
            for (std::set<fs::path>::iterator it = globbed.begin(); it != globbed.end();
                 ++it) {
                std::string file = fs::path((*it).filename()).string();

                if (file.length() < 5)
                    continue;

                if (file.compare(0, 5, "0000:") == 0) {
                    /* Un-bind the device. */
                    std::fstream unbind(
                        (devpath.string() + "unbind").c_str(), std::fstream::out);
                    unbind << file;
                    unbind.close();

                    /* Re-bind the device. */
                    std::cout << "Re-binding: " << file << " in " << devpath.string()
                              << std::endl;
                    std::fstream bind(
                        (devpath.string() + "bind").c_str(), std::fstream::out);
                    bind << file;
                    bind.close();
                }
            }
        }
    }

    return 0;
}

uhd::transport::usb_device_handle::sptr open_device(
    const uint16_t vid, const uint16_t pid, const bool user_supplied = false)
{
    std::vector<uhd::transport::usb_device_handle::sptr> handles;
    uhd::transport::usb_device_handle::sptr handle;
    vid_pid_t vp = {vid, pid};

    try {
        // try caller's VID/PID first
        std::vector<uhd::transport::usb_device_handle::vid_pid_pair_t> vid_pid_pair_list(
            1, uhd::transport::usb_device_handle::vid_pid_pair_t(vid, pid));
        handles = uhd::transport::usb_device_handle::get_device_list(vid_pid_pair_list);
        if (handles.empty()) {
            if (user_supplied) {
                std::cerr << (boost::format("Failed to open device with VID 0x%04x and "
                                            "PID 0x%04x - trying other known VID/PIDs")
                              % vid % pid)
                                 .str()
                          << std::endl;
            }

            // try known VID/PIDs next
            for (size_t i = 0; handles.empty() && i < known_vid_pid_vector.size(); i++) {
                vp = known_vid_pid_vector[i];
                handles =
                    uhd::transport::usb_device_handle::get_device_list(vp.vid, vp.pid);
            }
        }

        if (!handles.empty()) {
            handle = handles[0];
            std::cout << (boost::format("Device opened (VID=0x%04x,PID=0x%04x)") % vp.vid
                          % vp.pid)
                             .str()
                      << std::endl;
        }

        if (!handle)
            std::cerr << "Cannot open device" << std::endl;
    } catch (const std::exception&) {
        std::cerr << "Failed to communicate with the device!" << std::endl;
#ifdef UHD_PLATFORM_WIN32
        std::cerr << "The necessary drivers are not installed. Read the UHD Transport "
                     "Application Notes for "
                     "details:\nhttp://files.ettus.com/manual/page_transport.html"
                  << std::endl;
#endif /* UHD_PLATFORM_WIN32 */
        handle.reset();
    }

    return handle;
}

b200_iface::sptr make_b200_iface(const uhd::transport::usb_device_handle::sptr& handle)
{
    b200_iface::sptr b200;

    try {
        uhd::transport::usb_control::sptr usb_ctrl =
            uhd::transport::usb_control::make(handle, 0);
        b200 = b200_iface::make(usb_ctrl);

        if (!b200)
            std::cerr << "Cannot create device interface" << std::endl;
    } catch (const std::exception&) {
        std::cerr << "Failed to communicate with the device!" << std::endl;
#ifdef UHD_PLATFORM_WIN32
        std::cerr << "The necessary drivers are not installed. Read the UHD Transport "
                     "Application Notes for "
                     "details:\nhttp://files.ettus.com/manual/page_transport.html"
                  << std::endl;
#endif /* UHD_PLATFORM_WIN32 */
        b200.reset();
    }

    return b200;
}

int read_eeprom(b200_iface::sptr& b200, uhd::byte_vector_t& data)
{
    try {
        data = b200->read_eeprom(0x0, 0x0, 8);
    } catch (std::exception& e) {
        std::cerr << "Exception while reading EEPROM: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

int write_eeprom(b200_iface::sptr& b200, const uhd::byte_vector_t& data)
{
    try {
        b200->write_eeprom(0x0, 0x0, data);
    } catch (std::exception& e) {
        std::cerr << "Exception while writing EEPROM: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

int verify_eeprom(b200_iface::sptr& b200, const uhd::byte_vector_t& data)
{
    bool verified = true;
    uhd::byte_vector_t read_bytes;
    if (read_eeprom(b200, read_bytes))
        return -1;

    if (data.size() != read_bytes.size()) {
        std::cerr << "ERROR:  Only able to verify first "
                  << std::min(data.size(), read_bytes.size()) << " bytes." << std::endl;
        verified = false;
    }

    for (size_t i = 0; i < std::min(data.size(), read_bytes.size()); i++) {
        if (data[i] != read_bytes[i]) {
            verified = false;
            std::cerr << "Byte " << i << " Expected: " << data[i]
                      << ", Got: " << read_bytes[i] << std::endl;
        }
    }

    if (!verified) {
        std::cerr << "Verification failed" << std::endl;
        return -1;
    }

    return 0;
}

int write_and_verify_eeprom(b200_iface::sptr& b200, const uhd::byte_vector_t& data)
{
    if (write_eeprom(b200, data))
        return -1;
    if (verify_eeprom(b200, data))
        return -1;

    return 0;
}

int erase_eeprom(b200_iface::sptr& b200)
{
    uhd::byte_vector_t bytes(8, 0xFF);
    return write_and_verify_eeprom(b200, bytes);
}

std::tuple<uint16_t, uint16_t, size_t> get_eeprom_hdr_address(b200_iface::sptr& b200)
{
    // Check eeprom revision
    const auto signature = b200->read_eeprom(0x0, 0x0, 4);
    if (signature == NEW_EEPROM_SIGNATURE) {
        UHD_LOGGER_DEBUG("B2XX_FX3") << "Found NEW EEPROM layout (w/ bootloader)";
        return {EEPROM_DATA_ADDR_HIGH_BYTE, EEPROM_DATA_OLD_DATA_ADDR, 36};
    } else {
        UHD_LOGGER_DEBUG("B2XX_FX3") << "Found OLD EEPROM layout (w/o bootloader)";
        return {0x04, 0xDC, 36};
    }
}

uhd::byte_vector_t dump_eeprom(
    b200_iface::sptr& b200, uint16_t addr, uint16_t offset, size_t num_bytes)
{
    // Dump current eeprom data
    const uint16_t eeprom_addr = offset | (uint16_t(addr) << 8);
    UHD_LOGGER_DEBUG("B2XX_FX3")
        << "Reading EEPROM at address " << boost::format("0x%X") % (int)eeprom_addr
        << " with " << (int)num_bytes << " bytes";
    try {
        return b200->read_eeprom(addr, offset, num_bytes);
    } catch (std::exception& e) {
        std::cerr << "Exception while reading EEPROM: " << e.what() << std::endl;
        return {};
    }
}

int blank_eeprom(b200_iface::sptr& b200, uint16_t addr, uint16_t offset, size_t num_bytes)
{
    //! Erases any section of the EEPROM by overwriting with 0xFF
    //
    // This can be used to erase/blank the entire EEPROM, or any sectors of it.
    // Cf. erase_eeprom(), which only erases selected areas of the EEPROM.

    // copied from erase_eeprom(b200_iface::sptr& b200), but using variable size/length
    const uint16_t eeprom_addr = offset | (uint16_t(addr) << 8);
    uhd::byte_vector_t bytes(num_bytes, 0xFF);
    UHD_LOGGER_DEBUG("B2XX_FX3")
        << "Blanking EEPROM at address " << boost::format("0x%X") % (int)eeprom_addr
        << " with " << (int)num_bytes << " bytes";
    try {
        b200->write_eeprom(addr, offset, bytes);
    } catch (std::exception& e) {
        std::cerr << "Exception while writing EEPROM: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

int write_to_eeprom(b200_iface::sptr& b200,
    const uhd::byte_vector_t& data,
    uint16_t addr,
    uint16_t offset)
{
    // Write to eeprom
    // copied from write_eeprom(b200_iface::sptr& b200, const uhd::byte_vector_t&data)
    // Note: write_eeprom() uses hardcode address and offset (0x0, 0x0)
    const uint16_t eeprom_addr = offset | (uint16_t(addr) << 8);
    UHD_LOGGER_DEBUG("B2XX_FX3")
        << "Writing EEPROM at address " << boost::format("0x%X") % (int)eeprom_addr
        << " with " << (int)data.size() << " bytes";
    try {
        b200->write_eeprom(addr, offset, data);
    } catch (std::exception& e) {
        std::cerr << "Exception while writing EEPROM: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

int32_t main(int32_t argc, char* argv[])
{
    uint16_t vid, pid;
    std::string pid_str, vid_str, fw_file, fpga_file, bl_file, eeprom_file, writevid_str,
        writepid_str, serial_str;
    bool user_supplied_vid_pid = false;

    // clang-format off
    po::options_description visible("Allowed options");
    visible.add_options()(
        "help,h", "help message")(
        "vid,v", po::value<std::string>(&vid_str), "Specify VID of device to use.")(
        "pid,p", po::value<std::string>(&pid_str), "Specify PID of device to use.")(
        "speed,S", "Read back the USB mode currently in use.")(
        "reset-device,D", "Reset the B2xx Device.")(
        "reset-fpga,F", "Reset the FPGA (does not require re-programming.")(
        "reset-usb,U", "Reset the USB subsystem on your host computer.")(
        "load-fw,W", po::value<std::string>(&fw_file),
            "Load a firmware (hex) file into the FX3.")(
        "load-fpga,L", po::value<std::string>(&fpga_file),
            "Load a FPGA (bin) file into the FPGA.")(
        "load-bootloader,B", po::value<std::string>(&bl_file),
            "Load a bootloader (img) file into the EEPROM")(
        "query-bootloader,Q", "Check if bootloader is loaded.")(
        "unload-bootloader,u", "Remove bootloader.");

    // Hidden options provided for testing - use at your own risk!
    po::options_description hidden("Hidden options");
    hidden.add_options()(
        "init-device,I", "Initialize a B2xx device.")(
        "uninit-device", "Uninitialize a B2xx device.")(
        "read-eeprom,R", "Read first 8 bytes of EEPROM")(
        "erase-eeprom,E", "Erase first 8 bytes of EEPROM")(
        "dump-eeprom", po::value<std::string>(&eeprom_file), "Dump complete eeprom content into file")(
        "blank-eeprom", "Erase all bytes of EEPROM, incl. first 8 bytes")(
        "write-serial", po::value<std::string>(&serial_str), "Write serial number into EEPROM during init")(
        "write-vid", po::value<std::string>(&writevid_str), "Write VID field of EEPROM")(
        "write-pid", po::value<std::string>(&writepid_str), "Write PID field of EEPROM");
    // clang-format on

    po::options_description desc;
    desc.add(visible);
    desc.add(hidden);

    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (std::exception& e) {
        std::cerr << "Exception while parsing arguments: " << e.what() << std::endl;
        std::cout << boost::format("B2xx Utility Program %s") % visible << std::endl;
        return ~0;
    }

    if (vm.count("help")) {
        try {
            std::cout << boost::format("B2xx Utility Program %s") % visible << std::endl;
        } catch (...) {
        }
        return ~0;
    }

    if (vm.count("reset-usb")) {
        return reset_usb();
    }

    uhd::transport::usb_device_handle::sptr handle;
    b200_iface::sptr b200;

    vid = B200_VENDOR_ID; // Default
    pid = B200_PRODUCT_ID; // Default
    if (vm.count("vid") && vm.count("pid")) {
        try {
            vid = atoh(vid_str);
            pid = atoh(pid_str);
        } catch (std::exception& e) {
            std::cerr << "Exception while parsing VID and PID: " << e.what() << std::endl;
            return ~0;
        }
        user_supplied_vid_pid = true;
    }

    // open the device
    handle = open_device(vid, pid, user_supplied_vid_pid);
    if (!handle)
        return -1;
    std::cout << "B2xx detected..." << std::flush;

    // make the interface
    b200 = make_b200_iface(handle);
    if (!b200)
        return -1;
    std::cout << " Control of B2xx granted..." << std::endl << std::endl;

    // if we are supposed to load a new firmware image and one already exists, reset the
    // FX3 so we can load the new one
    if (vm.count("load-fw") && handle->firmware_loaded()) {
        std::cout << "Overwriting existing firmware" << std::endl;

        // before we reset, make sure we have a good firmware file
        if (!(fs::exists(fw_file))) {
            std::cerr << "Invalid firmware filepath: " << fw_file << std::endl;
            return -1;
        }

        // reset the device
        try {
            b200->reset_fx3();
        } catch (std::exception& e) {
            std::cerr << "Exception while resetting FX3: " << e.what() << std::endl;
        }

        // re-open device
        b200.reset();
        handle.reset();
        std::this_thread::sleep_for(
            std::chrono::seconds(2)); // wait 2 seconds for FX3 to reset
        handle = open_device(vid, pid);
        if (!handle)
            return -1;
        b200 = make_b200_iface(handle);
        if (!b200)
            return -1;
    }

    // Check to make sure firmware is loaded
    if (!(handle->firmware_loaded())) {
        std::cout << "Loading firmware" << std::endl;

        if (fw_file.empty())
            fw_file = uhd::find_image_path(B200_FW_FILE_NAME);

        if (fw_file.empty()) {
            std::cerr << "Firmware image not found!" << std::endl;
            return -1;
        }

        if (!(fs::exists(fw_file))) {
            std::cerr << "Invalid filepath: " << fw_file << std::endl;
            return -1;
        }

        // load firmware
        try {
            b200->load_firmware(fw_file);
        } catch (std::exception& e) {
            std::cerr << "Exception while loading firmware: " << e.what() << std::endl;
            return ~0;
        }

        // re-open device
        b200.reset();
        handle.reset();
        handle = open_device(vid, pid);
        if (!handle)
            return -1;
        b200 = make_b200_iface(handle);
        if (!b200)
            return -1;

        std::cout << "Firmware loaded" << std::endl;
    }

    // Added for testing purposes - not exposed
    if (vm.count("read-eeprom")) {
        uhd::byte_vector_t data;

        if (read_eeprom(b200, data))
            return -1;

        for (int i = 0; i < 8; i++)
            std::cout << i << ": " << boost::format("0x%X") % (int)data[i] << std::endl;

        return 0;
    }

    // Added for testing purposes - not exposed
    if (vm.count("erase-eeprom")) {
        if (erase_eeprom(b200))
            return -1;

        std::cout << "Erase Successful!" << std::endl;

        return 0;
    }

    // Added for testing purposes - not exposed
    if (vm.count("dump-eeprom")) {
        if (eeprom_file.empty()) {
            std::cerr << "EEPROM dump saves the EEPROM content to file, but no file path "
                         "was provided."
                      << std::endl;
            return -1;
        }
        std::ofstream eeprom_file_stream(eeprom_file, std::ios::out | std::ios::binary);
        if (eeprom_file_stream.is_open()) {
            std::cout << "Reading EEPROM and dumping content to file: " << eeprom_file
                      << std::endl;
            // read full eeprom in 256 byte increments
            const uint16_t EEPROM_ADDR_HIGH_BYTE = (EEPROM_SIZE >> 8) & 0xFF;
            for (uint16_t i = 0x00; i < EEPROM_ADDR_HIGH_BYTE; i++) {
                auto data = dump_eeprom(b200, i, 0x00, 0x100);
                eeprom_file_stream.write(
                    reinterpret_cast<const char*>(data.data()), data.size());
            }
            eeprom_file_stream.close();
        } else {
            std::cerr << "Failed to open file for writing: " << eeprom_file << std::endl;
            return -1;
        }

        std::cout << "EEPROM dump successful!" << std::endl;
    }

    // Added for testing purposes - not exposed
    if (vm.count("blank-eeprom")) {
        std::cout << "Blanking EEPROM..." << std::endl;

        // write full eeprom in 64 byte increments
        // EERROM_WRITE_PAGE_SIZE needs to be an even divisor of 0x100 (256 bytes)
        const size_t EERROM_WRITE_PAGE_SIZE =
            0x40; // EEPROM page write buffer size according to data sheet
        const uint16_t EEPROM_ADDR_HIGH_BYTE = (EEPROM_SIZE >> 8) & 0xFF;
        for (uint16_t i = 0x00; i < EEPROM_ADDR_HIGH_BYTE; i++) {
            for (size_t j = 0; j < 0x100; j += EERROM_WRITE_PAGE_SIZE) {
                blank_eeprom(b200, i, j, EERROM_WRITE_PAGE_SIZE);
            }
        }

        std::cout << "Blanking EEPROM completed, resetting device..." << std::endl;

        /* Reset the device! */
        try {
            b200->reset_fx3();
        } catch (const std::exception& e) {
            std::cerr << "Exception while resetting device: " << e.what() << std::endl;
            return -1;
        }

        std::cout << "Blanking EEPROM Process Complete." << std::endl << std::endl;

        return 0;
    }

    // Added for testing purposes - not exposed
    if (vm.count("uninit-device")) {
        // erase EEPROM
        erase_eeprom(b200);

        std::cout << "EEPROM uninitialized, resetting device..." << std::endl
                  << std::endl;

        // reset the device
        try {
            b200->reset_fx3();
        } catch (uhd::exception& e) {
            std::cerr << "Exception while resetting FX3: " << e.what() << std::endl;
            return -1;
        }

        std::cout << "Uninitialization Process Complete." << std::endl << std::endl;

        return 0;
    }

    /* If we are initializing the device, the VID/PID should default to the
     * Cypress VID/PID for the initial FW load, but we can initialize from any state. */
    if (vm.count("init-device")) {
        uint16_t writevid = B200_VENDOR_ID;
        uint16_t writepid = B200_PRODUCT_ID;

        /* Register function that will ensure FX3 reset is called.
           Function will be called even if an error during a parameter check occurred
           and ensures FW is unloaded and device falls back to previous state without
           needing to be power cycled. */
        auto scope_exit = uhd::utils::scope_exit::make([&b200]() {
            /* Reset the device! */
            try {
                std::cout << "Resetting device..." << std::endl << std::endl;
                b200->reset_fx3();
                return 0;
            } catch (const std::exception& e) {
                std::cerr << "Exception while resetting device: " << e.what()
                          << std::endl;
                return -1;
            }
        });

        /* Now, initialize the device. */
        // Added for testing purposes - not exposed
        if (vm.count("write-vid") && vm.count("write-pid")) {
            try {
                writevid = atoh(writevid_str);
                writepid = atoh(writepid_str);
            } catch (std::exception& e) {
                std::cerr << "Exception while parsing write VID and PID: " << e.what()
                          << std::endl;
                return ~0;
            }
        }

        /* Make serial number a required argument, and check length requirements,
           error if serial number is too long, or not defined */
        // Added for testing purposes - not exposed
        if (!vm.count("write-serial")) {
            std::cerr << "Serial number is required for initialization" << std::endl;
            return -1;
        } else {
            size_t EEPROM_SERIAL_DATA_LENGTH =
                EEPROM_SERIAL_LENGTH - 1; // Length of serial number data in EEPROM
            if (serial_str.size() == 0
                || serial_str.size() >= EEPROM_SERIAL_DATA_LENGTH) {
                std::cerr << "Serial number needs to be between 1 and "
                          << EEPROM_SERIAL_DATA_LENGTH << "chars long." << std::endl;
                return -1;
            }
        }

        std::cout << "Writing VID and PID to EEPROM..." << std::endl << std::endl;
        if (write_and_verify_eeprom(
                b200, construct_eeprom_init_value_vector(writevid, writepid)))
            return -1;

        if (vm.count("write-serial")) {
            std::cout << "Writing serial number to EEPROM..." << std::endl << std::endl;

            uint16_t eeprom_address, eeprom_offset;
            size_t eeprom_length;
            std::tie(eeprom_address, eeprom_offset, eeprom_length) =
                get_eeprom_hdr_address(b200);

            // convert serial_str to byte vector
            uhd::byte_vector_t serial_data;
            std::copy(
                serial_str.begin(), serial_str.end(), std::back_inserter(serial_data));
            // append null terminator
            serial_data.push_back(0x00);
            if (write_to_eeprom(b200,
                    serial_data,
                    eeprom_address,
                    eeprom_offset + EEPROM_SERIAL_OFFSET))
                return -1;
        }

        std::cout << "EEPROM initialized..." << std::endl << std::endl;

        /* Need to reset device to utilize modified EEPROM.
           Action is performed during scope exit. */

        std::cout << "Initialization Process Complete." << std::endl << std::endl;
        return 0;
    }

    if (vm.count("speed")) {
        uint8_t speed;
        try {
            speed = b200->get_usb_speed();
        } catch (uhd::exception& e) {
            std::cerr << "Exception while getting USB speed: " << e.what() << std::endl;
            return -1;
        }
        std::cout << "Currently operating at USB " << (int)speed << std::endl;
    }

    if (vm.count("reset-device")) {
        try {
            b200->reset_fx3();
        } catch (uhd::exception& e) {
            std::cerr << "Exception while resetting FX3: " << e.what() << std::endl;
            return -1;
        }

    } else if (vm.count("reset-fpga")) {
        try {
            b200->set_fpga_reset_pin(true);
        } catch (uhd::exception& e) {
            std::cerr << "Exception while resetting FPGA: " << e.what() << std::endl;
            return -1;
        }

    } else if (vm.count("load-fpga")) {
        std::cout << "Loading FPGA image (" << fpga_file << ")" << std::endl;
        uint32_t fx3_state;
        try {
            fx3_state = b200->load_fpga(fpga_file);
        } // returns 0 on success, or FX3 state on error
        catch (uhd::exception& e) {
            std::cerr << "Exception while loading FPGA: " << e.what() << std::endl;
            return ~0;
        }

        if (fx3_state != 0) {
            std::cerr << std::flush << "Error loading FPGA. FX3 state (" << fx3_state
                      << "): " << b200_iface::fx3_state_string(fx3_state) << std::endl;
            return ~0;
        }

        std::cout << "FPGA load complete, releasing USB interface..." << std::endl;
    } else if (vm.count("load-bootloader")) {
        if (bl_file.empty())
            bl_file = uhd::find_image_path(B200_BL_FILE_NAME);

        if (bl_file.empty()) {
            std::cerr << "Bootloader image not found!" << std::endl;
            return -1;
        }

        if (!(fs::exists(bl_file))) {
            std::cerr << "Invalid filepath: " << bl_file << std::endl;
            return -1;
        }

        std::cout << "Loading Bootloader image (" << bl_file << ")" << std::endl;

        // In the upgrade case, we need to migrate the EEPROM data to a new
        // location before loading the bootloader

        // Use the signature to detect the old EEPROM layout
        auto signature = b200->read_eeprom(0x0, 0x0, 4);
        if (signature == OLD_EEPROM_SIGNATURE) {
            std::cout << "Old EEPROM detected. Upgrading EEPROM image to latest revision."
                      << std::endl;

            // Read values that will be clobbered by the bootloader
            auto pidvid               = b200->read_eeprom(0x00, 0x04, 4);
            uhd::byte_vector_t vidpid = {pidvid[2], pidvid[3], pidvid[0], pidvid[1]};
            auto eeprom_data          = b200->read_eeprom(0x04, 0xDC, 36);

            // Write in default header
            b200->write_eeprom(
                EEPROM_DATA_ADDR_HIGH_BYTE, EEPROM_DATA_HEADER_ADDR, EEPROM_DATA_HEADER);
            // Write back data to the device
            b200->write_eeprom(
                EEPROM_DATA_ADDR_HIGH_BYTE, EEPROM_DATA_VID_PID_ADDR, vidpid);
            b200->write_eeprom(
                EEPROM_DATA_ADDR_HIGH_BYTE, EEPROM_DATA_OLD_DATA_ADDR, eeprom_data);
        }

        uint32_t fx3_state;
        try {
            fx3_state = b200->load_bootloader(bl_file);
        } // returns 0 on success, or FX3 state on error
        catch (uhd::exception& e) {
            std::cerr << "Exception while loading bootloader: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }

        if (fx3_state != 0) {
            std::cerr << std::flush << "Error loading bootloader. FX3 state ("
                      << fx3_state << "): " << b200_iface::fx3_state_string(fx3_state)
                      << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Bootloader load complete, resetting device..." << std::endl;

        // reset the device
        try {
            b200->reset_fx3();
        } catch (uhd::exception& e) {
            std::cerr << "Exception while resetting FX3: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    } else if (vm.count("query-bootloader")) {
        auto signature = b200->read_eeprom(0x0, 0x0, 4);
        if (signature != NEW_EEPROM_SIGNATURE) {
            std::cout << "No bootloader found on device" << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Bootloader is present" << std::endl;
    } else if (vm.count("unload-bootloader")) {
        auto signature = b200->read_eeprom(0x0, 0x0, 4);
        if (signature != NEW_EEPROM_SIGNATURE) {
            std::cout << "No bootloader found on device" << std::endl;
            return EXIT_FAILURE;
        }
        auto vidpid =
            b200->read_eeprom(EEPROM_DATA_ADDR_HIGH_BYTE, EEPROM_DATA_VID_PID_ADDR, 4);
        auto eeprom_data =
            b200->read_eeprom(EEPROM_DATA_ADDR_HIGH_BYTE, EEPROM_DATA_OLD_DATA_ADDR, 36);

        uhd::byte_vector_t first_bl_record(OLD_EEPROM_SIGNATURE);
        first_bl_record.push_back(vidpid[2]);
        first_bl_record.push_back(vidpid[3]);
        first_bl_record.push_back(vidpid[0]);
        first_bl_record.push_back(vidpid[1]);
        if (write_and_verify_eeprom(b200, first_bl_record)) {
            return EXIT_FAILURE;
        }
        b200->write_eeprom(0x04, 0xDC, eeprom_data);

        std::cout << "Bootloader unload complete, resetting device..." << std::endl;

        // reset the device
        try {
            b200->reset_fx3();
        } catch (uhd::exception& e) {
            std::cerr << "Exception while resetting FX3: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "Operation complete!  I did it!  I did it!" << std::endl;

    return 0;
}
