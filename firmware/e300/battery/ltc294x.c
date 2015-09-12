/* USRP E310 Firmware Linear Technology LTC294X driver
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

#include <stdbool.h>
#include <string.h>
#include <util/atomic.h>
#include <avr/interrupt.h>

#include "i2c_twi.h"
#include "io.h"
#include "ltc294x.h"
#include "utils.h"
#include "pmu.h"

static const uint8_t LTC294X_I2C_ADDR = 0x64;

#define ltc294x_read(reg, val) \
	(i2c_twi_read(LTC294X_I2C_ADDR, reg, val))

#define ltc294x_read16(reg, val) \
	(i2c_twi_read16(LTC294X_I2C_ADDR, reg, val))

#define ltc294x_write(reg, val) \
	(i2c_twi_write(LTC294X_I2C_ADDR, reg, val))

#define ltc294x_write16(reg, val) \
	(i2c_twi_write16(LTC294X_I2C_ADDR, reg, val))

static const uint8_t LTC294X_REG_STATUS	=	0x00;
static const uint8_t LTC294X_REG_CONTROL	=	0x01;
static const uint8_t LTC294X_REG_CHARGE_MSB	=	0x2;
static const uint8_t LTC294X_REG_CHARGE_LSB	=	0x3;
static const uint8_t LTC294X_REG_HIGH_TRESH_MSB	=	0x4;
static const uint8_t LTC294X_REG_HIGH_THRESH_LSB	=	0x5;
static const uint8_t LTC294X_REG_LOW_THRESH_MSB	=	0x6;
static const uint8_t LTC294X_REG_LOW_THRESH_LSB	=	0x7;
static const uint8_t LTC294X_REG_VOLTAGE_MSB	=	0x8;
static const uint8_t LTC294X_REG_VOLTAGE_LSB	=	0x9;
static const uint8_t LTC294X_REG_VOLTAGE_THRESH_HI	=	0xa;
static const uint8_t LTC294X_REG_VOLTAGE_THRESH_LO	=	0xb;
static const uint8_t LTC294X_REG_TEMP_MSB	=	0xc;
static const uint8_t LTC294X_REG_TEMP_LSB	=	0xd;
static const uint8_t LTC294X_REG_TEMP_THRESH_HI = 0xe;
static const uint8_t LTC294X_REG_TEMP_THRESH_LO = 0xf;

/* status register */
static const uint8_t LTC294X_CHIP_ID_MASK = BIT(7);
static const uint8_t LTC294X_CHIP_ID_SHIFT = 7;
static const uint8_t LTC294X_ACR_OVF_MASK = BIT(5);
static const uint8_t LTC294X_ACR_OVF_SHIFT = 5;
static const uint8_t LTC294X_TEMP_ALERT_MASK = BIT(4);
static const uint8_t LTC294X_TEMP_ALERT_SHIFT = 4;
static const uint8_t LTC294X_CH_ALERT_HIGH_MASK = BIT(3);
static const uint8_t LTC294X_CH_ALERT_HIGH_SHIFT = 3;
static const uint8_t LTC294X_CH_ALERT_LOW_MASK = BIT(2);
static const uint8_t LTC294X_CH_ALERT_LOW_SHIFT = 2;
static const uint8_t LTC294X_VOLT_ALERT_MASK = BIT(1);
static const uint8_t LTC294X_CH_VOLT_ALERT_SHIFT = 1;
static const uint8_t LTC294X_CH_UVOLT_MASK = BIT(0);
static const uint8_t LTC294X_CH_UVOLT_SHIFT = 0;

/* control register */
static const uint8_t LTC294X_ADC_MODE_MASK = BIT(7) | BIT(6);
static const uint8_t LTC294X_ADC_MODE_SHIFT = 6;
static const uint8_t LTC294X_PRESCALER_MASK = BIT(5) | BIT(4) | BIT(3);
static const uint8_t LTC294X_PRESCALER_SHIFT = 3;
static const uint8_t LTC294X_ALCC_CFG_MASK = BIT(2) | BIT(1);
static const uint8_t LTC294X_ALCC_CFG_SHIFT = 1;
static const uint8_t LTC294X_SHUTDOWN_MASK = BIT(0);
static const uint8_t LTC294X_SHUTDOWN_SHIFT = 0;

/* acrh register */

/* acrl register */

struct ltc294x_gauge {
	pmu_gauge_t pmu_gauge;
	volatile uint8_t status;
	uint16_t temp_thresh_high;
	uint16_t temp_thresh_low;
	uint16_t charge_lo_thesh;
	bool first_time;
};

static struct ltc294x_gauge gauge;

static io_pin_t FG_ALn_CC = IO_PA(0);

static uint16_t ltc294x_get_charge(void)
{
	uint16_t val;

	(void) ltc294x_read16(LTC294X_REG_CHARGE_MSB, &val);

	return val;
}

static void ltc294x_set_charge(uint16_t val)
{
	uint8_t ctrl_val;

	/* datasheet says to shutdown the analog part
	 * when writing the ACR */
	(void) ltc294x_read(LTC294X_REG_CONTROL, &ctrl_val);

	ctrl_val |= LTC294X_SHUTDOWN_MASK;

	(void) ltc294x_write(LTC294X_REG_CONTROL, ctrl_val);

	/* write the value ... */
	(void) ltc294x_write16(LTC294X_REG_CHARGE_MSB, val);

	ctrl_val &= ~LTC294X_SHUTDOWN_MASK;

	/* turn it on again ...*/
	(void) ltc294x_write(LTC294X_REG_CONTROL, ctrl_val);
}

static uint16_t ltc294x_get_temp(void)
{
	uint16_t val;

	(void) ltc294x_read16(LTC294X_REG_TEMP_MSB, &val);

	return val;
}

static uint16_t ltc294x_get_voltage(void)
{
	uint16_t val;

	(void) ltc294x_read16(LTC294X_REG_VOLTAGE_MSB, &val);

	return val;
}

static int8_t ltc294x_set_charge_thresh(bool high, uint16_t val)
{
	int8_t ret;

	ret = ltc294x_write16(
		high ? LTC294X_REG_HIGH_TRESH_MSB : LTC294X_REG_LOW_THRESH_MSB, val);
	if (ret)
		return ret;
	return 0;
}

static inline void ltc294x_pmu_set_charge_hi_thesh(uint16_t val)
{
	ltc294x_set_charge_thresh(true, val);
}

static inline void ltc294x_pmu_set_charge_lo_thesh(uint16_t val)
{
	ltc294x_set_charge_thresh(false, val);
}

uint8_t ltc294x_check_events(void)
{
	uint8_t status;
	uint8_t flags = 0;
	uint16_t value;

	ltc294x_read(LTC294X_REG_STATUS, &status);


	if (status && (status != gauge.status)) {

		if (status & LTC294X_CH_ALERT_HIGH_MASK)
			flags |= PMU_GAUGE_CHARGE_HI;
		else if (status & LTC294X_CH_ALERT_LOW_MASK)
			flags |= PMU_GAUGE_CHARGE_LO;

		if (status & LTC294X_VOLT_ALERT_MASK) {
			/*flags |= PMU_GAUGE_VOLT_HI;
			flags |= PMU_GAUGE_VOLT_LO; */
			/* TODO: Figure out which one */
		}

		if (status & LTC294X_ACR_OVF_MASK) {
			value = ltc294x_get_charge();
			if (value <= gauge.charge_lo_thesh)
				flags |= PMU_GAUGE_CHARGE_LO;
		}

		if (status & LTC294X_TEMP_ALERT_MASK) {
			value = ltc294x_get_temp();
			if (value > gauge.temp_thresh_high)
				flags |= PMU_GAUGE_TEMP_HI;
			else if (value <= gauge.temp_thresh_low)
				flags |= PMU_GAUGE_TEMP_LO;
		}
		gauge.status = status;
	}

	return flags;
}

static const pmu_gauge_ops_t ltc294x_pmu_gauge_ops = {
	.check_events = ltc294x_check_events,
	.get_temperature = ltc294x_get_temp,
	.get_charge = ltc294x_get_charge,
	.set_charge = ltc294x_set_charge,
	.set_low_threshold = ltc294x_pmu_set_charge_lo_thesh,
	.get_voltage = ltc294x_get_voltage,
};

int8_t ltc294x_init(ltc294x_model_t model)
{
	uint8_t val;
	int8_t ret;

	/* make input, set pullup */
	io_input_pin(FG_ALn_CC);
	io_set_pin(FG_ALn_CC);

	gauge.pmu_gauge.ops = &ltc294x_pmu_gauge_ops;

	ret = ltc294x_read(LTC294X_REG_STATUS, &val);
	if (ret)
		goto fail_i2c_read;
	val &= LTC294X_CHIP_ID_MASK;
	val >>= LTC294X_CHIP_ID_SHIFT;

	if (val != model)
		goto fail_id;

	/* set ACR to 0, this allows for calibrating by
	 * completely discharging the battery once */
	ltc294x_set_charge(0x0010);

	/* set low threshold to 10% assuming full is 0xeae4... */
	ltc294x_write16(LTC294X_REG_LOW_THRESH_MSB, 0x1794);
	gauge.charge_lo_thesh = 0x1794;

	/* set high threshold for temperature to 85 C */
	ltc294x_write16(LTC294X_REG_TEMP_THRESH_HI, 0x98);
	gauge.temp_thresh_high = 0x9800;

	/* set low threshold for temperature to -2 C */
	ltc294x_write16(LTC294X_REG_TEMP_THRESH_LO, 0x74);
	gauge.temp_thresh_low = 0x7400;

	ret = ltc294x_read(LTC294X_REG_CONTROL, &val);
	if (ret)
		goto fail_i2c_read;

	/* enable automatic temp conversion */
	val &= ~LTC294X_ADC_MODE_MASK;
	val |= 0x3 << LTC294X_ADC_MODE_SHIFT;

	/* set prescaler to 16 */
	val &= ~LTC294X_PRESCALER_MASK;
	val |= 0x4 << LTC294X_PRESCALER_SHIFT;

	/* disable IRQ mode */
	val &= ~LTC294X_ALCC_CFG_MASK;
	val |= 0x0 << LTC294X_ALCC_CFG_SHIFT;

	ret = ltc294x_write(LTC294X_REG_CONTROL, val);

	pmu_register_gauge(&gauge.pmu_gauge);

	return 0;

fail_id:
fail_i2c_read:
	return ret;
}
