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
#include "ad9516.h"
#include "spi.h"

#include "mdelay.h"
#include "hal_io.h"
#include "hal_uart.h"

/*!
 * \brief Lock Detect -- Return True if our PLL is locked
 */
bool clocks_lock_detect();

void 
clocks_init(void)
{
  // Set up basic clocking functions in AD9510
  //ad9510_write_reg(0x45, 0x01);

  //output_regs->phy_ctrl |= 0x01;		//PHY_RESET
  //output_regs->dac_ctrl |= 0x01;		//DAC_RESET

  ad9516_clocks_enable_fpga(true, 1);

  ad9516_clocks_enable_phy(true, 4);

  //issue a reset to the DCM so it locks up to the new freq
  output_regs->clk_ctrl |= CLK_RESET;

  //output_regs->phy_ctrl &= ~0x01;
  //output_regs->dac_ctrl &= ~0x01;
}

bool 
clocks_lock_detect()
{
    return (pic_regs->pending & PIC_CLKSTATUS);
}



