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
#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fstream>

using namespace uhd;
using namespace uhd::usrp;

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

//We only init the gpios when we have to use them (in the aux spi call).
//This way, the device discovery cannot unexport them from another process.
struct iface_gpios_type{
    typedef boost::shared_ptr<iface_gpios_type> sptr;
    iface_gpios_type(void):
        spi_sclk_gpio(65, "out"),
        spi_sen_gpio(186, "out"),
        spi_mosi_gpio(145, "out"),
        spi_miso_gpio(147, "in"){}
    gpio spi_sclk_gpio, spi_sen_gpio, spi_mosi_gpio, spi_miso_gpio;
};

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

        //perform a global reset after opening
        this->poke32(E100_REG_GLOBAL_RESET, 0);

        //and now is a good time to init the i2c
        this->i2c_init();
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
        boost::mutex::scoped_lock lock(_ioctl_mutex);

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
    static const boost::uint32_t i2c_datarate = 400000;
    static const boost::uint32_t wishbone_clk = 64000000; //FIXME should go somewhere else

    void i2c_init(void) {
        //init I2C FPGA interface.
        poke16(E100_REG_I2C_CTRL, 0x0000);
        //set prescalers to operate at 400kHz: WB_CLK is 64MHz...
        boost::uint16_t prescaler = wishbone_clk / (i2c_datarate*5) - 1;
        poke16(E100_REG_I2C_PRESCALER_LO, prescaler & 0xFF);
        poke16(E100_REG_I2C_PRESCALER_HI, (prescaler >> 8) & 0xFF);
        poke16(E100_REG_I2C_CTRL, I2C_CTRL_EN); //enable I2C core
    }

    void i2c_wait(void){
        for (size_t i = 0; i < 100; i++){
            if ((this->peek16(E100_REG_I2C_CMD_STATUS) & I2C_ST_TIP) == 0) return;
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }
        UHD_MSG(error) << "i2c_wait: timeout" << std::endl;
    }

    bool wait_chk_ack(void){
        i2c_wait();
        return (this->peek16(E100_REG_I2C_CMD_STATUS) & I2C_ST_RXACK) == 0;
    }

    void write_i2c(boost::uint8_t addr, const byte_vector_t &bytes){
        poke16(E100_REG_I2C_DATA, (addr << 1) | 0); //addr and read bit (0)
        poke16(E100_REG_I2C_CMD_STATUS, I2C_CMD_WR | I2C_CMD_START | (bytes.size() == 0 ? I2C_CMD_STOP : 0));

        //wait for previous transfer to complete
        if(!wait_chk_ack()) {
            poke16(E100_REG_I2C_CMD_STATUS, I2C_CMD_STOP);
            return;
        }

        for(size_t i = 0; i < bytes.size(); i++) {
            poke16(E100_REG_I2C_DATA, bytes[i]);
            poke16(E100_REG_I2C_CMD_STATUS, I2C_CMD_WR | ((i == (bytes.size() - 1)) ? I2C_CMD_STOP : 0));
            if(!wait_chk_ack()) {
                poke16(E100_REG_I2C_CMD_STATUS, I2C_CMD_STOP);
                return;
            }
        }
    }

    byte_vector_t read_i2c(boost::uint8_t addr, size_t num_bytes){
        byte_vector_t bytes;
        if(num_bytes == 0) return bytes;

        while (peek16(E100_REG_I2C_CMD_STATUS) & I2C_ST_BUSY);

        poke16(E100_REG_I2C_DATA, (addr << 1) | 1); //addr and read bit (1)
        poke16(E100_REG_I2C_CMD_STATUS, I2C_CMD_WR | I2C_CMD_START);
        //wait for previous transfer to complete
        if(!wait_chk_ack()) {
            poke16(E100_REG_I2C_CMD_STATUS, I2C_CMD_STOP);
        }
        for(; num_bytes > 0; num_bytes--) {
            poke16(E100_REG_I2C_CMD_STATUS, I2C_CMD_RD | ((num_bytes == 1) ? (I2C_CMD_STOP | I2C_CMD_NACK) : 0));
            i2c_wait();
            boost::uint8_t readback = peek16(E100_REG_I2C_DATA) & 0xFF;
            bytes.push_back(readback);
        }
        return bytes;
    }

    /*******************************************************************
     * SPI
     ******************************************************************/
    void spi_wait(void) {
        for (size_t i = 0; i < 100; i++){
            if ((this->peek16(E100_REG_SPI_CTRL) & SPI_CTRL_GO_BSY) == 0) return;
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }
        UHD_MSG(error) << "spi_wait: timeout" << std::endl;
    }

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

        UHD_ASSERT_THROW(num_bits <= 32 and (num_bits % 8) == 0);

        int edge_flags = ((config.miso_edge==spi_config_t::EDGE_FALL) ? SPI_CTRL_RXNEG : 0) |
                         ((config.mosi_edge==spi_config_t::EDGE_FALL) ? 0 : SPI_CTRL_TXNEG)
                         ;
        boost::uint16_t ctrl = SPI_CTRL_ASS | (SPI_CTRL_CHAR_LEN_MASK & num_bits) | edge_flags;

        spi_wait();
        poke16(E100_REG_SPI_DIV, 0x0001); // = fpga_clk / 4
        poke32(E100_REG_SPI_SS, which_slave & 0xFFFF);
        poke32(E100_REG_SPI_TXRX0, bits);
        poke16(E100_REG_SPI_CTRL, ctrl);
        poke16(E100_REG_SPI_CTRL, ctrl | SPI_CTRL_GO_BSY);

        if (not readback) return 0;
        spi_wait();
        return peek32(E100_REG_SPI_TXRX0);
    }

    boost::uint32_t bitbang_spi(
        boost::uint32_t bits, size_t num_bits, bool readback
    ){
        if (_gpios.get() == NULL) { //init on demand...
            _gpios = iface_gpios_type::sptr(new iface_gpios_type());
        }

        boost::uint32_t rb_bits = 0;
        _gpios->spi_sen_gpio(0);

        for (size_t i = 0; i < num_bits; i++){
            _gpios->spi_sclk_gpio(0);
            _gpios->spi_mosi_gpio((bits >> (num_bits-i-1)) & 0x1);
            boost::this_thread::sleep(boost::posix_time::microseconds(10));
            if (readback) rb_bits = (rb_bits << 1) | _gpios->spi_miso_gpio();
            _gpios->spi_sclk_gpio(1);
            boost::this_thread::sleep(boost::posix_time::microseconds(10));
        }

        _gpios->spi_sen_gpio(1);
        boost::this_thread::sleep(boost::posix_time::microseconds(100));
        return rb_bits;
    }

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
    boost::mutex _ioctl_mutex;
    iface_gpios_type::sptr _gpios;
};

/***********************************************************************
 * Public Make Function
 **********************************************************************/
e100_iface::sptr e100_iface::make(void){
    return sptr(new e100_iface_impl());
}
