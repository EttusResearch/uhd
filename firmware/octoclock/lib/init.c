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

/*
 * Welcome to the firmware code for the USRP Octoclock accessory product!
 *
 * Notes regarding this firmware:
 *      NOT in M103 compatibility mode
 *      CKOPT full rail-to-rail
 *      xtal osc
 *      16K CK (16K clock cycles)
 *      additional delay 65ms for Crystal Oscillator
 *      slowly rising power
 *
 * These settings are very conservative. If a lower power oscillator is
 * required, change CKOPT to '1' (UNPROGRAMMED).
 *
 * M103C     = [ ]
 * WDTON     = [ ]
 * OCDEN     = [ ]
 * JTAGEN    = [X]
 * SPIEN     = [X]
 * EESAVE    = [X]
 * BOOTSZ    = 4096W_F000
 * BOOTRST   = [X]
 * CKOPT     = [X]
 * BODLEVEL  = 2V7
 * BODEN     = [ ]
 * SUT_CKSEL = EXTHIFXTALRES_16KCK_64MS
 *
 * EXTENDED  = 0xFF (valid)
 * HIGH      = 0x80 (valid)
 * LOW       = 0xFF (valid)
 *
 */

#include <avr/io.h>

#include <octoclock.h>

void setup_atmel_io_ports(){
/*
 * PORT A
 *
 * pin# Sig   Our Functional Name
 *
 * p51 PA0    CLK_CDCE    to U205 pin 24 --   L-->H edge latches MOSI and MISO in CDCE18005
 * p50 PA1    CE_CDCE    	Low = Chip Enabled for SPI comm  to U205 pin 25
 * p49 PA2    MOSI_CDCE    Goes to CDCE18005 - U205 pin 23
 * p48 PA3    MISO_CDCE    Input	Comes from U205 pin 22
 * p47 PA4    PD_CDCE    	Low = Chip is in Power-Down state; is Hi for normal operation U205 pin 12
 * p46 PA5    SYNC_CDCE    Low = Chip is sync'd with interal dividers; Hi for normal operation U205 pin 14
 * p45 PA6    PPS_SEL    	Low --> PPS_EXT selected; Hi -> PPS_GPS selected;    to U203 pin 1
 * p44 PA7    gps_lock    Input	Comes from M9107 - U206 pin 3
 *
 */

// /pd_cdcd, /sync_code, /ce need to be 1 (disabled) to start
// all bits are outputs, except PA7 (gps_lock) and PA3 (MISO_CDCE) are inputs
PORTA = Bits_8(00100010);
DDRA =   1<<DDA6 | 1<<DDA5 | 1<<DDA4 | 1<<DDA2 | 1<<DDA1 | 1<<DDA0;

/*
 * Port B
 *
 * pin# Sig   Our Functional Name
 *
 * p10 PB0    Ethernet /SEN
 * p11 PB1    Ethernet SCLK
 * p12 PB2    Ethernet MOSI
 * p13 PB3    Ethernet MISO
 * p14 PB4    Not connected, set as output with value 0
 * p15 PB5    Ethernet /RESET  -- Set to HI for normal use, weak input
 * p16 PB6    Ethernet /WOL  --- Wake on LAN -- set, weak input
 * p17 PB7    Not connected, set as output with value 0
 *
 */

PORTB = Bits_8(01100001);   // Initial Value is all zeros
DDRB = Bits_8(11110111);    // MOSI is an output; the Not Connected pins are also outputs

/*
 * Port C
 *
 * pin# Sig   Our Functional Name
 *
 * p34 PC0    Not connected, set as output with value 0
 * p35 PC1    Reference Select Switch INPUT
 * p36 PC2    Not connected, set as output with value 0
 * p37 PC3    Not connected, set as output with value 0
 * p38 PC4    Not connected, set as output with value 0
 * p40 PC5    "Top LED" of D103 3-stack of green LEDs
 * p41 PC6    "Middle LED"
 * p43 PC7    "Bottom LED"
 *
 */

PORTC = 0;        // Initial Value is all zeros
DDRC =  ~( 1<<DDC1 );     // All bits are outputs, except PC1. including the 5 Not Connected bits

/*
 * Port D
 *
 * pin# Sig   Our Functional Name
 *
 * p25 PD0    Ethernet /INT input
 * p26 PD1    GPS NMEA bit, (INT1) INPUT
 * p27 PD2    GPS Serial Out  (RXD)  INPUT
 * p28 PD3    GPS Serial In   (TXD)        OUTPUT
 * p29 PD4    GPS Present, INPUT  hi = Present
 * p30 PD5    Not connected, set as output with value 0
 * p31 PD6    Not connected, set as output with value 0
 * p32 PD7    Not connected, set as output with value 0
 *
 */

PORTD = 0;        // Initial Value is all zeros
DDRD =  1<<DDD3;

/*
 * Port E
 *
 * pin# Sig Dir Our Functional Name
 *
 * p2 PE0 In    avr_rxd (Also MOSI [PDI] when used for SPI programming of the chip)
 * p3 PE1 Out   avr_txd (Also MISO [PDO] when used for SPI programming of the chip)
 * p4 PE2 In    avr_cts
 * p5 PE3 Out   avr_rts
 * p6 PE4 In    PPS_GPS
 * p7 PE5 In    PPS_EXT_n
 * p8 PE6 In    Not Connected
 * p9 PE7 In    Not Connected
 *
 */

PORTE = 0;
DDRE =  1<<DDE1; // make outputs, set to zero.  PE1 is usart0 TXD

/*
 * Port F
 *
 * Split into 2 nibbles; goes to Amp/Filter board to select ENABLE and two bits
 * to select band one bit per nibble is not connected.
 *
 * pin Sig Dir        Our Functional Name
 *
 * p61 PF0 Out        J117 pin 3  (J117 pins 1 and 2 are GND)
 * p60 PF1 Out        J117 pin 4
 * p59 PF2 Out        J117 pin 5
 * p58 PF3 Out        J117 pin 6
 * p57 PF4 Out        J118 pin 3  (J118 pins 1 and 2 are GND)
 * p56 PF5 Out        J118 pin 4
 * p55 PF6 Out        J118 pin 5
 * p54 PF7 Out        J118 pin 6
 *
 */

PORTF = 0;        // Initial Value is all zeros; be sure ENABLE bits are active high!!!!
DDRF =  0xff;    // All bits are outputs

}
