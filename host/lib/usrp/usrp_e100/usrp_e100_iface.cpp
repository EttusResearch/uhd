//
// Copyright 2010-2011 Ettus Research LLC
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

#include "usrp_e100_iface.hpp"
#include "usrp_e100_regs.hpp"
#include <uhd/exception.hpp>
#include <sys/ioctl.h> //ioctl
#include <fcntl.h> //open, close
#include <linux/usrp_e.h> //ioctl structures and constants
#include <boost/format.hpp>
#include <boost/thread.hpp> //mutex
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdexcept>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * I2C device node implementation wrapper
 **********************************************************************/
class i2c_dev_iface : public i2c_iface{
public:
    i2c_dev_iface(const std::string &node){
        if ((_node_fd = ::open(node.c_str(), O_RDWR)) < 0){
            throw uhd::io_error("Failed to open " + node);
        }
    }

    ~i2c_dev_iface(void){
        ::close(_node_fd);
    }

    void write_i2c(boost::uint8_t addr, const byte_vector_t &bytes){
        byte_vector_t rw_bytes(bytes);

        //setup the message
        i2c_msg msg;
        msg.addr = addr;
        msg.flags = 0;
        msg.len = bytes.size();
        msg.buf = &rw_bytes.front();

        //setup the data
        i2c_rdwr_ioctl_data data;
        data.msgs = &msg;
        data.nmsgs = 1;

        //call the ioctl
        UHD_ASSERT_THROW(::ioctl(_node_fd, I2C_RDWR, &data) >= 0);
    }

    byte_vector_t read_i2c(boost::uint8_t addr, size_t num_bytes){
        byte_vector_t bytes(num_bytes);

        //setup the message
        i2c_msg msg;
        msg.addr = addr;
        msg.flags = I2C_M_RD;
        msg.len = bytes.size();
        msg.buf = &bytes.front();

        //setup the data
        i2c_rdwr_ioctl_data data;
        data.msgs = &msg;
        data.nmsgs = 1;

        //call the ioctl
        UHD_ASSERT_THROW(::ioctl(_node_fd, I2C_RDWR, &data) >= 0);

        return bytes;
    }

private: int _node_fd;
};

/***********************************************************************
 * USRP-E100 interface implementation
 **********************************************************************/
class usrp_e100_iface_impl : public usrp_e100_iface{
public:

    int get_file_descriptor(void){
        return _node_fd;
    }

    /*******************************************************************
     * Structors
     ******************************************************************/
    usrp_e100_iface_impl(const std::string &node):
        _i2c_dev_iface(i2c_dev_iface("/dev/i2c-3"))
    {
        //open the device node and check file descriptor
        if ((_node_fd = ::open(node.c_str(), O_RDWR)) < 0){
            throw uhd::io_error("Failed to open " + node);
        }

        //very first thing, reset all the wishbone, always do first!
        //disabled for now: this->poke32(UE_REG_CLEAR_GLOBAL, 0);

        mb_eeprom = mboard_eeprom_t(get_i2c_dev_iface(), mboard_eeprom_t::MAP_E100);
    }

    ~usrp_e100_iface_impl(void){
        //close the device node file descriptor
        ::close(_node_fd);
    }

    /*******************************************************************
     * IOCTL: provides the communication base for all other calls
     ******************************************************************/
    void ioctl(int request, void *mem){
        boost::mutex::scoped_lock lock(_ctrl_mutex);

        if (::ioctl(_node_fd, request, mem) < 0){
            throw uhd::os_error(str(
                boost::format("ioctl failed with request %d") % request
            ));
        }
    }

    /*******************************************************************
     * I2C device node interface
     ******************************************************************/
    i2c_iface &get_i2c_dev_iface(void){
        return _i2c_dev_iface;
    }

    /*******************************************************************
     * Peek and Poke
     ******************************************************************/
    void poke32(boost::uint32_t addr, boost::uint32_t value){
        //load the data struct
        usrp_e_ctl32 data;
        data.offset = addr;
        data.count = 1;
        data.buf[0] = value;

        //call the ioctl
        this->ioctl(USRP_E_WRITE_CTL32, &data);
    }

    void poke16(boost::uint32_t addr, boost::uint16_t value){
        //load the data struct
        usrp_e_ctl16 data;
        data.offset = addr;
        data.count = 1;
        data.buf[0] = value;

        //call the ioctl
        this->ioctl(USRP_E_WRITE_CTL16, &data);
    }

    boost::uint32_t peek32(boost::uint32_t addr){
        //load the data struct
        usrp_e_ctl32 data;
        data.offset = addr;
        data.count = 1;

        //call the ioctl
        this->ioctl(USRP_E_READ_CTL32, &data);

        return data.buf[0];
    }

    boost::uint16_t peek16(boost::uint32_t addr){
        //load the data struct
        usrp_e_ctl16 data;
        data.offset = addr;
        data.count = 1;

        //call the ioctl
        this->ioctl(USRP_E_READ_CTL16, &data);

        return data.buf[0];
    }

    /*******************************************************************
     * I2C
     ******************************************************************/
    static const size_t max_i2c_data_bytes = 10;

    void write_i2c(boost::uint8_t addr, const byte_vector_t &bytes){
        //allocate some memory for this transaction
        UHD_ASSERT_THROW(bytes.size() <= max_i2c_data_bytes);
        boost::uint8_t mem[sizeof(usrp_e_i2c) + max_i2c_data_bytes];

        //load the data struct
        usrp_e_i2c *data = reinterpret_cast<usrp_e_i2c*>(mem);
        data->addr = addr;
        data->len = bytes.size();
        std::copy(bytes.begin(), bytes.end(), data->data);

        //call the spi ioctl
        this->ioctl(USRP_E_I2C_WRITE, data);
    }

    byte_vector_t read_i2c(boost::uint8_t addr, size_t num_bytes){
        //allocate some memory for this transaction
        UHD_ASSERT_THROW(num_bytes <= max_i2c_data_bytes);
        boost::uint8_t mem[sizeof(usrp_e_i2c) + max_i2c_data_bytes];

        //load the data struct
        usrp_e_i2c *data = reinterpret_cast<usrp_e_i2c*>(mem);
        data->addr = addr;
        data->len = num_bytes;

        //call the spi ioctl
        this->ioctl(USRP_E_I2C_READ, data);

        //unload the data
        byte_vector_t bytes(data->len);
        UHD_ASSERT_THROW(bytes.size() == num_bytes);
        std::copy(data->data, data->data+bytes.size(), bytes.begin());
        return bytes;
    }

    /*******************************************************************
     * SPI
     ******************************************************************/
    boost::uint32_t transact_spi(
        int which_slave,
        const spi_config_t &config,
        boost::uint32_t bits,
        size_t num_bits,
        bool readback
    ){
        //load data struct
        usrp_e_spi data;
        data.readback = (readback)? UE_SPI_TXRX : UE_SPI_TXONLY;
        data.slave = which_slave;
        data.length = num_bits;
        data.data = bits;

        //load the flags
        data.flags = 0;
        data.flags |= (config.miso_edge == spi_config_t::EDGE_RISE)? UE_SPI_LATCH_RISE : UE_SPI_LATCH_FALL;
        data.flags |= (config.mosi_edge == spi_config_t::EDGE_RISE)? UE_SPI_PUSH_FALL  : UE_SPI_PUSH_RISE;

        //call the spi ioctl
        this->ioctl(USRP_E_SPI, &data);

        //unload the data
        return data.data;
    }
    
    void write_uart(boost::uint8_t, const std::string &) {
        throw uhd::not_implemented_error("Unhandled command write_uart()");
    }
    
    std::string read_uart(boost::uint8_t) {
        throw uhd::not_implemented_error("Unhandled command read_uart()");
    }

private:
    int _node_fd;
    i2c_dev_iface _i2c_dev_iface;
    boost::mutex _ctrl_mutex;
};

/***********************************************************************
 * Public Make Function
 **********************************************************************/
usrp_e100_iface::sptr usrp_e100_iface::make(const std::string &node){
    return sptr(new usrp_e100_iface_impl(node));
}
