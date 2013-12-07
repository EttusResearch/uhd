/*
 * Copyright 2013 Ettus Research LLC
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
 *      no WDT
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
 * EESAVE    = [ ]
 * BOOTSZ    = 4096W_F000
 * BOOTRST   = [ ]
 * CKOPT     = [X]
 * BODLEVEL  = 2V7
 * BODEN     = [ ]
 * SUT_CKSEL = EXTHIFXTALRES_16KCK_64MS
 *
 * EXTENDED  = 0xFF (valid)
 * HIGH      = 0x89 (valid)
 * LOW       = 0xFF (valid)
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef On
#undef On
#endif

#ifdef Off
#undef Off
#endif

#define Off     (0)
#define On      (!Off)

#ifdef FALSE
#undef FALSE
#endif

#ifdef TRUE
#undef TRUE
#endif

#define FALSE   (0)
#define TRUE    (!FALSE)


// Important for the Serial Port, not used at the moment
#define FOSC    (7372800)
#define BAUD    (115200)

#define MYUBRR FOSC/16/BAUD-1

#define wait() for(uint16_t u=14000; u; u--) asm("nop");

#define CLK   (PA0) // Shift by 0 bits
#define CE_   (PA1) // Is really the "Chip Disable" signal, as Hi disables SPI
#define MOSI  (PA2)
#define MISO  (PA3)
#define PD_   (PA4)
#define SYNC_ (PA5)

// Top is 0, Mid is 1, and Bottom is 2
enum LEDs {Top, Middle, Bottom};

enum TI_Input_10_MHz {Primary_GPS, Secondary_Ext};

enum Levels {Lo, Hi};

void led(enum LEDs which, int turn_it_on) {

    // selects the proper bit
    uint8_t LED = 0x20 << which;

    if(turn_it_on)
        PORTC |= LED;
    else
        PORTC &= ~LED;
}

/*******************************************************************************
*   SPI routines
*******************************************************************************/

/* All macros evaluate to compile-time constants */


/* turn a numeric literal into a hex constant
 * (avoids problems with leading zeros)
 * 8-bit constants max value 0x11111111, always fits in unsigned long
 */
#define HEX__(n) 0x##n##LU

/* 8-bit conversion function */
#define B8__(x) ((x&0x0000000FLU)?1:0) \
    +((x&0x000000F0LU)?2:0) \
    +((x&0x00000F00LU)?4:0) \
    +((x&0x0000F000LU)?8:0) \
    +((x&0x000F0000LU)?16:0) \
    +((x&0x00F00000LU)?32:0) \
    +((x&0x0F000000LU)?64:0) \
    +((x&0xF0000000LU)?128:0)

/* for up to 8-bit binary constants */
#define Bits_8(d) ((unsigned char)B8__(HEX__(d)))

/* for up to 16-bit binary constants, MSB first */
#define Bits_16(dmsb,dlsb) (((unsigned short)Bits_8(dmsb)<<8) \
    + Bits_8(dlsb))

/* for up to 32-bit binary constants, MSB first */
#define Bits_32(dmsb,db2,db3,dlsb) (((unsigned long)Bits_8(dmsb)<<24) \
    + ((unsigned long)Bits_8(db2)<<16) \
    + ((unsigned long)Bits_8(db3)<<8) \
    + Bits_8(dlsb))

/* Sample usage:
 * Bits_8(01010101) = 85
 * Bits_16(10101010,01010101) = 43605
 * Bits_32(10000000,11111111,10101010,01010101) = 2164238933
 */

enum CDCE18005 {
        Reg0, Reg1, Reg2, Reg3, Reg4, Reg5, Reg6, Reg7,
        Reg8_Status_Control,
        Read_Command=0xE,
        RAM_EEPROM_Unlock=0x1F,
        RAM_EEPROM_Lock=0x3f
} TI_CDCE18005;

// Table of 32-bit constants to be written to the TI chip's registers. These are
// from the "Special Settings" on Page 35 of the datasheet.
// For the GPS's 10 MHz output
uint32_t table_Pri_Ref[] = {
    Bits_32(1,01010100,0,0),    // Reg 0
    Bits_32(1,01010100,0,0),    // Outputs LVCMOS Positive&Negative Active - Non-inverted
    Bits_32(1,01010100,0,0),
    Bits_32(1,01010100,0,0),
    Bits_32(1,01010100,0,0),    // All have output divide ratio to be 1; Aux Output is OFF
    Bits_32(0,0,1001,11010100), // Reg 5  LVCMOS in; p31 of TI datasheet
    Bits_32(1,0,0010000,0),     // Reg 6	// SCAS863A – NOVEMBER 2008 – REVISED JUNE 2011
    Bits_32(1,01000000,0,0),    // Reg 7
    Bits_32(0,0,1,10000000)     // Reg8  Status/Control
};

// For the External 10 MHz input LVDS with external termination, 
// Effectively DC coupled
uint32_t table_Sec_Ref[] = {
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
int table_size = sizeof (table_Pri_Ref) / sizeof(uint32_t);

void set_bit(uint8_t  bit_number, enum Levels bit_value) {

    if(bit_value == Hi)
        PORTA |= 1<<bit_number;
    else
        PORTA &= ~ (1<<bit_number);
}

bool get_bit(uint8_t  bit_number) {
    asm("nop");

    uint8_t portA = PINA;
    return (portA &  1<< bit_number) > 0 ? TRUE : FALSE;
}

// Send 32 bits to TI chip, LSB first.
// Don't worry about reading any bits back at this time
void send_SPI(uint32_t bits) {

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

void reset_TI_CDCE18005() {
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

void setup_TI_CDCE18005(enum TI_Input_10_MHz which_input) {
    // Send the table of data to init the clock distribution chip.  Uses SPI.
    uint32_t temp;

    if(which_input == Primary_GPS) {
        for(uint8_t i=0; i<table_size; i++){
            temp = table_Pri_Ref[i]<<4;
            temp |= i;
            send_SPI(temp); // Make sure the register's address is in the LSBs
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

uint32_t receive_SPI() {
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

uint32_t get_TI_CDCE18005(enum CDCE18005 which_register){
    uint32_t get_reg_value = 0;
    get_reg_value = (0xf0 & which_register << 4) | Read_Command;

    // This tells the TI chip to send us the reg. value requested
    send_SPI(get_reg_value);
    return receive_SPI();
}

bool check_TI_CDCE18005(enum TI_Input_10_MHz which_input,
        enum CDCE18005 which_register) {

    if(which_input == Primary_GPS){
        uint32_t read_value = get_TI_CDCE18005(which_register);
        return read_value == table_Pri_Ref[which_register];
    } else {
        uint32_t read_value = get_TI_CDCE18005(which_register);
        return read_value == table_Sec_Ref[which_register];
    }
}

void Setup_Atmel_IO_Ports() {
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
PORTA = Bits_8(00110010);
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

PORTB = Bits_8(01100001);        // Initial Value is all zeros
DDRB = 1<<DDB2 | 1<<DDB4 | 1<<DDB7;  // MOSI is an output; the Not Connected pins are also outputs

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
 * p26 PD1    GPS NMEA bit, output
 * p27 PD2    GPS Serial Out  (RXD; INT1)  INPUT
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

led(Middle,On);
setup_TI_CDCE18005(Primary_GPS);    // 10 MHz from Internal Source

led(Top,On);
PORTA |= (1<<PA6);    // PPS from Internal source
}

// NOT PRESENT unless proven so...
bool Global_GPS_Present = (bool)FALSE;
bool Global_Ext_Ref_Is_Present = (bool)FALSE;

void LEDs_Off(){
    led(Top,Off);
    led(Middle,Off);
    led(Bottom,Off);
}

void Force_Internal(){
    led(Top,On);
    led(Middle,Off);
    led(Bottom,On);

    setup_TI_CDCE18005(Primary_GPS);

    // Set PPS to Primary (1) n.b.:  "1" in general means "Internal" for all
    // such signals
    PORTA |= (1<<PA6);
}

void Force_External(){
    led(Top, Off);
    led(Middle, On);
    led(Bottom, On);

    setup_TI_CDCE18005(Secondary_Ext);

    // Set PPS to External
    PORTA &= ~(1<<PA6);
}

void Prefer_Internal(){

    if(Global_GPS_Present)
        Force_Internal();
    else if(Global_Ext_Ref_Is_Present)
        Force_External();
    else
        LEDs_Off();
}

void Prefer_External(){
    // if external is NOT OK, then force Internal
    if(Global_Ext_Ref_Is_Present)
        Force_External();
    else if(Global_GPS_Present)
        Force_Internal();
    else
        LEDs_Off();
}

bool Check_What_Is_Present(){

    // See if +5 scaled to 3.3 from GPSDO is there
    Global_GPS_Present = (PIND & (1<<DDD4)) != 0;

    volatile uint8_t portE = PINE;
    volatile uint8_t prev, now;

    // Get PREVIOUS state of the input
    prev = ( portE & (1 << DDE7) ?  1 : 0);

    for(uint16_t c=1; c; c++){
        portE = PINE;
        now = ( portE & (1 << DDE7) ?  1 : 0);

        if(prev != now){
            Global_Ext_Ref_Is_Present = (bool)TRUE;

        return (bool)TRUE;
        }
    }

  // Else, if it didn't wiggle in that time, then it didn't wiggle
  // So ext. is NOT present
  Global_Ext_Ref_Is_Present = (bool)FALSE;
  return (bool)FALSE;
}


bool get_Switch_State(){
    uint8_t  portC = PINC;

    // UP is prefer internal,
    // DOWN is prefer external
    return (bool)(portC &  (1<<DDC1) ? Off : On);
}

/*******************************************************************************
*   Main Routine
*******************************************************************************/

int main(void){

    bool Old_Switch_State, Current_Switch_State, Old_Global_Ext_Ref_Is_Present = FALSE;

    // Global Interrupt Disable --- enable with SEI if desired later
    asm("cli");

    Setup_Atmel_IO_Ports();

    /*
     * DO THIS FOREVER:
     *
     * get_switch_state
     *
     * if SWITCH_CHANGED:
     *
     *   if PREFER_INTERNAL:
     *     if INTERNAL_PRESENT do_internal
     *     else if EXTERNAL_PRESENT do_external
     *     else LEDs OFF
     *
     *   if PREFER_EXTERNAL:
     *     if EXTERNAL_PRESENT do_external
     *     else if INTERNAL_PRESENT do_internal
     *     else LEDs OFF
     *
     */

    Old_Switch_State = ! get_Switch_State();

    // Because down below, we use this to get state swap So we arbitrarily set
    // the PREVIOUS state to be the "other" state so that, below, we trigger
    // what happens when the switch changes This first "change" is therefore
    // artificial to keep the logic, below, cleaner
    while(TRUE) {
        // Set "Global_Ext_Ref_Is_Present" and "Global_GPS_Present"
        Check_What_Is_Present();

        // Off means "Prefer External" -- DOWN
        // On  means "Prefer Internal" -- UP
        Current_Switch_State = get_Switch_State();

        if( (Current_Switch_State != Old_Switch_State)  || 
            (Global_Ext_Ref_Is_Present != Old_Global_Ext_Ref_Is_Present) ) {

            Old_Switch_State = Current_Switch_State;
            Old_Global_Ext_Ref_Is_Present = Global_Ext_Ref_Is_Present;

            if(Current_Switch_State == On)
                Prefer_Internal();
            else
                Prefer_External();
        }
    }
}
