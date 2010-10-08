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

int is_valid_fpga_image(uint32_t addr) {
	uint8_t imgbuf[64];
	spi_flash_read(addr, 64, imgbuf);
	//we're just looking for leading 0xFF padding, followed by the sync bytes 0xAA 0x99
	int i = 0;
	for(i; i<63; i++) {
		if(imgbuf[i] == 0xFF) continue;
		if(imgbuf[i] == 0xAA && imgbuf[i+1] == 0x99) return 1;
	}
	
	return 0;
}

int is_valid_fw_image(uint32_t addr) {
	static const uint8_t fwheader[] = {0xB0, 0x00, 0x00, 0x00, 0xB8, 0x08}; //just lookin for a jump to anywhere located at the reset vector
	uint8_t buf[12];
	spi_flash_read(addr, 6, buf);
	return memcmp(buf, fwheader, 6) == 0;
}

void start_program(uint32_t addr)
{
	memcpy(0x00000000, addr+0x00000000, 36); //copy the whole vector table, with the reset vector, into boot RAM
	typedef void (*fptr_t)(void);
	(*(fptr_t) 0x00000000)();	// most likely no return
}
