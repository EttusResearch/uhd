//
// Copyright 2010-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <libusb.h>
#include <sstream>
#include <string>
#include <cmath>
#include <cstring>
#include <chrono>
#include <thread>

#include <stdint.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/functional/hash.hpp>

#include <b200_iface.hpp>
#include <uhd/config.hpp>
#include <uhd/transport/usb_control.hpp>
#include <uhd/transport/usb_device_handle.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/paths.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

struct vid_pid_t {
    uint16_t vid;
    uint16_t pid;
};
const static vid_pid_t known_vid_pids[] = {
    {FX3_VID, FX3_DEFAULT_PID},
    {FX3_VID, FX3_REENUM_PID},
    {B200_VENDOR_ID, B200_PRODUCT_ID},
    {B200_VENDOR_ID, B200MINI_PRODUCT_ID},
    {B200_VENDOR_ID, B205MINI_PRODUCT_ID},
    {B200_VENDOR_NI_ID, B200_PRODUCT_NI_ID},
    {B200_VENDOR_NI_ID, B210_PRODUCT_NI_ID}
};
const static std::vector<vid_pid_t> known_vid_pid_vector(known_vid_pids, known_vid_pids + (sizeof(known_vid_pids) / sizeof(known_vid_pids[0])));

static const size_t EEPROM_INIT_VALUE_VECTOR_SIZE = 8;
static uhd::byte_vector_t construct_eeprom_init_value_vector(uint16_t vid, uint16_t pid)
{
    uhd::byte_vector_t init_values(EEPROM_INIT_VALUE_VECTOR_SIZE);
    init_values.at(0) = 0x43;
    init_values.at(1) = 0x59;
    init_values.at(2) = 0x14;
    init_values.at(3) = 0xB2;
    init_values.at(4) = static_cast<uint8_t>(pid & 0xff);
    init_values.at(5) = static_cast<uint8_t>(pid >> 8);
    init_values.at(6) = static_cast<uint8_t>(vid & 0xff);
    init_values.at(7) = static_cast<uint8_t>(vid >> 8);
    return init_values;
}

//!used with lexical cast to parse a hex string
template <class T> struct to_hex{
    T value;
    operator T() const {return value;}
    friend std::istream& operator>>(std::istream& in, to_hex& out){
        in >> std::hex >> out.value;
        return in;
    }
};

//!parse hex-formatted ASCII text into an int
uint16_t atoh(const std::string &string){
    if (string.substr(0, 2) == "0x"){
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
    for(std::set<fs::path>::iterator found = path_list.begin();
            found != path_list.end(); ++found) {

        if(fs::exists(*found)) {

            fs::path devpath = *found;

            std::set<fs::path> globbed;

            /* Now, glob all of the files in the directory. */
            fs::directory_iterator end_itr;
            for(fs::directory_iterator itr(devpath); itr != end_itr; ++itr) {
                globbed.insert((*itr).path());
            }

            /* Check each file path string to see if it is a device file. */
            for(std::set<fs::path>::iterator it = globbed.begin();
                    it != globbed.end(); ++it) {

                std::string file = fs::path((*it).filename()).string();

                if (file.length() < 5)
                    continue;

                if(file.compare(0, 5, "0000:") == 0) {
                    /* Un-bind the device. */
                    std::fstream unbind((devpath.string() + "unbind").c_str(),
                            std::fstream::out);
                    unbind << file;
                    unbind.close();

                    /* Re-bind the device. */
                    std::cout << "Re-binding: " << file << " in "
                        << devpath.string() << std::endl;
                    std::fstream bind((devpath.string() + "bind").c_str(),
                            std::fstream::out);
                    bind << file;
                    bind.close();
                }
            }
        }
    }

    return 0;
}

uhd::transport::usb_device_handle::sptr open_device(const uint16_t vid, const uint16_t pid, const bool user_supplied = false)
{
    std::vector<uhd::transport::usb_device_handle::sptr> handles;
    uhd::transport::usb_device_handle::sptr handle;
    vid_pid_t vp = {vid, pid};

    try {
        // try caller's VID/PID first
        std::vector<uhd::transport::usb_device_handle::vid_pid_pair_t> vid_pid_pair_list(1,uhd::transport::usb_device_handle::vid_pid_pair_t(vid,pid));
        handles = uhd::transport::usb_device_handle::get_device_list(vid_pid_pair_list);
        if (handles.size() == 0)
        {
            if (user_supplied)
            {
                std::cerr << (boost::format("Failed to open device with VID 0x%04x and PID 0x%04x - trying other known VID/PIDs") % vid % pid).str() << std::endl;
            }

            // try known VID/PIDs next
            for (size_t i = 0; handles.size() == 0 && i < known_vid_pid_vector.size(); i++)
            {
                vp = known_vid_pid_vector[i];
                handles = uhd::transport::usb_device_handle::get_device_list(vp.vid, vp.pid);
            }

        }

        if (handles.size() > 0)
        {
            handle = handles[0];
            std::cout << (boost::format("Device opened (VID=0x%04x,PID=0x%04x)") % vp.vid % vp.pid).str() << std::endl;
        }

        if (!handle)
            std::cerr << "Cannot open device" << std::endl;
    }
    catch(const std::exception &) {
        std::cerr << "Failed to communicate with the device!" << std::endl;
        #ifdef UHD_PLATFORM_WIN32
        std::cerr << "The necessary drivers are not installed. Read the UHD Transport Application Notes for details:\nhttp://files.ettus.com/manual/page_transport.html" << std::endl;
        #endif /* UHD_PLATFORM_WIN32 */
        handle.reset();
    }

    return handle;
}

b200_iface::sptr make_b200_iface(const uhd::transport::usb_device_handle::sptr &handle)
{
    b200_iface::sptr b200;

    try {
        uhd::transport::usb_control::sptr usb_ctrl = uhd::transport::usb_control::make(handle, 0);
        b200 = b200_iface::make(usb_ctrl);

        if (!b200)
            std::cerr << "Cannot create device interface" << std::endl;
    }
    catch(const std::exception &) {
        std::cerr << "Failed to communicate with the device!" << std::endl;
        #ifdef UHD_PLATFORM_WIN32
        std::cerr << "The necessary drivers are not installed. Read the UHD Transport Application Notes for details:\nhttp://files.ettus.com/manual/page_transport.html" << std::endl;
        #endif /* UHD_PLATFORM_WIN32 */
        b200.reset();
    }

    return b200;
}

int read_eeprom(b200_iface::sptr& b200, uhd::byte_vector_t& data)
{
    try {
        data = b200->read_eeprom(0x0, 0x0, 8);
    } catch (std::exception &e) {
        std::cerr << "Exception while reading EEPROM: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

int write_eeprom(b200_iface::sptr& b200, const uhd::byte_vector_t& data)
{
    try {
      b200->write_eeprom(0x0, 0x0, data);
    } catch (std::exception &e) {
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

    if (data.size() != read_bytes.size())
    {
        std::cerr << "ERROR:  Only able to verify first " << std::min(data.size(), read_bytes.size()) << " bytes." << std::endl;
        verified = false;
    }

    for (size_t i = 0; i < std::min(data.size(), read_bytes.size()); i++) {
        if (data[i] != read_bytes[i]) {
            verified = false;
            std::cerr << "Byte " << i << " Expected: " << data[i] << ", Got: " << read_bytes[i] << std::endl;
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
    uhd::byte_vector_t bytes(8);

    memset(&bytes[0], 0xFF, 8);
    if (write_and_verify_eeprom(b200, bytes))
        return -1;

    return 0;
}

int32_t main(int32_t argc, char *argv[]) {
    uint16_t vid, pid;
    std::string pid_str, vid_str, fw_file, fpga_file, writevid_str, writepid_str;
    bool user_supplied_vid_pid = false;

    po::options_description visible("Allowed options");
    visible.add_options()
        ("help,h", "help message")
        ("vid,v", po::value<std::string>(&vid_str),
            "Specify VID of device to use.")
        ("pid,p", po::value<std::string>(&pid_str),
            "Specify PID of device to use.")
        ("speed,S", "Read back the USB mode currently in use.")
        ("reset-device,D", "Reset the B2xx Device.")
        ("reset-fpga,F", "Reset the FPGA (does not require re-programming.")
        ("reset-usb,U", "Reset the USB subsystem on your host computer.")
        ("load-fw,W", po::value<std::string>(&fw_file),
            "Load a firmware (hex) file into the FX3.")
        ("load-fpga,L", po::value<std::string>(&fpga_file),
            "Load a FPGA (bin) file into the FPGA.")
    ;

    // Hidden options provided for testing - use at your own risk!
    po::options_description hidden("Hidden options");
    hidden.add_options()
    ("init-device,I", "Initialize a B2xx device.")
    ("uninit-device", "Uninitialize a B2xx device.")
    ("read-eeprom,R", "Read first 8 bytes of EEPROM")
    ("erase-eeprom,E", "Erase first 8 bytes of EEPROM")
    ("write-vid", po::value<std::string>(&writevid_str),
        "Write VID field of EEPROM")
    ("write-pid", po::value<std::string>(&writepid_str),
        "Write PID field of EEPROM");

    po::options_description desc;
    desc.add(visible);
    desc.add(hidden);

    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (std::exception &e) {
        std::cerr << "Exception while parsing arguments: " << e.what() << std::endl;
        std::cout << boost::format("B2xx Utility Program %s") % visible << std::endl;
        return ~0;
    }

    if (vm.count("help")){
        try {
            std::cout << boost::format("B2xx Utility Program %s") % visible << std::endl;
        } catch(...) {}
        return ~0;
    }

    if (vm.count("reset-usb")) {
        return reset_usb();
    }

    uhd::transport::usb_device_handle::sptr handle;
    b200_iface::sptr b200;

    vid = B200_VENDOR_ID;   // Default
    pid = B200_PRODUCT_ID;  // Default
    if (vm.count("vid") && vm.count("pid"))
    {
        try {
            vid = atoh(vid_str);
            pid = atoh(pid_str);
        } catch (std::exception &e) {
            std::cerr << "Exception while parsing VID and PID: " << e.what() << std:: endl;
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

    // if we are supposed to load a new firmware image and one already exists, reset the FX3 so we can load the new one
    if (vm.count("load-fw") && handle->firmware_loaded())
    {
        std::cout << "Overwriting existing firmware" << std::endl;

        // before we reset, make sure we have a good firmware file
        if(!(fs::exists(fw_file)))
        {
            std::cerr << "Invalid firmware filepath: " << fw_file << std::endl;
            return -1;
        }

        // reset the device
        try {
            b200->reset_fx3();
        } catch (std::exception &e) {
            std::cerr << "Exception while reseting FX3: " << e.what() << std::endl;
        }

        // re-open device
        b200.reset();
        handle.reset();
        std::this_thread::sleep_for(std::chrono::seconds(2));    // wait 2 seconds for FX3 to reset
        handle = open_device(vid, pid);
        if (!handle)
            return -1;
        b200 = make_b200_iface(handle);
        if (!b200)
            return -1;
    }

    // Check to make sure firmware is loaded
    if (!(handle->firmware_loaded()))
    {
        std::cout << "Loading firmware" << std::endl;

        if (fw_file.empty())
            fw_file = uhd::find_image_path(B200_FW_FILE_NAME);

        if(fw_file.empty()) {
            std::cerr << "Firmware image not found!" << std::endl;
            return -1;
        }

        if(!(fs::exists(fw_file))) {
            std::cerr << "Invalid filepath: " << fw_file << std::endl;
            return -1;
        }

        // load firmware
        try {
            b200->load_firmware(fw_file);
        } catch (std::exception &e) {
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
    if (vm.count("read-eeprom"))
    {
        uhd::byte_vector_t data;

        if (read_eeprom(b200, data))
            return -1;

        for (int i = 0; i < 8; i++)
            std::cout << i << ": " << boost::format("0x%X") % (int)data[i] << std::endl;

        return 0;
    }

    // Added for testing purposes - not exposed
    if (vm.count("erase-eeprom"))
    {
        if (erase_eeprom(b200))
            return -1;

        std::cout << "Erase Successful!" << std::endl;

        return 0;
    }

    // Added for testing purposes - not exposed
    if (vm.count("uninit-device"))
    {
        // erase EEPROM
        erase_eeprom(b200);

        std::cout << "EEPROM uninitialized, resetting device..."
            << std::endl << std::endl;

        // reset the device
        try {
            b200->reset_fx3();
        } catch (uhd::exception &e) {
            std::cerr << "Exception while resetting FX3: " << e.what() << std::endl;
            return -1;
        }

        std::cout << "Uninitialization Process Complete."
            << std::endl << std::endl;

        return 0;
    }

    /* If we are initializing the device, the VID/PID should default to the
     * Cypress VID/PID for the initial FW load, but we can initialize from any state. */
    if (vm.count("init-device"))
    {
        uint16_t writevid = B200_VENDOR_ID;
        uint16_t writepid = B200_PRODUCT_ID;

        /* Now, initialize the device. */
           // Added for testing purposes - not exposed
        if (vm.count("write-vid") && vm.count("write-pid"))
        {
            try {
                  writevid = atoh(writevid_str);
                  writepid = atoh(writepid_str);
            } catch (std::exception &e) {
                  std::cerr << "Exception while parsing write VID and PID: " << e.what() << std:: endl;
                  return ~0;
            }
        }

        std::cout << "Writing VID and PID to EEPROM..." << std::endl << std::endl;
        if (write_and_verify_eeprom(b200, construct_eeprom_init_value_vector(writevid, writepid))) return -1;

        std::cout << "EEPROM initialized, resetting device..."
            << std::endl << std::endl;

        /* Reset the device! */
        try {
            b200->reset_fx3();
        } catch (const std::exception &e) {
            std::cerr << "Exception while resetting device: " << e.what() << std::endl;
            return -1;
        }

        std::cout << "Initialization Process Complete."
            << std::endl << std::endl;
        return 0;
    }

    uint8_t data_buffer[16];
    memset(data_buffer, 0x0, sizeof(data_buffer));

    if (vm.count("speed")){
        uint8_t speed;
        try {speed = b200->get_usb_speed();}
        catch (uhd::exception &e) {
            std::cerr << "Exception while getting USB speed: " << e.what() << std::endl;
            return -1;
        }
        std::cout << "Currently operating at USB " << (int) speed << std::endl;
    }

    if (vm.count("reset-device")) {
        try {b200->reset_fx3();}
        catch (uhd::exception &e) {
            std::cerr << "Exception while resetting FX3: " << e.what() << std::endl;
            return -1;
        }

    } else if (vm.count("reset-fpga")) {
        try {b200->set_fpga_reset_pin(true);}
        catch (uhd::exception &e) {
            std::cerr << "Exception while resetting FPGA: " << e.what() << std::endl;
            return -1;
        }

    } else if (vm.count("load-fpga")) {
        std::cout << "Loading FPGA image (" << fpga_file << ")" << std::endl;
        uint32_t fx3_state;
        try {fx3_state = b200->load_fpga(fpga_file);} // returns 0 on success, or FX3 state on error
        catch (uhd::exception &e) {
            std::cerr << "Exception while loading FPGA: " << e.what() << std::endl;
            return ~0;
        }

        if (fx3_state != 0) {
            std::cerr << std::flush << "Error loading FPGA. FX3 state ("
                << fx3_state << "): " << b200_iface::fx3_state_string(fx3_state) << std::endl;
            return ~0;
        }

        std::cout << "FPGA load complete, releasing USB interface..."
            << std::endl;
    }

    std::cout << "Operation complete!  I did it!  I did it!" << std::endl;

    return 0;
}

