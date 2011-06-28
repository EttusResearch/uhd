//
// Copyright 2011 Ettus Research LLC
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

#include "e100_ctrl.hpp"
#include "e100_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <sys/ioctl.h> //ioctl
#include <fcntl.h> //open, close
#include <linux/usrp_e.h> //ioctl structures and constants
#include <boost/thread/thread.hpp> //sleep
#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>
#include <fstream>

using namespace uhd;

/***********************************************************************
 * Sysfs GPIO wrapper class
 **********************************************************************/
class gpio{
public:
    gpio(const int num, const std::string &dir) : _num(num){
        this->set_xport("export");
        this->set_dir(dir);
        _value_file.open(str(boost::format("/sys/class/gpio/gpio%d/value") % num).c_str(), std::ios_base::in | std::ios_base::out);
    }
    ~gpio(void){
        _value_file.close();
        this->set_dir("in");
        this->set_xport("unexport");
    }
    void operator()(const int val){
        _value_file << val << std::endl << std::flush;
    }
    int operator()(void){
        std::string val;
        std::getline(_value_file, val);
        _value_file.seekg(0);
        return int(val.at(0) - '0') & 0x1;
    }
private:
    void set_xport(const std::string &xport){
        std::ofstream export_file(("/sys/class/gpio/" + xport).c_str());
        export_file << _num << std::endl << std::flush;
        export_file.close();
    }
    void set_dir(const std::string &dir){
        std::ofstream dir_file(str(boost::format("/sys/class/gpio/gpio%d/direction") % _num).c_str());
        dir_file << dir << std::endl << std::flush;
        dir_file.close();
    }
    const int _num;
    std::fstream _value_file;
};

/***********************************************************************
 * Aux spi implementation
 **********************************************************************/
class aux_spi_iface_impl : public spi_iface{
public:
    aux_spi_iface_impl(void):
        spi_sclk_gpio(65, "out"),
        spi_sen_gpio(186, "out"),
        spi_mosi_gpio(145, "out"),
        spi_miso_gpio(147, "in"){}

    boost::uint32_t transact_spi(
        int, const spi_config_t &, //not used params
        boost::uint32_t bits,
        size_t num_bits,
        bool readback
    ){
        boost::uint32_t rb_bits = 0;
        this->spi_sen_gpio(0);

        for (size_t i = 0; i < num_bits; i++){
            this->spi_sclk_gpio(0);
            this->spi_mosi_gpio((bits >> (num_bits-i-1)) & 0x1);
            boost::this_thread::sleep(boost::posix_time::microseconds(10));
            if (readback) rb_bits = (rb_bits << 1) | this->spi_miso_gpio();
            this->spi_sclk_gpio(1);
            boost::this_thread::sleep(boost::posix_time::microseconds(10));
        }

        this->spi_sen_gpio(1);
        boost::this_thread::sleep(boost::posix_time::microseconds(100));
        return rb_bits;
    }

private:
    gpio spi_sclk_gpio, spi_sen_gpio, spi_mosi_gpio, spi_miso_gpio;
};

uhd::spi_iface::sptr e100_ctrl::make_aux_spi_iface(void){
    return uhd::spi_iface::sptr(new aux_spi_iface_impl());
}

/***********************************************************************
 * I2C device node implementation wrapper
 **********************************************************************/
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
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

uhd::i2c_iface::sptr e100_ctrl::make_dev_i2c_iface(const std::string &node){
    return uhd::i2c_iface::sptr(new i2c_dev_iface(node));
}

/***********************************************************************
 * USRP-E100 control implementation
 **********************************************************************/
class e100_ctrl_impl : public e100_ctrl{
public:

    int get_file_descriptor(void){
        return _node_fd;
    }

    /*******************************************************************
     * Structors
     ******************************************************************/
    e100_ctrl_impl(const std::string &node){
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

        //perform a global reset after opening
        this->poke32(E100_REG_GLOBAL_RESET, 0);
    }

    ~e100_ctrl_impl(void){
        ::close(_node_fd);
    }

    /*******************************************************************
     * IOCTL: provides the communication base for all other calls
     ******************************************************************/
    void ioctl(int request, void *mem){
        boost::mutex::scoped_lock lock(_ioctl_mutex);

        if (::ioctl(_node_fd, request, mem) < 0){
            throw uhd::os_error(str(
                boost::format("ioctl failed with request %d") % request
            ));
        }
    }
    /*******************************************************************
     * Peek and Poke
     ******************************************************************/
    void poke32(wb_addr_type addr, boost::uint32_t value){
        //load the data struct
        usrp_e_ctl32 data;
        data.offset = addr;
        data.count = 1;
        data.buf[0] = value;

        //call the ioctl
        this->ioctl(USRP_E_WRITE_CTL32, &data);
    }

    void poke16(wb_addr_type addr, boost::uint16_t value){
        //load the data struct
        usrp_e_ctl16 data;
        data.offset = addr;
        data.count = 1;
        data.buf[0] = value;

        //call the ioctl
        this->ioctl(USRP_E_WRITE_CTL16, &data);
    }

    boost::uint32_t peek32(wb_addr_type addr){
        //load the data struct
        usrp_e_ctl32 data;
        data.offset = addr;
        data.count = 1;

        //call the ioctl
        this->ioctl(USRP_E_READ_CTL32, &data);

        return data.buf[0];
    }

    boost::uint16_t peek16(wb_addr_type addr){
        //load the data struct
        usrp_e_ctl16 data;
        data.offset = addr;
        data.count = 1;

        //call the ioctl
        this->ioctl(USRP_E_READ_CTL16, &data);

        return data.buf[0];
    }

private:
    int _node_fd;
    boost::mutex _ioctl_mutex;
};

/***********************************************************************
 * Public Make Function
 **********************************************************************/
e100_ctrl::sptr e100_ctrl::make(const std::string &node){
    return sptr(new e100_ctrl_impl(node));
}
