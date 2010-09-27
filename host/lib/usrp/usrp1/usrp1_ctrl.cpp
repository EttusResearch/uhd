//
// Copyright 2010 Ettus Research LLC
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

#include "usrp1_ctrl.hpp"
#include "usrp_commands.h" 
#include <uhd/transport/usb_control.hpp>
#include <boost/functional/hash.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

using namespace uhd;

enum firmware_code {
    USRP_FPGA_LOAD_SUCCESS,
    USRP_FPGA_ALREADY_LOADED,
    USRP_FIRMWARE_LOAD_SUCCESS,
    USRP_FIRMWARE_ALREADY_LOADED
};

#define FX2_FIRMWARE_LOAD 0xa0

static const bool load_img_msg = true;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
/*!
 * Create a file hash
 * The hash will be used to identify the loaded firmware and fpga image 
 * \param filename file used to generate hash value
 * \return hash value in a size_t type
 */
static size_t generate_hash(const char *filename)
{
    std::ifstream file(filename);
    if (!file)
        std::cerr << "error: cannot open input file " << filename << std::endl;

    size_t hash = 0;

    char ch;
    while (file.get(ch)) {
        boost::hash_combine(hash, ch);
    }

    if (!file.eof())
        std::cerr << "error: file error " << filename << std::endl;

    file.close(); 
    return hash; 
}


/*!
 * Verify checksum of a Intel HEX record 
 * \param record a line from an Intel HEX file
 * \return true if record is valid, false otherwise 
 */
static bool checksum(std::string *record)
{

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
bool parse_record(std::string *record, unsigned int &len,
                  unsigned int &addr, unsigned int &type,
                  unsigned char* data)
{
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
 * USRP control implementation for device discovery and configuration
 */
class usrp_ctrl_impl : public usrp_ctrl {
public:
    usrp_ctrl_impl(uhd::transport::usb_control::sptr ctrl_transport)
    {
        _ctrl_transport = ctrl_transport;
    }


    ~usrp_ctrl_impl(void)
    {
        /* NOP */
    }


    int usrp_load_firmware(std::string filestring, bool force)
    {
        const char *filename = filestring.c_str();

        size_t hash = generate_hash(filename);

        size_t loaded_hash;
        if (usrp_get_firmware_hash(loaded_hash) < 0) {
            std::cerr << "firmware hash retrieval failed" << std::endl;
            return -1;
        }

        if (!force && (hash == loaded_hash))
            return USRP_FIRMWARE_ALREADY_LOADED;

        //FIXME: verify types
        unsigned int len;
        unsigned int addr;
        unsigned int type;
        unsigned char data[512];

        int ret;
        std::ifstream file;
        file.open(filename, std::ifstream::in);

        if (!file.good()) {
            std::cerr << "cannot open firmware input file" << std::endl;
            return -1; 
        }

        unsigned char reset_y = 1;
        unsigned char reset_n = 0;

        //hit the reset line
        if (load_img_msg) std::cout << "Loading firmware image " << filestring << "..." << std::flush;
        usrp_control_write(FX2_FIRMWARE_LOAD, 0xe600, 0,
                           &reset_y, 1);
 
        while (!file.eof()) {
           std::string record;
           file >> record;
      
            //check for valid record 
            if (!checksum(&record) || 
                    !parse_record(&record, len, addr, type, data)) {
                std::cerr << "error: bad record" << std::endl;
                file.close();
                return -1;
            }

            //type 0x00 is data
            if (type == 0x00) {
               ret = usrp_control_write(FX2_FIRMWARE_LOAD, addr, 0,
                                        data, len);
               if (ret < 0) {
                    std::cerr << "error: usrp_control_write failed: ";
                    std::cerr << ret << std::endl;
                    file.close();
                    return -1; 
                }
            }  
            //type 0x01 is end 
            else if (type == 0x01) {
                usrp_control_write(FX2_FIRMWARE_LOAD, 0xe600, 0,
                                   &reset_n, 1);
                usrp_set_firmware_hash(hash);
                file.close();

                //wait for things to settle
                boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                if (load_img_msg) std::cout << " done" << std::endl;
                return USRP_FIRMWARE_LOAD_SUCCESS; 
            }
            //type anything else is unhandled
            else {
                std::cerr << "error: unsupported record" << std::endl;
                file.close();
                return -1; 
            }
        }

        //file did not end 
        std::cerr << "error: bad record" << std::endl;
        file.close();
        return -1;
    }


    int usrp_load_fpga(std::string filestring)
    {
        const char *filename = filestring.c_str();

        size_t hash = generate_hash(filename);

        size_t loaded_hash;
        if (usrp_get_fpga_hash(loaded_hash) < 0) {
            std::cerr << "fpga hash retrieval failed" << std::endl;
            return -1;
        }

        if (hash == loaded_hash)
            return USRP_FPGA_ALREADY_LOADED;
        const int ep0_size = 64;
        unsigned char buf[ep0_size];
        int ret;

        if (load_img_msg) std::cout << "Loading FPGA image: " << filestring << "..." << std::flush;
        std::ifstream file;
        file.open(filename, std::ios::in | std::ios::binary);
        if (not file.good()) {
            std::cerr << "cannot open fpga input file" << std::endl;
            file.close();
            return -1;
        }

        if (usrp_control_write_cmd(VRQ_FPGA_LOAD, 0, FL_BEGIN) < 0) {
            std::cerr << "fpga load error" << std::endl;
            file.close();
            return -1;
        }

        ssize_t n;
        while ((n = file.readsome((char *)buf, sizeof(buf))) > 0) {
            ret = usrp_control_write(VRQ_FPGA_LOAD, 0, FL_XFER,
                                     buf, n);
            if (ret != n) {
                std::cerr << "fpga load error " << ret << std::endl;
                file.close();
                return -1;
            }
        }
 
        if (usrp_control_write_cmd(VRQ_FPGA_LOAD, 0, FL_END) < 0) {
            std::cerr << "fpga load error" << std::endl;
            file.close();
            return -1;
        }

        usrp_set_fpga_hash(hash);
        file.close();
        if (load_img_msg) std::cout << " done" << std::endl;
        return 0; 
    }

    int usrp_load_eeprom(std::string filestring)
    {
        const char *filename = filestring.c_str();
        const boost::uint16_t i2c_addr = 0x50;

        //FIXME: verify types
        int len;
        unsigned int addr;
        unsigned char data[256];
        unsigned char sendbuf[17];

        int ret;
        std::ifstream file;
        file.open(filename, std::ifstream::in);

        if (!file.good()) {
            std::cerr << "cannot open EEPROM input file" << std::endl;
            return -1; 
        }

        file.read((char *)data, 256);
        len = file.gcount();

        if(len == 256) {
          std::cerr << "error: image size too large" << std::endl;
          file.close();
          return -1;
        }

        const int pagesize = 16;
        addr = 0;
        while(len > 0) {
          sendbuf[0] = addr;
          memcpy(sendbuf+1, &data[addr], len > pagesize ? pagesize : len);
          ret = usrp_i2c_write(i2c_addr, sendbuf, (len > pagesize ? pagesize : len)+1);
          if (ret < 0) {
            std::cerr << "error: usrp_i2c_write failed: ";
            std::cerr << ret << std::endl;
            file.close();
            return -1; 
          }
          addr += pagesize;
          len -= pagesize;
          boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }
        file.close();
        return 0;
    }


    int usrp_set_led(int led_num, bool on)
    {
        return usrp_control_write_cmd(VRQ_SET_LED, on, led_num);
    }


    int usrp_get_firmware_hash(size_t &hash)
    {
        return usrp_control_read(0xa0, USRP_HASH_SLOT_0_ADDR, 0, 
                                 (unsigned char*) &hash, sizeof(size_t));
    }


    int usrp_set_firmware_hash(size_t hash)
    {
        return usrp_control_write(0xa0, USRP_HASH_SLOT_0_ADDR, 0,
                                  (unsigned char*) &hash, sizeof(size_t));

    }


    int usrp_get_fpga_hash(size_t &hash)
    {
        return usrp_control_read(0xa0, USRP_HASH_SLOT_1_ADDR, 0,
                                 (unsigned char*) &hash, sizeof(size_t));
    }


    int usrp_set_fpga_hash(size_t hash)
    {
        return usrp_control_write(0xa0, USRP_HASH_SLOT_1_ADDR, 0,
                                  (unsigned char*) &hash, sizeof(size_t));
    }

    int usrp_tx_enable(bool on)
    {
        return usrp_control_write_cmd(VRQ_FPGA_SET_TX_ENABLE, on, 0);
    }


    int usrp_rx_enable(bool on)
    {
        return usrp_control_write_cmd(VRQ_FPGA_SET_RX_ENABLE, on, 0); 
    }


    int usrp_tx_reset(bool on)
    {
        return usrp_control_write_cmd(VRQ_FPGA_SET_TX_RESET, on, 0); 
    }


    int usrp_control_write(boost::uint8_t request,
                           boost::uint16_t value,
                           boost::uint16_t index,
                           unsigned char *buff,
                           boost::uint16_t length)
    {
        return _ctrl_transport->submit(VRT_VENDOR_OUT,     // bmReqeustType
                                       request,            // bRequest
                                       value,              // wValue
                                       index,              // wIndex
                                       buff,               // data
                                       length);            // wLength 
    }


    int usrp_control_read(boost::uint8_t request,
                          boost::uint16_t value,
                          boost::uint16_t index,
                          unsigned char *buff,
                          boost::uint16_t length)
    {
        return _ctrl_transport->submit(VRT_VENDOR_IN,      // bmReqeustType
                                       request,            // bRequest
                                       value,              // wValue
                                       index,              // wIndex
                                       buff,               // data
                                       length);            // wLength
    }


    int usrp_control_write_cmd(boost::uint8_t request, boost::uint16_t value, boost::uint16_t index)
    {
        return usrp_control_write(request, value, index, 0, 0);
    }

    int usrp_i2c_write(boost::uint16_t i2c_addr, unsigned char *buf, boost::uint16_t len)
    {
        return usrp_control_write(VRQ_I2C_WRITE, i2c_addr, 0, buf, len);
    }

    int usrp_i2c_read(boost::uint16_t i2c_addr, unsigned char *buf, boost::uint16_t len)
    {
        return usrp_control_read(VRQ_I2C_READ, i2c_addr, 0, buf, len);
    }



private:
    uhd::transport::usb_control::sptr _ctrl_transport;
};

/***********************************************************************
 * Public make function for usrp_ctrl interface
 **********************************************************************/
usrp_ctrl::sptr usrp_ctrl::make(uhd::transport::usb_control::sptr ctrl_transport){
    return sptr(new usrp_ctrl_impl(ctrl_transport));
}

