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

#include "i2c_core_200.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <boost/thread/thread.hpp> //sleep
#include <boost/thread/mutex.hpp>

#define REG_I2C_WR_PRESCALER_LO (1 << 3) | 0
#define REG_I2C_WR_PRESCALER_HI (1 << 3) | 1
#define REG_I2C_WR_CTRL         (1 << 3) | 2
#define REG_I2C_WR_DATA         (1 << 3) | 3
#define REG_I2C_WR_CMD          (1 << 3) | 4
#define REG_I2C_RD_DATA         (0 << 3) | 3
#define REG_I2C_RD_ST           (0 << 3) | 4

//
// STA, STO, RD, WR, and IACK bits are cleared automatically
//

#define	I2C_CTRL_EN	(1 << 7)	// core enable
#define	I2C_CTRL_IE	(1 << 6)	// interrupt enable

#define	I2C_CMD_START	(1 << 7)	// generate (repeated) start condition
#define I2C_CMD_STOP	(1 << 6)	// generate stop condition
#define	I2C_CMD_RD	(1 << 5)	// read from slave
#define I2C_CMD_WR	(1 << 4)	// write to slave
#define	I2C_CMD_NACK	(1 << 3)	// when a rcvr, send ACK (ACK=0) or NACK (ACK=1)
#define I2C_CMD_RSVD_2	(1 << 2)	// reserved
#define	I2C_CMD_RSVD_1	(1 << 1)	// reserved
#define I2C_CMD_IACK	(1 << 0)	// set to clear pending interrupt

#define I2C_ST_RXACK	(1 << 7)	// Received acknowledgement from slave (1 = NAK, 0 = ACK)
#define	I2C_ST_BUSY	(1 << 6)	// 1 after START signal detected; 0 after STOP signal detected
#define	I2C_ST_AL	(1 << 5)	// Arbitration lost.  1 when core lost arbitration
#define	I2C_ST_RSVD_4	(1 << 4)	// reserved
#define	I2C_ST_RSVD_3	(1 << 3)	// reserved
#define	I2C_ST_RSVD_2	(1 << 2)	// reserved
#define I2C_ST_TIP	(1 << 1)	// Transfer-in-progress
#define	I2C_ST_IP	(1 << 0)	// Interrupt pending

using namespace uhd;

class i2c_core_200_impl : public i2c_core_200{
public:
    i2c_core_200_impl(wb_iface::sptr iface, const size_t base, const size_t readback):
        _iface(iface), _base(base), _readback(readback)
    {
        //init I2C FPGA interface.
        this->poke(REG_I2C_WR_CTRL, 0x0000);
        //set prescalers to operate at 400kHz: WB_CLK is 64MHz...
        static const boost::uint32_t i2c_datarate = 400000;
        static const boost::uint32_t wishbone_clk = 64000000; //FIXME should go somewhere else
        boost::uint16_t prescaler = wishbone_clk / (i2c_datarate*5) - 1;
        this->poke(REG_I2C_WR_PRESCALER_LO, prescaler & 0xFF);
        this->poke(REG_I2C_WR_PRESCALER_HI, (prescaler >> 8) & 0xFF);
        this->poke(REG_I2C_WR_CTRL, I2C_CTRL_EN); //enable I2C core
    }

    void write_i2c(
        boost::uint8_t addr,
        const byte_vector_t &bytes
    ){
        this->poke(REG_I2C_WR_DATA, (addr << 1) | 0); //addr and read bit (0)
        this->poke(REG_I2C_WR_CMD, I2C_CMD_WR | I2C_CMD_START | (bytes.size() == 0 ? I2C_CMD_STOP : 0));

        //wait for previous transfer to complete
        if (not wait_chk_ack()) {
            this->poke(REG_I2C_WR_CMD, I2C_CMD_STOP);
            return;
        }

        for (size_t i = 0; i < bytes.size(); i++) {
            this->poke(REG_I2C_WR_DATA, bytes[i]);
            this->poke(REG_I2C_WR_CMD, I2C_CMD_WR | ((i == (bytes.size() - 1)) ? I2C_CMD_STOP : 0));
            if(!wait_chk_ack()) {
                this->poke(REG_I2C_WR_CMD, I2C_CMD_STOP);
                return;
            }
        }
    }

    byte_vector_t read_i2c(
        boost::uint8_t addr,
        size_t num_bytes
    ){
        byte_vector_t bytes;
        if (num_bytes == 0) return bytes;

        while (this->peek(REG_I2C_RD_ST) & I2C_ST_BUSY){
            /* NOP */
        }

        this->poke(REG_I2C_WR_DATA, (addr << 1) | 1); //addr and read bit (1)
        this->poke(REG_I2C_WR_CMD, I2C_CMD_WR | I2C_CMD_START);
        //wait for previous transfer to complete
        if (not wait_chk_ack()) {
            this->poke(REG_I2C_WR_CMD, I2C_CMD_STOP);
        }
        for (size_t i = 0; i < num_bytes; i++) {
            this->poke(REG_I2C_WR_CMD, I2C_CMD_RD | ((num_bytes == i+1) ? (I2C_CMD_STOP | I2C_CMD_NACK) : 0));
            i2c_wait();
            bytes.push_back(this->peek(REG_I2C_RD_DATA));
        }
        return bytes;
    }

private:
    void i2c_wait(void) {
        for (size_t i = 0; i < 100; i++){
            if ((this->peek(REG_I2C_RD_ST) & I2C_ST_TIP) == 0) return;
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }
        UHD_MSG(error) << "i2c_core_200: i2c_wait timeout" << std::endl;
    }

    bool wait_chk_ack(void){
        i2c_wait();
        return (this->peek(REG_I2C_RD_ST) & I2C_ST_RXACK) == 0;
    }

    void poke(const size_t what, const boost::uint8_t cmd)
    {
        boost::mutex::scoped_lock lock(_mutex);
        _iface->poke32(_base, (what << 8) | cmd);
    }

    boost::uint8_t peek(const size_t what)
    {
        boost::mutex::scoped_lock lock(_mutex);
        _iface->poke32(_base, what << 8);
        return boost::uint8_t(_iface->peek32(_readback));
    }

    wb_iface::sptr _iface;
    const size_t _base;
    const size_t _readback;
    boost::mutex _mutex;
};

i2c_core_200::sptr i2c_core_200::make(wb_iface::sptr iface, const size_t base, const size_t readback){
    return sptr(new i2c_core_200_impl(iface, base, readback));
}
