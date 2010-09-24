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

#include "usrp1_iface.hpp"
#include "usrp_commands.h"
#include <uhd/utils/assert.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/format.hpp>
#include <stdexcept>
#include <iostream>
#include <iomanip>

using namespace uhd;
using namespace uhd::transport;

static const bool iface_debug = false;

class usrp1_iface_impl : public usrp1_iface{
public:
    /*******************************************************************
     * Structors
     ******************************************************************/
    usrp1_iface_impl(usrp_ctrl::sptr ctrl_transport)
    {
        _ctrl_transport = ctrl_transport; 
    }

    ~usrp1_iface_impl(void)
    {
        /* NOP */
    }

    /*******************************************************************
     * Peek and Poke
     ******************************************************************/
    void poke32(boost::uint32_t addr, boost::uint32_t value)
    {
        boost::uint32_t swapped = byteswap(value);

        if (iface_debug) {
            std::cout.fill('0');
            std::cout << "poke32(";
            std::cout << std::dec << std::setw(2) << addr << ", 0x";
            std::cout << std::hex << std::setw(8) << value << ")" << std::endl;
        }

        boost::uint8_t w_index_h = SPI_ENABLE_FPGA & 0xff;
        boost::uint8_t w_index_l = (SPI_FMT_MSB | SPI_FMT_HDR_1) & 0xff;

        int ret =_ctrl_transport->usrp_control_write(
                                          VRQ_SPI_WRITE,
                                          addr & 0x7f,
                                          (w_index_h << 8) | (w_index_l << 0),
                                          (unsigned char*) &swapped,
                                          sizeof(boost::uint32_t));

        if (ret < 0)
            std::cerr << "USRP: failed memory write: " << ret << std::endl;
    }

    void poke16(boost::uint32_t, boost::uint16_t)
    {
        //fpga only handles 32 bit writes
        std::cerr << "USRP: unsupported operation: poke16()" << std::endl;
    }

    boost::uint32_t peek32(boost::uint32_t addr)
    {
        boost::uint32_t value_out;

        boost::uint8_t w_index_h = SPI_ENABLE_FPGA & 0xff;
        boost::uint8_t w_index_l = (SPI_FMT_MSB | SPI_FMT_HDR_1) & 0xff;

        int ret = _ctrl_transport->usrp_control_read(
                                          VRQ_SPI_READ,
                                          0x80 | (addr & 0x7f),
                                          (w_index_h << 8) | (w_index_l << 0),
                                          (unsigned char*) &value_out,
                                          sizeof(boost::uint32_t));

        if (ret < 0)
            std::cerr << "USRP: failed memory read: " << ret << std::endl;

        return byteswap(value_out);
    }

    boost::uint16_t peek16(boost::uint32_t addr)
    {
        boost::uint32_t val = peek32(addr);
        return boost::uint16_t(val & 0xff);
    }

    /*******************************************************************
     * I2C
     ******************************************************************/
    static const size_t max_i2c_data_bytes = 64;

    //TODO: make this handle EEPROM page sizes. right now you can't write over a 16-byte boundary.
    //to accomplish this you'll have to have addr offset as a separate parameter.

    void write_i2c(boost::uint8_t addr, const byte_vector_t &bytes)
    {
        UHD_ASSERT_THROW(bytes.size() < max_i2c_data_bytes);

        unsigned char buff[max_i2c_data_bytes];
        std::copy(bytes.begin(), bytes.end(), buff);

        int ret = _ctrl_transport->usrp_i2c_write(addr & 0xff,
                                             buff,
                                             bytes.size());

        // TODO throw and catch i2c failures during eeprom read
        if (iface_debug && (ret < 0))
            std::cerr << "USRP: failed i2c write: " << ret << std::endl;
    }

    byte_vector_t read_i2c(boost::uint8_t addr, size_t num_bytes)
    {
        UHD_ASSERT_THROW(num_bytes < max_i2c_data_bytes);

        unsigned char buff[max_i2c_data_bytes];
        int ret = _ctrl_transport->usrp_i2c_read(addr & 0xff,
                                            buff,
                                            num_bytes);

        // TODO throw and catch i2c failures during eeprom read
        if (iface_debug && ((ret < 0) || (unsigned)ret < (num_bytes))) {
            std::cerr << "USRP: failed i2c read: " << ret << std::endl;
            return byte_vector_t(num_bytes, 0xff); 
        }

        byte_vector_t out_bytes;
        for (size_t i = 0; i < num_bytes; i++)
            out_bytes.push_back(buff[i]);

        return out_bytes; 
    }

    /*******************************************************************
     * SPI
     *
     * For non-readback transactions use the SPI_WRITE command, which is
     * simpler and uses the USB control buffer for OUT data. No data
     * needs to be returned.
     *
     * For readback transactions use SPI_TRANSACT, which places up to
     * 4 bytes of OUT data in the device request fields and uses the
     * control buffer for IN data.
     ******************************************************************/
    boost::uint32_t transact_spi(int which_slave,
                                 const spi_config_t &,
                                 boost::uint32_t bits,
                                 size_t num_bits,
                                 bool readback)
    {
        UHD_ASSERT_THROW((num_bits <= 32) && !(num_bits % 8));
        size_t num_bytes = num_bits / 8;

        // Byteswap on num_bytes
        unsigned char buff[4] = { 0 };
        for (size_t i = 1; i <= num_bytes; i++)
            buff[num_bytes - i] = (bits >> ((i - 1) * 8)) & 0xff;

        if (readback) {
            boost::uint8_t w_len_h = which_slave & 0xff;
            boost::uint8_t w_len_l = num_bytes & 0xff;

            int ret = _ctrl_transport->usrp_control_read(
                                         VRQ_SPI_TRANSACT,
                                         (buff[0] << 8) | (buff[1] << 0), 
                                         (buff[2] << 8) | (buff[3] << 0),
                                         buff,
                                         (w_len_h << 8) | (w_len_l << 0));

            if (ret < 0) {
                std::cout << "USRP: failed SPI readback transaction: "
                          << std::dec << ret << std::endl;
            }

            boost::uint32_t val = (((boost::uint32_t)buff[0]) <<  0) |
                                  (((boost::uint32_t)buff[1]) <<  8) |
                                  (((boost::uint32_t)buff[2]) << 16) |
                                  (((boost::uint32_t)buff[3]) << 24);
            return val; 
        }
        else {
            boost::uint8_t w_index_h = which_slave & 0xff;
            boost::uint8_t w_index_l = (SPI_FMT_MSB | SPI_FMT_HDR_0) & 0xff;

            int ret =_ctrl_transport->usrp_control_write(
                                          VRQ_SPI_WRITE,
                                          0x00,
                                          (w_index_h << 8) | (w_index_l << 0),
                                          buff, num_bytes);

            if (ret < 0) {
                std::cout << "USRP: failed SPI transaction: "
                          << std::dec << ret << std::endl;
            }

            return 0;
        }
    }

    /*******************************************************************
     * Firmware 
     *
     * This call is deprecated.
     ******************************************************************/
    void write_firmware_cmd(boost::uint8_t request,
                            boost::uint16_t value,
                            boost::uint16_t index,
                            unsigned char *buff,
                            boost::uint16_t length)
    {
        int ret;

        if (request & 0x80) {
            ret = _ctrl_transport->usrp_control_read(request,
                                                     value,
                                                     index,
                                                     buff,
                                                     length);
        }
        else {
            ret = _ctrl_transport->usrp_control_write(request,
                                                      value,
                                                      index,
                                                      buff,
                                                      length);
        }

        if (ret < 0)
            std::cerr << "USRP: failed firmware command: " << ret << std::endl;
    }

private:
    usrp_ctrl::sptr _ctrl_transport;
};

/***********************************************************************
 * Public Make Function
 **********************************************************************/
usrp1_iface::sptr usrp1_iface::make(usrp_ctrl::sptr ctrl_transport)
{
    return sptr(new usrp1_iface_impl(ctrl_transport));
}
