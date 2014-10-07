
// Copyright 2012 Ettus Research LLC
/*
 * Copyright 2007 Free Software Foundation, Inc.
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

// NOTE: this driver left shifts a "7bit" I2C address by one bit and puts a R/W bit as bit 0.
// Some devices may specify there I2C address assuming this R/W bit is already in place.

#include <wb_i2c.h>

typedef struct {
  volatile uint32_t  prescaler_lo;	// r/w
  volatile uint32_t  prescaler_hi;	// r/w
  volatile uint32_t  ctrl;		// r/w
  volatile uint32_t  data;		// wr = transmit reg; rd = receive reg
  volatile uint32_t  cmd_status;	// wr = command reg;  rd = status reg
} i2c_regs_t;

#define i2c_regs ((i2c_regs_t *) base)

#define	I2C_CTRL_EN	(1 << 7)	// core enable
#define	I2C_CTRL_IE	(1 << 6)	// interrupt enable

//
// STA, STO, RD, WR, and IACK bits are cleared automatically
//
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

void wb_i2c_init(const uint32_t base, const size_t clk_rate)
{
    // prescaler divisor values for 100 kHz I2C [uses 5 * SCLK internally]
    const uint16_t prescaler = (clk_rate/(5 * 400000)) - 1;
  i2c_regs->prescaler_lo = prescaler & 0xff;
  i2c_regs->prescaler_hi = (prescaler >> 8) & 0xff;

  i2c_regs->ctrl = I2C_CTRL_EN; //| I2C_CTRL_IE;	// enable core
}

static inline void
wait_for_xfer(const uint32_t base)
{
  while (i2c_regs->cmd_status & I2C_ST_TIP)	// wait for xfer to complete
    ;
}

static inline bool
wait_chk_ack(const uint32_t base)
{
  wait_for_xfer(base);

  if ((i2c_regs->cmd_status & I2C_ST_RXACK) != 0){	// target NAK'd
    return false;
  }
  return true;
}

bool wb_i2c_read(const uint32_t base, const uint8_t i2c_addr, uint8_t *buf, size_t len)
{
  if (len == 0)			// reading zero bytes always works
    return true;

  while (i2c_regs->cmd_status & I2C_ST_BUSY)
    ;

  i2c_regs->data = (i2c_addr << 1) | 1;	 // 7 bit address and read bit (1)
  // generate START and write addr
  i2c_regs->cmd_status = I2C_CMD_WR | I2C_CMD_START;
  if (!wait_chk_ack(base))
    goto fail;

  for (; len > 0; buf++, len--){
    i2c_regs->cmd_status = I2C_CMD_RD | (len == 1 ? (I2C_CMD_NACK | I2C_CMD_STOP) : 0);
    wait_for_xfer(base);
    *buf = i2c_regs->data;
  }
  return true;

 fail:
  i2c_regs->cmd_status = I2C_CMD_STOP;  // generate STOP
  return false;
}

bool wb_i2c_write(const uint32_t base, const uint8_t i2c_addr, const uint8_t *buf, size_t len)
{
  while (i2c_regs->cmd_status & I2C_ST_BUSY)
    ;

  i2c_regs->data = (i2c_addr << 1) | 0;	 // 7 bit address and write bit (0)

  // generate START and write addr (and maybe STOP)
  i2c_regs->cmd_status = I2C_CMD_WR | I2C_CMD_START | (len == 0 ? I2C_CMD_STOP : 0);
  if (!wait_chk_ack(base))
    goto fail;

  for (; len > 0; buf++, len--){
    i2c_regs->data = *buf;
    i2c_regs->cmd_status = I2C_CMD_WR | (len == 1 ? I2C_CMD_STOP : 0);
    if (!wait_chk_ack(base))
      goto fail;
  }
  return true;

 fail:
  i2c_regs->cmd_status = I2C_CMD_STOP;  // generate STOP
  return false;
}

