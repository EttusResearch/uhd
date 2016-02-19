/* USRP E310 Firmware Texas Instruments BQ2419x driver
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

#include "bq2419x.h"
#include "io.h"
#include "i2c_twi.h"
#include "interrupt.h"
#include "pmu.h"
#include "mcu_settings.h"
#include "utils.h"

#include <stdbool.h>
#include <string.h>

#include <util/atomic.h>
#include <util/delay.h>

static const uint8_t BQ2419X_I2C_ADDR = 0x6b;

#define bq2419x_read(reg, val) \
	(i2c_twi_read(BQ2419X_I2C_ADDR, reg, val))

#define bq2419x_write(reg, val) \
	(i2c_twi_write(BQ2419X_I2C_ADDR, reg, val))

/* register addresses */
static const uint8_t BQ2419X_REG_INPUT_SRC_CTL		=	0x00;
static const uint8_t BQ2419X_REG_PWR_ON_CONFIG		=	0x01;
static const uint8_t BQ2419X_REG_CHARGE_CURRENT		=	0x02;
static const uint8_t BQ2419X_REG_PRE_TERM_CURRENT	=	0x03;
static const uint8_t BQ2419X_REG_CHARGE_VOLTAGE		=	0x04;
static const uint8_t BQ2419X_REG_TIMER_CONTROL		=	0x05;
static const uint8_t BQ2419X_REG_THERMAL_REG_CTRL	=	0x06;
static const uint8_t BQ2419X_REG_MISC_OPERATION		=	0x07;
static const uint8_t BQ2419X_REG_SYSTEM_STATUS		=	0x08;
static const uint8_t BQ2419X_REG_FAULT				=	0x09;
static const uint8_t BQ2419X_REG_VENDOR_PART_REV	=	0x0a;

/* input source control register (REG00) */
static const uint8_t BQ2419X_EN_HIZ_MASK = BIT(7);
static const uint8_t BQ2419X_EN_HIZ_SHIFT = 7;

/* power on configuration register (REG01) */
static const uint8_t BQ2419X_REGISTER_RESET_MASK	=	BIT(7);
static const uint8_t BQ2419X_REGISTER_RESET_SHIFT	=	7;
static const uint8_t BQ2419X_I2C_TIMER_RESET	=	BIT(6);
static const uint8_t BQ2419X_I2C_TIMER_SHIFT	=	BIT(6);

static const uint8_t BQ2419X_CHARGE_CFG_MASK	=	BIT(5) | BIT(4);
static const uint8_t BQ2419X_CHARGE_CFG_SHIFT	=	4;
static const uint8_t BQ2419X_SYS_MIN_MASK	=	BIT(3) | BIT(2) | BIT(1);
static const uint8_t BQ2419X_SYS_MIN_SHIFT	=	1;

/* charge current control register (REG02) */
static const uint8_t BQ2419X_ICHG_MASK	=	BIT(7) | BIT(6) \
    | BIT(5) | BIT(4) | BIT(3) | BIT(2);
static const uint8_t BQ2419X_ICHG_SHIFT	=	2;
/* reserved */
static const uint8_t BQ2419X_FORCE_20_PCT_MASK	=	BIT(0);
static const uint8_t BQ2419X_FORCE_20_PCT_SHIFT	=	0;

static const uint8_t BQ2419X_CHARGE_CFG_DISABLED	=	0x00;
static const uint8_t BQ2419X_CHARGE_CFG_CHARGE	=	0x01;
static const uint8_t BQ2419X_CHARGE_CFG_OTG	=	0x03;

/* pre charge / termination current control register (REG03) */

/* charge voltage control register (REG04) */

/* charge / termination register (REG05) */
static const uint8_t BQ2419X_EN_TERM_MASK = BIT(7);
static const uint8_t BQ2419X_EN_TERM_SHIFT = BIT(7);
static const uint8_t BQ2419X_TERM_STAT_MASK = BIT(6);
static const uint8_t BQ2419X_TERM_STAT_SHIFT = 6;
static const uint8_t BQ2419X_WDT_MASK	= BIT(5) | BIT(4);
static const uint8_t BQ2419X_WDT_SHIFT	= 4;
static const uint8_t BQ2419X_EN_TIMER_MASK	= BIT(3);
static const uint8_t BQ2419X_EN_TIMER_SHIFT	= 3;

/* ir compensation / thermal regulation control register (REG06) */
static const uint8_t BQ2419X_BAT_COMP_MASK = BIT(7) | BIT(6) | BIT(5);
static const uint8_t BQ2419X_BAT_COMP_SHIFT = 5;
static const uint8_t BQ2419X_TREG_MASK = BIT(1) | BIT(0);
static const uint8_t BQ2419X_TREG_SHIFT = 0;

/* misc operation register (REG07) */
static const uint8_t BQ2419X_DPDM_EN_MASK	=	BIT(7);
static const uint8_t BQ2419X_DPDM_EN_SHIFT	=	7;
static const uint8_t BQ2419X_TMR2X_EN_MASK	=	BIT(6);
static const uint8_t BQ2419X_TMR2X_EN_SHIFT	=	6;
static const uint8_t BQ2419X_BATFET_DISABLE_MASK	= BIT(5);
static const uint8_t BQ2419X_BATFET_DISABLE_SHIFT	= 5;

static const uint8_t BQ2419X_JEITA_VSET_MASK	= BIT(4);
static const uint8_t BQ2419X_JEITA_VSET_SHIFT	= BIT(4);
/* reserved bits */
/* reserved bits */
static const uint8_t BQ2419X_INT_MASK_MASK	= BIT(1) | BIT(0);
static const uint8_t BQ2419X_INT_MASK_SHIFT	= 0;

/* system status register (REG08) */
static const uint8_t BQ2419X_VBUS_STAT_MASK	=	BIT(7) | BIT(6);
static const uint8_t BQ2419X_VBUS_STAT_SHIFT	=	6;
static const uint8_t BQ2419X_CHG_STAT_MASK	=	BIT(5) | BIT(4);
static const uint8_t BQ2419X_CHG_STAT_SHIFT	=	4;
static const uint8_t BQ2419X_DPM_STAT_MASK	=	BIT(3);
static const uint8_t BQ2419X_DPM_STAT_SHIFT	=	3;
static const uint8_t BQ2419X_PG_STAT_MASK	=	BIT(2);
static const uint8_t BQ2419X_PG_STAT_SHIFT	=	2;
static const uint8_t BQ2419X_THERM_STAT_MASK	=	BIT(1);
static const uint8_t BQ2419X_THERM_STAT_SHIFT	=	1;
static const uint8_t BQ2419X_VSYS_STAT_MASK	=	BIT(0);
static const uint8_t BQ2419X_VSYS_STAT_SHIFT	=	0;

static const uint8_t BQ2419X_CHARGE_STATUS_NOT_CHARGING	= 0x00;
static const uint8_t BQ2419X_CHARGE_STATUS_PRE_CHARGE	= 0x01;
static const uint8_t BQ2419X_CHARGE_STATUS_FAST_CHARGE	= 0x02;
static const uint8_t BQ2419X_CHARGE_STATUS_DONE	= 0x03;

/* fault register (REG09) */
static const uint8_t BQ2419X_WATCHDOG_FAULT_MASK	=	BIT(7);
static const uint8_t BQ2419X_WATCHDOG_FAULT_SHIFT	=	7;
static const uint8_t BQ2419X_BOOST_FAULT_MASK	=	BIT(6);
static const uint8_t BQ2419X_BOOST_FAULT_SHIFT	=	6;
static const uint8_t BQ2419X_CHG_FAULT_MASK	=	BIT(5) | BIT(4);
static const uint8_t BQ2419X_CHG_FAULT_SHIFT	=	4;
static const uint8_t BQ2419X_BAT_FAULT_MASK	=	BIT(3);
static const uint8_t BQ2419X_BAT_FAULT_SHIFT	=	3;
static const uint8_t BQ2419X_NTC_FAULT_MASK	=	BIT(2) | BIT(1) | BIT(0);
static const uint8_t BQ2419X_NTC_FAULT_SHIFT = 0;

static io_pin_t CHG_IRQ	= IO_PB(1);

typedef struct bq2419x_pmu_charger
{
	pmu_charger_t pmu_charger;
	uint8_t fault;
	uint8_t status;
	bool first_time;

	bool battery_status_valid;
	bool battery_health_valid;
	bool charger_health_valid;

	volatile bool event;
} bq2419x_pmu_charger_t;

static bq2419x_pmu_charger_t charger;

static volatile bool bq2419x_event = false;

int8_t bq2419x_set_charger(bool on)
{
	uint8_t config;
	int8_t ret;

	ret = bq2419x_read(BQ2419X_REG_PWR_ON_CONFIG, &config);
	if (ret)
		return ret;

	config &= ~BQ2419X_CHARGE_CFG_MASK;
	if (on)
		config |= 1 << BQ2419X_CHARGE_CFG_SHIFT;

	ret = bq2419x_write(BQ2419X_REG_PWR_ON_CONFIG, config);
	if (ret)
		return ret;

	return 0;
}

static int8_t bq2419x_reset(void)
{
	uint8_t config;
	int8_t ret;
	uint8_t retry = 100;

	ret = bq2419x_read(BQ2419X_REG_PWR_ON_CONFIG, &config);
	if (ret)
		return ret;

	config |= BQ2419X_REGISTER_RESET_MASK;
	ret = bq2419x_write(BQ2419X_REG_PWR_ON_CONFIG, config);

	do {
		ret = bq2419x_read(BQ2419X_REG_PWR_ON_CONFIG, &config);
		if (!(config & BQ2419X_REGISTER_RESET_MASK))
			return 0;
		_delay_ms(10);
	} while (retry--);

	return ret;
}

static void bq2419x_set_host_mode(void)
{
	uint8_t timer_ctrl;

	/* to disable watchdog, we need to clear the WDT bits */
	bq2419x_read(BQ2419X_REG_TIMER_CONTROL, &timer_ctrl);
	timer_ctrl &= ~BQ2419X_WDT_MASK;
	bq2419x_write(BQ2419X_REG_TIMER_CONTROL, timer_ctrl);
}


static enum pmu_charge_type bq2419x_charger_get_charge_type(pmu_charger_t *pmu_charger)
{
	uint8_t val;

	(void) pmu_charger;

	bq2419x_read(BQ2419X_REG_PWR_ON_CONFIG, &val);

	val &= BQ2419X_CHARGE_CFG_MASK;
	val >>= BQ2419X_CHARGE_CFG_SHIFT;

	/* if check if charging is disabled */
	if (!val)
		return PMU_CHARGE_TYPE_NONE;

	bq2419x_read(BQ2419X_REG_CHARGE_CURRENT, &val);

	val &= BQ2419X_FORCE_20_PCT_MASK;
	val >>= BQ2419X_FORCE_20_PCT_SHIFT;

	if (val)
		return PMU_CHARGE_TYPE_TRICKLE;
	else
		return PMU_CHARGE_TYPE_FAST;
}

static enum pmu_health bq2419x_charger_get_health(pmu_charger_t *pmu_charger)
{
	uint8_t fault;
	int8_t ret;
	struct bq2419x_pmu_charger *bq2419x_charger;

	bq2419x_charger = container_of(
		pmu_charger, struct bq2419x_pmu_charger, pmu_charger);

	if (bq2419x_charger->charger_health_valid) {
		fault = bq2419x_charger->fault;
		bq2419x_charger->charger_health_valid = false;
	} else {
		ret = bq2419x_read(BQ2419X_REG_FAULT, &fault);
		if (ret)
			return PMU_HEALTH_UNKNOWN;
	}

	/* if BOOST_FAULT then report overvoltage */
	if (fault & BQ2419X_BOOST_FAULT_MASK) {
		return PMU_HEALTH_OVERVOLTAGE;
	} else {
		fault &= BQ2419X_CHG_FAULT_MASK;
		fault >>= BQ2419X_CHG_FAULT_SHIFT;
		switch (fault) {
		case 0x0:
			/* all is well */
			return PMU_HEALTH_GOOD;
		case 0x1:
			/* input fault, could be over- or under-voltage
			 * and we can't tell which, so we report unspec */
			return PMU_HEALTH_UNSPEC_FAIL;
		case 0x2:
			/* thermal shutdown */
			return PMU_HEALTH_OVERHEAT;
		case 0x3:
			/* the charge safety timer expired */
			return PMU_HEALTH_SAFETY_TIMER_EXPIRE;
		default:
			return PMU_HEALTH_UNKNOWN;
		}
	}
}

static enum pmu_status bq2419x_battery_get_status(pmu_charger_t *pmu_charger)
{
	uint8_t fault, ss_reg;
	int8_t ret;
	struct bq2419x_pmu_charger *bq2419x_charger;

	bq2419x_charger = container_of(
		pmu_charger, struct bq2419x_pmu_charger, pmu_charger);

	if (bq2419x_charger->battery_status_valid) {
		fault = bq2419x_charger->fault;
		ss_reg = bq2419x_charger->status;
		bq2419x_charger->battery_status_valid = false;
	} else {
		ret = bq2419x_read(BQ2419X_REG_FAULT, &fault);
		if (ret)
			return ret;

		ret = bq2419x_read(BQ2419X_REG_SYSTEM_STATUS, &ss_reg);
		if (ret)
			return ret;
	}

	fault &= BQ2419X_CHG_FAULT_MASK;
	fault >>= BQ2419X_CHG_FAULT_SHIFT;

	/* the battery is discharging if either
	 * - we don't have a good power source
	 * - we have a charge fault */
	if (!(ss_reg & BQ2419X_PG_STAT_MASK) || fault) {
		return PMU_STATUS_DISCHARGING;
	} else {
		ss_reg &= BQ2419X_CHG_STAT_MASK;
		ss_reg >>= BQ2419X_CHG_STAT_SHIFT;

		switch(ss_reg) {
		case 0x0: /* not charging */
			return PMU_STATUS_NOT_CHARGING;
		case 0x1: /* pre charging */
		case 0x2: /* fast charging */
			return PMU_STATUS_CHARGING;
		case 0x3: /* charge termination done */
			return PMU_STATUS_FULL;
		}
	}
	return PMU_STATUS_NOT_CHARGING;
}

static bool bq2419x_battery_get_online(pmu_charger_t *pmu_charger)
{
	uint8_t batfet_disable;
	int8_t ret;

	(void) pmu_charger;

	ret = bq2419x_read(BQ2419X_REG_MISC_OPERATION, &batfet_disable);
	if (ret)
		return false;

	batfet_disable &= BQ2419X_BATFET_DISABLE_MASK;
	batfet_disable >>= BQ2419X_BATFET_DISABLE_SHIFT;

	return !batfet_disable;
}

static enum pmu_health bq2419x_battery_get_health(pmu_charger_t *pmu_charger)
{
	uint8_t fault;
	int8_t ret;
	struct bq2419x_pmu_charger *bq2419x_charger;

	bq2419x_charger = container_of(
		pmu_charger, struct bq2419x_pmu_charger, pmu_charger);

	if (bq2419x_charger->battery_health_valid) {
		fault = bq2419x_charger->fault;
		bq2419x_charger->battery_health_valid = false;
	} else {
		ret = bq2419x_read(BQ2419X_REG_FAULT, &fault);
		if (ret)
			return ret;
	}

	if (fault & BQ2419X_BAT_FAULT_MASK)
		return PMU_HEALTH_OVERVOLTAGE;
	else {
		fault &= BQ2419X_NTC_FAULT_MASK;
		fault >>= BQ2419X_NTC_FAULT_SHIFT;
		switch (fault) {
		case 0x0:
			/* all is well */
			return PMU_HEALTH_GOOD;
		case 0x1:
		case 0x3:
		case 0x5:
			/* either TS1 cold, TS2 cold, or both cold */
			return PMU_HEALTH_COLD;
		case 0x2:
		case 0x4:
		case 0x6:
			/* either TS1 hot, TS2 hot, or both hot */
			return PMU_HEALTH_OVERHEAT;
		default:
			return PMU_HEALTH_UNKNOWN;
		}
	}
}

static bool bq2419x_charger_get_online(pmu_charger_t *pmu_charger)
{
	uint8_t val;
	int8_t ret;

	(void) pmu_charger;

	ret = bq2419x_read(BQ2419X_REG_SYSTEM_STATUS, &val);
	if (ret)
		return ret;

	/* check the power good bit */
	return !!(BQ2419X_PG_STAT_MASK & val);
}

static inline bool bq2419x_get_irq(void)
{
	/* the CHG_IRQ line is low active */
	return !io_test_pin(CHG_IRQ);
}

uint8_t bq2419x_pmu_charger_check_events(pmu_charger_t *pmu_charger)
{
	uint8_t flags;
	int8_t ret;
	uint8_t status;
	uint8_t fault;
	uint8_t isc;
	volatile bool event;
	bq2419x_pmu_charger_t *bq2419x_charger;

	bq2419x_charger = container_of(
		pmu_charger, struct bq2419x_pmu_charger, pmu_charger);

	event = false;
	flags = PMU_CHARGER_EVENT_NONE;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (bq2419x_charger->event) {
			bq2419x_charger->event = false;
			event = true;
		}
	}

	if (event) {

		ret = bq2419x_read(BQ2419X_REG_SYSTEM_STATUS, &status);
		if (ret)
			return ret;

		if (status != bq2419x_charger->status) {
			if ((bq2419x_charger->status & BQ2419X_PG_STAT_MASK) &&
					!(status & BQ2419X_PG_STAT_MASK)) {
				/* we're in host mode and need to turn off HIZ
				 * when PG_STAT goes 1->0, in order to have the
				 * battery supply the juice */
				ret = bq2419x_read(BQ2419X_REG_INPUT_SRC_CTL,
						&isc);
				if (ret)
					return 0;

				isc &= ~BQ2419X_EN_HIZ_MASK;
				ret = bq2419x_write(BQ2419X_REG_INPUT_SRC_CTL,
						isc);
				if (ret)
					return 0;
			}

			if ((bq2419x_charger->status & BQ2419X_CHG_STAT_MASK)
				!= (status & BQ2419X_CHG_STAT_MASK)) {
				if ((status & BQ2419X_CHG_STAT_MASK) >> BQ2419X_CHG_STAT_SHIFT == 0x3)
					flags |= PMU_CHARGER_EVENT_CHARGE_DONE;
			}
			bq2419x_charger->status = status;
			flags |= PMU_CHARGER_EVENT_STATUS_CHANGE;
		}

		ret = bq2419x_read(BQ2419X_REG_FAULT, &fault);
		if (ret)
			return ret;

		if (fault != bq2419x_charger->fault) {
			bq2419x_charger->fault = fault;
			bq2419x_charger->battery_status_valid = true;
			bq2419x_charger->battery_health_valid = true;
			bq2419x_charger->charger_health_valid = true;
			flags |= PMU_CHARGER_EVENT_FAULT_CHANGE;
		}

		if (!bq2419x_charger->first_time)
			bq2419x_charger->first_time = true;
	}
	return flags;
}

irqreturn_t bq2419x_irq_handler(void)
{
	/* we check if the device indicates an event
	 * if so we are the source of the IRQ,
	 * so set the flag to deal with it later.
	 * Otherwise we indicate to check the other devices,
	 * by returning IRQ_NONE
	 */
	if (bq2419x_get_irq()) {
		charger.event = true;
		return IRQ_HANDLED;
	}
	(void) charger.event;
	return IRQ_NONE;
}

static const pmu_charger_ops_t bq2419x_pmu_charger_ops = {
	.set_charger_voltage	=	NULL,
	.set_charger_current	=	NULL,

	.get_charge_type	=	bq2419x_charger_get_charge_type,
	.set_charge_type	=	NULL,
	.get_charger_health	=	bq2419x_charger_get_health,
	.get_charger_online	=	bq2419x_charger_get_online,

	.get_battery_health	=	bq2419x_battery_get_health,
	.get_battery_status	=	bq2419x_battery_get_status,
	.get_battery_online	=	bq2419x_battery_get_online,

	.check_events	=	bq2419x_pmu_charger_check_events,
};

int8_t bq2419x_init(void)
{
	uint8_t id, input_src_ctrl, ir_comp;
	int8_t ret;

	/* initialize the state struct */
	memset(&charger, 0, sizeof(charger));
	charger.pmu_charger.ops = &bq2419x_pmu_charger_ops;
	charger.first_time = true;

	/* check vendor register to verify we're looking at
	 * a TI BQ2419x chip */
	ret = bq2419x_read(BQ2419X_REG_VENDOR_PART_REV, &id);
	if (ret)
		goto fail_i2c_read;

	/* set charge IRQ pin as input */
	io_input_pin(CHG_IRQ);
	io_set_pin(CHG_IRQ);

	bq2419x_reset();
	bq2419x_set_host_mode();

	/* we leave the other registers at default values
	 * BQ2419X_REG_PWR_ON_CONFIG:
	 * - minimum system voltage limit (default) 101 3.5V
	 * BQ2419X_REG_CHARGE_VOLTAGE:
	 * - fast charge current limit (default) 011000 2048mA
	 * BQ2419X_REG_PRE_TERM_CURRENT:
	 * - pre-charge current limit (default) 0001 256mA
	 * - termination current limit (default) 0001 256mA
	 * BQ2419X_REG_CHARGE_VOLTAGE:
	 * - charge voltage limit (default) 101100 4.208V
	 */

	bq2419x_read(BQ2419X_REG_INPUT_SRC_CTL, &input_src_ctrl);

	/* set a 3A limit */
	input_src_ctrl |= 0x7;

	ret = bq2419x_write(BQ2419X_REG_INPUT_SRC_CTL, input_src_ctrl);
	if (ret)
		return ret;

	/* compensate for 20m r_sense */
	ret = bq2419x_read(BQ2419X_REG_THERMAL_REG_CTRL, &ir_comp);
	if (ret)
		return ret;
	ir_comp &= ~BQ2419X_BAT_COMP_MASK;
	ir_comp |= (0x02 << BQ2419X_BAT_COMP_SHIFT);

	/* set thermal regulation to 120 C */
	ir_comp &= ~BQ2419X_TREG_MASK;
	ir_comp |= (0x03 << BQ2419X_TREG_SHIFT);

	ret = bq2419x_write(BQ2419X_REG_THERMAL_REG_CTRL, ir_comp);
	if (ret)
		return ret;

	pmu_register_charger(&charger.pmu_charger);

	return 0;

fail_i2c_read:
	return 1;
}
