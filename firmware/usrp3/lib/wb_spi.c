/*
 * Copyright 2014 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wb_spi.h>
#include <trace.h>

typedef struct {
  volatile uint32_t data0;
  volatile uint32_t data1;
  volatile uint32_t data2;
  volatile uint32_t data3;
  volatile uint32_t ctrl_status;
  volatile uint32_t clkdiv;
  volatile uint32_t slavesel;
} wb_spi_regs_t;

#define WB_SPI_REGS(base) ((wb_spi_regs_t *) base)

// Masks for different parts of CTRL reg
#define WB_SPI_CTRL_AUTO_SS     (1 << 13)
#define WB_SPI_CTRL_IE          (1 << 12)
#define WB_SPI_CTRL_LSB         (1 << 11)
#define WB_SPI_CTRL_TXNEG       (1 << 10)
#define WB_SPI_CTRL_RXNEG       (1 << 9)
#define WB_SPI_CTRL_GO_BSY      (1 << 8)
#define WB_SPI_CTRL_LENGTH(x)   (x & 0x7F)

static inline uint32_t _wb_spi_get_flags(const wb_spi_slave_t* slave)
{
    uint32_t flags = 0;
    //If the SPI slave samples on the rising edge then shift
    //data out on the falling edge.
    if (slave->mosi_edge == RISING) flags |= WB_SPI_CTRL_TXNEG;
    //If the SPI slave drives on the rising edge then shift
    //data in on the falling edge.
    if (slave->miso_edge == RISING) flags |= WB_SPI_CTRL_RXNEG;
    if (slave->lsb_first)           flags |= WB_SPI_CTRL_LSB;
    return flags;
}

static inline void _wait_for_xfer(const wb_spi_slave_t* slave)
{
    while (WB_SPI_REGS(slave->base)->ctrl_status & WB_SPI_CTRL_GO_BSY) {
        /*NOP*/
    }
}

void wb_spi_init(const wb_spi_slave_t* slave)
{
    WB_SPI_REGS(slave->base)->clkdiv = slave->clk_div;
    WB_SPI_REGS(slave->base)->slavesel = 0;

    //Do a dummy transaction with no slave selected to prime the engine
    uint32_t ctrl = WB_SPI_CTRL_LENGTH(8) | _wb_spi_get_flags(slave);
    WB_SPI_REGS(slave->base)->ctrl_status = ctrl | WB_SPI_CTRL_GO_BSY;
    _wait_for_xfer(slave);
}

void _wb_spi_transact_buf(
    const wb_spi_slave_t* slave, wb_spi_rw_mode_t rw_mode,
    const void* mosi_buf, void* miso_buf, uint32_t length,
    bool auto_slave_sel)
{
    if (length == 0) return;

    //Wait for previous transaction to finish
    _wait_for_xfer(slave);

    //Write SPI data register(s)
    if (mosi_buf) {
        uint8_t* mosi_bytes = (uint8_t*) mosi_buf;
        uint8_t bits_left = length;
        for (uint32_t reg_index = 0; reg_index < 4; reg_index++) {
            uint32_t word = 0;
            if (bits_left < 32) {
                if (bits_left <= 8) {
                    word = (uint32_t) mosi_bytes[0];
                } else if (bits_left <= 16) {
                    word = (((uint32_t) mosi_bytes[1]) << 0) |
                           (((uint32_t) mosi_bytes[0]) << 8);
                } else if (bits_left <= 24) {
                    word = (((uint32_t) mosi_bytes[2]) << 0) |
                           (((uint32_t) mosi_bytes[1]) << 8) |
                           (((uint32_t) mosi_bytes[0]) << 16);
                } else {
                    word = *((uint32_t*) mosi_bytes);
                }
                bits_left = 0;
            } else {
                word = *((uint32_t*) mosi_bytes);
                mosi_bytes += 4;
                bits_left -= 32;
            }

            switch (reg_index) {
                case 0: WB_SPI_REGS(slave->base)->data0 = word; break;
                case 1: WB_SPI_REGS(slave->base)->data1 = word; break;
                case 2: WB_SPI_REGS(slave->base)->data2 = word; break;
                case 3: WB_SPI_REGS(slave->base)->data3 = word; break;
            }

            if (bits_left == 0) break;
        }
    }

    //Compute flags for slave and write control register
    uint32_t ctrl = WB_SPI_CTRL_LENGTH(length) | _wb_spi_get_flags(slave);
    if (auto_slave_sel) ctrl |= WB_SPI_CTRL_AUTO_SS;
    WB_SPI_REGS(slave->base)->ctrl_status = ctrl;

    // Tell it which SPI slave device to access
    WB_SPI_REGS(slave->base)->slavesel    = slave->slave_sel;

    //Go go go!
    WB_SPI_REGS(slave->base)->ctrl_status = ctrl | WB_SPI_CTRL_GO_BSY;

    if (rw_mode == WRITE_READ) {
        //Wait for SPI read operation to complete
        _wait_for_xfer(slave);

        if (miso_buf) {
            //Read SPI data registers
            uint8_t* miso_bytes = (uint8_t*) miso_buf;
            uint8_t bits_left = length;
            for (uint32_t reg_index = 0; reg_index < 4; reg_index++) {
                uint32_t word = 0;
                switch (reg_index) {
                    case 0: word = WB_SPI_REGS(slave->base)->data0; break;
                    case 1: word = WB_SPI_REGS(slave->base)->data1; break;
                    case 2: word = WB_SPI_REGS(slave->base)->data2; break;
                    case 3: word = WB_SPI_REGS(slave->base)->data3; break;
                }

                if (bits_left < 32) {
                    if (bits_left <= 8) {
                        miso_bytes[0] = word & 0xFF;
                    } else if (bits_left <= 16) {
                        miso_bytes[1] = word & 0xFF;
                        miso_bytes[0] = (word >> 8) & 0xFF;
                    } else if (bits_left <= 24) {
                        miso_bytes[2] = word & 0xFF;
                        miso_bytes[1] = (word >> 8) & 0xFF;
                        miso_bytes[0] = (word >> 16) & 0xFF;
                    } else {
                        *((uint32_t*) miso_bytes) = word;
                    }
                    bits_left = 0;
                } else {
                    *((uint32_t*) miso_bytes) = word;
                    miso_bytes += 4;
                    bits_left -= 32;
                }

                if (bits_left == 0) break;
            }
        }
    }
}

void wb_spi_transact(
    const wb_spi_slave_t* slave, wb_spi_rw_mode_t rw_mode,
    const void* mosi_buf, void* miso_buf, uint32_t length)
{
    return _wb_spi_transact_buf(slave, rw_mode, mosi_buf, miso_buf, length, true);
}

void wb_spi_transact_man_ss(
    const wb_spi_slave_t* slave, wb_spi_rw_mode_t rw_mode,
    const void* mosi_buf, void* miso_buf, uint32_t length)
{
    return _wb_spi_transact_buf(slave, rw_mode, mosi_buf, miso_buf, length, false);
}

void wb_spi_slave_select(const wb_spi_slave_t* slave)
{
    //Wait for previous transactions to finish
    _wait_for_xfer(slave);
    //Disable auto slave select
    WB_SPI_REGS(slave->base)->ctrl_status = _wb_spi_get_flags(slave);
    //Manually select slave
    WB_SPI_REGS(slave->base)->slavesel    = slave->slave_sel;
}

void wb_spi_slave_deselect(const wb_spi_slave_t* slave)
{
    //Wait for previous transactions to finish
    _wait_for_xfer(slave);
    //Disable auto slave select
    WB_SPI_REGS(slave->base)->ctrl_status = _wb_spi_get_flags(slave);
    //Manually deselect slave
    WB_SPI_REGS(slave->base)->slavesel    = 0;
}
