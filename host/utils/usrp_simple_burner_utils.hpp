//
// Copyright 2012 Ettus Research LLC
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

#include <iostream>
#include <math.h>
#include <stdint.h>

#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <uhd/exception.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/msg.hpp>

#define UDP_FW_UPDATE_PORT 49154
#define UDP_MAX_XFER_BYTES 1024
#define UDP_TIMEOUT 3
#define UDP_POLL_INTERVAL 0.10 //in seconds
#define USRP2_FW_PROTO_VERSION 7 //should be unused after r6
#define USRP2_UDP_UPDATE_PORT 49154
#define FLASH_DATA_PACKET_SIZE 256
#define FPGA_IMAGE_SIZE_BYTES 1572864
#define FW_IMAGE_SIZE_BYTES 31744
#define PROD_FPGA_IMAGE_LOCATION_ADDR 0x00180000
#define PROD_FW_IMAGE_LOCATION_ADDR 0x00300000
#define SAFE_FPGA_IMAGE_LOCATION_ADDR 0x00000000
#define SAFE_FW_IMAGE_LOCATION_ADDR 0x003F0000

using namespace uhd;
using namespace uhd::transport;
namespace asio = boost::asio;

typedef enum {
    USRP2_FW_UPDATE_ID_WAT = ' ',

    USRP2_FW_UPDATE_ID_OHAI_LOL = 'a',
    USRP2_FW_UPDATE_ID_OHAI_OMG = 'A',

    USRP2_FW_UPDATE_ID_WATS_TEH_FLASH_INFO_LOL = 'f',
    USRP2_FW_UPDATE_ID_HERES_TEH_FLASH_INFO_OMG = 'F',

    USRP2_FW_UPDATE_ID_ERASE_TEH_FLASHES_LOL = 'e',
    USRP2_FW_UPDATE_ID_ERASING_TEH_FLASHES_OMG = 'E',

    USRP2_FW_UPDATE_ID_R_U_DONE_ERASING_LOL = 'd',
    USRP2_FW_UPDATE_ID_IM_DONE_ERASING_OMG = 'D',
    USRP2_FW_UPDATE_ID_NOPE_NOT_DONE_ERASING_OMG = 'B',

    USRP2_FW_UPDATE_ID_WRITE_TEH_FLASHES_LOL = 'w',
    USRP2_FW_UPDATE_ID_WROTE_TEH_FLASHES_OMG = 'W',

    USRP2_FW_UPDATE_ID_READ_TEH_FLASHES_LOL = 'r',
    USRP2_FW_UPDATE_ID_KK_READ_TEH_FLASHES_OMG = 'R',

    USRP2_FW_UPDATE_ID_RESET_MAH_COMPUTORZ_LOL = 's',
    USRP2_FW_UPDATE_ID_RESETTIN_TEH_COMPUTORZ_OMG = 'S',

    USRP2_FW_UPDATE_ID_I_CAN_HAS_HW_REV_LOL = 'v',
    USRP2_FW_UPDATE_ID_HERES_TEH_HW_REV_OMG = 'V',

    USRP2_FW_UPDATE_ID_KTHXBAI = '~' 

} usrp2_fw_update_id_t;

typedef struct {
    uint32_t proto_ver;
    uint32_t id; 
    uint32_t seq;
    union {
        uint32_t ip_addr;
        uint32_t hw_rev;
        struct {
            uint32_t flash_addr;
            uint32_t length;
            uint8_t  data[256];
        } flash_args;
        struct {
            uint32_t sector_size_bytes;
            uint32_t memory_size_bytes;
        } flash_info_args;
    } data;
} usrp2_fw_update_data_t;
