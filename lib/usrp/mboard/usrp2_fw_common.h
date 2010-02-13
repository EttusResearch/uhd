//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_USRP2_FW_COMMON_H
#define INCLUDED_USRP2_FW_COMMON_H

/*!
 * Structs and constants for usrp2 communication.
 * This header is shared by the firmware and host code.
 * Therefore, this header may only contain valid C code.
 */
#ifdef __cplusplus
extern "C" {
#endif

// udp ports for the usrp2 communication
// Dynamic and/or private ports: 49152-65535
#define USRP2_UDP_CTRL_PORT 49152
#define USRP2_UDP_DATA_PORT 49153

typedef enum{
    USRP2_CTRL_ID_HUH_WHAT,
    //USRP2_CTRL_ID_FOR_SURE, //TODO error condition enums
    //USRP2_CTRL_ID_SUX_MAN,
    USRP2_CTRL_ID_GIVE_ME_YOUR_IP_ADDR_BRO,
    USRP2_CTRL_ID_THIS_IS_MY_IP_ADDR_DUDE,
    USRP2_CTRL_ID_HERE_IS_A_NEW_IP_ADDR_BRO,
    USRP2_CTRL_ID_GIVE_ME_YOUR_MAC_ADDR_BRO,
    USRP2_CTRL_ID_THIS_IS_MY_MAC_ADDR_DUDE,
    USRP2_CTRL_ID_HERE_IS_A_NEW_MAC_ADDR_BRO
} usrp2_ctrl_id_t;

typedef struct{
    uint32_t id;
    uint32_t seq;
    union{
        uint32_t ip_addr;
        uint8_t mac_addr[6];
        /*struct {
            uint8_t bank;
            uint16_t ddr;
            uint16_t mask;
        } gpio_ddr_args;
        struct {
            uint8_t bank;
            uint16_t val;
            uint16_t mask;
        } gpio_val_args;*/
    } data;
} usrp2_ctrl_data_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_USRP2_FW_COMMON_H */
