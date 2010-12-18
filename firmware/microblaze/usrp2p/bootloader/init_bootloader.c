/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

#include <memory_map.h>
#include <nonstdio.h>
#include <hal_io.h>
#include <xilinx_s3_icap.h>
#include <spi_flash.h>
#include <spi_flash_private.h>
//#include <clocks.h>
#include <ihex.h>
#include <bootloader_utils.h>
#include <string.h>
#include <hal_uart.h>
#include <i2c.h>
#include "usrp2/fw_common.h"

//void pic_interrupt_handler() __attribute__ ((interrupt_handler));

void pic_interrupt_handler()
{
  // nop stub
}

void load_ihex(void) { //simple IHEX parser to load proper records into RAM. loads program when it receives end of record.
	char buf[128]; //input data buffer
	uint8_t ihx[32]; //ihex data buffer

	ihex_record_t ihex_record;
	ihex_record.data = ihx;

	while(1) {
		gets(buf);

		if(!ihex_parse(buf, &ihex_record)) { //RAM data record is valid
			if(ihex_record.addr >= RAM_BASE) { //it's expecting to see FULLY RELOCATED IHX RECORDS. every address referenced to 0x8000, including vectors.
				memcpy((void *) (ihex_record.addr), ihex_record.data, ihex_record.length);
				puts("OK");
			} else if(ihex_record.type == 1) { //end of record
				puts("OK");
				//load main firmware
				start_program();
				puts("ERROR: main image returned! Back in IHEX load mode.");
			} else puts("NOK"); //RAM loads do not support extended segment address records (04) -- upper 16 bits are always "0".
		} else puts("NOK");
	}
}

void delay(uint32_t t) {
	while(t-- != 0) asm("NOP");
}

int main(int argc, char *argv[]) {
  hal_disable_ints();	// In case we got here via jmp 0x0
	output_regs->leds = 0xFF;
	delay(5000);
	output_regs->leds = 0x00;
	hal_uart_init();
	spif_init();
	i2c_init(); //for EEPROM
	puts("USRP2+ bootloader super ultra ZPU edition\n");
	
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
				puts("ERROR: no safe firmware image available. I am a brick. Feel free to load IHEX to RAM.");
				load_ihex();
			}
	}
	
	if(!production_image) {
		puts("Checking for valid production FPGA image...");
		if(is_valid_fpga_image(PROD_FPGA_IMAGE_LOCATION_ADDR)) {
			puts("Valid production FPGA image found. Attempting to boot.");
			set_safe_booted_flag(1);
			delay(300); //so serial output can finish
			icap_reload_fpga(PROD_FPGA_IMAGE_LOCATION_ADDR);
		}
		puts("No valid production FPGA image found.\nAttempting to load production firmware...");
	}
	if(is_valid_fw_image(PROD_FW_IMAGE_LOCATION_ADDR)) {
		puts("Valid production firmware found. Loading...");
		spi_flash_read(PROD_FW_IMAGE_LOCATION_ADDR, FW_IMAGE_SIZE_BYTES, (void *)RAM_BASE);
		puts("Finished loading. Starting image.");
		delay(300);
		start_program();
		puts("ERROR: Return from main program! This should never happen!");
		//if this happens, though, the safest thing to do is reboot the whole FPGA and start over.
		delay(300);
		icap_reload_fpga(SAFE_FPGA_IMAGE_LOCATION_ADDR);
		return 1;
	}
	puts("No valid production firmware found. Trying safe firmware...");
	if(is_valid_fw_image(SAFE_FW_IMAGE_LOCATION_ADDR)) {
		spi_flash_read(SAFE_FW_IMAGE_LOCATION_ADDR, FW_IMAGE_SIZE_BYTES, (void *)RAM_BASE);
		puts("Finished loading. Starting image.");
		delay(300);
		start_program();
		puts("ERROR: return from main program! This should never happen!");
		delay(300);
		icap_reload_fpga(SAFE_FPGA_IMAGE_LOCATION_ADDR);
		return 1;
	}
	puts("ERROR: no safe firmware image available. I am a brick. Feel free to load IHEX to RAM.");
	load_ihex();

	return 0;
}
