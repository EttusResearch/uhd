//
// Copyright 2010-2013 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <boost/functional/hash.hpp>

#include <b200_iface.hpp>
#include <uhd/config.hpp>
#include <uhd/transport/usb_control.hpp>
#include <uhd/transport/usb_device_handle.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/images.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;


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
boost::uint16_t atoh(const std::string &string){
    if (string.substr(0, 2) == "0x"){
        return boost::lexical_cast<to_hex<boost::uint16_t> >(string);
    }
    return boost::lexical_cast<boost::uint16_t>(string);
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

uhd::transport::usb_device_handle::sptr open_device(const boost::uint16_t vid, const boost::uint16_t pid)
{
    std::vector<uhd::transport::usb_device_handle::sptr> handles;
    uhd::transport::usb_device_handle::sptr handle;

    try {
        handles = uhd::transport::usb_device_handle::get_device_list(vid, pid);                             // try caller's VID/PID first
        if (handles.size() == 0)
            handles = uhd::transport::usb_device_handle::get_device_list(FX3_VID, FX3_DEFAULT_PID);         // try default Cypress FX3 VID/PID next
        if (handles.size() == 0)
            handles = uhd::transport::usb_device_handle::get_device_list(FX3_VID, FX3_REENUM_PID);          // try reenumerated Cypress FX3 VID/PID next
        if (handles.size() == 0)
            handles = uhd::transport::usb_device_handle::get_device_list(B200_VENDOR_ID, B200_PRODUCT_ID);  // try default B200 VID/PID last

        if (handles.size() > 0)
            handle = handles[0];

        if (!handle)
            std::cerr << "Cannot open device" << std::endl;
    }
    catch(const std::exception &e) {
        std::cerr << "Failed to communicate with the device!" << std::endl;
        #ifdef UHD_PLATFORM_WIN32
        std::cerr << "The necessary drivers are not installed. Read the UHD Transport Application Notes for details." << std::endl;
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
    catch(const std::exception &e) {
        std::cerr << "Failed to communicate with the device!" << std::endl;
        #ifdef UHD_PLATFORM_WIN32
        std::cerr << "The necessary drivers are not installed. Read the UHD Transport Application Notes for details." << std::endl;
        #endif /* UHD_PLATFORM_WIN32 */
        b200.reset();
    }

    return b200;
}

boost::int32_t main(boost::int32_t argc, char *argv[]) {
    boost::uint16_t vid, pid;
    std::string pid_str, vid_str, fw_file, fpga_file;

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
        ("init-device,I", "Initialize a B2xx device.")
        ("load-fw,W", po::value<std::string>(&fw_file),
            "Load a firmware (hex) file into the FX3.")
        ("load-fpga,L", po::value<std::string>(&fpga_file),
            "Load a FPGA (bin) file into the FPGA.")
    ;

    // Hidden options provided for testing - use at your own risk!
    po::options_description hidden("Hidden options");
    hidden.add_options()
    ("uninit-device,U", "Uninitialize a B2xx device.")
    ("read-eeprom,R", "Read first 8 bytes of EEPROM")
    ("erase-eeprom,E", "Erase first 8 bytes of EEPROM");

    po::options_description desc;
    desc.add(visible);
    desc.add(hidden);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")){
        std::cout << boost::format("B2xx Utility Program %s") % visible << std::endl;
        return ~0;
    } else if (vm.count("reset-usb")) {
        return reset_usb();
    }

    uhd::transport::usb_device_handle::sptr handle;
    b200_iface::sptr b200;

    vid = B200_VENDOR_ID;   // Default
    pid = B200_PRODUCT_ID;  // Default
    if (vm.count("vid"))
        vid = atoh(vid_str);
    if (vm.count("pid"))
        pid = atoh(pid_str);

    // open the device
    handle = open_device(vid, pid);
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

        // reset the device
        b200->reset_fx3();

        // re-open device
        b200.reset();
        handle.reset();
        boost::this_thread::sleep(boost::posix_time::seconds(2));    // wait 2 seconds for FX3 to reset
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
        b200->load_firmware(fw_file);

        // re-open device
        b200.reset();
        handle.reset();
        handle = open_device(vid, pid);
        if (!handle)
            return -1;
        b200 = make_b200_iface(handle);
        if (!b200)
            return -1;
    }

    // Added for testing purposes - not exposed
    if (vm.count("read-eeprom"))
    {
        uhd::byte_vector_t data;

        try {
            data = b200->read_eeprom(0x0, 0x0, 8);
        } catch (std::exception &e) {
            std::cerr << "Exception while reading EEPROM: " << e.what() << std::endl;
            return -1;
        }
        for (int i = 0; i < 8; i++)
            std::cout << i << ": " << boost::format("0x%X") % (int)data[i] << std::endl;

        return 0;
    }

    // Added for testing purposes - not exposed
    if (vm.count("erase-eeprom"))
    {
        uhd::byte_vector_t bytes(8);
        memset(&bytes[0], 0xFF, 8);
        try {
            b200->write_eeprom(0x0, 0x0, bytes);
        } catch (uhd::exception &e) {
            std::cerr << "Exception while writing to EEPROM: " << e.what() << std::endl;
            return -1;
        }

        // verify
        uhd::byte_vector_t read_bytes(8);
        try {
            read_bytes = b200->read_eeprom(0x0, 0x0, 8);
        } catch (uhd::exception &e) {
            std::cerr << "Exception while reading from EEPROM: " << e.what() << std::endl;
            return -1;
        }
        bool verified = true;
        for (int i = 0; i < 8; i++) {
            if (bytes[i] != read_bytes[i]) {
                verified = false;
                std::cerr << "Expected: " << bytes[i] << ", Got: " << read_bytes[i] << std::endl;
            }
        }
        if (!verified) {
            std::cerr << "Verification failed" << std::endl;
            return -1;
        }

        std::cout << "Erase Successful!" << std::endl;

        return 0;
    }

    // Added for testing purposes - not exposed
    if (vm.count("uninit-device"))
    {
        // uninitialize the device
        uhd::byte_vector_t bytes(8);
        memset(&bytes[0], 0xFF, 8);

        try {
            b200->write_eeprom(0x0, 0x0, bytes);
        } catch (uhd::exception &e) {
            std::cerr << "Exception while writing to EEPROM: " << e.what() << std::endl;
            return -1;
        }

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
        /* Now, initialize the device. */
        uhd::byte_vector_t bytes(8);
        bytes[0] = 0x43;
        bytes[1] = 0x59;
        bytes[2] = 0x14;
        bytes[3] = 0xB2;
        bytes[4] = (B200_PRODUCT_ID & 0xff);
        bytes[5] = (B200_PRODUCT_ID >> 8);
        bytes[6] = (B200_VENDOR_ID & 0xff);
        bytes[7] = (B200_VENDOR_ID >> 8);

        try {
            b200->write_eeprom(0x0, 0x0, bytes);
        } catch (uhd::exception &e) {
            std::cerr << "Exception while writing to EEPROM: " << e.what() << std::endl;
            return -1;
        }

        std::cout << "EEPROM initialized, resetting device..."
            << std::endl << std::endl;

        /* Reset the device! */
        try {
            b200->reset_fx3();
        } catch (const std::exception &e) {
            std::cerr << "Exceptions while resetting device: " << e.what() << std::endl;
            return -1;
        }

        std::cout << "Initialization Process Complete."
            << std::endl << std::endl;
        return 0;
    }

    boost::uint8_t data_buffer[16];
    memset(data_buffer, 0x0, sizeof(data_buffer));

    if (vm.count("speed")){
        boost::uint8_t speed;
        try {speed = b200->get_usb_speed();}
        catch (uhd::exception &e) {
            std::cerr << "Exception while getting USB speed: " << e.what() << std::endl;
            return -1;
        }
        std::cout << "Currently operating at USB " << (int) speed << std::endl;

    } else if (vm.count("reset-device")) {
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

    } else if (vm.count("load-fw")) {
        std::cout << "Firmware load complete, releasing USB interface..."
            << std::endl;

    } else if (vm.count("load-fpga")) {
        std::cout << "Loading FPGA image (" << fpga_file << ")" << std::endl;
        uint32_t fx3_state;
        try {fx3_state = b200->load_fpga(fpga_file);} // returns 0 on success, or FX3 state on error
        catch (uhd::exception &e) {
            std::cerr << "Exception while loading FPGA: " << e.what() << std::endl;
            return ~0;
        }

        if (fx3_state != 0) {
            std::cerr << std::flush << "Error loading FPGA. FX3 state: "
            << fx3_state << std::endl;
            return ~0;
        }

        std::cout << "FPGA load complete, releasing USB interface..."
            << std::endl;

    } else {
        std::cout << boost::format("B2xx Utility Program %s") % visible << std::endl;
        return ~0;
    }

    std::cout << "Operation complete!  I did it!  I did it!" << std::endl;

    return 0;
}

