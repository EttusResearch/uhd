/* -*- c -*- */
/*
 * Copyright 2009-2011 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This code is bootloader f/w for the slot 0 fpga image.  It's job is
 * to figure out which fpga image should be loaded, and then to load
 * that image from the SPI flash.  (FIXME handle retries, errors,
 * etc.)
 *
 * If the center button is down during boot, it loads firwmare
 * from 0:0 instead of its normal action.
 */

#include <stdlib.h>
#include <hal_io.h>
#include <nonstdio.h>
#include <mdelay.h>
#include <quadradio/flashdir.h>
#include <xilinx_v5_icap.h>
#include <bootconfig.h>
#include <bootconfig_private.h>
#include <spi_flash.h>
#include <string.h>
#include <bootloader_utils.h>
#include <hal_interrupts.h>

#define VERBOSE 1

#define	OUR_FPGA_IMAGE_NUMBER	0	// this code only runs in slot 0

void hal_uart_init(void);
void spif_init(void);
void i2c_init(void);
void bootconfig_init(void);

void pic_interrupt_handler() __attribute__ ((interrupt_handler));

void pic_interrupt_handler()
{
  // nop stub
}

static int
flash_addr_of_fpga_slot(unsigned int fpga_slot)
{
  const struct flashdir *fd = get_flashdir();
  return fd->slot[fpga_slot + fd->fpga_slot0].start << spi_flash_log2_sector_size();
}


/*
 * If the first 256 bytes of the image contain the string of bytes,
 * ff ff ff ff aa 99 55 66, we consider it a likely bitstream.
 */
static bool
looks_like_a_bitstream(unsigned int fpga_slot)
{
  unsigned char buf[256];
  static const unsigned char pattern[] = {
    0xff, 0xff, 0xff, 0xff, 0xaa, 0x99, 0x55, 0x66
  };

  // Read the first 256 bytes of the bitstream
  spi_flash_read(flash_addr_of_fpga_slot(fpga_slot), sizeof(buf), buf);

  for (int i = 0; i <= sizeof(buf) - sizeof(pattern); i++)
    if (memcmp(pattern, &buf[i], sizeof(pattern)) == 0)
      return true;

  return false;
}

static bool
plausible_bootconfig(bootconfig_t bc)
{
  // Are the fields in range?
  if (!validate_bootconfig(bc))
    return false;

  if (!looks_like_a_bitstream(map_fpga_image_number_to_fpga_slot(bc.fpga_image_number)))
    return false;

  return true;
}

// Attempt to boot the fpga image specified in next_boot
static void
initial_boot_attempt(eeprom_boot_info_t *ee)
{
  if (ee->next_boot.fpga_image_number == OUR_FPGA_IMAGE_NUMBER){
    load_firmware();
    return;
  }

  ee->nattempts = 1;
  _bc_write_eeprom_shadow();

  unsigned int target_slot =
    map_fpga_image_number_to_fpga_slot(ee->next_boot.fpga_image_number);
  int flash_addr = flash_addr_of_fpga_slot(target_slot);

  putstr("fpga_bootloader: chaining to ");
  puthex4(ee->next_boot.fpga_image_number);
  putchar(':');
  puthex4(ee->next_boot.firmware_image_number);
  newline();
  mdelay(100);

  while (1){
    icap_reload_fpga(flash_addr);
  }
}

int
main(int argc, char **argv)
{
  hal_disable_ints();	// In case we got here via jmp 0x0
  hal_uart_init();
  i2c_init();
  bootconfig_init();	// Must come after i2c_init.
  spif_init();		// Needed for get_flashdir.

  sr_leds->leds =  0xAAAA;

  putstr("\n\n>>> fpga_bootloader <<<\n");

  putstr("\nBOOTSTS ");
  int bootsts = icap_read_config_reg(rBOOTSTS);
  puthex32_nl(bootsts);
  putstr("STAT    ");
  int stat = icap_read_config_reg(rSTAT);
  puthex32_nl(stat);

  bool fallback =
    ((bootsts & (BOOTSTS_VALID_0 | BOOTSTS_FALLBACK_0))
     == (BOOTSTS_VALID_0 | BOOTSTS_FALLBACK_0));

  if (fallback){
    puts("FALLBACK_0 is set");
    // FIXME handle fallback condition.
  }

  const struct flashdir *fd = get_flashdir();
  if (fd == 0)
    abort();

  eeprom_boot_info_t *ee = _bc_get_eeprom_shadow();

  if (VERBOSE){
    putstr("nattempts: ");
    puthex8_nl(ee->nattempts);
  }

  mdelay(500);	// wait for low-pass on switches
  putstr("switches: "); puthex32_nl(readback->switches);

  bool center_btn_down = (readback->switches & BTN_CENTER) != 0;
  if (center_btn_down){
    putstr("Center button is down!\n");
    // Force boot of image 0:0
    ee->next_boot = make_bootconfig(0, 0);
  }

  // if next_boot is valid, try it
  if (plausible_bootconfig(ee->next_boot))
    initial_boot_attempt(ee);	// no return

  // if default_boot is valid, try it
  if (plausible_bootconfig(ee->default_boot)){
    ee->next_boot = ee->default_boot;
    initial_boot_attempt(ee);	// no return
  }

  // If we're here, we're in trouble.  Try all of them...
  for (int i = 0; i < 4; i++){
    bootconfig_t bc = make_bootconfig(i, 0);
    if (plausible_bootconfig(bc)){
      ee->next_boot = bc;
      initial_boot_attempt(ee);	// no return
    }
  }

  // FIXME, try to find something we can load
  puts("\n!!! Failed to find a valid FPGA bitstream!\n\n");

  return 0;
}
