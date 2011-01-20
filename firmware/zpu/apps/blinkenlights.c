/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

#include "memory_map.h"
#include <nonstdio.h>

int main(int argc, char *argv[]) {

	uint32_t c = 0;
	uint8_t i = 0;

	output_regs->led_src = 0;

	while(1) {
		//delay(5000000);
		for(c=0;c<50000;c++) asm("NOP");
		output_regs->leds = (i++ % 2) ? 0xFF : 0x00; //blink everything on that register
	}

	return 0;
}

//void delay(uint32_t t) {
//	while(t-- != 0) asm("NOP");
//}
