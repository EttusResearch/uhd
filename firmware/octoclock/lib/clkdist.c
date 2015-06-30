/*
 * Copyright 2014 Ettus Research LLC
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

#include <avr/io.h>

#include <octoclock.h>
#include <clkdist.h>
#include <state.h>

#include <util/delay.h>

#define wait() for(uint16_t u=14000; u; u--) asm("nop");

#define CLK   (PA0) // Shift by 0 bits
#define CE_   (PA1) // Is really the "Chip Disable" signal, as Hi disables SPI
#define MOSI  (PA2)
#define MISO  (PA3)
#define PD_   (PA4)
#define SYNC_ (PA5)

// Table of 32-bit constants to be written to the TI chip's registers. These are
// from the "Special Settings" on Page 35 of the datasheet.
// For the GPS's 10 MHz output
static const uint32_t table_Pri_Ref[] = {
    Bits_32(1,01010100,0,0),    // Reg 0
    Bits_32(1,01010100,0,0),    // Outputs LVCMOS Positive&Negative Active - Non-inverted
    Bits_32(1,01010100,0,0),
    Bits_32(1,01010100,0,0),
    Bits_32(1,01010100,0,0),    // All have output divide ratio to be 1; Aux Output is OFF
    Bits_32(0,0,1001,11010100), // Reg 5  LVCMOS in; p31 of TI datasheet
    Bits_32(1,0,0010000,0),     // Reg 6    // SCAS863A <96> NOVEMBER 2008 <96> REVISED JUNE 2011
    Bits_32(1,01000000,0,0),    // Reg 7
    Bits_32(0,0,1,10000000)     // Reg8  Status/Control
};

// For the External 10 MHz input LVDS with external termination, 
// Effectively DC coupled
static const uint32_t table_Sec_Ref[] = {
    Bits_32(0001,01010100,0,100000),    // Reg 0 -- use Secondary Reference for all channels
    Bits_32(0001,01010100,0,100000),    // Outputs LVCMOS Positive&Negative Active - Non-inverted
    Bits_32(0001,01010100,0,100000),
    Bits_32(0001,01010100,0,100000),
    Bits_32(0001,01010100,0,100000),
    Bits_32(0,0,1,10011011),            // Reg 5, Failsafe OFF   b5.11 = 0
    Bits_32(1,0,10000,0),               // Reg 6; try again
    Bits_32(1,01000000,0,0),
    Bits_32(0,0,1,10000000)             // Reg8  Status/Control
};

// Table 19 conflicts with Tables 5 thru 9 - in how LVCMOS outputs are defined
// extra error in Table 9, for bits 24 and 25
static int table_size = sizeof (table_Pri_Ref) / sizeof(uint32_t);

static void set_bit(uint8_t  bit_number, Levels bit_value) {

    if(bit_value == Hi)
        PORTA |= 1<<bit_number;
    else
        PORTA &= ~ (1<<bit_number);
}

static bool get_bit(uint8_t  bit_number) {
    asm("nop");

    uint8_t portA = PINA;
    return (portA &  1<< bit_number) > 0 ? true : false;
}

// Send 32 bits to TI chip, LSB first.
// Don't worry about reading any bits back at this time
static void send_SPI(uint32_t bits) {
    // Basically, when the clock is low, one can set MOSI to anything, as it's
    // ignored.
    set_bit(CE_, Lo);    // Start SPI transaction with TI chip

    // Send each bit, LSB first, add a bit of delay before the clock, and then
    // toggle the clock line.
    for (uint8_t i=0; i<32; i++) {
        set_bit(MOSI, ((bits & (1UL<<i)) ? Hi : Lo) );
        asm("nop");
        set_bit(CLK, Hi);
        set_bit(CLK, Lo);
    }

    // OK, transaction is over
    set_bit(CE_, Hi);
}

static uint32_t receive_SPI() {
    uint32_t bits = 0;

    set_bit(CE_, Hi); // Make sure we're inactive
    set_bit(CLK, Lo); // and clk line is inactive, too
    set_bit(MOSI,Lo); // Make our bit output zero, for good measure
    set_bit(CE_, Lo); // Start SPI transaction with TI chip; MOSI is don't care

    // For each bit we are receiving, prep, clock in the bit LSB first
    for (uint8_t i=0; i<32; i++){
        bits >>= 1;
        set_bit(CLK, Hi);
        if( get_bit(MISO) ) bits |= 0x80000000;
        set_bit(CLK, Lo);
    }

    // OK, transaction is over
    set_bit(CE_, Hi);

    // Ditch the lower 4 bits, which only contain the address
    return (uint32_t)(bits >> 4);
}

void setup_TI_CDCE18005(TI_Input_10_MHz which_input) {
    // Send the table of data to init the clock distribution chip.  Uses SPI.
    uint32_t temp;

    if(which_input == Primary_GPS) {
        for(uint8_t i=0; i<table_size; i++){
            temp = table_Pri_Ref[i]<<4;
            temp |= i;
            // Make sure the register's address is in the LSBs
            send_SPI(temp);
        }
    } else {
        // is Secondary_Ext -- External 10 MHz input from SMA connector
        for(uint8_t i=0; i<table_size; i++){
            temp = table_Sec_Ref[i]<<4;
            temp |= i;
            // Make sure the register's address is in the LSBs
            send_SPI(temp);
         }
    }
}

void reset_TI_CDCE18005(void) {
    // First, reset the chip.  Or, if you will, pull /SYNC low then high
    set_bit(CE_, Hi);
    set_bit(PD_, Lo);
    wait();

    // Out of Power Down state
    set_bit(PD_, Hi);
    wait();

    set_bit(SYNC_, Lo);
    wait();
    set_bit(SYNC_, Hi);

    wait();
}

uint32_t get_TI_CDCE18005(CDCE18005 which_register){
    uint32_t get_reg_value = 0;
    get_reg_value = (0xf0 & (which_register << 4)) | Read_Command;

    // This tells the TI chip to send us the reg. value requested
    send_SPI(get_reg_value);
    return receive_SPI();
}

void set_TI_CDCE18005(CDCE18005 which_register, uint32_t bits){
    send_SPI((bits << 4) | which_register);
}

bool check_TI_CDCE18005(TI_Input_10_MHz which_input,
        CDCE18005 which_register) {

    if(which_input == Primary_GPS){
        uint32_t read_value = get_TI_CDCE18005(which_register);
        return read_value == table_Pri_Ref[which_register];
    } else {
        uint32_t read_value = get_TI_CDCE18005(which_register);
        return read_value == table_Sec_Ref[which_register];
    }
}
