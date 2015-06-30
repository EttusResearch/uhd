/*
 * Copyright 2013-2014 Ettus Research LLC
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
 *      persistent EEPROM
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
 * BOOTSZ    = 4095W_F000
 * BOOTRST   = [ ]
 * CKOPT     = [X]
 * BODLEVEL  = 2V7
 * BODEN     = [ ]
 * SUT_CKSEL = EXTHIFXTALRES_16KCK_64MS
 *
 * EXTENDED  = 0xFF (valid)
 * HIGH      = 0x81 (valid)
 * LOW       = 0xFF (valid)
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <avrlibdefs.h>
#include <octoclock.h>
#include <debug.h>
#include <clkdist.h>
#include <state.h>
#include <network.h>
#include <usart.h>
#include <net/udp_handlers.h>

/*
 * Timer 3 (16-bit)
 *  * Set input capture trigger to rising edge
 *  * Set prescaler to 1
 *  * Enable overflow interrupt
 *  * Set timer to 0
 */
#define TIMER3_INIT() TCCR3B = (1 << ICES3) | (1 << CS30); \
                      ETIMSK |= (1 << TOIE3); \
                      TCNT3 = 0; \
                      ICR3 = 0;

/*
 * We use TIMER3 as a watchdog timer for external reference
 * detection. Once a signal is detected, we allow for five
 * timer overflows (~26 ms) without another signal before
 * deciding that there is no external reference connected.
 */
#define EXT_REF_TIMEOUT 5

static volatile uint16_t num_overflows = 0;
static uint16_t current_num_overflows = 0;
static uint16_t prev_num_overflows = 0;
static uint16_t current_ICR3 = 0;
static uint16_t prev_ICR3 = 0;
static ref_t        prev_ref           = NO_REF;
static switch_pos_t prev_switch_pos    = PREFER_EXTERNAL;
bool top = false;

ISR(TIMER3_OVF_vect){
    num_overflows++;
}

/*******************************************************************************
*   Main Routine
*******************************************************************************/

int main(void){

    /*
     * Initializations
     */
    cli();

    // Make sure interrupts belong to us
    MCUCR = (1<<IVCE);
    MCUCR = 0;

    // Initialize global variables
    g_ext_ref_present = false;
    g_gps_present = false;
    g_switch_pos = PREFER_INTERNAL;
    g_ref = NO_REF;

    // Atmega128
    setup_atmel_io_ports();

    // Reset ClkDist chip
    reset_TI_CDCE18005();

    // GPSDO communication
    usart_init();

    // Ethernet stack
    network_init();
    register_udp_listener(OCTOCLOCK_UDP_CTRL_PORT,  handle_udp_ctrl_packet);
    register_udp_listener(OCTOCLOCK_UDP_GPSDO_PORT, handle_udp_gpsdo_packet);

    // Timers
    TIMER1_INIT(); // Network
    TIMER3_INIT(); // External reference check

    // Debug (does nothing when not in debug mode)
    DEBUG_INIT();
    DEBUG_LOG(" "); // Force a newline between runs

    leds_off();

    sei();

    // Check if GPS present (should only need to happen once)
    g_gps_present = (PIND & (1<<DDD4));

    // State of previous iteration
    prev_ref           = NO_REF;
    prev_switch_pos    = PREFER_EXTERNAL;
    cli();
    prev_ICR3          = ICR3;
    sei();
    prev_num_overflows = 0;

    /*
     * Main loop
     */
    while(true){
        // Check switch position
        g_switch_pos = (PINC & (1<<DDC1)) ? PREFER_EXTERNAL : PREFER_INTERNAL;

        /*
         * Check input capture pin for external reference detection.
         *
         * 16-bit reads could be corrupted during an interrupt, so
         * disable interrupts for safety.
         */
        cli();
        current_ICR3          = ICR3;
        current_num_overflows = num_overflows;
        sei();

        // Signal detected, reset timer
        if(current_ICR3 != prev_ICR3){
            cli();
            TCNT3 = 0;
            num_overflows = 0;
            sei();
            g_ext_ref_present = true;
        }

        // Timeout, no external reference detected
        if(current_num_overflows >= EXT_REF_TIMEOUT){
            g_ext_ref_present = false;
        }

        // Determine and set reference based on state
        if(!g_gps_present && !g_ext_ref_present)     g_ref = NO_REF;
        else if(g_gps_present && !g_ext_ref_present) g_ref = INTERNAL;
        else if(!g_gps_present && g_ext_ref_present) g_ref = EXTERNAL;
        else g_ref = (g_switch_pos == PREFER_INTERNAL) ? INTERNAL : EXTERNAL;

        if((g_ref != prev_ref) || (g_switch_pos != prev_switch_pos)){
            if(g_switch_pos == PREFER_INTERNAL) prefer_internal();
            else prefer_external();
        }

        // Record this iteration's state
        prev_ref           = g_ref;
        prev_switch_pos    = g_switch_pos;
        prev_ICR3          = current_ICR3;
        prev_num_overflows = current_num_overflows;

        // Handle incoming Ethernet packets
        network_check();
    }
}
