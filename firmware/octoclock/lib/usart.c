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

#include <octoclock.h>
#include <usart.h>

#include <util/delay.h>
#include <avr/io.h>

void usart_init(void){
    UCSR1B = (1 << TXEN) | (1 << RXEN) | (1 << RXCIE); //Turn on TX/RX circuitry, enable RX interrupts
    UCSR1C = (3 << UCSZ0); //Use 8-bit character sizes, 1 stop bit, no parity
    UBRR1H = (uint8_t)(BAUD_PRESCALE >> 8);
    UBRR1L = (uint8_t)BAUD_PRESCALE;
}

char usart_getc(void){
    while((UCSR1A & (1 << RXC)) == 0);

    return UDR1;
}

void usart_putc(char ch){
    while((UCSR1A & (1 << UDRE1)) == 0);

    UDR1 = ch;
}
