//
// Copyright 2011-2012 Ettus Research LLC
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
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <sys/ioctl.h> //ioctl
#include <fcntl.h> //open, close
#include <linux/usrp_e.h> //ioctl structures and constants
#include <poll.h> //poll
#include <boost/thread/thread.hpp> //sleep
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <fstream>

using namespace uhd;
using namespace uhd::transport;

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
 * Protection for dual GPIO access - sometimes MISO, sometimes have resp
 **********************************************************************/
static boost::mutex gpio_irq_resp_mutex;

/***********************************************************************
 * Aux spi implementation
 **********************************************************************/
class aux_spi_iface_impl : public spi_iface{
public:
    aux_spi_iface_impl(void):
        spi_sclk_gpio(65, "out"),
        spi_sen_gpio(186, "out"),
        spi_mosi_gpio(145, "out"),
        spi_miso_gpio(147, "in")
    {
        this->spi_sen_gpio(1); //not selected
        this->spi_sclk_gpio(0); //into reset
        this->spi_sclk_gpio(1); //out of reset
    }

    boost::uint32_t transact_spi(
        int, const spi_config_t &, //not used params
        boost::uint32_t bits,
        size_t num_bits,
        bool readback
    ){
        boost::mutex::scoped_lock lock(gpio_irq_resp_mutex);

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
 * UART control implementation
 **********************************************************************/
#include <termios.h>
#include <cstring>
class uart_dev_iface : public uart_iface{
public:
    uart_dev_iface(const std::string &node){
        if ((_node_fd = ::open(node.c_str(), O_RDWR | O_NONBLOCK)) < 0){
            throw uhd::io_error("Failed to open " + node);
        }

        //init the tty settings w/ termios
        termios tio;
        std::memset(&tio,0,sizeof(tio));
        tio.c_iflag = IGNCR; //Ignore CR
        tio.c_oflag = OPOST | ONLCR; //Map NL to CR-NL on output
        tio.c_cflag = CS8 | CREAD | CLOCAL; // 8n1
        tio.c_lflag = 0;

        cfsetospeed(&tio, B115200);            // 115200 baud
        cfsetispeed(&tio, B115200);            // 115200 baud

        tcsetattr(_node_fd, TCSANOW, &tio);
    }

    void write_uart(const std::string &buf){
        const ssize_t ret = ::write(_node_fd, buf.c_str(), buf.size());
        if (size_t(ret) != buf.size()) UHD_LOG << ret;
    }

    std::string read_uart(double timeout){
        const boost::system_time exit_time = boost::get_system_time() + boost::posix_time::milliseconds(long(timeout*1000));

        std::string line;
        while(true){
            char ch;
            const ssize_t ret = ::read(_node_fd, &ch, 1);

            //got a character -> process it
            if (ret == 1){
                const bool flush  = ch == '\n' or ch == '\r';
                if (flush and line.empty()) continue; //avoid flushing on empty lines
                line += std::string(1, ch);
                if (flush) break;
            }

            //didnt get a character, check the timeout
            else if (boost::get_system_time() > exit_time){
                break;
            }

            //otherwise sleep for a bit
            else{
                boost::this_thread::sleep(boost::posix_time::milliseconds(10));
            }
        }
        return line;
    }

private: int _node_fd;
};

uhd::uart_iface::sptr e100_ctrl::make_gps_uart_iface(const std::string &node){
    return uhd::uart_iface::sptr(new uart_dev_iface(node));
}

/***********************************************************************
 * Simple managed buffers
 **********************************************************************/
struct e100_simpl_mrb : managed_recv_buffer
{
    usrp_e_ctl32 data;
    e100_ctrl *ctrl;

    void release(void)
    {
        //NOP
    }

    sptr get_new(void)
    {
        const size_t max_words32 = 8; //.LAST_ADDR(10'h00f)) resp_fifo_to_gpmc

        //load the data struct
        data.offset = 0;
        data.count = max_words32;

        //call the ioctl
        ctrl->ioctl(USRP_E_READ_CTL32, &data);

        if (data.buf[0] == 0 or ~data.buf[0] == 0) return sptr(); //bad VRT hdr, treat like timeout

        return make(this, data.buf, sizeof(data.buf));
    }
};

struct e100_simpl_msb : managed_send_buffer
{
    usrp_e_ctl32 data;
    e100_ctrl *ctrl;

    void release(void)
    {
        const size_t max_words32 = 8; //.LAST_ADDR(10'h00f)) resp_fifo_to_gpmc

        //load the data struct
        data.offset = 0;
        data.count = max_words32;

        //call the ioctl
        ctrl->ioctl(USRP_E_WRITE_CTL32, &data);
    }

    sptr get_new(void)
    {
        return make(this, data.buf, sizeof(data.buf));
    }
};

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

        std::ofstream edge_file("/sys/class/gpio/gpio147/edge");
        edge_file << "rising" << std::endl << std::flush;
        edge_file.close();
        _irq_fd = ::open("/sys/class/gpio/gpio147/value", O_RDONLY);
        if (_irq_fd < 0) UHD_MSG(error) << "Unable to open GPIO for IRQ\n";
    }

    ~e100_ctrl_impl(void){
        ::close(_irq_fd);
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
     * The managed buffer interface
     ******************************************************************/
    UHD_INLINE bool resp_read(void)
    {
        //thread stuff ensures that this GPIO isnt shared
        boost::mutex::scoped_lock lock(gpio_irq_resp_mutex);

        //perform a read of the GPIO IRQ state
        char ch;
        ::read(_irq_fd, &ch, sizeof(ch));
        ::lseek(_irq_fd, SEEK_SET, 0);
        return ch == '1';
    }

    UHD_INLINE bool resp_wait(const double timeout)
    {
        //perform a check, if it fails, poll
        if (this->resp_read()) return true;

        //poll IRQ GPIO for some action
        pollfd pfd;
        pfd.fd = _irq_fd;
        pfd.events = POLLPRI | POLLERR;
        ::poll(&pfd, 1, long(timeout*1000)/*ms*/);

        //perform a GPIO read again for result
        return this->resp_read();
    }

    managed_recv_buffer::sptr get_recv_buff(double timeout)
    {
        if (not this->resp_wait(timeout))
        {
            return managed_recv_buffer::sptr();
        }

        _mrb.ctrl = this;
        return _mrb.get_new();
    }

    managed_send_buffer::sptr get_send_buff(double)
    {
        _msb.ctrl = this;
        return _msb.get_new();
    }

    size_t get_num_recv_frames(void) const{
        return 1;
    }

    size_t get_recv_frame_size(void) const{
        return sizeof(_mrb.data.buf);
    }

    size_t get_num_send_frames(void) const{
        return 1;
    }

    size_t get_send_frame_size(void) const{
        return sizeof(_msb.data.buf);
    }

private:
    int _node_fd;
    int _irq_fd;
    boost::mutex _ioctl_mutex;
    e100_simpl_mrb _mrb;
    e100_simpl_msb _msb;
};

/***********************************************************************
 * Public Make Function
 **********************************************************************/
e100_ctrl::sptr e100_ctrl::make(const std::string &node){
    return sptr(new e100_ctrl_impl(node));
}
