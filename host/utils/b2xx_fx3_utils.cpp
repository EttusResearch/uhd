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

#include <uhd/config.hpp>
#include <uhd/utils/msg.hpp>

const static boost::uint16_t FX3_VID = 0x04b4;
const static boost::uint16_t FX3_DEFAULT_PID = 0x00f3;
const static boost::uint16_t FX3_REENUM_PID = 0x00f0;
const static boost::uint16_t B2XX_VID = 0x2500;
const static boost::uint16_t B2XX_PID = 0x0020;

const static boost::uint8_t VRT_VENDOR_OUT = (LIBUSB_REQUEST_TYPE_VENDOR
                                              | LIBUSB_ENDPOINT_OUT);
const static boost::uint8_t VRT_VENDOR_IN = (LIBUSB_REQUEST_TYPE_VENDOR
                                             | LIBUSB_ENDPOINT_IN);
const static boost::uint8_t FX3_FIRMWARE_LOAD = 0xA0;

const static boost::uint8_t B2XX_VREQ_FPGA_START = 0x02;
const static boost::uint8_t B2XX_VREQ_FPGA_DATA = 0x12;
const static boost::uint8_t B2XX_VREQ_SET_FPGA_HASH = 0x1C;
const static boost::uint8_t B2XX_VREQ_GET_FPGA_HASH = 0x1D;
const static boost::uint8_t B2XX_VREQ_FPGA_RESET = 0x62;
const static boost::uint8_t B2XX_VREQ_GET_USB = 0x80;
const static boost::uint8_t B2XX_VREQ_GET_STATUS = 0x83;
const static boost::uint8_t B2XX_VREQ_FX3_RESET = 0x99;
const static boost::uint8_t B2XX_VREQ_EEPROM_WRITE = 0xBA;

const static boost::uint8_t FX3_STATE_FPGA_READY = 0x00;
const static boost::uint8_t FX3_STATE_CONFIGURING_FPGA = 0x01;
const static boost::uint8_t FX3_STATE_BUSY = 0x02;
const static boost::uint8_t FX3_STATE_RUNNING = 0x03;

typedef boost::uint32_t hash_type;
typedef std::vector<boost::uint8_t> byte_vector_t;


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


/*!
 * Create a file hash
 * The hash will be used to identify the loaded firmware and fpga image
 * \param filename file used to generate hash value
 * \return hash value in a size_t type
 */
static hash_type generate_hash(const char *filename)
{
    std::ifstream file(filename);
    if (not file){
        std::cerr << std::string("cannot open input file ") + filename
            << std::endl;
    }

    size_t hash = 0;

    char ch;
    while (file.get(ch)) {
        boost::hash_combine(hash, ch);
    }

    if (not file.eof()){
        std::cerr << std::string("file error ") + filename << std::endl;
    }

    file.close();
    return hash_type(hash);
}

/*!
 * Verify checksum of a Intel HEX record
 * \param record a line from an Intel HEX file
 * \return true if record is valid, false otherwise
 */
bool checksum(std::string *record) {

    size_t len = record->length();
    unsigned int i;
    unsigned char sum = 0;
    unsigned int val;

    for (i = 1; i < len; i += 2) {
        std::istringstream(record->substr(i, 2)) >> std::hex >> val;
        sum += val;
    }

    if (sum == 0)
       return true;
    else
       return false;
}


/*!
 * Parse Intel HEX record
 *
 * \param record a line from an Intel HEX file
 * \param len output length of record
 * \param addr output address
 * \param type output type
 * \param data output data
 * \return true if record is sucessfully read, false on error
 */
bool parse_record(std::string *record, boost::uint16_t &len, boost::uint16_t &addr,
        uint16_t &type, unsigned char* data) {

    unsigned int i;
    std::string _data;
    unsigned int val;

    if (record->substr(0, 1) != ":")
        return false;

    std::istringstream(record->substr(1, 2)) >> std::hex >> len;
    std::istringstream(record->substr(3, 4)) >> std::hex >> addr;
    std::istringstream(record->substr(7, 2)) >> std::hex >> type;

    for (i = 0; i < len; i++) {
        std::istringstream(record->substr(9 + 2 * i, 2)) >> std::hex >> val;
        data[i] = (unsigned char) val;
    }

    return true;
}


/*!
 * Write data to the FX3.
 *
 * \param dev_handle the libusb-1.0 device handle
 * \param request the usb transfer request type
 * \param value the USB bValue
 * \param index the USB bIndex
 * \param buff the data to write
 * \param length the number of bytes to write
 * \return the number of bytes written
 */
libusb_error fx3_control_write(libusb_device_handle *dev_handle, boost::uint8_t request,
            boost::uint16_t value, boost::uint16_t index, unsigned char *buff,
            boost::uint16_t length, boost::uint32_t timeout = 0) {

#if 0
    if(DEBUG) {
        std::cout << "Writing: <" << std::hex << std::setw(6) << std::showbase \
            << std::internal << std::setfill('0') << (int) request \
            << ", " << std::setw(6) << (int) VRT_VENDOR_OUT \
            << ", " << std::setw(6) << value \
            << ", " << std::setw(6) << index \
            << ", " << std::dec << std::setw(2) << length \
            << ">" << std::endl;

        std::cout << "\t\tData: 0x " << std::noshowbase;

        for(int count = 0; count < length; count++) {
            std::cout << std::hex << std::setw(2) << (int) buff[count] << " ";
        }

        std::cout << std::showbase << std::endl;
    }
#endif

    return (libusb_error) libusb_control_transfer(dev_handle, VRT_VENDOR_OUT, request, \
               value, index, buff, length, timeout);
}


/*!
 * Read data from the FX3.
 *
 * \param dev_handle the libusb-1.0 device handle
 * \param request the usb transfer request type
 * \param value the USB bValue
 * \param index the USB bIndex
 * \param buff a buffer to store the read bytes to
 * \param length the number of bytes to read
 * \return the number of bytes read
 */
libusb_error fx3_control_read(libusb_device_handle *dev_handle, boost::uint8_t request,
            boost::uint16_t value, boost::uint16_t index, unsigned char *buff,
            boost::uint16_t length, boost::uint32_t timeout = 0) {

#if 0
    if(DEBUG) {
        std::cout << "Reading: <" << std::hex << std::setw(6) << std::showbase \
            << std::internal << std::setfill('0') << (int) request \
            << ", " << std::setw(6) << (int) VRT_VENDOR_IN \
            << ", " << std::setw(6) << value \
            << ", " << std::setw(6) << index \
            << ", " << std::dec << std::setw(2) << length \
            << ">" << std::endl << std::endl;
    }
#endif

    return (libusb_error) libusb_control_transfer(dev_handle, VRT_VENDOR_IN, request, \
               value, index, buff, length, timeout);
}


void write_eeprom(libusb_device_handle *dev_handle, boost::uint8_t addr,
        boost::uint8_t offset, const byte_vector_t &bytes) {
    fx3_control_write(dev_handle, B2XX_VREQ_EEPROM_WRITE,
                        0, offset | (boost::uint16_t(addr) << 8),
                        (unsigned char *) &bytes[0],
                        bytes.size());
}


boost::uint8_t get_fx3_status(libusb_device_handle *dev_handle) {

    unsigned char rx_data[1];

    fx3_control_read(dev_handle, B2XX_VREQ_GET_STATUS, 0x00, 0x00, rx_data, 1);

    return boost::lexical_cast<boost::uint8_t>(rx_data[0]);
}

void usrp_get_fpga_hash(libusb_device_handle *dev_handle, hash_type &hash) {
    fx3_control_read(dev_handle, B2XX_VREQ_GET_FPGA_HASH, 0x00, 0x00,
            (unsigned char*) &hash, 4, 500);
}

void usrp_set_fpga_hash(libusb_device_handle *dev_handle, hash_type hash) {
    fx3_control_write(dev_handle, B2XX_VREQ_SET_FPGA_HASH, 0x00, 0x00,
            (unsigned char*) &hash, 4);
}

boost::int32_t load_fpga(libusb_device_handle *dev_handle,
        const std::string filestring) {

    if (filestring.empty())
    {
        std::cerr << "load_fpga: input file is empty." << std::endl;
        exit(-1);
    }

    boost::uint8_t fx3_state = 0;

    const char *filename = filestring.c_str();

    hash_type hash = generate_hash(filename);
    hash_type loaded_hash; usrp_get_fpga_hash(dev_handle, loaded_hash);
    if (hash == loaded_hash) return 0;

    size_t file_size = 0;
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
        file_size = file.tellg();
    }

    std::ifstream file;
    file.open(filename, std::ios::in | std::ios::binary);

    if(!file.good()) {
        std::cerr << "load_fpga: cannot open FPGA input file." << std::endl;
        exit(-1);
    }

    do {
        fx3_state = get_fx3_status(dev_handle);
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    } while(fx3_state != FX3_STATE_FPGA_READY);

    std::cout << "Loading FPGA image: " \
        << filestring << "..." << std::flush;

    unsigned char out_buff[64];
    memset(out_buff, 0x00, sizeof(out_buff));
    fx3_control_write(dev_handle, B2XX_VREQ_FPGA_START, 0, 0, out_buff, 1, 1000);

    do {
        fx3_state = get_fx3_status(dev_handle);
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    } while(fx3_state != FX3_STATE_CONFIGURING_FPGA);


    size_t bytes_sent = 0;
    while(!file.eof()) {
        file.read((char *) out_buff, sizeof(out_buff));
        const std::streamsize n = file.gcount();
        if(n == 0) continue;

        boost::uint16_t transfer_count = boost::uint16_t(n);

        /* Send the data to the device. */
        fx3_control_write(dev_handle, B2XX_VREQ_FPGA_DATA, 0, 0, out_buff,
                transfer_count, 5000);

        if (bytes_sent == 0) std::cout << "  0%" << std::flush;
        const size_t percent_before = size_t((bytes_sent*100)/file_size);
        bytes_sent += transfer_count;
        const size_t percent_after = size_t((bytes_sent*100)/file_size);
        if (percent_before/10 != percent_after/10) {
            std::cout << "\b\b\b\b" << std::setw(3) << percent_after
                << "%" << std::flush;
        }
    }

    file.close();

    do {
        fx3_state = get_fx3_status(dev_handle);
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    } while(fx3_state != FX3_STATE_RUNNING);

    usrp_set_fpga_hash(dev_handle, hash);

    std::cout << "\b\b\b\b done" << std::endl;

    return 0;
}


/*!
 * Program the FX3 with a firmware file (Intel HEX format)
 *
 * \param dev_handle the libusb-1.0 device handle
 * \param filestring the filename of the firmware file
 * \return 0 for success, otherwise error code
 */
boost::int32_t fx3_load_firmware(libusb_device_handle *dev_handle, \
        std::string filestring) {

    if (filestring.empty())
    {
        std::cerr << "fx3_load_firmware: input file is empty." << std::endl;
        exit(-1);
    }

    const char *filename = filestring.c_str();

    /* Fields used in each USB control transfer. */
    boost::uint16_t len = 0;
    boost::uint16_t type = 0;
    boost::uint16_t lower_address_bits = 0x0000;
    unsigned char data[512];

    /* Can be set by the Intel HEX record 0x04, used for all 0x00 records
        * thereafter. Note this field takes the place of the 'index' parameter in
        * libusb calls, and is necessary for FX3's 32-bit addressing. */
    boost::uint16_t upper_address_bits = 0x0000;

    std::ifstream file;
    file.open(filename, std::ifstream::in);

    if(!file.good()) {
        std::cerr << "fx3_load_firmware: cannot open firmware input file"
            << std::endl;
        exit(-1);
    }

    std::cout << "Loading firmware image: " \
        << filestring << "..." << std::flush;

    while (!file.eof()) {
        boost::int32_t ret = 0;
        std::string record;
        file >> record;

        /* Check for valid Intel HEX record. */
        if (!checksum(&record) || !parse_record(&record, len, \
                    lower_address_bits, type, data)) {
            std::cerr << "fx3_load_firmware: bad intel hex record checksum"
                    << std::endl;
        }

        /* Type 0x00: Data. */
        if (type == 0x00) {
            ret = fx3_control_write(dev_handle, FX3_FIRMWARE_LOAD, \
                    lower_address_bits, upper_address_bits, data, len);

            if (ret < 0) {
                std::cerr << "usrp_load_firmware: usrp_control_write failed"
                    << std::endl;
            }
        }

        /* Type 0x01: EOF. */
        else if (type == 0x01) {
            if (lower_address_bits != 0x0000 || len != 0 ) {
                std::cerr << "fx3_load_firmware: For EOF record, address must be 0, length must be 0." << std::endl;
            }

            /* Successful termination! */
            file.close();

            /* Let the system settle. */
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            return 0;
        }

        /* Type 0x04: Extended Linear Address Record. */
        else if (type == 0x04) {
            if (lower_address_bits != 0x0000 || len != 2 ) {
                std::cerr << "fx3_load_firmware: For ELA record, address must be 0, length must be 2." << std::endl;
            }

            upper_address_bits = ((boost::uint16_t)((data[0] & 0x00FF) << 8))\
                                    + ((boost::uint16_t)(data[1] & 0x00FF));
        }

        /* Type 0x05: Start Linear Address Record. */
        else if (type == 0x05) {
            if (lower_address_bits != 0x0000 || len != 4 ) {
                std::cerr << "fx3_load_firmware: For SLA record, address must be 0, length must be 4." << std::endl;
            }

            /* The firmware load is complete.  We now need to tell the CPU
                * to jump to an execution address start point, now contained within
                * the data field.  Parse these address bits out, and then push the
                * instruction. */
            upper_address_bits = ((boost::uint16_t)((data[0] & 0x00FF) << 8))\
                                    + ((boost::uint16_t)(data[1] & 0x00FF));
            lower_address_bits = ((boost::uint16_t)((data[2] & 0x00FF) << 8))\
                                    + ((boost::uint16_t)(data[3] & 0x00FF));

            fx3_control_write(dev_handle, FX3_FIRMWARE_LOAD, lower_address_bits, \
                    upper_address_bits, 0, 0);

            std::cout << " done" << std::endl;
        }

        /* If we receive an unknown record type, error out. */
        else {
            std::cerr << "fx3_load_firmware: unsupported record type." << std::endl;
        }
    }

    /* There was no valid EOF. */
    std::cerr << "fx3_load_firmware: No EOF record found." << std::cout;
    return ~0;
}


boost::int32_t main(boost::int32_t argc, char *argv[]) {
    boost::uint16_t vid, pid;
    std::string pid_str, vid_str, fw_file, fpga_file;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "help message")
        ("vid,v", po::value<std::string>(&vid_str)->default_value("0x2500"),
            "Specify VID of device to use.")
        ("pid,p", po::value<std::string>(&pid_str)->default_value("0x0020"),
            "Specify PID of device to use.")
        ("speed,S", "Read back the USB mode currently in use.")
        ("reset-device,D", "Reset the B2xx Device.")
        ("reset-fpga,F", "Reset the FPGA (does not require re-programming.")
        ("reset-usb,U", "Reset the USB subsystem on your host computer.")
        ("init-device,I", "Initialize a B2xx device.")
        ("load-fw,W", po::value<std::string>(&fw_file)->default_value(""),
            "Load a firmware (hex) file into the FX3.")
        ("load-fpga,L", po::value<std::string>(&fpga_file)->default_value(""),
            "Load a FPGA (bin) file into the FPGA.")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")){
        std::cout << boost::format("B2xx Utilitiy Program %s") % desc << std::endl;
        return ~0;
    } else if (vm.count("reset-usb")) {
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

                    std::string file = (*it).filename().string();

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

    vid = atoh(vid_str);
    pid = atoh(pid_str);

    /* Pointer to pointer of device, used to retrieve a list of devices. */
    libusb_device **devs;
    libusb_device_handle *dev_handle;
    libusb_context *ctx = NULL;
    libusb_error error_code;

    libusb_init(&ctx);
    libusb_set_debug(ctx, 3);
    libusb_get_device_list(ctx, &devs);

    /* If we are initializing the device, the VID/PID will default to the
     * Cypress VID/PID for the initial FW load. */
    if (vm.count("init-device")) {
        dev_handle = libusb_open_device_with_vid_pid(ctx, FX3_VID,
                FX3_DEFAULT_PID);
        if(dev_handle == NULL) {
            std::cerr << "Cannot open device with vid: " << vid << " and pid: "
                << pid << std::endl;
            return -1;
        } else { std::cout << "Uninitialized B2xx detected..." << std::flush; }
        libusb_free_device_list(devs, 1);

        /* Find out if kernel driver is attached, and if so, detach it. */
        if(libusb_kernel_driver_active(dev_handle, 0) == 1) {
            std::cout << " Competing Driver Identified... " << std::flush;

            if(libusb_detach_kernel_driver(dev_handle, 0) == 0) {
                std::cout << " Competing Driver Destroyed!" << std::flush;
            }
        }
        libusb_claim_interface(dev_handle, 0);
        std::cout << " Control of B2xx granted..." << std::endl << std::endl;

        /* Load the FW. */
        error_code = (libusb_error) fx3_load_firmware(dev_handle, fw_file);
        if(error_code != 0) {
            std::cerr << std::flush << "Error loading firmware. Error code: "
                << error_code << std::endl;
            libusb_release_interface(dev_handle, 0);
            libusb_close(dev_handle);
            libusb_exit(ctx);
            return ~0;
        }

        /* Let the device re-enumerate. */
        libusb_release_interface(dev_handle, 0);
        libusb_close(dev_handle);
        dev_handle = libusb_open_device_with_vid_pid(ctx, FX3_VID,
                FX3_REENUM_PID);
        if(dev_handle == NULL) {
            std::cerr << "Cannot open device with vid: " << vid << " and pid: "
                << pid << std::endl;
            return -1;
        } else {
            std::cout << "Detected in-progress init of B2xx..." << std::flush;
        }
        //libusb_free_device_list(devs, 1);
        libusb_claim_interface(dev_handle, 0);
        std::cout << " Reenumeration complete, Device claimed..."
            << std::endl;

        /* Now, initialize the device. */
        byte_vector_t bytes(8);
        bytes[0] = 0x43;
        bytes[1] = 0x59;
        bytes[2] = 0x14;
        bytes[3] = 0xB2;
        bytes[4] = (B2XX_PID & 0xff);
        bytes[5] = (B2XX_PID >> 8);
        bytes[6] = (B2XX_VID & 0xff);
        bytes[7] = (B2XX_VID >> 8);
        write_eeprom(dev_handle, 0x0, 0x0, bytes);
        std::cout << "EEPROM initialized, resetting device..."
            << std::endl << std::endl;

        /* Reset the device! */
        boost::uint8_t data_buffer[1];
        fx3_control_write(dev_handle, B2XX_VREQ_FX3_RESET,
                0x00, 0x00, data_buffer, 1, 5000);

        std::cout << "Initialization Process Complete."
            << std::endl << std::endl;
        libusb_release_interface(dev_handle, 0);
        libusb_close(dev_handle);
        libusb_exit(ctx);
        return 0;
    }

    dev_handle = libusb_open_device_with_vid_pid(ctx, vid, pid);
    if(dev_handle == NULL) {
        std::cerr << "Cannot open device with vid: " << vid << " and pid: "
            << pid << std::endl;
            return -1;
    } else { std::cout << "Reactor Core Online..." << std::flush; }
    libusb_free_device_list(devs, 1);

    /* Find out if kernel driver is attached, and if so, detach it. */
    if(libusb_kernel_driver_active(dev_handle, 0) == 1) {
        std::cout << " Competing Driver Identified... " << std::flush;

        if(libusb_detach_kernel_driver(dev_handle, 0) == 0) {
            std::cout << " Competing Driver Destroyed!" << std::flush;
        }
    }

    /* Claim interface 0 of device. */
    error_code = (libusb_error) libusb_claim_interface(dev_handle, 0);
    std::cout << " All Systems Nominal..." << std::endl << std::endl;

    boost::uint8_t data_buffer[16];
    memset(data_buffer, 0x0, sizeof(data_buffer));

    if (vm.count("speed")){
        error_code = fx3_control_read(dev_handle, B2XX_VREQ_GET_USB,
                0x00, 0x00, data_buffer, 1, 5000);

        boost::uint8_t speed = boost::lexical_cast<boost::uint8_t>(data_buffer[0]);

        std::cout << "Currently operating at USB " << (int) speed << std::endl;

    } else if (vm.count("reset-device")) {
        error_code = fx3_control_write(dev_handle, B2XX_VREQ_FX3_RESET,
                0x00, 0x00, data_buffer, 1, 5000);

    } else if (vm.count("reset-fpga")) {
        error_code = fx3_control_write(dev_handle, B2XX_VREQ_FPGA_RESET,
                0x00, 0x00, data_buffer, 1, 5000);

    } else if (vm.count("load-fw")) {
        error_code = (libusb_error) fx3_load_firmware(dev_handle, fw_file);

        if(error_code != 0) {
            std::cerr << std::flush << "Error loading firmware. Error code: "
                << error_code << std::endl;
            libusb_release_interface(dev_handle, 0);
            libusb_close(dev_handle);
            libusb_exit(ctx);
            return ~0;
        }

        std::cout << "Firmware load complete, releasing USB interface..."
            << std::endl;

    } else if (vm.count("load-fpga")) {
        error_code = (libusb_error) load_fpga(dev_handle, fpga_file);

        if(error_code != 0) {
            std::cerr << std::flush << "Error loading FPGA. Error code: "
                << error_code << std::endl;
            libusb_release_interface(dev_handle, 0);
            libusb_close(dev_handle);
            libusb_exit(ctx);
            return ~0;
        }

        std::cout << "FPGA load complete, releasing USB interface..."
            << std::endl;

    } else {
        std::cout << boost::format("B2xx Utilitiy Program %s") % desc << std::endl;
        libusb_release_interface(dev_handle, 0);
        libusb_close(dev_handle);
        libusb_exit(ctx);
        return ~0;
    }

    std::cout << std::endl << "Reactor Shutting Down..." << std::endl;

    error_code = (libusb_error) libusb_release_interface(dev_handle, 0);
    libusb_close(dev_handle);
    libusb_exit(ctx);

    return 0;
}

