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
 * BOOTSZ    = 4096W_F000
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

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <avr/eeprom.h>
#include <avr/io.h>

#include <octoclock.h>
#include <clkdist.h>
#include <debug.h>
#include <state.h>
#include <network.h>
#include <usart.h>
#include <gpsdo.h>
#include <net/enc28j60.h>
#include <net/udp_handlers.h>

/*******************************************************************************
*   Main Routine
*******************************************************************************/

int main(void){

    asm("cli");

    setup_atmel_io_ports();
    network_init();

    #ifndef DEBUG
    asm("sei");
    #endif

    init_udp_listeners();
    register_udp_listener(OCTOCLOCK_UDP_CTRL_PORT, handle_udp_ctrl_packet);
    register_udp_listener(OCTOCLOCK_UDP_GPSDO_PORT, handle_udp_gpsdo_packet);

    DEBUG_INIT(); // Does nothing when not in debug mode
    DEBUG_LOG(" "); //Force a newline between runs
    usart_init();

    //Set initial ClkDist and front panel settings
    led(Middle,true);
    setup_TI_CDCE18005(Primary_GPS);    // 10 MHz from Internal Source

    led(Top,true);
    PORTA |= (1<<PA6);    // PPS from Internal source

    TIMER0_INIT();
    TIMER1_INIT();

    while(true) {
        network_check();
    }
}
