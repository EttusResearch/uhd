/* USRP E310 Firmware
 * Copyright (C) 2014 Ettus Research
 * This file is part of the USRP E310 Firmware
 * The USRP E310 Firmware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * The USRP E310 Firmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with the USRP E310 Firmware. If not, see <http://www.gnu.org/licenses/>.
 */

#include <avr/io.h>
#include <errno.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "i2c_twi.h"
#include "spi.h"
#include "pmu.h"
#include "interrupt.h"
#include "io.h"
#include "fpga.h"
#include "eeprom.h"
#include "utils.h"

/* setup the fuses for compilation with avr-gcc
 * - use the internal 8 MHz oscillator
 * - slowly rising power (startup time)
 * - save the eeprom between flashes, leave SPI enabled for flashing
 */
FUSES = {
	.low	=	(FUSE_CKSEL0 & FUSE_SUT0),
	.high	=	(FUSE_EESAVE & FUSE_SPIEN),
	.extended = EFUSE_DEFAULT,
};


int main(void)
{
	/* if a reset was caused by watchdog, clear flag, enable wd change bit, disable */
	if (MCUSR & BIT(WDRF)) {
		MCUSR &= ~BIT(WDRF);
		WDTCSR |= BIT(WDCE) | BIT(WDE);
		WDTCSR = 0x00;
	}

	i2c_twi_init(I2C_SPEED_100K);
	spi_init(SPI_TYPE_MASTER, SPI_MSB_FIRST, SPI_MODE_0, SPI_SPEED_2M);

	pmu_init();

	if (eeprom_get_autoboot())
		pmu_power_on();

	sei();

	while (1) {
		pmu_handle_events();
	}

	return 0;
}
