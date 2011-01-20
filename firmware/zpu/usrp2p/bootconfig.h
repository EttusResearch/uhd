/* -*- c -*- */
/*
 * Copyright 2009-2011 Ettus Research LLC
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef INCLUDED_BOOTCONFIG_H
#define INCLUDED_BOOTCONFIG_H

#include <stdbool.h>

typedef struct {
  unsigned char	fpga_image_number;
  unsigned char	firmware_image_number;
} bootconfig_t;

static inline bootconfig_t
make_bootconfig(unsigned char fpga_image_number, unsigned char firmware_image_number)
{
  bootconfig_t r;
  r.fpga_image_number = fpga_image_number;
  r.firmware_image_number = firmware_image_number;
  return r;
}

void bootconfig_init(void);	/* One time call to initialize */

/*!
 * \return default boot configuration
 */
bootconfig_t bootconfig_get_default(void);

/*!
 * \brief Set the default boot configuration.
 */
bool bootconfig_set_default(bootconfig_t bc);

/*!
 * \brief attempt to boot the given fpga and software image.
 *
 * If successful, this routine does not return.
 * If it fail for some reason, it returns.
 */
void bootconfig_boot(bootconfig_t bc);

#endif /* INCLUDED_BOOTCONFIG_H */
