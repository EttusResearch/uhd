/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

//contains routines for loading programs from Flash. depends on Flash libraries.
#include <string.h>
#include <bootloader_utils.h>
#include <spi_flash.h>


int is_valid_fpga_image(uint32_t addr) {
	static const uint8_t fpgaheader[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xAA, 0x99}; //AA 99 is the standard Xilinx sync sequence, and it's always prefixed with 0xFF padding
	uint8_t buf[10];
	spi_flash_read(addr, 6, buf);
	return memcmp(buf, fpgaheader, 6) == 0;
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

