//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/i2c_core_100_wb32.hpp>
#include <chrono>
#include <thread>

#define REG_I2C_PRESCALER_LO _base + 0
#define REG_I2C_PRESCALER_HI _base + 4
#define REG_I2C_CTRL         _base + 8
#define REG_I2C_DATA         _base + 12
#define REG_I2C_CMD_STATUS   _base + 16

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

i2c_core_100_wb32::~i2c_core_100_wb32(void){
    /* NOP */
}

class i2c_core_100_wb32_wb32_impl : public i2c_core_100_wb32{
public:
    i2c_core_100_wb32_wb32_impl(wb_iface::sptr iface, const size_t base):
        _iface(iface), _base(base)
    {
        //init I2C FPGA interface.
        _iface->poke32(REG_I2C_CTRL, 0x0000);
        _iface->poke32(REG_I2C_CTRL, I2C_CTRL_EN); //enable I2C core
    }

    void set_clock_rate(const double rate)
    {
        static const uint32_t i2c_datarate = 400000;
        uint16_t prescaler = uint16_t(rate / (i2c_datarate*5) - 1);
        _iface->poke32(REG_I2C_PRESCALER_LO, prescaler & 0xFF);
        _iface->poke32(REG_I2C_PRESCALER_HI, (prescaler >> 8) & 0xFF);
    }

    void write_i2c(
        uint16_t addr,
        const byte_vector_t &bytes
    ){
        _iface->poke32(REG_I2C_DATA, (addr << 1) | 0); //addr and read bit (0)
        _iface->poke32(REG_I2C_CMD_STATUS, I2C_CMD_WR | I2C_CMD_START | (bytes.size() == 0 ? I2C_CMD_STOP : 0));

        //wait for previous transfer to complete
        if (not wait_chk_ack()) {
            _iface->poke32(REG_I2C_CMD_STATUS, I2C_CMD_STOP);
            return;
        }

        for (size_t i = 0; i < bytes.size(); i++) {
            _iface->poke32(REG_I2C_DATA, bytes[i]);
            _iface->poke32(REG_I2C_CMD_STATUS, I2C_CMD_WR | ((i == (bytes.size() - 1)) ? I2C_CMD_STOP : 0));
            if(!wait_chk_ack()) {
                _iface->poke32(REG_I2C_CMD_STATUS, I2C_CMD_STOP);
                return;
            }
        }
    }

    byte_vector_t read_i2c(
        uint16_t addr,
        size_t num_bytes
    ){
        byte_vector_t bytes;
        if (num_bytes == 0) return bytes;

        while (_iface->peek32(REG_I2C_CMD_STATUS) & I2C_ST_BUSY){
            /* NOP */
        }

        _iface->poke32(REG_I2C_DATA, (addr << 1) | 1); //addr and read bit (1)
        _iface->poke32(REG_I2C_CMD_STATUS, I2C_CMD_WR | I2C_CMD_START);
        //wait for previous transfer to complete
        if (not wait_chk_ack()) {
            _iface->poke32(REG_I2C_CMD_STATUS, I2C_CMD_STOP);
        }
        for (size_t i = 0; i < num_bytes; i++) {
            _iface->poke32(REG_I2C_CMD_STATUS, I2C_CMD_RD | ((num_bytes == i+1) ? (I2C_CMD_STOP | I2C_CMD_NACK) : 0));
            i2c_wait();
            bytes.push_back(uint8_t(_iface->peek32(REG_I2C_DATA)));
        }
        return bytes;
    }

    //override read_eeprom so we can write once, read all N bytes
    //the default implementation calls read i2c once per byte
    byte_vector_t read_eeprom(uint16_t addr, uint16_t offset, size_t num_bytes)
    {
        this->write_i2c(addr, byte_vector_t(1, uint8_t(offset)));
        return this->read_i2c(addr, num_bytes);
    }

private:
    void i2c_wait(void) {
        for (size_t i = 0; i < 10; i++)
        {
            if ((_iface->peek32(REG_I2C_CMD_STATUS) & I2C_ST_TIP) == 0) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        UHD_LOGGER_ERROR("CORES") << "i2c_core_100_wb32: i2c_wait timeout" ;
    }

    bool wait_chk_ack(void){
        i2c_wait();
        return (_iface->peek32(REG_I2C_CMD_STATUS) & I2C_ST_RXACK) == 0;
    }

    wb_iface::sptr _iface;
    const size_t _base;
};

i2c_core_100_wb32::sptr i2c_core_100_wb32::make(wb_iface::sptr iface, const size_t base)
{
    return sptr(new i2c_core_100_wb32_wb32_impl(iface, base));
}
