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

#include <avrlibdefs.h>
#include <debug.h>
#include <octoclock.h>
#include <clkdist.h>
#include <state.h>

// Global state variables
volatile bool g_ext_ref_present    = false;
volatile bool g_gps_present        = false;
volatile switch_pos_t g_switch_pos = PREFER_INTERNAL;
volatile ref_t g_ref               = NO_REF;

void led(led_t which, bool on){
    // selects the proper bit
    uint8_t LED = 0x20 << which;

    if(on)
        PORTC |= LED;
    else
        PORTC &= ~LED;
}

void leds_off(void){
    led(LED_TOP,    false);
    led(LED_MIDDLE, false);
    led(LED_BOTTOM, false);
}

static void force_internal(void){
    led(LED_TOP,    true);
    led(LED_MIDDLE, false);
    led(LED_BOTTOM, true);

    // Tell ClkDist chip to use internal signals
    cli();
    setup_TI_CDCE18005(Primary_GPS);
    sei();

    // Set PPS to internal
    PORTA |= (1<<PA6);
}

static void force_external(void){
    led(LED_TOP,    false);
    led(LED_MIDDLE, true);
    led(LED_BOTTOM, true);

    // Tell Clkdist chip to use external signals
    cli();
    setup_TI_CDCE18005(Secondary_Ext);
    sei();

    // Set PPS to external
    PORTA &= ~(1<<PA6);
}

void prefer_internal(void){
    // If internal is NOT OK, then force external
    if(g_gps_present)
        force_internal();
    else if(g_ext_ref_present)
        force_external();
    else
        leds_off();
}

void prefer_external(void){
    // If external is NOT OK, then force internal
    if(g_ext_ref_present)
        force_external();
    else if(g_gps_present)
        force_internal();
    else
        leds_off();
}
