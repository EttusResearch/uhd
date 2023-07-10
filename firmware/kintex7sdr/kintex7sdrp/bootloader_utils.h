/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

#include <stdint.h>

//we're working in bytes and byte addresses so we can run the same code with Flash chips of different sector sizes.
//it's really 1463736, but rounded up to 1.5MB
//Really-really it's 10 MB! =)
#define FPGA_IMAGE_SIZE_BYTES 0xA00000	//10MB
//16K
#define FW_IMAGE_SIZE_BYTES 0x3fff

#define SAFE_FPGA_IMAGE_LOCATION_ADDR 0x00000000
//#define SAFE_FW_IMAGE_LOCATION_ADDR 0x003F0000		//4MB
//#define PROD_FPGA_IMAGE_LOCATION_ADDR 0x00180000	//1.5MB
//#define PROD_FW_IMAGE_LOCATION_ADDR 0x00300000		//3MB

#define SAFE_FW_IMAGE_LOCATION_ADDR 0x00f00000		//15MB
#define PROD_FPGA_IMAGE_LOCATION_ADDR 0x00500000	//5MB
#define PROD_FW_IMAGE_LOCATION_ADDR 0x00f80000		//15.5MB

int is_valid_fpga_image(uint32_t addr);
int is_valid_fw_image(uint32_t addr);
void start_program(void);
void do_the_bootload_thing(void);
