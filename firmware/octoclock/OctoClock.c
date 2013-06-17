/*
 * OctoClock.c
 *
 * V1.00 -- May 2013
 *
 *
 * V1.03 -- Correct the switch to be UP for Internal and DOWN for External
 *          This means that the bat handle "points at" (sort of) the lower-left LED, which
 *          is the "STATUS" LED, which gets lit up when the external 10 MHz is present
 *          The "10 MHz Signal Detect" code accepts a very wide range of "10 MHz" signals
 *          23 April 2013
 *
 *
 * V1.02 -- Make LEDs consistent with Chassis - Top LED is INTERNAL; middle is EXTERNAL; bottom is STATUS
 *
 * STATUS is ON if the 10 MHz external input is present.   19 April 2013
 *
 *
 * V1.01: Modify TI chip initialization to be in differentail mode
 * which allows 10 MHz input down to 0.1 Volts according to the datasheet.
 *
 *
 * New Version that supports CLOCK board Version 1.0
 *
 * Author: Michael@Cheponis.Com with code borrowed liberally from
 * previous AVR projects
 *
 */

/*
 * Copyright 2013 Ettus Research LLC
 */






/* CLKSEL0 = 1   SUT1..0 is 11   CKOPT = 0   CKSEL3..1 is 111  => big output swing, long time to start up */
/*

NOT in M103 compatibility mode,  no WDT, CKOPT full rail-to-rail  xtal osc, 16K CK (16K clock cycles),
additional delay 65ms for Crystal Oscillator, slowly rising power

Very conservative settings; if lower power osc required, change CKOPT to '1'  (UNPROGRAMMED)  or, if you will,
CKOPT = [ ]



M103C = [ ]
WDTON = [ ]
OCDEN = [ ]
JTAGEN = [X]
SPIEN = [X]
EESAVE = [ ]
BOOTSZ = 4096W_F000
BOOTRST = [ ]
CKOPT = [X]
BODLEVEL = 2V7
BODEN = [ ]
SUT_CKSEL = EXTHIFXTALRES_16KCK_64MS

EXTENDED = 0xFF (valid)
HIGH = 0x89 (valid)
LOW = 0xFF (valid)


*/

// No interrupts are required

#include "OctoClock-io.h"

#include <avr/io.h>
#include <avr/interrupt.h>


#ifdef On
#undef On
#endif

#ifdef Off
#undef OFf
#endif

#define Off	(0)
#define On (!Off)


// Important for the Serial Port, not used at the moment
#define	FOSC	(7372800)
#define BAUD	(115200)

#define MYUBRR FOSC/16/BAUD-1


#define wait() for(u16 u=14000; u; u--) asm("nop");



enum LEDs {Top,Middle,Bottom}; // Top is 0, Mid is 1, and Bottom is 2

void
led(enum LEDs which, int turn_it_on){
	
	u8 LED = 0x20 << which; // selects the proper bit
	
	if(turn_it_on)
		PORTC |= LED;
	else
		PORTC &= ~LED;
	
}


enum TI_Input_10_MHz {Primary_GPS, Secondary_Ext};

void setup_TI_CDCE18005(enum TI_Input_10_MHz);




/*****************************************************************************************

			SPI routines

******************************************************************************************/



/* All macros evaluate to compile-time constants */
 
/* *** helper macros * * */
 
/* turn a numeric literal into a hex constant
 (avoids problems with leading zeros)
 8-bit constants max value 0x11111111, always fits in unsigned long
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

// Damn, that is SERIOUS magic ... ;-)  Yes, I know how it works
// but it's pretty cool....

 
/* *** user macros *** */
 
/* for upto 8-bit binary constants */
 #define Bits_8(d) ((unsigned char)B8__(HEX__(d)))
 
/* for upto 16-bit binary constants, MSB first */
 #define Bits_16(dmsb,dlsb) (((unsigned short)Bits_8(dmsb)<<8) \
 + Bits_8(dlsb))
 
/* for upto 32-bit binary constants, MSB first */
 #define Bits_32(dmsb,db2,db3,dlsb) (((unsigned long)Bits_8(dmsb)<<24) \
 + ((unsigned long)Bits_8(db2)<<16) \
 + ((unsigned long)Bits_8(db3)<<8) \
 + Bits_8(dlsb))
 
/* Sample usage:
 Bits_8(01010101) = 85
 Bits_16(10101010,01010101) = 43605
 Bits_32(10000000,11111111,10101010,01010101) = 2164238933
 */
 


enum CDCE18005 {Reg0, Reg1, Reg2, Reg3, Reg4, Reg5, Reg6, Reg7, Reg8_Status_Control,
	  Read_Command=0xE, RAM_EEPROM_Unlock=0x1F, RAM_EEPROM_Lock=0x3f} 
	  TI_CDCE18005;

// Table of 32-bit constants to be written to the TI chip's registers.

// Damn, inconsistent data sheet!  Special settigns see p35 of TI datasheet

// For the GPS's 10 MHz output
u32 table_Pri_Ref[] = {

	Bits_32(1,01010100,0,0),	// Reg 0
	Bits_32(1,01010100,0,0),	// Outputs LVCMOS Positive&Negative Active - Non-inverted
	Bits_32(1,01010100,0,0),
	Bits_32(1,01010100,0,0),
	Bits_32(1,01010100,0,0),	// All have output divide ratio to be 1; Aux Output is OFF
	
	Bits_32(0,0,1001,11010100),  // Reg 5  LVCMOS in; p31 of TI datasheet
	
	Bits_32(1,0,0010000,0),		// Reg 6	// SCAS863A – NOVEMBER 2008 – REVISED JUNE 2011
	
	Bits_32(1,01000000,0,0),	// Reg 7
	//        76543210
	Bits_32(0,0,1,10000000) // Reg8  Status/Control
};


// Looks like it's doing the correct thing re: SPI interface
// This is *definitely* AC coupled.  I removed those resistors to +3.3 and ground
// signal looked no different with differential measurement.  Added 240+470 to 
// center tap of secondary side to bias up to approx 1.2V for proper LVDS
//
// For the External 10 MHz input   LVDS with external termination  -- Effectively DC coupled

u32 table_Sec_Ref[] = {
	Bits_32(0001,01010100,0,100000),// Reg 0 -- use Secondary Reference for all channels
	Bits_32(0001,01010100,0,100000),// Outputs LVCMOS Positive&Negative Active - Non-inverted
	Bits_32(0001,01010100,0,100000),
	Bits_32(0001,01010100,0,100000),
	Bits_32(0001,01010100,0,100000),
	
//	Bits_32(0,0,00001000,10010111),  // Reg 5  LVDS with External Termination p32 of TI datasheet
//	Bits_32(0,0,00001000,11010111),  // Reg 5  LVDS with INTERNAL Termination p32 of TI datasheet

// May 2013 -- Turn OFF the LVDS Safe Mode, as it supposedly causes input thresholds to be increased.

//     	Bits_32(0,0,1001,10011011),  // Reg 5, try again.  Pretty soon, try new board...

      	Bits_32(0,0,1,10011011),  // Reg 5, Failsafe OFF   b5.11 = 0
		  

//       	Bits_32(0,0,1001,11011011),  // Reg 5, try again.  Pretty soon, try new board...
	// Try with DC input termination;  bit 6 is a "1"  2013 March
	// Seems to not work correctly.

	
//	Bits_32(1,0,0000000,0),  // Reg 6; note that 6.12 must be 1 for LVDS w/External Termination, 0 int
//	Bits_32(1,0,0000000,0),  // Reg 6; try Internal and DC coupling
	Bits_32(1,0,10000,0),  // Reg 6; try again
	
	Bits_32(1,01000000,0,0),
	Bits_32(0,0,1,10000000) // Reg8  Status/Control
};

//; Table 19 conflicts with Tables 5 thru 9 - in how LVCMOS outputs are defined
// extra error in Table 9, for bits 24 and 25
//
// Could combine these into just table[][] with 1st subscript being 0 or 1 for Primary or Secondary
// Maybe want to to that.

int table_size = sizeof (table_Pri_Ref) / sizeof(u32);
//int table_size = 1; // Testing read and write of Register 0 -- don't want excess SPI transactions
//NOTE!!! Still need to shift left by 4 and OR in register, as defined in TI_CDCE18005 enum, above.


enum Levels {Lo, Hi};
	
#define CLK	(PA0) // Shift by 0 bits  (PA.0)
#define CE_	(PA1) // Is really the "Chip Disable" signal, as Hi disables SPI
#define MOSI	(PA2)
#define MISO	(PA3)
#define PD_	(PA4)
#define SYNC_	(PA5)

void
set_bit(u8  bit_number, enum Levels bit_value){

 if(bit_value == Hi)
	PORTA |= 1<<bit_number;
 else
	PORTA &= ~ (1<<bit_number);
}


bool
get_bit(u8  bit_number){
	asm("nop");
	
	u8 portA = PINA;	// Maybe something is strange they way PORTA is read?
//	USART_Transmit( hex_table [0xf & (portA >> 4)], Control );
//	USART_Transmit( hex_table [0xf & portA], Control );
//	USART_Transmit(CR, Control); USART_Transmit(LF,Control);
	
	return (portA &  1<< bit_number) > 0 ? TRUE : FALSE;
	//return (portA & 8) != 0; // It's always MISO, so nail it for the moment
}


void
send_SPI(u32 bits){
// Send 32 bits to TI chip, LSB first.
// Don't worry about reading any bits back at this
// time, although for production, may want to do that
// as an error-check / integrity check.

/*
#define CLK		(PA0) // Shift by 0 bits  (PA.0)
#define CE_		(PA1) // Is really the "Chip Disable" signal, as Hi disables SPI
#define MOSI	(PA2)
#define MISO	(PA3)
#define PD_		(PA4)
#define SYNC_	(PA5)
*/

//Basically, when the clock is low, one can set MOSI to anything, as it's ignored.

 set_bit(CE_, Lo);	// Start SPI transaction with TI chip
 
 for (u8 i=0; i<32; i++){  // Foreach bit we need to send
	set_bit(MOSI, ((bits & (1UL<<i)) ? Hi : Lo) );   // LSB first
	asm("nop"); // Need a little more delay before L->H on clock; (REALLY?)
	set_bit(CLK, Hi);
	set_bit(CLK, Lo);  // Pulse the clock to clock in the bit
 }
// 	USART_Transmit(CR, Control); USART_Transmit(LF,Control);
 //set_bit(MOSI, Lo); // Not needed, but keeps all bits zeros except /CE when idle
 set_bit(CE_, Hi);  // OK, transaction is over
//	USART_Transmit(CR, Control); USART_Transmit(LF,Control);
}

// Takes about 7.6 ms to init all regs (as seen on scope)
// There is a very interesting phenomenon that is occurring --- The bit-to-bit time 
// at the beginning of transmission is 15 usec.  However, as the number of bits
// shifted to the left increases (as i increases in the for() loop )
// the time between bits elongates.  It's about 37 usec between bits 
// 30 and 31 (the last 2 bits).  It's kinda cool, because it's easy to
// know when the new word begins because the clock pulses will be
// closer together.  

// See if it checks: (15+37)/2 = 26 usec between average bits
// 32 bits * 9 words * 26 usec = 7.49 ms --- but have to add
// in the little bit of time that CE_ goes high; so 7.6 ms
// is a very reasonable number.  (Assumes linear increase in
// time as the number of shifts goes up, which seems to
// work OK here.)
//
// Of course, using a table instead of doing those shifts all the
// time would fix this; but it (should not) doesn't matter for this
// SPI interface.
//
// So far, the first word looks good, and the beginning of writing
// Register 1 also looks good.
//






// enum TI_Input_10_MHz {Primary_GPS, Secondary_Ext};

void
reset_TI_CDCE18005(){
// First, reset the chip.  Or, if you will, pull /SYNC low then high
set_bit(CE_, Hi);
set_bit(PD_, Lo);
wait(); // This should put the EEPROM bits into the RAM -- we don't care, but should init the chip

set_bit(PD_, Hi); // Out of Power Down state
wait();

set_bit(SYNC_, Lo);
wait();
set_bit(SYNC_, Hi);

wait();
// Now, by gosh, that darn chip ought to be fully enabled!
}
	
void
setup_TI_CDCE18005(enum TI_Input_10_MHz which_input){
 // Send the table of data to init the clock distribution chip.  Uses SPI.
 u32 temp;
 
 //reset_TI_CDCE18005();   // This REALLY should not be necessary
 
 if(which_input == Primary_GPS){
	 
	 for(u8 i=0; i<table_size; i++){
		 temp = table_Pri_Ref[i]<<4;
		 temp |= i;
		// print_u32(temp); // Debug *mac* -- correct
		 send_SPI(temp); // Make sure the register's address is in the LSBs
	 }	
 }	
 else { // is Secondary_Ext -- External 10 MHz input from SMA connector
		
		 for(u8 i=0; i<table_size; i++){
			 temp = table_Sec_Ref[i]<<4;
			 temp |= i;
			 send_SPI(temp); // Make sure the register's address is in the LSBs
		 }
 }
}		 
u32 
receive_SPI(){
 u32 bits;
	
 bits = 0;
 
 set_bit(CE_, Hi); // Make sure we're inactive
 set_bit(CLK, Lo); // and clk line is inactive, too
 set_bit(MOSI,Lo); // Make our bit output zero, for good measure
 

 set_bit(CE_, Lo);	// Start SPI transaction with TI chip; MOSI is don't care

	for (u8 i=0; i<32; i++){  // Foreach bit we need to get
		bits >>= 1; // get ready for next bit - NOTE: Only do this if we REALLY are putting in another bit
		set_bit(CLK, Hi);	// CPU is so slow, it easily meets setup & hold times
		//                            76543210
		if( get_bit(MISO) ) bits |= 0x80000000; // because we receive the LSB first
		set_bit(CLK, Lo);  // Pulse the clock to clock in the bit
	}
 set_bit(CE_, Hi);  // OK, transaction is over

 return (u32)(bits >> 4); // Ditch the lower 4 bits, which only contain the address
}

u32
get_TI_CDCE18005(enum CDCE18005 which_register){
	u32 get_reg_value;
	
	get_reg_value = 0;
	get_reg_value = (0xf0 & which_register << 4) | Read_Command;
	send_SPI(get_reg_value); // This tells the TI chip to send us the reg. value requested
	return receive_SPI();
};


bool
check_TI_CDCE18005(enum TI_Input_10_MHz which_input, enum CDCE18005 which_register)	{
  //		USART_Transmit(CR, Control); USART_Transmit(LF,Control); //reset_TI_CDCE18005();
	if(which_input == Primary_GPS){
		u32 read_value = get_TI_CDCE18005(which_register);
		return read_value == table_Pri_Ref[which_register];	
	}
	else {
		u32 read_value = get_TI_CDCE18005(which_register);
		return read_value == table_Sec_Ref[which_register];
	}
};
// This could obviously be done more elegantly to share more code; but this is
// simple and easy to understand	








void
Setup_Atmel_IO_Ports(){
	
	
/////////////////////////////////////////////////////////////////////////////					
/*
 * PORT A
 * 
 *pin# Sig	Our Functional Name
 *
 * p51 PA0	CLK_CDCE	to U205 pin 24 --   L-->H edge latches MOSI and MISO in CDCE18005
 * p50 PA1	CE_CDCE		Low = Chip Enabled for SPI comm  to U205 pin 25
 * p49 PA2	MOSI_CDCE	Goes to CDCE18005 - U205 pin 23
 * p48 PA3	MISO_CDCE	Input	Comes from U205 pin 22
 * p47 PA4	PD_CDCE		Low = Chip is in Power-Down state; is Hi for normal operation U205 pin 12
 * p46 PA5	SYNC_CDCE	Low = Chip is sync'd with interal dividers; Hi for normal operation U205 pin 14
 * p45 PA6	PPS_SEL		Low --> PPS_EXT selected; Hi -> PPS_GPS selected;    to U203 pin 1
 * p44 PA7	gps_lock	Input	Comes from M9107 - U206 pin 3
 *
 */

// Bit #:  76543210
PORTA = Bits_8(00110010); // /pd_cdcd, /sync_code, /ce need to be 1 (disabled) to start
DDRA =   1<<DDA6 | 1<<DDA5 | 1<<DDA4 | 1<<DDA2 | 1<<DDA1 | 1<<DDA0; //// all bits are outputs, except PA7 (gps_lock) and PA3 (MISO_CDCE) are inputs


					
/////////////////////////////////////////////////////////////////////////////					
/*
 * Port B
 *
 *pin# Sig	Our Functional Name
 *
 * p10 PB0	Ethernet /SEN
 * p11 PB1	Ethernet SCLK
 * p12 PB2	Ethernet MOSI
 * p13 PB3	Ethernet MISO
 * p14 PB4	Not connected, set as output with value 0
 * p15 PB5	Ethernet /RESET  -- Set to HI for normal use, weak input
 * p16 PB6	Ethernet /WOL  --- Wake on LAN -- set, weak input
 * p17 PB7	Not connected, set as output with value 0
 *
 */
 

 PORTB = Bits_8(01100001);		// Initial Value is all zeros
 DDRB = 1<<DDB2 | 1<<DDB4 | 1<<DDB7;  // MOSI is an output; the Not Connected pins are also outputs


					
/////////////////////////////////////////////////////////////////////////////					
/*
 * Port C
 *
 *pin# Sig	Our Functional Name
 *
 * p34 PC0	Not connected, set as output with value 0
 * p35 PC1	Reference Select Switch INPUT
 * p36 PC2	Not connected, set as output with value 0
 * p37 PC3	Not connected, set as output with value 0
 * p38 PC4	Not connected, set as output with value 0
 * p40 PC5	"Top LED" of D103 3-stack of green LEDs
 * p41 PC6	"Middle LED"
 * p43 PC7	"Bottom LED"
 *
 */
PORTC = 0;		// Initial Value is all zeros
DDRC =  ~( 1<<DDC1 ); 	// All bits are outputs, except PC1. including the 5 Not Connected bits

 

/////////////////////////////////////////////////////////////////////////////					
/*
 * Port D
 *
 *pin# Sig	Our Functional Name
 *
 * p25 PD0	Ethernet /INT input
 * p26 PD1	GPS NMEA bit, output
 * p27 PD2	GPS Serial Out  (RXD; INT1)  INPUT
 * p28 PD3	GPS Serial In   (TXD)        OUTPUT
 * p29 PD4	GPS Present, INPUT  hi = Present
 * p30 PD5	Not connected, set as output with value 0
 * p31 PD6	Not connected, set as output with value 0
 * p32 PD7	Not connected, set as output with value 0
 *
 */
PORTD = 0;		// Initial Value is all zeros
DDRD =  1<<DDD3;

					

/////////////////////////////////////////////////////////////////////////////					
/*
 * Port E
 *
 *pin# Sig Dir	Our Functional Name
 *
 * p2 PE0 In	avr_rxd	(Also MOSI [PDI] when used for SPI programming of the chip)
 * p3 PE1 Out	avr_txd (Also MISO [PDO] when used for SPI programming of the chip)
 * p4 PE2 In	avr_cts
 * p5 PE3 Out	avr_rts  DUE TO MOD, make this an input, too (as we go direct GPSDO to FPGA via level translators)
 * p6 PE4 In	PPS_GPS
 * p7 PE5 In	PPS_EXT_n
 * p8 PE6 In	Not Connected
 * p9 PE7 In	Not Connected
 *
 */
PORTE = 0;
DDRE =  1<<DDE1; // make outputs, set to zero.  PE1 is usart0 TXD


/////////////////////////////////////////////////////////////////////////////					
/*
 * Port F
 *
 * Split into 2 nibbles; goes to Amp/Filter board to select ENABLE and two bits to select band
 * one bit per nibble is not connected.
 *
 * pin Sig Dir		Our Functional Name
 * num
 *
 * p61 PF0 Out		J117 pin 3  (J117 pins 1 and 2 are GND)
 * p60 PF1 Out		J117 pin 4
 * p59 PF2 Out		J117 pin 5
 * p58 PF3 Out		J117 pin 6
 * p57 PF4 Out		J118 pin 3  (J118 pins 1 and 2 are GND)
 * p56 PF5 Out		J118 pin 4
 * p55 PF6 Out		J118 pin 5
 * p54 PF7 Out		J118 pin 6
 *
 */
 

PORTF = 0;		// Initial Value is all zeros; be sure ENABLE bits are active high!!!!
DDRF =  0xff;	// All bits are outputs



	
led(Middle,On);
setup_TI_CDCE18005(Primary_GPS);	// 10 MHz from Internal Source

led(Top,On); 
PORTA |= (1<<PA6);	// PPS from Internal source


}

/////////////////////////////////////////////////////////////////////////////

//enum TI_Input_10_MHz {Primary_GPS, Secondary_Ext};

//setup_TI_CDCE18005(enum TI_Input_10_MHz);

bool  Global_GPS_Present = (bool)FALSE;
bool Global_Ext_Ref_Is_Present = (bool)FALSE; // NOT PRESENT unless proven so...
// This was initially global becasue it was to be set in an interrupt routine
// But it turned out interrupts were not needed.  But kept this in because
// although it's a Global, it is the only one, and it makes it easier to
// go back and use interrupts if absolutely necessary.  It could be
// removed and replaced with some local variable that gets passed
// around, but, really, it seems OK to me like this.



void
LEDs_Off(){
 led(Top,Off);
 led(Middle,Off);
 led(Bottom,Off);
}


void
Force_Internal(){
  // led(Middle,On);
 led(Top,On);
 led(Middle,Off);
 led(Bottom,On);

 setup_TI_CDCE18005(Primary_GPS);

 // Set PPS to Primary (1) n.b.:  "1" in general means "Internal" for all such signals
 PORTA |= (1<<PA6);	// PPS from Internal source
}


void
Force_External(){
  // led(Middle, Off);
  led(Top, Off);
  led(Middle, On);
  led(Bottom, On);

 setup_TI_CDCE18005(Secondary_Ext);

 // Set PPS to External (0
 PORTA &= ~(1<<PA6);	// PPS from External source
}


/////////////////////////////////////////////////////////////////////////////

void
Prefer_Internal(){

  if(Global_GPS_Present)
    Force_Internal();
  else if(Global_Ext_Ref_Is_Present)
    Force_External();
  else
    LEDs_Off();
}





void
Prefer_External(){   // IF EXTERNAL IS OK, then do this stuff
  // if external is NOT OK, then force Internal
  if(Global_Ext_Ref_Is_Present)
    Force_External();
  else if(Global_GPS_Present)
    Force_Internal();
  else
    LEDs_Off();
}
 


// Turns out, we don't need interrupts


#if 0
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

u8 Global_Tick_Counter = (u8)0;
u8 Global_Ext_Ref_Detect_Counter = (u8)0;

// External Reference Detect interrupt; nominally at 610 Hz (10 MHz / 2**14 )
ISR ( _VECTOR(1)){
  asm("cli");	// Global Interrupt Disable --- enable with SEI if desired later


  Global_Ext_Ref_Detect_Counter++ ;  // We reset this elsewhere

  asm("sei");	// Global Interrupt Enable
}


// Timer 0 Overflow Handler
ISR ( _VECTOR(16)){
  static u8 led_state = Off;

  asm("cli");	// Global Interrupt Disable --- enable with SEI if desired later

  led_state = (led_state ? Off : On);

  asm("sei");	// Global Interrupt Enable
}

//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



void
Setup_Atmel_Interrupts(){
  // Timer 0 is all we need -- but simplest if both Timer 0 AND IRQ1 (ext_ref_detect 610 Hz signal) also
  // Nah, don't need this...
}

#endif



bool
Check_What_Is_Present(){

  Global_GPS_Present = (PIND & (1<<DDD4)) != 0; // See if +5 scaled to 3.3 from GPSDO is there


  volatile u8 portE = PINE;
  volatile u8 prev, now;

  prev = ( portE & (1 << DDE7) ?  1 : 0); // Get PREVIOUS state of the input
  for(u16 c=1; c; c++){
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


bool
get_Switch_State(){
 u8  portC = PINC;

 // return (bool)(portC &  (1<<DDC1) ? On : Off); 
 return (bool)(portC &  (1<<DDC1) ? Off : On);  // UP is prefer internal,
                                                // DOWN is prefer external
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//                            M A I N                                      //
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

	
int 
main(void){

  bool Old_Switch_State, Current_Switch_State, Old_Global_Ext_Ref_Is_Present = FALSE;



 asm("cli");	// Global Interrupt Disable --- enable with SEI if desired later

 Setup_Atmel_IO_Ports();

 // Setup_Atmel_Interrupts();


 /*
  * DO THIS FOREVER:
  *  
  *  
  * get_switch_state
  *  
  * if SWITCH_CHANGED:
  *  
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

 // Because down below, we use this to get state swap...
 // So we arbitrarily set the PREVIOUS state to be the "other" state
 // so that, below, we trigger what happens when the switch changes
 // This first "change" is therefore artificial to keep the logic, below, cleaner
  
 while(TRUE) {
   Check_What_Is_Present(); // Set "Global_Ext_Ref_Is_Present" and "Global_GPS_Present"

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
   } // if()  checking for different switch status


 } // WHILE() loop


} /*end  "main" of  Program 'OctoClock.c */
