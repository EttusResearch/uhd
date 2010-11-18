/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

//#include <stdio.h>
#include <stdlib.h>
#include <memory_map.h>
#include <nonstdio.h>
#include <hal_io.h>
#include <hal_uart.h>
#include <xilinx_s3_icap.h>
#include <spi_flash.h>
#include <spi_flash_private.h>
//#include <clocks.h>
#include <ihex.h>
#include <bootloader_utils.h>

//uses UART to load files to Flash in Intel HEX 16-bit format.
//this is one example of a "safe" firmware image to be loaded by a bootloader into main RAM.
//this CANNOT write to main RAM, since it is resident there.

//Sector 0-31: Safe FPGA bootloader image
//Sector 32-63: Production bootloader image
//Sector 64: Production firmware image
//Sector 127: Safe firmware image


void uart_flash_loader(void) {

	char buf[256]; //input data buffer
	uint8_t ihx[32]; //ihex data buffer
	uint32_t slot_offset = PROD_FW_IMAGE_LOCATION_ADDR; //initial slot offset to program to.
	uint32_t extended_addr = 0x00000000; //extended Intel hex segment address

	size_t sector_size = spi_flash_log2_sector_size();
	ihex_record_t ihex_record;
	ihex_record.data = ihx;
	int i;


	//not gonna win a turing prize for my C text parsing
	while(1) {
		gets(buf);
		if(!strncmp(buf, "!SECTORSIZE", 7)) { //return the sector size in log format
			putstr("OK ");
			puthex8((uint32_t) sector_size); //err, this should probably be decimal for human readability. we do have itoa now...
			putstr("\n");
		}
		else if(!strncmp(buf, "!SETADDR", 7)) { //set start address for programming
			slot_offset = atol(&buf[8]);
			puts("OK");
//			puthex32(slot_offset);
//			putstr("\n");
		}
		else if(!strncmp(buf, "!ERASE", 6)) { //erase a sector
			uint32_t sector = atol(&buf[6]);
			uint32_t size = 2 << (sector_size-1);
			uint32_t addr = sector << sector_size;

			spi_flash_erase(addr, size); //we DO NOT implement write protection here. it is up to the HOST PROGRAM to not issue an ERASE unless it means it.
																	 //unfortunately the Flash cannot write-protect the segments that really matter, so we only use global write-protect
																	 //as a means of avoiding accidental writes from runaway code / garbage on the SPI bus.
			puts("OK");
		}
//can't exactly run firmware if you're already executing out of main RAM
/*		else if(!strncmp(buf, "!RUNSFW", 7)) {
			if(is_valid_fw_image(SAFE_FW_IMAGE_LOCATION_ADDR)) {
				puts("OK");
				spi_flash_read(SAFE_FW_IMAGE_LOCATION_ADDR, FW_IMAGE_SIZE_BYTES, (void *)RAM_BASE);
				start_program(RAM_BASE);
			} else {
				puts("NOK");
			}
		}
		else if(!strncmp(buf, "!RUNPFW", 7)) {
			if(is_valid_fw_image(PROD_FW_IMAGE_LOCATION_ADDR)) {
				puts("OK");
				spi_flash_read(PROD_FW_IMAGE_LOCATION_ADDR, FW_IMAGE_SIZE_BYTES-1, (void *)RAM_BASE);
				start_program(RAM_BASE);
			} else {
				puts("NOK");
			}
		}
*/
		else if(!strncmp(buf, "!RUNPFPGA", 8)) {
			if(is_valid_fpga_image(PROD_FPGA_IMAGE_LOCATION_ADDR)) {
				puts("OK");
				//icap_reload_fpga(PROD_FPGA_IMAGE_LOCATION_ADDR);
			} else {
				puts("NOK");
			}				
		}
		else if(!strncmp(buf, "!RUNSFPGA", 8)) {
			if(is_valid_fpga_image(SAFE_FPGA_IMAGE_LOCATION_ADDR)) {
				puts("OK");
				//icap_reload_fpga(SAFE_FPGA_IMAGE_LOCATION_ADDR);
			} else {
				puts("NOK");
			}
		}
		else if(!strncmp(buf, "!READ", 5)) {
			uint32_t addr = atol(&buf[5]);
			spi_flash_read(addr, 16, ihx);
			for(i=0; i < 16; i++) {
				puthex8(ihx[i]);
			}
			putstr("\n");
		}

		else if(!ihex_parse(buf, &ihex_record)) { //last, try to see if the input was a valid IHEX line
			switch (ihex_record.type) {
				case 0:
					spi_flash_program(ihex_record.addr + slot_offset + extended_addr, ihex_record.length, ihex_record.data);
					puts("OK");
					break;
				case 1:
					//here we would expect a CRC checking or something else to take place. for now we do nothing.
					//well, we set the extended segment addr back to 0
					extended_addr = 0;
					puts("DONE");
					break;
				case 4:
					//set the upper 16 bits of the address
					extended_addr = ((ihex_record.data[0] << 8) + ihex_record.data[1]) << 16;
					puts("OK");
					break;
				default:
					puts("NOK");
			}
		}
		else puts("NOK");
	} //while(1)
}

void delay(uint32_t t) {
	while(t-- != 0) asm("NOP");
}

int main(int argc, char *argv[]) {
	uint8_t buf[32];
	int i = 0;

  hal_disable_ints();	// In case we got here via jmp 0x0

//	delay(10000000);

	//before anything else you might want to blinkenlights just to indicate code startup to the user.

  hal_uart_init();
	spif_init();
//	i2c_init(); //for EEPROM
	puts("USRP2+ UART firmware loader");

//	puts("Debug: loading production image, 10 bytes.");

//	spi_flash_read(PROD_FW_IMAGE_LOCATION_ADDR, 10, buf);
//	for(i=0; i < 10; i++) {
//		puthex8(buf[i]);
//	}

	uart_flash_loader();

 	//shouldn't get here. should reboot.
	puts("shit happened.\n");

	return 0;
}
