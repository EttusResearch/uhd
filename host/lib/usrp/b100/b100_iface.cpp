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

#include "b100_impl.hpp"
#include "usrp_commands.h"
#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/msg.hpp>
#include <boost/format.hpp>
#include <iomanip>
#include <iostream>

//FOR TESTING ONLY
#include "b100_regs.hpp"
#include <boost/thread/thread.hpp>
#include "usrp_i2c_addr.h"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * Constants
 **********************************************************************/
static const bool iface_debug = true;

/***********************************************************************
 * I2C + FX2 implementation wrapper
 **********************************************************************/
class b100_i2c_fx2_iface : public i2c_iface{
public:
    b100_i2c_fx2_iface(uhd::usrp::fx2_ctrl::sptr fx2_ctrl){
        _fx2_ctrl = fx2_ctrl;
    }

    void write_i2c(boost::uint8_t addr, const byte_vector_t &bytes)
    {
        UHD_ASSERT_THROW(bytes.size() < max_i2c_data_bytes);

        unsigned char buff[max_i2c_data_bytes];
        std::copy(bytes.begin(), bytes.end(), buff);

        int ret = _fx2_ctrl->usrp_i2c_write(addr & 0xff,
                                             buff,
                                             bytes.size());

        if (iface_debug && (ret < 0))
            uhd::runtime_error("USRP: failed i2c write");
    }

    byte_vector_t read_i2c(boost::uint8_t addr, size_t num_bytes)
    {
      UHD_ASSERT_THROW(num_bytes < max_i2c_data_bytes);

      unsigned char buff[max_i2c_data_bytes];
      int ret = _fx2_ctrl->usrp_i2c_read(addr & 0xff,
                                            buff,
                                            num_bytes);

      if (iface_debug && ((ret < 0) || (unsigned)ret < (num_bytes)))
          uhd::runtime_error("USRP: failed i2c read");

      byte_vector_t out_bytes;
      for (size_t i = 0; i < num_bytes; i++)
          out_bytes.push_back(buff[i]);

      return out_bytes;
    }

private:
    static const size_t max_i2c_data_bytes = 64;
    uhd::usrp::fx2_ctrl::sptr _fx2_ctrl;
};

/***********************************************************************
 * USRP-E100 interface implementation
 **********************************************************************/
class b100_iface_impl : public b100_iface{
public:
    /*******************************************************************
     * Structors
     ******************************************************************/
    b100_iface_impl(uhd::usrp::fx2_ctrl::sptr fx2_ctrl,
                      b100_ctrl::sptr fpga_ctrl) :
                      _fx2_i2c_iface(fx2_ctrl),
                      _fx2_ctrl(fx2_ctrl),
                      _fpga_ctrl(fpga_ctrl)
    {
        this->check_fw_compat();
        if (fpga_ctrl.get() != NULL){
            enable_gpif(1);
            i2c_init();
            this->check_fpga_compat();
        }
        mb_eeprom = mboard_eeprom_t(get_fx2_i2c_iface(), mboard_eeprom_t::MAP_B000);
    }

    void check_fw_compat(void){
        unsigned char data[4]; //useless data buffer
        const boost::uint16_t fw_compat_num = _fx2_ctrl->usrp_control_read(
            VRQ_FW_COMPAT, 0, 0, data, sizeof(data)
        );
        if (fw_compat_num != B100_FW_COMPAT_NUM){
            throw uhd::runtime_error(str(boost::format(
                "Expected firmware compatibility number 0x%x, but got 0x%x:\n"
                "The firmware build is not compatible with the host code build."
            ) % B100_FW_COMPAT_NUM % fw_compat_num));
        }
    }

    void check_fpga_compat(void){
        const boost::uint16_t fpga_compat_num = this->peek16(B100_REG_MISC_COMPAT);
        if (fpga_compat_num != B100_FPGA_COMPAT_NUM){
            throw uhd::runtime_error(str(boost::format(
                "Expected FPGA compatibility number 0x%x, but got 0x%x:\n"
                "The FPGA build is not compatible with the host code build."
            ) % B100_FPGA_COMPAT_NUM % fpga_compat_num));
        }
    }

    ~b100_iface_impl(void)
    {
        /* NOP */
    }

    /*******************************************************************
     * Peek and Poke
     ******************************************************************/

    void poke(boost::uint32_t addr, const ctrl_data_t &data) {
        boost::mutex::scoped_lock lock(_ctrl_mutex);
        _fpga_ctrl->write(addr, data);
    }

    ctrl_data_t peek(boost::uint32_t addr, size_t len) {
        boost::mutex::scoped_lock lock(_ctrl_mutex);
        return _fpga_ctrl->read(addr, len);
    }

    void poke16(boost::uint32_t addr, boost::uint16_t value)
    {
        ctrl_data_t words(1);
        words[0] = value;
        poke(addr, words);
    }

    void poke32(boost::uint32_t addr, boost::uint32_t value)
    {
        //just a subset of poke() to maintain compatibility
        ctrl_data_t words(2);
        words[0] = value & 0x0000FFFF;
        words[1] = value >> 16;
        poke(addr, words);
    }

    boost::uint32_t peek32(boost::uint32_t addr)
    {
        ctrl_data_t words = peek(addr, 2);
        return boost::uint32_t((boost::uint32_t(words[1]) << 16) | words[0]);
    }

    boost::uint16_t peek16(boost::uint32_t addr)
    {
        ctrl_data_t words = peek(addr, 1);
        return boost::uint16_t(words[0]);
    }

    /*******************************************************************
     * I2C
     ******************************************************************/
    static const boost::uint32_t i2c_datarate = 400000;
    static const boost::uint32_t wishbone_clk = 64000000; //FIXME should go somewhere else

    void i2c_init(void) {
        //init I2C FPGA interface.
        poke16(B100_REG_I2C_CTRL, 0x0000);
        //set prescalers to operate at 400kHz: WB_CLK is 64MHz...
        boost::uint16_t prescaler = wishbone_clk / (i2c_datarate*5) - 1;
        poke16(B100_REG_I2C_PRESCALER_LO, prescaler & 0xFF);
        poke16(B100_REG_I2C_PRESCALER_HI, (prescaler >> 8) & 0xFF);
        poke16(B100_REG_I2C_CTRL, I2C_CTRL_EN); //enable I2C core
    }

    static const size_t max_i2c_data_bytes = 64;

    void i2c_wait_for_xfer(void)
    {
        while(this->peek16(B100_REG_I2C_CMD_STATUS) & I2C_ST_TIP)
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    bool wait_chk_ack(void) {
        i2c_wait_for_xfer();
        return (this->peek16(B100_REG_I2C_CMD_STATUS) & I2C_ST_RXACK) == 0;
    }

    void write_i2c(boost::uint8_t addr, const byte_vector_t &bytes)
    {
        poke16(B100_REG_I2C_DATA, (addr << 1) | 0); //addr and read bit (0)
        poke16(B100_REG_I2C_CMD_STATUS, I2C_CMD_WR | I2C_CMD_START | (bytes.size() == 0 ? I2C_CMD_STOP : 0));

        //wait for previous transfer to complete
        if(!wait_chk_ack()) {
            poke16(B100_REG_I2C_CMD_STATUS, I2C_CMD_STOP);
            return;
        }

        for(size_t i = 0; i < bytes.size(); i++) {
            poke16(B100_REG_I2C_DATA, bytes[i]);
            poke16(B100_REG_I2C_CMD_STATUS, I2C_CMD_WR | ((i == (bytes.size() - 1)) ? I2C_CMD_STOP : 0));
            if(!wait_chk_ack()) {
                poke16(B100_REG_I2C_CMD_STATUS, I2C_CMD_STOP);
                return;
            }
        }
    }

    byte_vector_t read_i2c(boost::uint8_t addr, size_t num_bytes)
    {
        byte_vector_t bytes;
        if(num_bytes == 0) return bytes;

        while (peek16(B100_REG_I2C_CMD_STATUS) & I2C_ST_BUSY);

        poke16(B100_REG_I2C_DATA, (addr << 1) | 1); //addr and read bit (1)
        poke16(B100_REG_I2C_CMD_STATUS, I2C_CMD_WR | I2C_CMD_START);
        //wait for previous transfer to complete
        if(!wait_chk_ack()) {
            poke16(B100_REG_I2C_CMD_STATUS, I2C_CMD_STOP);
        }

        for(; num_bytes > 0; num_bytes--) {
            poke16(B100_REG_I2C_CMD_STATUS, I2C_CMD_RD | ((num_bytes == 1) ? (I2C_CMD_STOP | I2C_CMD_NACK) : 0));
            i2c_wait_for_xfer();
            boost::uint8_t readback = peek16(B100_REG_I2C_DATA) & 0xFF;
            bytes.push_back(readback);
        }
        return bytes;
    }

    i2c_iface &get_fx2_i2c_iface(void){
        return _fx2_i2c_iface;
    }

    /*******************************************************************
     * SPI interface
     * Eventually this will be replaced with a control-channel system
     * to let the firmware do the actual write/readback cycles.
     * This keeps the bandwidth on the control channel down.
     ******************************************************************/

    void spi_wait(void) {
        while(peek32(B100_REG_SPI_CTRL) & SPI_CTRL_GO_BSY);
    }

    boost::uint32_t transact_spi(int which_slave,
                                 const spi_config_t &config,
                                 boost::uint32_t bits,
                                 size_t num_bits,
                                 bool readback)
    {
        UHD_ASSERT_THROW((num_bits <= 32) && !(num_bits % 8));

        int edge_flags = ((config.miso_edge==spi_config_t::EDGE_FALL) ? SPI_CTRL_RXNEG : 0) |
                         ((config.mosi_edge==spi_config_t::EDGE_FALL) ? 0 : SPI_CTRL_TXNEG)
                         ;

        boost::uint16_t ctrl = SPI_CTRL_ASS | (SPI_CTRL_CHAR_LEN_MASK & num_bits) | edge_flags;

        poke16(B100_REG_SPI_DIV, 0x0001); // = fpga_clk / 4
        poke32(B100_REG_SPI_SS, which_slave & 0xFFFF);
        poke32(B100_REG_SPI_TXRX0, bits);
        poke16(B100_REG_SPI_CTRL, ctrl);

        poke16(B100_REG_SPI_CTRL, ctrl | SPI_CTRL_GO_BSY);
        if(readback) {
            spi_wait();
            return peek32(B100_REG_SPI_TXRX0);
        }
        else {
            return 0;
        }
    }

    void reset_gpif(boost::uint16_t ep) {
        _fx2_ctrl->usrp_control_write(VRQ_RESET_GPIF, ep, ep, 0, 0);
    }

    void enable_gpif(bool en) {
        _fx2_ctrl->usrp_control_write(VRQ_ENABLE_GPIF, en ? 1 : 0, 0, 0, 0);
    }

    void clear_fpga_fifo(void) {
        _fx2_ctrl->usrp_control_write(VRQ_CLEAR_FPGA_FIFO, 0, 0, 0, 0);
    }

    void write_uart(boost::uint8_t, const std::string &) {
        throw uhd::not_implemented_error("Unhandled command write_uart()");
    }
    
    std::string read_uart(boost::uint8_t) {
        throw uhd::not_implemented_error("Unhandled command read_uart()");
    }

private:
    b100_i2c_fx2_iface _fx2_i2c_iface;
    uhd::usrp::fx2_ctrl::sptr _fx2_ctrl;
    b100_ctrl::sptr _fpga_ctrl;
    boost::mutex _ctrl_mutex;
};

/***********************************************************************
 * Public Make Function
 **********************************************************************/
b100_iface::sptr b100_iface::make(uhd::usrp::fx2_ctrl::sptr fx2_ctrl,
                                      b100_ctrl::sptr fpga_ctrl)
{
    return b100_iface::sptr(new b100_iface_impl(fx2_ctrl, fpga_ctrl));
}
