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

#include <avr/interrupt.h>

#include <octoclock.h>
#include <gpsdo.h>
#include <usart.h>

void send_gpsdo_cmd(char* buf, uint8_t size){
    for(uint8_t i = 0; i < size; i++) usart_putc(buf[i]);
}

//Serial out
ISR(USART1_RX_vect){
    gpsdo_buf[gpsdo_state.pos] = UDR1;

    if(gpsdo_state.pos == (POOLSIZE-1)){
        gpsdo_state.num_wraps++;
        gpsdo_state.pos = 0;
    }
    else gpsdo_state.pos++;
}
