/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

#include <stdint.h>

//we're working in bytes and byte addresses so we can run the same code with Flash chips of different sector sizes.
//it's really 1463736, but rounded up to 1.5MB
#define FPGA_IMAGE_SIZE_BYTES 1572864
//instead of 32K, we write 31K because we're using the top 1K for stack space!
#define FW_IMAGE_SIZE_BYTES 31744

#define SAFE_FPGA_IMAGE_LOCATION_ADDR 0x00000000
#define SAFE_FW_IMAGE_LOCATION_ADDR 0x003F0000
#define PROD_FPGA_IMAGE_LOCATION_ADDR 0x00180000
#define PROD_FW_IMAGE_LOCATION_ADDR 0x00300000

int is_valid_fpga_image(uint32_t addr);
int is_valid_fw_image(uint32_t addr);
void start_program(uint32_t addr);
