//
// Copyright 2012-2013 Ettus Research LLC
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

#include "b200_iface.hpp"

#include "../../utils/ihex.hpp"
#include <uhd/config.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <boost/functional/hash.hpp>
#include <boost/thread/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <iomanip>
#include <libusb.h>

//! libusb_error_name is only in newer API
#ifndef HAVE_LIBUSB_ERROR_NAME
    #define libusb_error_name(code) \
        str(boost::format("LIBUSB_ERROR_CODE %d") % code)
#endif

using namespace uhd;
using namespace uhd::transport;

static const bool load_img_msg = true;

const static boost::uint8_t FX3_FIRMWARE_LOAD = 0xA0;
const static boost::uint8_t VRT_VENDOR_OUT = (LIBUSB_REQUEST_TYPE_VENDOR
                                              | LIBUSB_ENDPOINT_OUT);
const static boost::uint8_t VRT_VENDOR_IN = (LIBUSB_REQUEST_TYPE_VENDOR
                                             | LIBUSB_ENDPOINT_IN);
const static boost::uint8_t B200_VREQ_FPGA_START = 0x02;
const static boost::uint8_t B200_VREQ_FPGA_DATA = 0x12;
const static boost::uint8_t B200_VREQ_GET_COMPAT = 0x15;
const static boost::uint8_t B200_VREQ_SET_FPGA_HASH = 0x1C;
const static boost::uint8_t B200_VREQ_GET_FPGA_HASH = 0x1D;
const static boost::uint8_t B200_VREQ_SET_FW_HASH = 0x1E;
const static boost::uint8_t B200_VREQ_GET_FW_HASH = 0x1F;
const static boost::uint8_t B200_VREQ_LOOP = 0x22;
const static boost::uint8_t B200_VREQ_FPGA_CONFIG = 0x55;
const static boost::uint8_t B200_VREQ_FPGA_RESET = 0x62;
const static boost::uint8_t B200_VREQ_GPIF_RESET = 0x72;
const static boost::uint8_t B200_VREQ_GET_USB = 0x80;
const static boost::uint8_t B200_VREQ_GET_STATUS = 0x83;
const static boost::uint8_t B200_VREQ_FX3_RESET = 0x99;
const static boost::uint8_t B200_VREQ_EEPROM_WRITE = 0xBA;
const static boost::uint8_t B200_VREQ_EEPROM_READ = 0xBB;

const static boost::uint8_t FX3_STATE_UNDEFINED = 0x00;
const static boost::uint8_t FX3_STATE_FPGA_READY = 0x01;
const static boost::uint8_t FX3_STATE_CONFIGURING_FPGA = 0x02;
const static boost::uint8_t FX3_STATE_BUSY = 0x03;
const static boost::uint8_t FX3_STATE_RUNNING = 0x04;
const static boost::uint8_t FX3_STATE_UNCONFIGURED = 0x05;
const static boost::uint8_t FX3_STATE_ERROR = 0x06;

const static int VREQ_MAX_SIZE_USB2 = 64;
const static int VREQ_MAX_SIZE_USB3 = 512;
const static int VREQ_DEFAULT_SIZE  = VREQ_MAX_SIZE_USB2;
const static int VREQ_MAX_SIZE      = VREQ_MAX_SIZE_USB3;

typedef boost::uint32_t hash_type;


/***********************************************************************
 * Helper Functions
 **********************************************************************/
/*!
 * Create a file hash
 * The hash will be used to identify the loaded fpga image
 * \param filename file used to generate hash value
 * \return hash value in a uint32_t type
 */
static hash_type generate_hash(const char *filename)
{
    if (filename == NULL)
        return hash_type(0);

    std::ifstream file(filename);
    if (not file){
        throw uhd::io_error(std::string("cannot open input file ") + filename);
    }

    hash_type hash = 0;

    char ch;
    long long count = 0;
    while (file.get(ch)) {
        count++;
        //hash algorithm derived from boost hash_combine
        //http://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
        hash ^= ch + 0x9e3779b9 + (hash<<6) + (hash>>2);
    }

    if (count == 0){
        throw uhd::io_error(std::string("empty input file ") + filename);
    }

    if (not file.eof()){
        throw uhd::io_error(std::string("file error ") + filename);
    }

    file.close();
    return hash_type(hash);
}


/***********************************************************************
 * The implementation class
 **********************************************************************/
class b200_iface_impl : public b200_iface{
public:

    b200_iface_impl(usb_control::sptr usb_ctrl):
        _usb_ctrl(usb_ctrl) {
        //NOP
    }

    int fx3_control_write(boost::uint8_t request,
                           boost::uint16_t value,
                           boost::uint16_t index,
                           unsigned char *buff,
                           boost::uint16_t length,
                           boost::uint32_t timeout = 0) {
        return _usb_ctrl->submit(VRT_VENDOR_OUT,        // bmReqeustType
                                   request,             // bRequest
                                   value,               // wValue
                                   index,               // wIndex
                                   buff,                // data
                                   length,              // wLength
                                   timeout);            // timeout
    }

    int fx3_control_read(boost::uint8_t request,
                           boost::uint16_t value,
                           boost::uint16_t index,
                           unsigned char *buff,
                           boost::uint16_t length,
                           boost::uint32_t timeout = 0) {
        return _usb_ctrl->submit(VRT_VENDOR_IN,         // bmReqeustType
                                   request,             // bRequest
                                   value,               // wValue
                                   index,               // wIndex
                                   buff,                // data
                                   length,              // wLength
                                   timeout);            // timeout
    }

    void write_i2c(UHD_UNUSED(boost::uint16_t addr), UHD_UNUSED(const byte_vector_t &bytes))
    {
        throw uhd::not_implemented_error("b200 write i2c");
    }


    byte_vector_t read_i2c(UHD_UNUSED(boost::uint16_t addr), UHD_UNUSED(size_t num_bytes))
    {
        throw uhd::not_implemented_error("b200 read i2c");
    }

    void write_eeprom(boost::uint16_t addr, boost::uint16_t offset,
            const byte_vector_t &bytes) {
        int ret = fx3_control_write(B200_VREQ_EEPROM_WRITE,
                          0, offset | (boost::uint16_t(addr) << 8),
                          (unsigned char *) &bytes[0],
                          bytes.size());

        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to write EEPROM (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if ((size_t)ret != bytes.size())
            throw uhd::io_error((boost::format("Short write on write EEPROM (expecting: %d, returned: %d)") % bytes.size() % ret).str());
    }

    byte_vector_t read_eeprom(
        boost::uint16_t addr,
        boost::uint16_t offset,
        size_t num_bytes) {
        byte_vector_t recv_bytes(num_bytes);
        int bytes_read = fx3_control_read(B200_VREQ_EEPROM_READ,
                         0, offset | (boost::uint16_t(addr) << 8),
                         (unsigned char*) &recv_bytes[0],
                         num_bytes);

        if (bytes_read < 0)
            throw uhd::io_error((boost::format("Failed to read EEPROM (%d: %s)") % bytes_read % libusb_error_name(bytes_read)).str());
        else if ((size_t)bytes_read != num_bytes)
            throw uhd::io_error((boost::format("Short read on read EEPROM (expecting: %d, returned: %d)") % num_bytes % bytes_read).str());

        return recv_bytes;
    }

    void load_firmware(const std::string filestring, UHD_UNUSED(bool force) = false)
    {
        if (load_img_msg)
            UHD_MSG(status) << "Loading firmware image: "
                            << filestring << "..." << std::flush;

        ihex_reader file_reader(filestring);
        try {
            file_reader.read(
                boost::bind(
                    &b200_iface_impl::fx3_control_write, this,
                    FX3_FIRMWARE_LOAD, _1, _2, _3, _4, 0
                )
            );
        } catch (const uhd::io_error &e) {
            throw uhd::io_error(str(boost::format("Could not load firmware: \n%s") % e.what()));
        }

        UHD_MSG(status) << std::endl;

        //TODO
        //usrp_set_firmware_hash(hash); //set hash before reset

        /* Success! Let the system settle. */
        // TODO: Replace this with a polling loop in the FX3, or find out
        // what the actual, correct timeout value is.
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }

    void reset_fx3(void) {
        unsigned char data[4];
        memset(data, 0x00, sizeof(data));
        const int bytes_to_send = sizeof(data);

        int ret = fx3_control_write(B200_VREQ_FX3_RESET, 0x00, 0x00, data, bytes_to_send);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to reset FX3 (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_send)
            throw uhd::io_error((boost::format("Short write on reset FX3 (expecting: %d, returned: %d)") % bytes_to_send % ret).str());
    }

    void reset_gpif(void) {
        unsigned char data[4];
        memset(data, 0x00, sizeof(data));
        const int bytes_to_send = sizeof(data);

        int ret = fx3_control_write(B200_VREQ_GPIF_RESET, 0x00, 0x00, data, bytes_to_send);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to reset GPIF (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_send)
            throw uhd::io_error((boost::format("Short write on reset GPIF (expecting: %d, returned: %d)") % bytes_to_send % ret).str());
    }

    void set_fpga_reset_pin(const bool reset) {
        unsigned char data[4];
        memset(data, (reset)? 0xFF : 0x00, sizeof(data));

        UHD_THROW_INVALID_CODE_PATH();

        // Below is dead code as long as UHD_THROW_INVALID_CODE_PATH(); is declared above.
        // It is preserved here in a comment in case it is needed later:
        /*
        const int bytes_to_send = sizeof(data);

        int ret = fx3_control_write(B200_VREQ_FPGA_RESET, 0x00, 0x00, data, bytes_to_send);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to reset FPGA (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_send)
            throw uhd::io_error((boost::format("Short write on reset FPGA (expecting: %d, returned: %d)") % bytes_to_send % ret).str());
        */
    }

    boost::uint8_t get_usb_speed(void) {

        unsigned char rx_data[1];
        memset(rx_data, 0x00, sizeof(rx_data));
        const int bytes_to_recv = sizeof(rx_data);

        int ret = fx3_control_read(B200_VREQ_GET_USB, 0x00, 0x00, rx_data, bytes_to_recv);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to get USB speed (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_recv)
            throw uhd::io_error((boost::format("Short read on get USB speed (expecting: %d, returned: %d)") % bytes_to_recv % ret).str());

        return boost::lexical_cast<boost::uint8_t>(rx_data[0]);
    }

    boost::uint8_t get_fx3_status(void) {

        unsigned char rx_data[1];
        memset(rx_data, 0x00, sizeof(rx_data));
        const int bytes_to_recv = sizeof(rx_data);

        int ret = fx3_control_read(B200_VREQ_GET_STATUS, 0x00, 0x00, rx_data, bytes_to_recv);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to get FX3 status (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_recv)
            throw uhd::io_error((boost::format("Short read on get FX3 status (expecting: %d, returned: %d)") % bytes_to_recv % ret).str());

        return boost::lexical_cast<boost::uint8_t>(rx_data[0]);
    }

    boost::uint16_t get_compat_num(void) {

        unsigned char rx_data[2];
        memset(rx_data, 0x00, sizeof(rx_data));
        const int bytes_to_recv = sizeof(rx_data);

        int ret = fx3_control_read(B200_VREQ_GET_COMPAT , 0x00, 0x00, rx_data, bytes_to_recv);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to get compat num (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_recv)
            throw uhd::io_error((boost::format("Short read on get compat num (expecting: %d, returned: %d)") % bytes_to_recv % ret).str());

        return (((uint16_t)rx_data[0]) << 8) | rx_data[1];
    }

    void usrp_get_firmware_hash(hash_type &hash) {
        const int bytes_to_recv = 4;
        if (sizeof(hash_type) != bytes_to_recv)
            throw uhd::type_error((boost::format("hash_type is %d bytes but transfer length is %d bytes") % sizeof(hash_type) % bytes_to_recv).str());

        int ret = fx3_control_read(B200_VREQ_GET_FW_HASH, 0x00, 0x00, (unsigned char*) &hash, bytes_to_recv, 500);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to get firmware hash (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_recv)
            throw uhd::io_error((boost::format("Short read on get firmware hash (expecting: %d, returned: %d)") % bytes_to_recv % ret).str());
    }

    void usrp_set_firmware_hash(hash_type hash) {
        const int bytes_to_send = 4;
        if (sizeof(hash_type) != bytes_to_send)
            throw uhd::type_error((boost::format("hash_type is %d bytes but transfer length is %d bytes") % sizeof(hash_type) % bytes_to_send).str());

        int ret = fx3_control_write(B200_VREQ_SET_FW_HASH, 0x00, 0x00, (unsigned char*) &hash, bytes_to_send);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to set firmware hash (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_send)
            throw uhd::io_error((boost::format("Short write on set firmware hash (expecting: %d, returned: %d)") % bytes_to_send % ret).str());
    }

    void usrp_get_fpga_hash(hash_type &hash) {
        const int bytes_to_recv = 4;
        if (sizeof(hash_type) != bytes_to_recv)
            throw uhd::type_error((boost::format("hash_type is %d bytes but transfer length is %d bytes") % sizeof(hash_type) % bytes_to_recv).str());

        int ret = fx3_control_read(B200_VREQ_GET_FPGA_HASH, 0x00, 0x00, (unsigned char*) &hash, bytes_to_recv, 500);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to get FPGA hash (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_recv)
            throw uhd::io_error((boost::format("Short read on get FPGA hash (expecting: %d, returned: %d)") % bytes_to_recv % ret).str());
    }

    void usrp_set_fpga_hash(hash_type hash) {
        const int bytes_to_send = 4;
        if (sizeof(hash_type) != bytes_to_send)
            throw uhd::type_error((boost::format("hash_type is %d bytes but transfer length is %d bytes") % sizeof(hash_type) % bytes_to_send).str());

        int ret = fx3_control_write(B200_VREQ_SET_FPGA_HASH, 0x00, 0x00, (unsigned char*) &hash, bytes_to_send);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to set FPGA hash (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_send)
            throw uhd::io_error((boost::format("Short write on set FPGA hash (expecting: %d, returned: %d)") % bytes_to_send % ret).str());
    }

    boost::uint32_t load_fpga(const std::string filestring, bool force) {

        boost::uint8_t fx3_state = 0;
        boost::uint32_t wait_count;
        int ret = 0;
        int bytes_to_xfer = 0;

        const char *filename = filestring.c_str();

        hash_type hash = generate_hash(filename);
        hash_type loaded_hash; usrp_get_fpga_hash(loaded_hash);
        if (hash == loaded_hash and !force) return 0;

        // Establish default largest possible control request transfer size based on operating USB speed
        int transfer_size = VREQ_DEFAULT_SIZE;
        int current_usb_speed = get_usb_speed();
        if (current_usb_speed == 3)
            transfer_size = VREQ_MAX_SIZE_USB3;
        else if (current_usb_speed != 2)
            throw uhd::io_error("load_fpga: get_usb_speed returned invalid USB speed (not 2 or 3).");

        UHD_ASSERT_THROW(transfer_size <= VREQ_MAX_SIZE);

        unsigned char out_buff[VREQ_MAX_SIZE];

        // Request loopback read, which will indicate the firmware's current control request buffer size
        // Make sure that if operating as USB2, requested length is within spec
        int ntoread = std::min(transfer_size, (int)sizeof(out_buff));
        int nread = fx3_control_read(B200_VREQ_LOOP, 0, 0, out_buff, ntoread, 1000);
        if (nread < 0)
            throw uhd::io_error((boost::format("load_fpga: unable to complete firmware loopback request (%d: %s)") % nread % libusb_error_name(nread)).str());
        else if (nread != ntoread)
            throw uhd::io_error((boost::format("load_fpga: short read on firmware loopback request (expecting: %d, returned: %d)") % ntoread % nread).str());
        transfer_size = std::min(transfer_size, nread); // Select the smaller value

        size_t file_size = 0;
        {
            std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
            file_size = size_t(file.tellg());
        }

        std::ifstream file;
        file.open(filename, std::ios::in | std::ios::binary);

        if (!file.good()) {
            throw uhd::io_error("load_fpga: cannot open FPGA input file.");
        }

        // Zero the hash, in case we abort programming another image and revert to the previously programmed image
        usrp_set_fpga_hash(0);

        memset(out_buff, 0x00, sizeof(out_buff));
        bytes_to_xfer = 1;
        ret = fx3_control_write(B200_VREQ_FPGA_CONFIG, 0, 0, out_buff, bytes_to_xfer, 1000);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to start FPGA config (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_xfer)
            throw uhd::io_error((boost::format("Short write on start FPGA config (expecting: %d, returned: %d)") % bytes_to_xfer % ret).str());

        wait_count = 0;
        do {
            fx3_state = get_fx3_status();

            if((wait_count >= 500) || (fx3_state == FX3_STATE_ERROR) || (fx3_state == FX3_STATE_UNDEFINED)) {
                return fx3_state;
            }

            boost::this_thread::sleep(boost::posix_time::milliseconds(10));

            wait_count++;
        } while(fx3_state != FX3_STATE_FPGA_READY);

        if (load_img_msg) UHD_MSG(status) << "Loading FPGA image: " \
            << filestring << "..." << std::flush;

        bytes_to_xfer = 1;
        ret = fx3_control_write(B200_VREQ_FPGA_START, 0, 0, out_buff, bytes_to_xfer, 1000);
        if (ret < 0)
            throw uhd::io_error((boost::format("Failed to start FPGA bitstream (%d: %s)") % ret % libusb_error_name(ret)).str());
        else if (ret != bytes_to_xfer)
            throw uhd::io_error((boost::format("Short write on start FPGA bitstream (expecting: %d, returned: %d)") % bytes_to_xfer % ret).str());

        wait_count = 0;
        do {
            fx3_state = get_fx3_status();

            if((wait_count >= 1000) || (fx3_state == FX3_STATE_ERROR) || (fx3_state == FX3_STATE_UNDEFINED)) {
                return fx3_state;
            }

            boost::this_thread::sleep(boost::posix_time::milliseconds(10));

            wait_count++;
        } while(fx3_state != FX3_STATE_CONFIGURING_FPGA);

        size_t bytes_sent = 0;
        while (!file.eof()) {
            file.read((char *) out_buff, transfer_size);
            const std::streamsize n = file.gcount();
            if(n == 0)
                continue;

            boost::uint16_t transfer_count = boost::uint16_t(n);

            /* Send the data to the device. */
            int nwritten = fx3_control_write(B200_VREQ_FPGA_DATA, 0, 0, out_buff, transfer_count, 5000);
            if (nwritten < 0)
                throw uhd::io_error((boost::format("load_fpga: cannot write bitstream to FX3 (%d: %s)") % nwritten % libusb_error_name(nwritten)).str());
            else if (nwritten != transfer_count)
                throw uhd::io_error((boost::format("load_fpga: short write while transferring bitstream to FX3  (expecting: %d, returned: %d)") % transfer_count % nwritten).str());

            if (load_img_msg)
            {
                if (bytes_sent == 0) UHD_MSG(status) << "  0%" << std::flush;
                const size_t percent_before = size_t((bytes_sent*100)/file_size);
                bytes_sent += transfer_count;
                const size_t percent_after = size_t((bytes_sent*100)/file_size);
                if (percent_before != percent_after)
                {
                    UHD_MSG(status) << "\b\b\b\b" << std::setw(3) << percent_after << "%" << std::flush;
                }
            }
        }

        file.close();

        wait_count = 0;
        do {
            fx3_state = get_fx3_status();

            if((wait_count >= 500) || (fx3_state == FX3_STATE_ERROR) || (fx3_state == FX3_STATE_UNDEFINED)) {
                return fx3_state;
            }

            boost::this_thread::sleep(boost::posix_time::milliseconds(10));

            wait_count++;
        } while(fx3_state != FX3_STATE_RUNNING);

        usrp_set_fpga_hash(hash);

        if (load_img_msg)
            UHD_MSG(status) << "\b\b\b\b done" << std::endl;

        return 0;
    }

private:
    usb_control::sptr _usb_ctrl;
};


std::string b200_iface::fx3_state_string(boost::uint8_t state)
{
    switch (state)
    {
    case FX3_STATE_FPGA_READY:
        return std::string("Ready");
    case FX3_STATE_CONFIGURING_FPGA:
        return std::string("Configuring FPGA");
    case FX3_STATE_BUSY:
        return std::string("Busy");
    case FX3_STATE_RUNNING:
        return std::string("Running");
    case FX3_STATE_UNCONFIGURED:
        return std::string("Unconfigured");
    case FX3_STATE_ERROR:
        return std::string("Error");
    default:
        break;
    }
    return std::string("Unknown");
}

/***********************************************************************
 * Make an instance of the implementation
 **********************************************************************/
b200_iface::sptr b200_iface::make(usb_control::sptr usb_ctrl)
{
    return sptr(new b200_iface_impl(usb_ctrl));
}
