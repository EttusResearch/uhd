/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

#include <memory_map.h>
#include <hal_io.h>
#include <hal_uart.h>
#include <xilinx_s3_icap.h>
#include <nonstdio.h>
#include <spi_flash.h>
#include <spi.h>
#include <clocks.h>
#include <string.h>

//just a test to write to SPI flash and retrieve the same values.
//uses the MOBFLEET SPI flash library

void delay(uint32_t t) {
	while(t-- != 0) asm("NOP");
}

int main(int argc, char *argv[]) {
	uint16_t i, t;
	uint8_t buf[260];
	const uint8_t testdata[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C};

  hal_disable_ints();	// In case we got here via jmp 0x0
//	spi_init();
  hal_uart_init();
//	clocks_init(); //set up AD9510, enable FPGA clock @ 1x divisor

	puts("SPI Flash test\n");
	puts("Initializing SPI\n");

	spif_init();
	delay(800000);
	puts("Erasing sector 1\n");
	spi_flash_erase(0x00010000, 256);
	delay(800000);
	puts("Reading back data\n");
	spi_flash_read(0x00010000, 256, buf);
	delay(800000);

	t=1;
	for(i=4; i<250; i++) {
		if(buf[i] != 0xFF) t=0;
	}

	if(!t) puts("Data was not initialized to 0xFF. Unsuccessful erase or read\n");
	else puts("Data initialized to 0xFF, erase confirmed\n");

	puts("Writing test buffer\n");
	spi_flash_program(0x00010000, 16, testdata);
	//memset(buf, 0, 256);

	delay(800000);
	puts("Wrote data, reading back\n");

	spi_flash_read(0x00010000, 16, buf);

	if(memcmp(testdata, buf, 16)) puts("Data is not the same between read and write. Unsuccessful write or read\n");
	else puts("Successful write! Flash write correct\n");

	return 0;
}
