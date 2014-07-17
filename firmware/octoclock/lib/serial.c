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

#include <stdbool.h>

#include <octoclock.h>
#include <serial.h>

#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

void serial_init(volatile uint8_t* port, uint8_t index){
    *port |= _BV(index);
}

static void _serial_tx_send(uint8_t* buffer, volatile uint8_t* port, uint8_t index){
    const uint8_t delay = BAUD_115200_DELAY;
    uint8_t countdown;

    for(uint8_t i = 0; i < 10; ++i){
        if(buffer[i]) *port |= _BV(index);
        else *port &= ~_BV(index);

        countdown = delay;
        while(--countdown) asm("nop");
    }
}

static void _serial_tx_char(char c, volatile uint8_t* port, uint8_t index){
	uint8_t buffer[10];
	uint8_t i = 0;

	buffer[i++] = 0;	// START
	for (int idx = 0; idx < 8; ++idx)
	buffer[i++] = (((uint8_t)(c) & ((uint8_t)1<<((idx)))) ? 0x01 : 0x00);	// Endianness: 7-
	buffer[i++] = 1;	// STOP

	_serial_tx_send(buffer, port, index);
}

void serial_tx_P(const char* message, volatile uint8_t* port, uint8_t index, bool newline){
    char c = pgm_read_byte(message);
    if(c == '\0') return;

    do{
        _serial_tx_char(c, port, index);
        c = pgm_read_byte(++message);
    } while(c != '\0');

    if(newline){
        _serial_tx_char('\r', port, index);
        _serial_tx_char('\n', port, index);
    }

    *port |= _BV(index);
}

void serial_tx(const char* message, volatile uint8_t* port, uint8_t index, bool newline){
    if (message[0] == '\0')
    return;

    do
    {
        _serial_tx_char(*message, port, index);
    } while (*(++message) != '\0');

    if (newline){
        _serial_tx_char('\r', port, index);
        _serial_tx_char('\n', port, index);
    }

    *port |= _BV(index);
}

void serial_tx_byte(uint8_t byte, volatile uint8_t* port, uint8_t index, bool newline){
    char ch[4];
    ch[0] = '0' + (byte / 100);
    ch[1] = '0' + ((byte % 100) / 10);
    ch[2] = '0' + (byte % 10);
    ch[3] = '\0';
    serial_tx(ch, port, index, newline);
}

void serial_tx_hex(uint8_t byte, volatile uint8_t* port, uint8_t index, bool newline){
    char ch[3];
    uint8_t _byte = byte >> 4;
    if (_byte < 10)
        ch[0] = '0' + _byte;
    else
        ch[0] = 'A' + (_byte - 10);
    byte &= 0x0F;
    if (byte < 10)
        ch[1] = '0' + byte;
    else
        ch[1] = 'A' + (byte - 10);
    ch[2] = '\0';
    serial_tx(ch, port, index, newline);
}

char serial_rx_char(volatile uint8_t* port, uint8_t index){
    char c = 0;
    const uint8_t delay = BAUD_115200_DELAY;
    uint8_t countdown;

    //Wait for character to appear, 0 will act as start marker
    while(*port & _BV(index));

    //With start marker there, wait for next bit
    countdown = delay;
    while(--countdown) asm("nop");

    for(uint8_t i = 0; i < 8; ++i){
        if(*port & _BV(index)) c &= (uint8_t)(1 << i);

        countdown = delay;
        while(--countdown) asm("nop");
    }

    return c;
}

//Assume ready (probably risky)
char serial_rx_char_nowait(volatile uint8_t* port, uint8_t index){
    char c = 0;
    const uint8_t delay = BAUD_115200_DELAY;
    uint8_t countdown;

    //Wait for start marker to pass
    countdown = delay;
    while(--countdown) asm("nop");

    for(uint8_t i = 0; i < 8; ++i){
        if(*port & _BV(index)) c &= (uint8_t)(1 << i);

        countdown = delay;
        while(--countdown) asm("nop");
    }

    return c;
}
