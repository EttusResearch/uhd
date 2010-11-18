//
// Copyright 2010 Ettus Research LLC
//
/*
 * Copyright 2007,2008 Free Software Foundation, Inc.
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
 
 //i2c_async.c: asynchronous (interrupt-driven) routines for I2C.
 //separated out here so we can have a small I2C lib for bootloader and
 //retain interrupt-driven I2C for the main app.

#include "memory_map.h"
#include "stdint.h"
#include <string.h>
#include "pic.h"
#include "nonstdio.h"
#include "i2c_async.h"
 
 //asynchronous (interrupt-driven) i2c state variables
volatile uint8_t i2c_buf[17]; //tx/rx data transfer buffer
volatile uint8_t *volatile i2c_bufptr = i2c_buf; //ptr to current position
volatile uint8_t i2c_len = 0; //length remaining in current transfer
volatile i2c_state_t i2c_state = I2C_STATE_IDLE; //current I2C transfer state
i2c_dir_t i2c_dir; //I2C transfer direction

void  (*volatile i2c_callback)(void); //function pointer to i2c callback to be called when transaction is complete
static void i2c_irq_handler(unsigned irq);
inline void i2c_async_err(void);

void i2c_register_handler(void) {
    pic_register_handler(IRQ_I2C, i2c_irq_handler);
}


static void i2c_irq_handler(unsigned irq) {
//i2c state machine.

  //printf("I2C irq handler\n");
  //first let's make sure nothing is f'ed up
  //TODO: uncomment this error checking when we have some way to handle errors
//  if(((i2c_regs->cmd_status & I2C_ST_RXACK) != 0) && i2c_dir == I2C_DIR_WRITE) { //we got a NACK and we didn't send it
//    printf("\tNACK received\n");
//    i2c_async_err();
//    return;
//  }// else printf("\tACK received, proceeding\n");

  if(i2c_regs->cmd_status & I2C_ST_AL) { 
    printf("\tArbitration lost!\n");
    i2c_async_err();
    return;
  }

  if(i2c_regs->cmd_status & I2C_ST_TIP) {
    //printf("\tI2C still busy in interrupt\n");
    return;
  }

  //now decide what to do
  switch(i2c_state) {

  case I2C_STATE_IDLE:
    //this is an error. in idle state, we shouldn't be transferring data, and the fact that the IRQ fired is terrible bad.
    printf("AAAAAHHHHH INTERRUPT IN THE IDLE STATE AAAHHHHHHHHH\n");
    i2c_async_err();
    break;

  case I2C_STATE_CONTROL_BYTE_SENT: //here we've sent the control byte, and we're either clocking data in or out now, but we haven't received a byte yet.
  case I2C_STATE_DATA:      //here we're sending/receiving data and if we're receiving there's data in the data reg

    //if(i2c_state == I2C_STATE_DATA) printf("\tI2C in state DATA with dir=%d and len=%d\n", i2c_dir, i2c_len);
    //else printf("\tI2C in state CONTROL_BYTE_SENT with dir=%d and len=%d\n", i2c_dir, i2c_len);

    if(i2c_dir == I2C_DIR_READ) {
      if(i2c_state == I2C_STATE_DATA) *(i2c_bufptr++) = i2c_regs->data;
      //printf("\tRead %x\n", *(i2c_bufptr-1));
      //set up another data byte
      if(i2c_len > 1) //only one more byte to transfer
        i2c_regs->cmd_status = I2C_CMD_RD;
      else
        i2c_regs->cmd_status = I2C_CMD_RD | I2C_CMD_NACK | I2C_CMD_STOP;
    }
    else if(i2c_dir == I2C_DIR_WRITE) {
      //write a byte
      //printf("\tWriting %x\n", *i2c_bufptr);
      i2c_regs->data = *(i2c_bufptr++);
      if(i2c_len > 1)
        i2c_regs->cmd_status = I2C_CMD_WR;
      else {
        //printf("\tGenerating STOP\n");
        i2c_regs->cmd_status = I2C_CMD_WR | I2C_CMD_STOP;
      }
    };
    i2c_len--;
    if(i2c_len == 0) i2c_state = I2C_STATE_LAST_BYTE;
    else i2c_state = I2C_STATE_DATA; //takes care of the addr_sent->data transition
    break;


  case I2C_STATE_LAST_BYTE: //here we've already sent the last read request and the last data is waiting for us.
    //printf("\tI2C in state LAST BYTE\n");

    if(i2c_dir == I2C_DIR_READ) {
      *(i2c_bufptr++) = i2c_regs->data;
      //printf("\tRead %x\n", *(i2c_bufptr-1));
      i2c_state = I2C_STATE_DATA_READY;
    } else {
      i2c_state = I2C_STATE_IDLE;
    }
    i2c_regs->ctrl &= ~I2C_CTRL_IE; //disable interrupts until next time

    if(i2c_callback) {
      i2c_callback(); //if we registered a callback, call it!
    }

    break;


  default: //terrible things have happened.
    break;
  }

}

void i2c_register_callback(void (*volatile callback)(void)) {
  i2c_callback = callback;
}

inline void i2c_async_err(void) {
  i2c_state = I2C_STATE_IDLE;
  i2c_regs->ctrl &= ~I2C_CTRL_IE;
  printf("I2C error\n");
//TODO: set an error flag instead of just dropping things on the floor
  i2c_regs->cmd_status = I2C_CMD_STOP;
}

bool i2c_async_read(uint8_t addr, unsigned int len) {
  //printf("Starting async read\n");
  if(i2c_state != I2C_STATE_IDLE) return false; //sorry mario but your i2c is in another castle
  if(len == 0) return true; //just idiot-proofing
  if(len > sizeof(i2c_buf)) return false;

  //disable I2C interrupts and clear pending interrupts on the I2C device
  i2c_regs->ctrl &= ~I2C_CTRL_IE;
  i2c_regs->cmd_status |= I2C_CMD_IACK;

  i2c_len = len;
  i2c_dir = I2C_DIR_READ;
  i2c_bufptr = i2c_buf;
  //then set up the transfer by issuing the control byte
  i2c_regs->ctrl |= I2C_CTRL_IE;
  i2c_regs->data = (addr << 1) | 0x01; //7 bit addr and read bit
  i2c_regs->cmd_status = I2C_CMD_WR | I2C_CMD_START; //generate start & start writing addr
  //update the state so the irq handler knows what's going on
  i2c_state = I2C_STATE_CONTROL_BYTE_SENT;
  return true;
}

bool i2c_async_write(uint8_t addr, const uint8_t *buf, unsigned int len) {
  //printf("Starting async write\n");
  if(i2c_state != I2C_STATE_IDLE) return false; //sorry mario but your i2c is in another castle
  if(len > sizeof(i2c_buf)) return false;

  //disable I2C interrupts and clear pending interrupts on the I2C device
  i2c_regs->ctrl &= ~I2C_CTRL_IE;
  i2c_regs->cmd_status |= I2C_CMD_IACK;

  //copy the buffer into our own if writing
  memcpy((void *)i2c_buf, buf, len);

  i2c_len = len;
  i2c_dir = I2C_DIR_WRITE;
  i2c_bufptr = i2c_buf;
  //then set up the transfer by issuing the control byte
  i2c_regs->ctrl |= I2C_CTRL_IE;
  i2c_regs->data = (addr << 1) | 0x00; //7 bit addr and read bit
  i2c_regs->cmd_status = I2C_CMD_WR | I2C_CMD_START; //generate start & start writing addr
  //update the state so the irq handler knows what's going on
  i2c_state = I2C_STATE_CONTROL_BYTE_SENT;

  return true;
}

//TODO: determine if it's better to read sequentially into the user's buffer, copy on transfer complete, or copy on request (shown below). probably best to copy on request.
bool i2c_async_data_ready(void *buf) {
  if(i2c_state == I2C_STATE_DATA_READY) {
    i2c_state = I2C_STATE_IDLE;
    memcpy(buf, (void *)i2c_buf, (i2c_bufptr - i2c_buf)); //TODO: not really comfortable with this
    //printf("Copying %d bytes to user buffer\n", i2c_bufptr-i2c_buf);
    return true;
  }
  return false;
}

