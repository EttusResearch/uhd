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

#include "e100_iface.hpp"
#include "e100_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <sys/ioctl.h> //ioctl
#include <fcntl.h> //open, close
#include <linux/usrp_e.h> //ioctl structures and constants
#include <boost/thread/thread.hpp> //sleep
#include <boost/format.hpp>
#include <boost/thread/mutex.hpp>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <iostream>
#include <fstream>

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
class e100_iface_impl : public e100_iface{
public:

    int get_file_descriptor(void){
        return _node_fd;
    }

    void open(const std::string &node){
        UHD_MSG(status) << "Opening device node " << node << "..." << std::endl;

        //open the device node and check file descriptor
        if ((_node_fd = ::open(node.c_str(), O_RDWR)) < 0){
            throw uhd::io_error("Failed to open " + node);
        }

        //check the module compatibility number
        int module_compat_num = ::ioctl(_node_fd, USRP_E_GET_COMPAT_NUMBER, NULL);
        if (module_compat_num != USRP_E_COMPAT_NUMBER){
            throw uhd::runtime_error(str(boost::format(
                "Expected module compatibility number 0x%x, but got 0x%x:\n"
                "The module build is not compatible with the host code build."
            ) % USRP_E_COMPAT_NUMBER % module_compat_num));
        }
    }

    void close(void){
        ::close(_node_fd);
        _node_fd = -1;
    }

    /*******************************************************************
     * Structors
     ******************************************************************/
    e100_iface_impl(void):
        _node_fd(-1),
        _i2c_dev_iface(i2c_dev_iface("/dev/i2c-3"))
    {
        mb_eeprom = mboard_eeprom_t(get_i2c_dev_iface(), mboard_eeprom_t::MAP_E100);
    }

    ~e100_iface_impl(void){
        if (_node_fd >= 0) this->close();
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
        if (which_slave == UE_SPI_SS_AD9522) return bitbang_spi(
            bits, num_bits, readback
        );

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

    boost::uint32_t bitbang_spi(
        boost::uint32_t bits,
        size_t num_bits,
        bool readback
    ){
        boost::uint32_t rb_bits = 0;

        _spi_bitbanger.spi_sen_gpio_write(0);

        for (size_t i = 0; i < num_bits; i++){
            _spi_bitbanger.spi_sclk_gpio_write(0);
            _spi_bitbanger.spi_mosi_gpio_write((bits >> (num_bits-i-1)) & 0x1);
            boost::this_thread::sleep(boost::posix_time::microseconds(10));
            if (readback) rb_bits = (rb_bits << 1) | _spi_bitbanger.spi_miso_gpio_read();
            _spi_bitbanger.spi_sclk_gpio_write(1);
            boost::this_thread::sleep(boost::posix_time::microseconds(10));
        }

        _spi_bitbanger.spi_sen_gpio_write(1);
        boost::this_thread::sleep(boost::posix_time::microseconds(100));

        return rb_bits;
    }

    class bitbang_spi_guts{
    public:
        bitbang_spi_guts(void){
            //setup gpio pin directions
            this->set_gpio_direction(spi_sclk_gpio, "out");
            this->set_gpio_direction(spi_sen_gpio, "out");
            this->set_gpio_direction(spi_mosi_gpio, "out");
            this->set_gpio_direction(spi_miso_gpio, "in");

            //open the gpio pin values
            _spi_sclk_gpio_value.open(str(boost::format("/sys/class/gpio/gpio%d/value") % spi_sclk_gpio).c_str());
            _spi_sen_gpio_value.open(str(boost::format("/sys/class/gpio/gpio%d/value") % spi_sen_gpio).c_str());
            _spi_mosi_gpio_value.open(str(boost::format("/sys/class/gpio/gpio%d/value") % spi_mosi_gpio).c_str());
            _spi_miso_gpio_value.open(str(boost::format("/sys/class/gpio/gpio%d/value") % spi_miso_gpio).c_str());
        }

        ~bitbang_spi_guts(void){
            this->set_gpio_direction(spi_sclk_gpio, "in");
            this->set_gpio_direction(spi_sen_gpio, "in");
            this->set_gpio_direction(spi_mosi_gpio, "in");
        }

        void spi_sclk_gpio_write(int val){
            _spi_sclk_gpio_value << val << std::endl << std::flush;
        }

        void spi_sen_gpio_write(int val){
            _spi_sen_gpio_value << val << std::endl << std::flush;
        }

        void spi_mosi_gpio_write(int val){
            _spi_mosi_gpio_value << val << std::endl << std::flush;
        }

        int spi_miso_gpio_read(void){
            std::string val;
            std::getline(_spi_miso_gpio_value, val);
            _spi_miso_gpio_value.seekg(0);
            return int(val.at(0) - '0') & 0x1;
        }

    private:
        enum{
            spi_sclk_gpio = 65,
            spi_sen_gpio = 186,
            spi_mosi_gpio = 145,
            spi_miso_gpio = 147,
        };

        void set_gpio_direction(int gpio_num, const std::string &dir){
            std::ofstream export_file("/sys/class/gpio/export");
            export_file << gpio_num << std::endl << std::flush;
            export_file.close();

            std::ofstream dir_file(str(boost::format("/sys/class/gpio/gpio%d/direction") % gpio_num).c_str());
            dir_file << dir << std::endl << std::flush;
            dir_file.close();
        }

        std::ofstream _spi_sclk_gpio_value, _spi_sen_gpio_value, _spi_mosi_gpio_value;
        std::ifstream _spi_miso_gpio_value;
    };

    /*******************************************************************
     * UART
     ******************************************************************/
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
    bitbang_spi_guts _spi_bitbanger;
};

/***********************************************************************
 * Public Make Function
 **********************************************************************/
e100_iface::sptr e100_iface::make(void){
    return sptr(new e100_iface_impl());
}
