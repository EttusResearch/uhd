/*
 * Copyright 2014-2015 Ettus Research LLC
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
#include <stdint.h>
#include <string.h>

#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <avrlibdefs.h>
#include <octoclock.h>
#include <debug.h>
#include <network.h>

#include <net/enc28j60.h>

#include "octoclock/common.h"

#define TIME_PASSED (TCNT1 > TIMER1_ONE_SECOND)

/*
 * States
 */
static bool received_cmd = false; // Received "PREPARE_FW_BURN_CMD" signal
static bool done_burning = false; // Received "FINALIZE_BURNING_CMD" signal
static bool app_checked  = false; // Ran validation check on firmware

/*
 * After new firmware is burned onto the device, the bootloader calculates its
 * CRC and burns it into the EEPROM. When the device boots, this CRC is used
 * to validate the firmware before loading it. This struct represents how the
 * information is stored in the EEPROM.
 */
typedef struct {
    uint16_t fw_len;
    uint16_t fw_crc;
} crc_info_t;

static crc_info_t crc_info;

/*
 * What actually burns the firmware onto the device.
 *
 * Source: http://www.atmel.com/webdoc/AVRLibcReferenceManual/group__avr__boot.html
 */
static void boot_program_page(uint8_t *buf, uint16_t page){
    // Disable interrupts
    uint8_t sreg = SREG;
    cli();

    eeprom_busy_wait();

    boot_page_erase(page);
    boot_spm_busy_wait(); // Wait until the memory is erased.

    for(uint16_t i = 0; i < SPM_PAGESIZE; i += 2){
        // Set up little-endian word.
        uint16_t w = *buf++;
        w += ((*buf++) << 8);

        boot_page_fill(page + i, w);
    }

    boot_page_write(page); // Store buffer in flash page.
    boot_spm_busy_wait();  // Wait until the memory is written.

    // Reenable RWW-section again. We need this if we want to jump back
    // to the application after bootloading.
    boot_rww_enable();

    // Restore interrupt state
    SREG = sreg;
    sei();
}

/*
 * Load firmware at given address into packet to send to host.
 */
static void read_firmware(uint16_t addr, octoclock_packet_t *pkt_out){
    for(size_t i = 0; i < SPM_PAGESIZE; i++){
        pkt_out->data[i] = pgm_read_byte(addr+i);
    }
}

/*
 * Calculate the CRC of the current firmware.
 *
 * Adapted from _crc16_update in <util/crc16.h>.
 */
static void calculate_crc(uint16_t *crc, uint16_t len){
    *crc = 0xFFFF;

    for(size_t i = 0; i < len; i++){
        *crc ^= pgm_read_byte(i);
        for(uint8_t j = 0; j < 8; ++j){
            if(*crc & 1) *crc = (*crc >> 1) ^ 0xA001;
            else *crc = (*crc >> 1);
        }
    }
}

/*
 * Calculate the CRC of the current firmware. If it matches the
 * CRC burned into the EEPROM, the firmware is considered valid,
 * and the bootloader can load it.
 */
static bool valid_app(){
    crc_info_t crc_eeprom_info;
    eeprom_read_block(&crc_eeprom_info, (void*)OCTOCLOCK_EEPROM_APP_LEN, 4);

    calculate_crc(&(crc_info.fw_crc), crc_eeprom_info.fw_len);
    return (crc_info.fw_crc == crc_eeprom_info.fw_crc);
}

/*
 * UDP handlers
 */
void handle_udp_query_packet(
    struct socket_address src, struct socket_address dst,
    unsigned char *payload, int payload_len
){
    const octoclock_packet_t *pkt_in = (octoclock_packet_t*)payload;

    // Respond to uhd::device::find(), identify as bootloader
    if(pkt_in->code == OCTOCLOCK_QUERY_CMD){
        octoclock_packet_t pkt_out;
        pkt_out.proto_ver = OCTOCLOCK_BOOTLOADER_PROTO_VER;
        pkt_out.sequence = pkt_in->sequence;
        pkt_out.code = OCTOCLOCK_QUERY_ACK;
        pkt_out.len = 0;
        send_udp_pkt(OCTOCLOCK_UDP_CTRL_PORT, src, (void*)&pkt_out, sizeof(octoclock_packet_t));
    }
}

void handle_udp_fw_packet(
    struct socket_address src, struct socket_address dst,
    unsigned char *payload, int payload_len
){
    octoclock_packet_t *pkt_in = (octoclock_packet_t*)payload;
    octoclock_packet_t pkt_out;
    pkt_out.proto_ver = OCTOCLOCK_BOOTLOADER_PROTO_VER;
    pkt_out.sequence = pkt_in->sequence;
    pkt_out.len = 0;

    switch(pkt_in->code){
        case PREPARE_FW_BURN_CMD:
            received_cmd = true;
            done_burning = false;
            crc_info.fw_crc = pkt_in->crc;
            crc_info.fw_len = pkt_in->len;
            pkt_out.code = FW_BURN_READY_ACK;
            break;

        // Burn firmware sent from the host
        case FILE_TRANSFER_CMD:
            boot_program_page(pkt_in->data, pkt_in->addr);
            pkt_out.code = FILE_TRANSFER_ACK;
            pkt_out.addr = pkt_in->addr;
            break;

        // Send firmware back to the host for verification
        case READ_FW_CMD:
            pkt_out.code = READ_FW_ACK;
            read_firmware(pkt_in->addr, &pkt_out);
            break;

        // Calculate the CRC of the new firmware and finish
        case FINALIZE_BURNING_CMD:
            done_burning = true;
            eeprom_write_block(&crc_info, (void*)OCTOCLOCK_EEPROM_APP_LEN, 4);
            pkt_out.code = FINALIZE_BURNING_ACK;
            break;

        default:
            break;
    }
    send_udp_pkt(OCTOCLOCK_UDP_FW_PORT, src, (void*)&pkt_out, sizeof(octoclock_packet_t));
}

void handle_udp_eeprom_packet(
    struct socket_address src, struct socket_address dst,
    unsigned char *payload, int payload_len
){
    octoclock_packet_t *pkt_in = (octoclock_packet_t*)payload;
    octoclock_packet_t pkt_out;
    pkt_out.proto_ver = OCTOCLOCK_BOOTLOADER_PROTO_VER;
    pkt_out.sequence = pkt_in->sequence;
    pkt_out.len = 0;

    // Restore OctoClock's EEPROM to factory state
    if(pkt_in->proto_ver == OCTOCLOCK_FW_COMPAT_NUM){
        switch(pkt_in->code){
            case CLEAR_EEPROM_CMD:
                received_cmd = true;
                uint8_t blank_eeprom[103]; // 103 is offset of CRC info
                memset(blank_eeprom, 0xFF, 103);
                eeprom_write_block(blank_eeprom, 0, 103);
                pkt_out.code = CLEAR_EEPROM_ACK;
                send_udp_pkt(OCTOCLOCK_UDP_EEPROM_PORT, src, (void*)&pkt_out, sizeof(octoclock_packet_t));
                break;

            default:
                break;
        }
    }
}

int main(void){

    // Disable watchdog timer
    wdt_disable();

    // Give interrupts to bootloader
    MCUCR = (1<<IVCE);
    MCUCR = (1<<IVSEL);
    cli();

    // Atmega128
    setup_atmel_io_ports();

    // Start timer
    TIMER1_INIT();

    // Ethernet stack
    network_init();
    register_udp_listener(OCTOCLOCK_UDP_CTRL_PORT, handle_udp_query_packet);
    register_udp_listener(OCTOCLOCK_UDP_FW_PORT, handle_udp_fw_packet);
    register_udp_listener(OCTOCLOCK_UDP_EEPROM_PORT, handle_udp_eeprom_packet);

    // Turn LED's on to show we're in the bootloader
    PORTC |= 0x20;
    PORTC |= (0x20<<1);
    PORTC |= (0x20<<2);

    /*
     * This loop determines whether the OctoClock will remain in its bootloader
     * state or if it will load the main firmware. After five seconds, it will
     * check to see if valid firmware is installed. If so, it will immediately
     * load it. Otherwise, it will remain here until firmware is installed.
     *
     * This process can be stopped by an instruction from the firmware burner
     * utility, at which point the OctoClock will remain in bootloader state until
     * instructed by the utility to exit the loop and load the new firmware.
     */
    while(true){
        if(done_burning){
            if(valid_app()) break;
            else done_burning = false; // Burning somehow failed and wasn't caught
        }
        if(!app_checked && !received_cmd && TIME_PASSED){
            app_checked = true;
            if(valid_app()) break;
        }

        network_check();
    }

    // Turn LED's off before moving to application
    PORTC &= ~0x20;
    PORTC &= ~(0x20<<1);
    PORTC &= ~(0x20<<2);

    /*
     * At this point, the bootloader has determined that there is valid
     * firmware installed on the device and that it is OK to load it.
     */
    TIMER1_DISABLE();
    MCUCR = (1<<IVCE);
    MCUCR = 0;
    cli();
    asm("jmp 0000");
}
