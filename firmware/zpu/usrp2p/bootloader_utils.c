/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

//contains routines for loading programs from Flash. depends on Flash libraries.
//also contains routines for reading / writing EEPROM flags for the bootloader
#include <stdbool.h>
#include <string.h>
#include <bootloader_utils.h>
#include <spi_flash.h>
#include <i2c.h>
#include <memory_map.h>
#include <nonstdio.h>
#include <xilinx_s3_icap.h>
#include <mdelay.h>
#include "spi.h"

#define BUTTON_PUSHED ((router_status->irqs & PIC_BUTTON) ? 0 : 1)

int is_valid_fpga_image(uint32_t addr) {
//	printf("is_valid_fpga_image(): starting with addr=%x...\n", addr);
	uint8_t imgbuf[64];
	spi_flash_read(addr, 64, imgbuf);
	//we're just looking for leading 0xFF padding, followed by the sync bytes 0xAA 0x99
	for(size_t i = 0; i<63; i++) {
		if(imgbuf[i] == 0xFF) continue;
		if(imgbuf[i] == 0xAA && imgbuf[i+1] == 0x99) {
			//printf("is_valid_fpga_image(): found valid FPGA image\n");
			return 1;
		}
	}
	
	return 0;
}

int is_valid_fw_image(uint32_t addr) {
	static const uint8_t fwheader[] = {0x0b, 0x0b, 0x0b, 0x0b}; //just lookin for a jump to anywhere located at the reset vector
	//printf("is_valid_fw_image(): starting with addr=%x...\n", addr);
	uint8_t buf[12];
	spi_flash_read(addr, 4, buf);
	//printf("is_valid_fw_image(): read ");
	//for(int i = 0; i < 5; i++) printf("%x ", buf[i]);
	//printf("\n");
	return memcmp(buf, fwheader, 4) == 0;
}

void start_program(void)
{
	//ignoring the addr now
	//all this does is tap that register
	*((volatile uint32_t *) SR_ADDR_BLDRDONE) = 1;
}

void do_the_bootload_thing(void) {
	spif_init(); //initialize SPI flash clock
	
    bool production_image = find_safe_booted_flag();
	set_safe_booted_flag(0); //haven't booted yet
	
	if(BUTTON_PUSHED) { //see memory_map.h
		puts("Starting USRP2+ in safe mode. Loading safe firmware.");
        return;
	}
	
	if(!production_image) {
		puts("Checking for valid production FPGA image...");
		if(is_valid_fpga_image(PROD_FPGA_IMAGE_LOCATION_ADDR)) {
			puts("Valid production FPGA image found. Attempting to boot.");
			set_safe_booted_flag(1);
			mdelay(300); //so serial output can finish
			icap_reload_fpga(PROD_FPGA_IMAGE_LOCATION_ADDR);
		}
		puts("No valid production FPGA image found.\nFalling through to built-in firmware.");
		return;
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
	puts("No valid production firmware found. Falling through to built-in firmware.");
	/*
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
    puts("ERROR: no safe firmware image available. Falling through to built-in firmware.");
    */
}
