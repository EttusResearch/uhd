/*
 * Copyright 2010-2012 Ettus Research LLC
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

//Routines to handle updating the SPI Flash firmware via UDP

#include <net_common.h>
#include "memory_map.h"
#include "usrp2/fw_common.h"
#include "spi.h"
#include "spi_flash.h"
#include <nonstdio.h>
#include <string.h>
#include "ethernet.h"
#include "udp_fw_update.h"
#include "xilinx_s3_icap.h"
#include "i2c.h"

uint16_t get_hw_rev(void) {
    uint16_t tmp;
    eeprom_read(USRP2_I2C_ADDR_MBOARD, USRP2_EE_MBOARD_REV, &tmp, sizeof(tmp));
    return tmp;
}

spi_flash_async_state_t spi_flash_async_state;

//Firmware update packet handler
void handle_udp_fw_update_packet(struct socket_address src, struct socket_address dst,
                                 unsigned char *payload, int payload_len) {

  const usrp2_fw_update_data_t *update_data_in = (usrp2_fw_update_data_t *) payload;

  usrp2_fw_update_data_t update_data_out;
  usrp2_fw_update_id_t update_data_in_id = update_data_in->id;

  //ensure that the protocol versions match
/*  if (payload_len >= sizeof(uint32_t) && update_data_in->proto_ver != USRP2_FW_COMPAT_NUM){
    printf("!Error in update packet handler: Expected compatibility number %d, but got %d\n",
        USRP2_FW_COMPAT_NUM, update_data_in->proto_ver
      );
      update_data_in_id = USRP2_FW_UPDATE_ID_OHAI_LOL; //so we can respond
  }
*/
  //ensure that this is not a short packet
  if (payload_len < sizeof(usrp2_fw_update_data_t)){
      printf("!Error in update packet handler: Expected payload length %d, but got %d\n",
          (int)sizeof(usrp2_fw_update_data_t), payload_len
      );
      update_data_in_id = USRP2_FW_UPDATE_ID_WAT;
  }

  switch(update_data_in_id) {
  case USRP2_FW_UPDATE_ID_OHAI_LOL: //why hello there you handsome devil
    update_data_out.id = USRP2_FW_UPDATE_ID_OHAI_OMG;
    memcpy(&update_data_out.data.ip_addr, (void *)get_ip_addr(), sizeof(struct ip_addr));
    //this is to stop streaming for the folks who think updating while streaming is a good idea
    sr_rx_ctrl0->clear = 1;
    sr_rx_ctrl1->clear = 1;
    sr_tx_ctrl->cyc_per_up = 0;
    break;

  case USRP2_FW_UPDATE_ID_WATS_TEH_FLASH_INFO_LOL: //query sector size, memory size so the host can mind the boundaries
    update_data_out.data.flash_info_args.sector_size_bytes = spi_flash_sector_size();
    update_data_out.data.flash_info_args.memory_size_bytes = spi_flash_memory_size();
    update_data_out.id = USRP2_FW_UPDATE_ID_HERES_TEH_FLASH_INFO_OMG;
    break;

  case USRP2_FW_UPDATE_ID_I_CAN_HAS_HW_REV_LOL: //get the hardware revision of the platform for validation checking
    update_data_out.data.hw_rev = (uint32_t) get_hw_rev();
    update_data_out.id = USRP2_FW_UPDATE_ID_HERES_TEH_HW_REV_OMG;
    break;

  case USRP2_FW_UPDATE_ID_ERASE_TEH_FLASHES_LOL: //out with the old
    spi_flash_async_erase_start(&spi_flash_async_state, update_data_in->data.flash_args.flash_addr, update_data_in->data.flash_args.length);
    update_data_out.id = USRP2_FW_UPDATE_ID_ERASING_TEH_FLASHES_OMG;
    break;

  case USRP2_FW_UPDATE_ID_R_U_DONE_ERASING_LOL:
    //poll for done, set something in the reply packet
    //spi_flash_async_erase_poll() also advances the state machine, so you should call it reasonably often to get things done quicker
    if(spi_flash_async_erase_poll(&spi_flash_async_state)) update_data_out.id = USRP2_FW_UPDATE_ID_IM_DONE_ERASING_OMG;
    else update_data_out.id = USRP2_FW_UPDATE_ID_NOPE_NOT_DONE_ERASING_OMG;
    break;

  case USRP2_FW_UPDATE_ID_WRITE_TEH_FLASHES_LOL: //and in with the new
    //spi_flash_program() goes pretty quick compared to page erases, so we don't bother polling -- it'll come back in some milliseconds
    //if it doesn't come back fast enough, we'll just write smaller packets at a time until it does
    spi_flash_program(update_data_in->data.flash_args.flash_addr, update_data_in->data.flash_args.length, update_data_in->data.flash_args.data);
    update_data_out.id = USRP2_FW_UPDATE_ID_WROTE_TEH_FLASHES_OMG;
    break;

  case USRP2_FW_UPDATE_ID_READ_TEH_FLASHES_LOL: //for verify
    spi_flash_read(update_data_in->data.flash_args.flash_addr,  update_data_in->data.flash_args.length, update_data_out.data.flash_args.data);
    update_data_out.id = USRP2_FW_UPDATE_ID_KK_READ_TEH_FLASHES_OMG;
    break;

  case USRP2_FW_UPDATE_ID_RESET_MAH_COMPUTORZ_LOL: //for if we ever get the ICAP working
    //should reset via icap_reload_fpga(uint32_t flash_address);
    update_data_out.id = USRP2_FW_UPDATE_ID_RESETTIN_TEH_COMPUTORZ_OMG;
    //you should note that if you get a reply packet to this the reset has obviously failed
    icap_reload_fpga(0);
    break;

//  case USRP2_FW_UPDATE_ID_KTHXBAI: //see ya
//    break;

  default: //uhhhh
    update_data_out.id = USRP2_FW_UPDATE_ID_WAT;
  }
  send_udp_pkt(USRP2_UDP_UPDATE_PORT, src, &update_data_out, sizeof(update_data_out));
}
