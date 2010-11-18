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
//#include <spi_flash_private.h>
#include <clocks.h>
#include <ihex.h>
#include <bootloader_utils.h>
#include <string.h>
#include <hal_uart.h>
#include <spi.h>

//so this is just an evolving file used to set up and test different bits of hardware (EEPROM, clock chip, A/D, D/A, PHY)
void delay(uint32_t t) {
	while(t-- != 0) asm("NOP");
}

int main(int argc, char *argv[]) {

	hal_disable_ints();
  hal_uart_init();
	spi_init();

	puts("Hardware testbed. Init clocks...");

	clocks_init();

	//now, hopefully, we should be running at 100MHz instead of 50MHz, meaning our UART is twice as fast and we're talking at 230400.

	while(1) {
		delay(500000);
		puts("Eat at Joe's.");
	}





	return 0;
}
