/*
 * debug.c
 */

#include "config.h"
#include "debug.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "io.h"
#include "power.h"
#include "global.h"

#define DEBUG_BLINK_DELAY	250	// ms

#ifdef ATTINY88_DIP

#define SERIAL_DEBUG_INDEX			6
#define SERIAL_DEBUG_PORT			PORTD
static io_pin_t SERIAL_DEBUG      = IO_PD(SERIAL_DEBUG_INDEX);

#else
/*
#ifdef I2C_REWORK
//static io_pin_t SERIAL_DEBUG      = IO_PC(1);	// EN1
#else
//static io_pin_t SERIAL_DEBUG      = EN4;
#endif // I2C_REWORK
*/
// No good: PWR_EN4 trace still connected to LTC3675
//#define SERIAL_DEBUG_INDEX			1
//#define SERIAL_DEBUG_PORT			PORTA
//static io_pin_t SERIAL_DEBUG      = IO_PA(SERIAL_DEBUG_INDEX);

// AVR_MISO
#define SERIAL_DEBUG_INDEX			4
#define SERIAL_DEBUG_PORT			PORTB
static io_pin_t SERIAL_DEBUG      = IO_PB(SERIAL_DEBUG_INDEX);

#endif // ATTINY88_DIP
/*
#ifdef DEBUG

#else

#endif // DEBUG
*/
#ifdef DEBUG

#ifdef ATTINY88_DIP
static io_pin_t DEBUG_1 = IO_PB(6);
static io_pin_t DEBUG_2	= IO_PB(7);
#endif // ATTINY88_DIP

void debug_init()
{
	io_output_pin(DEBUG_1);
	io_output_pin(DEBUG_2);
	
	io_enable_pin(DEBUG_1, true);
	io_enable_pin(DEBUG_2, true);
#ifdef ENABLE_SERIAL	
	io_set_pin(SERIAL_DEBUG);
	io_output_pin(SERIAL_DEBUG);
#endif // ENABLE_SERIAL
}

#else

void debug_init()
{
#ifdef ENABLE_SERIAL
	io_set_pin(SERIAL_DEBUG);
	io_output_pin(SERIAL_DEBUG);
#endif // ENABLE_SERIAL
}

#endif // DEBUG

#if defined(DEBUG) && !defined(DEBUG_VOID)

void debug_set(io_pin_t pin, bool enable)
{
	io_enable_pin(pin, !enable);
}

void debug_blink(uint8_t count)
{
	io_enable_pin(DEBUG_1, false);
	io_enable_pin(DEBUG_2, true);
	_delay_ms(DEBUG_BLINK_DELAY * 2);

	for (; count > 0; count--) {
		io_enable_pin(DEBUG_2, false);
		_delay_ms(DEBUG_BLINK_DELAY);
		io_enable_pin(DEBUG_2, true);
		_delay_ms(DEBUG_BLINK_DELAY);
	}

	io_enable_pin(DEBUG_1, true);
	io_enable_pin(DEBUG_2, true);
	_delay_ms(DEBUG_BLINK_DELAY * 2);
}

void debug_blink_rev(uint8_t count)
{
	io_enable_pin(DEBUG_2, false);
	io_enable_pin(DEBUG_1, true);
	_delay_ms(DEBUG_BLINK_DELAY * 2);

	for (; count > 0; count--) {
		io_enable_pin(DEBUG_1, false);
		_delay_ms(DEBUG_BLINK_DELAY);
		io_enable_pin(DEBUG_1, true);
		_delay_ms(DEBUG_BLINK_DELAY);
	}

	io_enable_pin(DEBUG_2, true);
	io_enable_pin(DEBUG_1, true);
	_delay_ms(DEBUG_BLINK_DELAY * 2);
}

void debug_blink2(uint8_t count)
{
	io_enable_pin(DEBUG_1, true);
	io_enable_pin(DEBUG_2, true);
	_delay_ms(DEBUG_BLINK_DELAY * 2);

	bool b = false;
	for (; count > 0; count--) {
		io_enable_pin(DEBUG_1, b);
		io_enable_pin(DEBUG_2, b);
		_delay_ms(DEBUG_BLINK_DELAY);
		b = !b;
	}

	io_enable_pin(DEBUG_1, true);
	io_enable_pin(DEBUG_2, true);
	_delay_ms(DEBUG_BLINK_DELAY * 2);
}

void debug_wait(void)
{
	io_enable_pin(DEBUG_1, true);
	io_enable_pin(DEBUG_2, true);
	
	bool b = false;
	while (true)
	{
		io_enable_pin(DEBUG_1, b);
		io_enable_pin(DEBUG_2, !b);
		
		_delay_ms(DEBUG_BLINK_DELAY);
		
		b = !b;
	}
	
	io_enable_pin(DEBUG_1, true);
	io_enable_pin(DEBUG_2, true);
}

#else

#ifndef DEBUG_VOID

void debug_blink_rev(uint8_t count)
{
	charge_set_led(true);
	_delay_ms(DEBUG_BLINK_DELAY * 4);

	for (; count > 0; count--) {
		charge_set_led(false);
		_delay_ms(DEBUG_BLINK_DELAY);
		charge_set_led(true);
		_delay_ms(DEBUG_BLINK_DELAY * 2);
	}

	_delay_ms(DEBUG_BLINK_DELAY * 2);
	charge_set_led(false);
	_delay_ms(DEBUG_BLINK_DELAY * 4);
}
#endif // DEBUG_VOID

#endif // DEBUG

#ifdef ENABLE_SERIAL

static void _serial_tx(uint8_t* buffer)
{
	//uint8_t time_fix = 0;
	// 3333/2 - 10
	// 650
	//	[-2 for DEV] -20 works (perhaps different USB-Serial converter)
	//	[-20 for PRD] Which board?
	//	+20 Board #5 (-0: 3.592, -10: 3.280)
	const uint16_t delay = 650+20;
	uint16_t countdown;
	
	for (uint8_t j = 0; j < 10; ++j)
	{
		if (buffer[j])
		SERIAL_DEBUG_PORT |= _BV(SERIAL_DEBUG_INDEX);
		else
		SERIAL_DEBUG_PORT &= ~_BV(SERIAL_DEBUG_INDEX);
		
		countdown = delay;
		while (--countdown)
		__asm("nop");
	}
}

static void _serial_tx_char(char c)
{
	uint8_t buffer[10];
	uint8_t i = 0;
	
	buffer[i++] = 0;	// START
	for (int idx = 0; idx < 8; ++idx)
	buffer[i++] = (((uint8_t)(c) & ((uint8_t)1<<((idx)))) ? 0x01 : 0x00);	// Endianness: 7-
	buffer[i++] = 1;	// STOP
	
	_serial_tx(buffer);
}

void debug_log_ex_P(const char* message, bool new_line)
{
	char c = pgm_read_byte(message);
	if (c == '\0')
		return;
	
	pmc_mask_irqs(true);

	do
	{
		_serial_tx_char(c);
		c = pgm_read_byte(++message);
	} while (c != '\0');
	
	if (new_line)
		_serial_tx_char('\n');

	io_set_pin(SERIAL_DEBUG);
	
	pmc_mask_irqs(false);
}

void _debug_log_ex(const char* message, bool new_line)
{
	if (message[0] == '\0')
	return;
	
	pmc_mask_irqs(true);

	do
	{
		_serial_tx_char(*message);
	} while (*(++message) != '\0');
	
	if (new_line)
	_serial_tx_char('\n');

	io_set_pin(SERIAL_DEBUG);
	
	pmc_mask_irqs(false);
}

void debug_log_byte_ex(uint8_t n, bool new_line)
{
	char ch[4];
	ch[0] = '0' + (n / 100);
	ch[1] = '0' + ((n % 100) / 10);
	ch[2] = '0' + (n % 10);
	ch[3] = '\0';
	_debug_log_ex(ch, new_line);
}

void debug_log_hex_ex(uint8_t n, bool new_line)
{
	char ch[4];
	ch[0] = 'x';
	uint8_t _n = n >> 4;
	if (_n < 10)
		ch[1] = '0' + _n;
	else
		ch[1] = 'A' + (_n - 10);
	n &= 0x0F;
	if (n < 10)
		ch[2] = '0' + n;
	else
		ch[2] = 'A' + (n - 10);
	ch[3] = '\0';
	_debug_log_ex(ch, new_line);
}

#endif // ENABLE_SERIAL
