/* USRP E310 Firmware Linear Technology LTC3765 driver
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

#include "fpga.h"
#include "i2c_twi.h"
#include "io.h"
#include "interrupt.h"
#include "ltc3675.h"
#include "mcu_settings.h"
#include "utils.h"
#include "timer.h"

#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>

static const uint8_t LTC3675_I2C_ADDR = 0x09;

#define ltc3675_read(reg, val) \
	(i2c_twi_read(LTC3675_I2C_ADDR, reg, val))

#define ltc3675_write(reg, val) \
	(i2c_twi_write(LTC3675_I2C_ADDR, reg, val))

/* registers */
static const uint8_t LTC3675_REG_NONE	= 0x00;
static const uint8_t LTC3675_REG_BUCK1	= 0x01;
static const uint8_t LTC3675_REG_BUCK2	= 0x02;
static const uint8_t LTC3675_REG_BUCK3	= 0x03;
static const uint8_t LTC3675_REG_BUCK4	= 0x04;
static const uint8_t LTC3675_REG_BOOST	= 0x05;
static const uint8_t LTC3675_REG_BUCK_BOOST	= 0x06;
static const uint8_t LTC3675_REG_LED_CONFIG	= 0x07;
static const uint8_t LTC3675_REG_LED_DAC	= 0x08;
static const uint8_t LTC3675_REG_UVOT	= 0x09;
static const uint8_t LTC3675_REG_RSTB	= 0x0a;
static const uint8_t LTC3675_REG_IRQB_MASK	= 0x0b;
static const uint8_t LTC3675_REG_RT_STATUS	= 0x0c;
static const uint8_t LTC3675_REG_LAT_STATUS	= 0x0d;
static const uint8_t LTC3675_REG_CLEAR_IRQ	= 0x0f;

static const uint8_t LTC3675_UNDER_VOLTAGE_MASK	=	BIT(7);
static const uint8_t LTC3675_UNDER_VOLTAGE_SHIFT	=	7;
static const uint8_t LTC3675_OVER_TEMPERATURE_MASK	=	BIT(6);
static const uint8_t LTC3675_OVER_TEMPERATURE_SHIFT	=	6;
static const uint8_t LTC3675_BUCK_BOOST_PGOOD_MASK	=	BIT(5);
static const uint8_t LTC3675_BUCK_BOOST_PGOOD_SHIFT	=	5;
static const uint8_t LTC3675_BOOST_PGOOD_MASK	=	BIT(4);
static const uint8_t LTC3675_BOOST_PGOOD_SHIFT	=	4;
static const uint8_t LTC3675_BUCK4_PGOOD_MASK		=	BIT(3);
static const uint8_t LTC3675_BUCK4_PGOOD_SHIFT		=	3;
static const uint8_t LTC3675_BUCK3_PGOOD_MASK		=	BIT(2);
static const uint8_t LTC3675_BUCK3_PGOOD_SHIFT		=	2;
static const uint8_t LTC3675_BUCK2_PGOOD_MASK		=	BIT(1);
static const uint8_t LTC3675_BUCK2_PGOOD_SHIFT		=	1;
static const uint8_t LTC3675_BUCK1_PGOOD_MASK		=	BIT(0);
static const uint8_t LTC3675_BUCK1_PGOOD_SHIFT		=	0;

static const uint8_t LTC3675_ENABLE_REGISTER_BIT = 0x80;

struct ltc3675_button {
	pmu_button_t pmu_button;

	volatile bool onswitch_press_event;
	volatile bool onswitch_release_event;
	volatile bool onswitch_last_state;

	volatile bool poweroff_event;

	volatile bool wakeup_event;
};

static struct ltc3675_button button;

/** arbitrary wait to give the external supply can settle */
static const uint8_t LTC3675_REG_ENABLE_DELAY = 10;

static io_pin_t PWR_IRQ = IO_PD(0);
static io_pin_t WAKEUP  = IO_PD(2);
static io_pin_t ONSWITCH_DB = IO_PD(3);
static io_pin_t PWR_RESET = IO_PD(4);

static void ltc3675_clear_interrupts(void)
{
	ltc3675_write(LTC3675_REG_CLEAR_IRQ, 0x00);
	ltc3675_write(LTC3675_REG_NONE, 0x00);
}

static int8_t ltc3675_set_regulator_helper(uint8_t reg, bool on)
{
	int8_t ret;
	uint8_t val;
	ret = ltc3675_read(reg, &val);
	if (ret)
		goto fail_i2c_read;
	if (on)
		val |= LTC3675_ENABLE_REGISTER_BIT;
	else
		val &= ~LTC3675_ENABLE_REGISTER_BIT;
	ret = ltc3675_write(reg, val);
	if (ret)
		goto fail_i2c_write;

	if (on)
		_delay_ms(LTC3675_REG_ENABLE_DELAY);

	return 0;

fail_i2c_write:
fail_i2c_read:
	return ret;
}

static inline int8_t ltc3675_get_realtime_status(uint8_t *val)
{
	int8_t ret;

	ret = ltc3675_read(LTC3675_REG_RT_STATUS, val);
	if (ret)
		return ret;
	return 0;
}

static bool ltc3675_get_power_good(uint8_t mask)
{
	uint8_t val;
	int8_t ret;

	ret = ltc3675_get_realtime_status(&val);
	if (ret)
		return false;

	return !!(mask & val);
}

static int8_t ltc3675_set_regulator(pmu_regulator_t *reg, bool on)
{
	int8_t ret;
	bool status;
	ltc3675_pmu_regulator_t *pmu;

	pmu = container_of(reg, ltc3675_pmu_regulator_t, pmu_reg);

	switch (pmu->ltc3675_reg) {
	case LTC3675_REG_1: /* master */
	case LTC3675_REG_2: /* slave */
		ret = ltc3675_set_regulator_helper(LTC3675_REG_BUCK1, on);
		if (ret)
			return ret;
		status = ltc3675_get_power_good(LTC3675_BUCK1_PGOOD_MASK);
		return (status == on) ? 0 : -1;
	case LTC3675_REG_3: /* master */
	case LTC3675_REG_4: /* slave */
		ret = ltc3675_set_regulator_helper(LTC3675_REG_BUCK3, on);
		if (ret)
			return ret;
		status = ltc3675_get_power_good(LTC3675_BUCK3_PGOOD_MASK);
		return (status == on) ? 0 : -1;
	case LTC3675_REG_5:
		ret = ltc3675_set_regulator_helper(LTC3675_REG_BOOST, on);
		if (ret)
			return ret;
		status = ltc3675_get_power_good(LTC3675_BOOST_PGOOD_MASK);
		return (status == on) ? 0 : -1;
	case LTC3675_REG_6: /* single */
		ret = ltc3675_set_regulator_helper(LTC3675_REG_BUCK_BOOST, on);
		if (ret)
			return ret;
		status = ltc3675_get_power_good(LTC3675_BUCK_BOOST_PGOOD_MASK);
		return (status == on) ? 0 : -1;
	default:
		return -1;
	}

	return 0;
}

static int8_t ltc3675_set_voltage(pmu_regulator_t *reg, uint16_t v)
{
	uint32_t r_fb, r;
	uint16_t vmax, r_dac;
	uint8_t addr, val;
	int8_t ret;
	ltc3675_pmu_regulator_t *pmu;

	pmu = container_of(reg, ltc3675_pmu_regulator_t, pmu_reg);

	switch (pmu->ltc3675_reg) {
	case LTC3675_REG_1: /* 1A Buck */
	case LTC3675_REG_2: /* 1A Buck */
		vmax = 1500;
		addr = LTC3675_REG_BUCK1;
		break;
	case LTC3675_REG_3: /* 500mA Buck */
	case LTC3675_REG_4: /* 500mA Buck */
		vmax = 1800;
		addr = LTC3675_REG_BUCK3;
		break;
	case LTC3675_REG_5: /* 1A Boost */
		vmax = 5000;
		addr = LTC3675_REG_BOOST;
		break;
	case LTC3675_REG_6: /* 1 A Buck-Boost */
		vmax = 3300;
		addr = LTC3675_REG_BUCK_BOOST;
		break;
	default:
		return -1; /* TODO: Should return useful error code */
	}

	if (v > vmax)
		return -1; /* TODO: Should return useful error code. */

	r_fb = ((uint32_t) vmax * 1000) / (uint32_t) 800; /* 800mV full-scale feedback voltage */
	r = ((uint32_t) v * 1000) / r_fb ;

	if (r < 450)
		return -1;

	r_dac = (16 * ((uint16_t) r - 450)) / (800 - 450);

	ret = ltc3675_read(addr, &val);
	if (ret)
		return ret;

	val = (val & 0xf0) | ((uint8_t) r_dac);
	ret = ltc3675_write(addr, val);
	if (ret)
		return ret;

	return 0;
}

static uint8_t ltc3675_button_check_events(pmu_button_t *pmu_button)
{
	uint8_t flags;
	struct ltc3675_button *ltc3675_button;
	flags = 0x00;
	ltc3675_button = container_of(
		pmu_button, struct ltc3675_button, pmu_button);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (ltc3675_button->onswitch_press_event) {
			ltc3675_button->onswitch_press_event = false;
			flags |= PMU_BUTTON_EVENT_MASK_PRESS;
		}
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (ltc3675_button->onswitch_release_event) {
			ltc3675_button->onswitch_release_event = false;
			flags |= PMU_BUTTON_EVENT_MASK_RELEASE;
		}
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (ltc3675_button->wakeup_event) {
			ltc3675_button->wakeup_event = false;
			flags |= PMU_BUTTON_EVENT_MASK_WAKEUP;
		}
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (ltc3675_button->poweroff_event) {
			ltc3675_button->poweroff_event = false;
			flags |= PMU_BUTTON_EVENT_MASK_POWERDOWN;
		}
	}
	return flags;
}

static const pmu_button_ops_t ltc3675_pmu_button_ops = {
	.check_events = ltc3675_button_check_events,
};


int8_t ltc3675_init(void)
{
	uint8_t id;
	int8_t ret;

	ret = ltc3675_read(LTC3675_REG_LED_CONFIG, &id);
	if (ret)
		return ret;

	button.pmu_button.ops = &ltc3675_pmu_button_ops;
	button.onswitch_last_state = io_test_pin(ONSWITCH_DB);

	/* setup the input pins with pull-up for open drain */
	io_input_pin(PWR_IRQ);
	io_set_pin(PWR_IRQ);
	io_input_pin(WAKEUP);
	io_set_pin(WAKEUP);
	io_input_pin(ONSWITCH_DB);
	io_set_pin(ONSWITCH_DB);
	io_input_pin(PWR_RESET);
	io_set_pin(PWR_RESET);

	/* clear the old interrupts */
	ltc3675_clear_interrupts();

	/* setup interrupt masks on chip to be notified of any faults */
	ret = ltc3675_write(LTC3675_REG_IRQB_MASK, 0xff);
	if (ret)
		goto fail_i2c_write_mask;

	/* program warning @ 3.4V */
	ret = ltc3675_write(LTC3675_REG_UVOT, 0x70);
	if (ret)
		goto fail_i2c_write_uvot;

	pmu_register_button(&button.pmu_button);

	return 0;

fail_i2c_write_uvot:
fail_i2c_write_mask:
	return ret;
}

int8_t ltc3675_check_reg_events(pmu_regulator_t *reg)
{
	return 0;
}

const pmu_regulator_ops_t ltc3675_ops = {
	.set_voltage = ltc3675_set_voltage,
	.set_regulator = ltc3675_set_regulator,
	.check_events	=	ltc3675_check_reg_events
};

/* PD(3) ONSWITCH_DB (PB_STAT), any change */
irqreturn_t ltc3675_button_change_irq_handler(void)
{
	bool pin_state;

	pin_state = io_test_pin(ONSWITCH_DB);

	/* the pushbutton is active low, therefore backwards logic */
	if (pin_state && !button.onswitch_last_state) {
		button.onswitch_release_event = true;
		timer1_stop();
	} else if (!pin_state && button.onswitch_last_state) {
		button.onswitch_press_event = true;
		timer1_start();
	}
	button.onswitch_last_state = pin_state;

	return IRQ_HANDLED;
}

irqreturn_t ltc3675_button_wakeup_irq_handler(void)
{
	button.wakeup_event = true;

	return IRQ_HANDLED;
}

irqreturn_t ltc3675_button_timer_irq_handler(void)
{
	/* if we got here, the timer overflowed,
	 * meaning the user pressed the button long enough */
	button.poweroff_event = true;

	return IRQ_HANDLED;
}
