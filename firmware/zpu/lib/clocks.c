//
// Copyright 2010-2011 Ettus Research LLC
//
/*
 * Copyright 2008 Free Software Foundation, Inc.
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

#include <clocks.h>
#include <stdbool.h>
#include "memory_map.h"
#include "ad9510.h"
#include "spi.h"

/*!
 * \brief Lock Detect -- Return True if our PLL is locked
 */
bool clocks_lock_detect();

/*!
 * \brief Enable or disable fpga clock.  Disabling would wedge and require a power cycle.
 */
void clocks_enable_fpga_clk(bool enable, int divisor);

void 
clocks_init(void)
{
  // Set up basic clocking functions in AD9510
  ad9510_write_reg(0x45, 0x01);

  //enable the 100MHz clock output to the FPGA for 50MHz CPU clock
  clocks_enable_fpga_clk(true, 1);

  //! Cannot SPI wait since SPI is on DSP clock
  //! because DSP clock goes away until DCM reset.
  //! However, spi is quick, the cpu is slow, its already ready...
  //spi_wait();

  //wait for the clock to stabilize
  while(!clocks_lock_detect());

  //issue a reset to the DCM so it locks up to the new freq
  output_regs->clk_ctrl |= CLK_RESET;
}

bool 
clocks_lock_detect()
{
    return (pic_regs->pending & PIC_CLKSTATUS);
}

int inline
clocks_gen_div(int divisor)
{
  int L,H;
  L = (divisor>>1)-1;
  H = divisor-L-2;
  return (L<<4)|H;
}

#define CLOCK_OUT_EN 0x08
#define CLOCK_OUT_DIS_CMOS 0x01
#define CLOCK_OUT_DIS_PECL 0x02
#define CLOCK_DIV_DIS 0x80
#define CLOCK_DIV_EN 0x00

#define CLOCK_MODE_PECL 1
#define CLOCK_MODE_LVDS 2
#define CLOCK_MODE_CMOS 3

//CHANGED: set to PECL for default behavior
void 
clocks_enable_XXX_clk(bool enable, int divisor, int reg_en, int reg_div, int mode)
{
  int enable_word, div_word, div_en_word;

  switch(mode) {
  case CLOCK_MODE_LVDS :
    enable_word = enable ? 0x02 : 0x03;
    break;
  case CLOCK_MODE_CMOS :
    enable_word = enable ? 0x08 : 0x09;
    break;
  case CLOCK_MODE_PECL :
	default:
    enable_word = enable ? 0x08 : 0x0A;
    break;
  }
  if(enable && (divisor>1)) {
    div_word = clocks_gen_div(divisor);
    div_en_word = CLOCK_DIV_EN;
  }
  else {
    div_word = 0;
    div_en_word = CLOCK_DIV_DIS;
  }

  ad9510_write_reg(reg_en,enable_word); // Output en/dis
  ad9510_write_reg(reg_div,div_word); // Set divisor
  ad9510_write_reg(reg_div+1,div_en_word); // Enable or Bypass Divider
  ad9510_write_reg(0x5A, 0x01);  // Update Regs
}

// Clock 1
void
clocks_enable_fpga_clk(bool enable, int divisor)
{
  clocks_enable_XXX_clk(enable,divisor,0x3D,0x4A,CLOCK_MODE_PECL);
}
