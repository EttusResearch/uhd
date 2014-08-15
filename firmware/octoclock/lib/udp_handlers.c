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

#include <string.h>

#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include <octoclock.h>
#include <gpsdo.h>
#include <network.h>
#include <state.h>
#include <net/udp_handlers.h>

void handle_udp_ctrl_packet(
    struct socket_address src, struct socket_address dst,
    unsigned char *payload, int payload_len
){
    const octoclock_packet_t *pkt_in = (octoclock_packet_t*)payload;
    octoclock_packet_t pkt_out;
    pkt_out.proto_ver = OCTOCLOCK_FW_COMPAT_NUM;
    pkt_out.sequence = pkt_in->sequence;

    //If the firmware is incompatible, only respond to queries
    if(pkt_in->code == OCTOCLOCK_QUERY_CMD){
        pkt_out.code = OCTOCLOCK_QUERY_ACK;
        pkt_out.len = 0;
        send_udp_pkt(OCTOCLOCK_UDP_CTRL_PORT, src, (void*)&pkt_out, sizeof(octoclock_packet_t));
    }
    else if(pkt_in->proto_ver == OCTOCLOCK_FW_COMPAT_NUM){
        switch(pkt_in->code){
            case SEND_EEPROM_CMD:
                pkt_out.code = SEND_EEPROM_ACK;
                pkt_out.len = sizeof(octoclock_fw_eeprom_t);

                octoclock_fw_eeprom_t *eeprom_info = (octoclock_fw_eeprom_t*)pkt_out.data;

                //Read values from EEPROM into packet
                eeprom_read_block(eeprom_info, 0, sizeof(octoclock_fw_eeprom_t));

                //If EEPROM network fields are not fully populated, copy defaults
                if(using_network_defaults){
                    _MAC_ADDR(eeprom_info->mac_addr, 0x00,0x80,0x2F,0x11,0x22,0x33);
                    eeprom_info->ip_addr = default_ip;
                    eeprom_info->dr_addr = default_dr;
                    eeprom_info->netmask = default_netmask;
                }

                //Check if strings or revision is empty
                if(eeprom_info->serial[0] == 0xFF) memset(eeprom_info->serial, 0, 10);
                if(eeprom_info->name[0] == 0xFF) memset(eeprom_info->name, 0, 10);
                if(eeprom_info->revision == 0xFF) eeprom_info->revision = 0;
                break;

            case BURN_EEPROM_CMD:{
                //Confirm length of data
                if(pkt_in->len != sizeof(octoclock_fw_eeprom_t)){
                    pkt_out.code = BURN_EEPROM_FAILURE_ACK;
                    break;
                }

                /*
                 * In all cases, a full octoclock_fw_eeprom_t is written to lower the overall
                 * number of writes due to this EEPROM's smaller amount of safe writes.
                 * It is up to the host to make sure that the values that should be
                 * preserved are present in the octoclock_fw_eeprom_t struct.
                 */
                const octoclock_fw_eeprom_t *eeprom_pkt = (octoclock_fw_eeprom_t*)pkt_in->data;
                pkt_out.len = 0;

                //Write EEPROM data from packet
                eeprom_write_block(eeprom_pkt, 0, sizeof(octoclock_fw_eeprom_t));

                //Read back and compare to packet to confirm successful write
                uint8_t eeprom_contents[sizeof(octoclock_fw_eeprom_t)];
                eeprom_read_block(eeprom_contents, 0, sizeof(octoclock_fw_eeprom_t));
                uint8_t n = memcmp(eeprom_contents, eeprom_pkt, sizeof(octoclock_fw_eeprom_t));
                pkt_out.code = n ? BURN_EEPROM_FAILURE_ACK
                                 : BURN_EEPROM_SUCCESS_ACK;
                break;
            }

            case SEND_STATE_CMD:
                pkt_out.code = SEND_STATE_ACK;
                pkt_out.len = sizeof(octoclock_state_t);

                //Populate octoclock_state_t fields
                octoclock_state_t *state = (octoclock_state_t*)pkt_out.data;
                state->external_detected = global_ext_ref_is_present ? 1 : 0;
                state->gps_detected = (PIND & _BV(DDD4)) ? 1 : 0;
                state->which_ref = (uint8_t)which_ref();
                state->switch_pos = (uint8_t)get_switch_pos();
                break;

            case RESET_CMD:
                pkt_out.code = RESET_ACK;
                send_udp_pkt(OCTOCLOCK_UDP_CTRL_PORT, src, (void*)&pkt_out, sizeof(octoclock_packet_t));
                wdt_enable(WDTO_30MS);
                while(1);
                return;

            default:
                return;
        }

        send_udp_pkt(OCTOCLOCK_UDP_CTRL_PORT, src, (void*)&pkt_out, sizeof(octoclock_packet_t));
    }
}

void handle_udp_gpsdo_packet(
    struct socket_address src, struct socket_address dst,
    unsigned char *payload, int payload_len
){
    const octoclock_packet_t *pkt_in = (octoclock_packet_t*)payload;
    octoclock_packet_t pkt_out;
    pkt_out.proto_ver = OCTOCLOCK_FW_COMPAT_NUM;
    pkt_out.sequence = pkt_in->sequence;

    if(pkt_in->proto_ver == OCTOCLOCK_FW_COMPAT_NUM){
        switch(pkt_in->code){
            case HOST_SEND_TO_GPSDO_CMD:
                send_gpsdo_cmd((char*)pkt_in->data, pkt_in->len);
                pkt_out.code = HOST_SEND_TO_GPSDO_ACK;
                pkt_out.len = 0;
                break;

            case SEND_POOLSIZE_CMD:
                pkt_out.code = SEND_POOLSIZE_ACK;
                pkt_out.len = 0;
                pkt_out.poolsize = POOLSIZE;
                break;

            case SEND_CACHE_STATE_CMD:
                pkt_out.code = SEND_CACHE_STATE_ACK;
                pkt_out.state = gpsdo_state;
                break;

            case SEND_GPSDO_CACHE_CMD:
                pkt_out.code = SEND_GPSDO_CACHE_ACK;
                pkt_out.state = gpsdo_state;
                memcpy(pkt_out.data, gpsdo_buf, POOLSIZE);
                break;

            default:
                return;
        }

        send_udp_pkt(OCTOCLOCK_UDP_GPSDO_PORT, src, (void*)&pkt_out, sizeof(octoclock_packet_t));
    }
}
