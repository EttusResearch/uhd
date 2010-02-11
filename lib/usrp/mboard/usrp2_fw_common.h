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
// Dynamic and/or private ports: 49152–65535
#define USRP2_UDP_CTRL_PORT 49152
#define USRP2_UDP_DATA_PORT 49153

typedef enum{
    USRP2_CTRL_ID_HELLO
} usrp2_ctrl_id_t;

typedef struct{
    uint32_t id;
    uint32_t seq;
    union{
        struct{
            uint32_t ip_addr;
            uint8_t  mac_addr[6];
            uint8_t  _padding[2];
        } discovery_addrs;
    } data;
} usrp2_ctrl_data_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_USRP2_FW_COMMON_H */
