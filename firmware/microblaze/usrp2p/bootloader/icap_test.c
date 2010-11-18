/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

#include <memory_map.h>
#include <hal_io.h>
#include <xilinx_s3_icap.h>
#include <nonstdio.h>

void delay(uint32_t t) {
	while(t-- != 0) asm("NOP");
}


int main(int argc, char *argv[]) {
	pic_init();
	hal_uart_init();
	puts("\nStarting delay...\n");

	output_regs->leds = 0xFF;
	delay(4000000);
	output_regs->leds = 0x00;
	delay(4000000);

	puts("Rebooting FPGA to 0x00000000\n");
	icap_reload_fpga((uint32_t)0x00000000);

	return 0;
}
