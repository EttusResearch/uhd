//
// Copyright 2010-2011 Ettus Research LLC
//
/*
 * Copyright 2007,2008 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <lwip/ip.h>
#include <lwip/udp.h>
#include "u2_init.h"
#include "memory_map.h"
#include "spi.h"
#include "hal_io.h"
#include "pic.h"
#include <stdbool.h>
#include "ethernet.h"
#include "nonstdio.h"
#include <net/padded_eth_hdr.h>
#include <net_common.h>
#include "memcpy_wa.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "clocks.h"
#include "usrp2/fw_common.h"
#include <i2c.h>
#include <ethertype.h>
#include <arp_cache.h>
#include "udp_fw_update.h"
#include "pkt_ctrl.h"
#include "banal.h"
#include <bootloader_utils.h>
#include <spi_flash.h>
#include <xilinx_s3_icap.h>
#include <mdelay.h>

static void handle_inp_packet(uint32_t *buff, size_t num_lines){

  //test if its an ip recovery packet
  typedef struct{
      padded_eth_hdr_t eth_hdr;
      char code[4];
      union {
        struct ip_addr ip_addr;
      } data;
  }recovery_packet_t;
  recovery_packet_t *recovery_packet = (recovery_packet_t *)buff;
  if (recovery_packet->eth_hdr.ethertype == 0xbeee && strncmp(recovery_packet->code, "addr", 4) == 0){
      printf("Got ip recovery packet: "); print_ip_addr(&recovery_packet->data.ip_addr); newline();
      set_ip_addr(&recovery_packet->data.ip_addr);
      return;
  }

  //pass it to the slow-path handler
  handle_eth_packet(buff, num_lines);
}


//------------------------------------------------------------------

/*
 * Called when eth phy state changes (w/ interrupts disabled)
 */
void link_changed_callback(int speed){
    printf("\neth link changed: speed = %d\n", speed);
    if (speed != 0){
        hal_set_leds(LED_RJ45, LED_RJ45);
        pkt_ctrl_set_routing_mode(PKT_CTRL_ROUTING_MODE_MASTER);
        send_gratuitous_arp();
    }
    else{
        hal_set_leds(0x0, LED_RJ45);
        pkt_ctrl_set_routing_mode(PKT_CTRL_ROUTING_MODE_SLAVE);
    }
}

static void do_the_bootload_thing(void) {

    bool production_image = find_safe_booted_flag();
	set_safe_booted_flag(0); //haven't booted yet
	
	if(BUTTON_PUSHED) { //see memory_map.h
		puts("Starting USRP2+ in safe mode.");
		if(is_valid_fw_image(SAFE_FW_IMAGE_LOCATION_ADDR)) {
				set_safe_booted_flag(1); //let the firmware know it's the safe image
				spi_flash_read(SAFE_FW_IMAGE_LOCATION_ADDR, FW_IMAGE_SIZE_BYTES, (void *)RAM_BASE);
				start_program();
				puts("ERROR: return from main program! This should never happen!");
				icap_reload_fpga(SAFE_FPGA_IMAGE_LOCATION_ADDR);
			} else {
				puts("ERROR: no safe firmware image available. I am a brick. Feel free to reprogram me via the UDP burner.");
				return;
			}
	}
	
	if(!production_image) {
		puts("Checking for valid production FPGA image...");
		if(is_valid_fpga_image(PROD_FPGA_IMAGE_LOCATION_ADDR)) {
			puts("Valid production FPGA image found. Attempting to boot.");
			set_safe_booted_flag(1);
			mdelay(300); //so serial output can finish
			icap_reload_fpga(PROD_FPGA_IMAGE_LOCATION_ADDR);
		}
		puts("No valid production FPGA image found.\nAttempting to load production firmware...");
	}
	if(is_valid_fw_image(PROD_FW_IMAGE_LOCATION_ADDR)) {
		puts("Valid production firmware found. Loading...");
		spi_flash_read(PROD_FW_IMAGE_LOCATION_ADDR, FW_IMAGE_SIZE_BYTES, (void *)RAM_BASE);
		puts("Finished loading. Starting image.");
		mdelay(300);
		start_program();
		puts("ERROR: Return from main program! This should never happen!");
		//if this happens, though, the safest thing to do is reboot the whole FPGA and start over.
		mdelay(300);
		icap_reload_fpga(SAFE_FPGA_IMAGE_LOCATION_ADDR);
		return;
	}
	puts("No valid production firmware found. Trying safe firmware...");
	if(is_valid_fw_image(SAFE_FW_IMAGE_LOCATION_ADDR)) {
		spi_flash_read(SAFE_FW_IMAGE_LOCATION_ADDR, FW_IMAGE_SIZE_BYTES, (void *)RAM_BASE);
		puts("Finished loading. Starting image.");
		mdelay(300);
		start_program();
		puts("ERROR: return from main program! This should never happen!");
		mdelay(300);
		icap_reload_fpga(SAFE_FPGA_IMAGE_LOCATION_ADDR);
		return;
	}
    puts("ERROR: no safe firmware image available. I am a brick. Feel free to reprogram me via the UDP burner.");
}

int
main(void)
{
  u2_init();

  set_default_mac_addr();
  set_default_ip_addr();

  putstr("\nN210 bootloader, UDP edition\n");
  print_mac_addr(ethernet_mac_addr()); newline();
  print_ip_addr(get_ip_addr()); newline();
  printf("FPGA compatibility number: %d\n", USRP2_FPGA_COMPAT_NUM);
  printf("Firmware compatibility number: %d\n", USRP2_FW_COMPAT_NUM);
  
  do_the_bootload_thing();
  
  //if we reach this point, the bootloader has fallen through, so we initalize the UDP handler for updating

  //1) register the addresses into the network stack
  //TODO: make this a fixed IP address, don't depend on EEPROM
  register_addrs(ethernet_mac_addr(), get_ip_addr());
  pkt_ctrl_program_inspector(get_ip_addr(), USRP2_UDP_DSP0_PORT);

  //2) register callbacks for udp ports we service
  init_udp_listeners();
  register_udp_listener(USRP2_UDP_UPDATE_PORT, handle_udp_fw_update_packet);
  
  pkt_ctrl_set_routing_mode(PKT_CTRL_ROUTING_MODE_SLAVE);

  //4) setup ethernet hardware to bring the link up
  ethernet_register_link_changed_callback(link_changed_callback);
  ethernet_init();

  while(true){

    size_t num_lines;
    void *buff = pkt_ctrl_claim_incoming_buffer(&num_lines);
    if (buff != NULL){
        handle_inp_packet((uint32_t *)buff, num_lines);
        pkt_ctrl_release_incoming_buffer();
    }

    pic_interrupt_handler();
  }
}
