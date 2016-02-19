/* USRP E310 Firmware PMU
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

#include "adc.h"
#include "bq2419x.h"
#include "eeprom.h"
#include "fpga.h"
#include "mcu_settings.h"
#include "io.h"
#include "led.h"
#include "ltc3675.h"
#include "ltc294x.h"
#include "tps54478.h"
#include "timer.h"
#include "utils.h"

#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <util/atomic.h>

void pmu_power_on(void);
void pmu_power_down(void);

/* if we sense less than 2000 mV we assume battery is not there */
static const uint16_t PMU_BAT_MIN_VOLTAGE = 2000;

/* wait 10 ms, such random, so magic, wow */
static const uint8_t PMU_FPGA_RESET_DELAY = 10;

/* more magic wait constants */
static const uint8_t PMU_USB_CLK_WAIT = 200;
static const uint8_t PMU_FTDI_WAIT = 100;

static io_pin_t VBAT = IO_PC(0);
static io_pin_t POWER_LED = IO_PC(7);
static io_pin_t CHARGE = IO_PD(1);
static io_pin_t USB_RESETn = IO_PA(2);
static io_pin_t FTDI_RESETn = IO_PB(6);
static io_pin_t FTDI_CBUS3 = IO_PB(7);
static io_pin_t USB_CLK_EN = IO_PA(1);
static io_pin_t AVR_RESET = IO_PC(6);
static io_pin_t AVR_IRQ = IO_PD(5);
static io_pin_t PS_POR = IO_PD(6);
static io_pin_t PS_SRST = IO_PD(7);
static io_pin_t OVERTEMP = IO_PC(2);
static io_pin_t PANICn = IO_PC(1);

static uint16_t last_full_charge;
static uint16_t charge_on_last_unplug;
static bool battery_present_last;

static bool panic_last;

static const uint8_t PMU_BLINK_ERROR_DELAY_MS = 250;
static const uint8_t PMU_BLINK_ERROR_TICKS_PER_BLINK = 10;

typedef enum pmu_state {
	OFF,
	BOOT,
	SHUTDOWN,
	ON
} pmu_state_t;

static pmu_state_t state;
static volatile bool pmu_fpga_event;

typedef enum pmu_error {
	PMU_ERROR_NONE = 0x00,
	PMU_ERROR_LOW_VOLTAGE = 0x01,
	PMU_ERROR_REG_LOW_VOLTAGE = 0x02,
	PMU_ERROR_FPGA_POWER = 0x03,
	PMU_ERROR_DRAM_POWER = 0x04,
	PMU_ERROR_1_8V = 0x05,
	PMU_ERROR_3_3V = 0x06,
	PMU_ERROR_TX_POWER = 0x07,
	PMU_ERROR_CHARGER_TEMP = 0x08,
	PMU_ERROR_CHARGER_ERROR = 0x09,
	PMU_ERROR_BATTERY_LOW = 0x0a,
	PMU_ERROR_GAUGE_TEMP = 0x0b,
	PMU_ERROR_GLOBAL_TEMP = 0x0c,
} pmu_error_t;

static volatile pmu_error_t pmu_error;

/* this cannot be static const because
 * 'All the expressions in an initializer for an object
 *  that has static storage duration shall be constant expressions
 *  or string literals.' [section 6.7.8/4, C standard */
#ifdef DDR3L
#define DRAM_VOLTAGE 1350
#else
#define DRAM_VOLTAGE 0
#endif /* DDR3L */

static ltc3675_pmu_regulator_t PS_VDRAM = {
	.pmu_reg = {
		.ops = &ltc3675_ops,
		.powered = false,
		.voltage = DRAM_VOLTAGE /* DRAM_VOLTAGE */,
		.error_code = PMU_ERROR_DRAM_POWER,
	},
	.ltc3675_reg = LTC3675_REG_1,
};

static ltc3675_pmu_regulator_t PS_PERIPHERALS_1_8 = {
	.pmu_reg = {
		.ops = &ltc3675_ops,
		.powered = false,
		.voltage = 0 /*1800 hardware default? */,
		.error_code = PMU_ERROR_1_8V,
	},
	.ltc3675_reg = LTC3675_REG_3,
};

static ltc3675_pmu_regulator_t PS_PERIPHERALS_3_3 = {
	.pmu_reg = {
		.ops = &ltc3675_ops,
		.powered = false,
		.voltage = 0 /*3300 hardware default */,
		.error_code = PMU_ERROR_3_3V,
	},
	.ltc3675_reg = LTC3675_REG_6,
};

static ltc3675_pmu_regulator_t PS_TX = {
	.pmu_reg = {
		.ops = &ltc3675_ops,
		.powered = false,
		.voltage = 0 /*5000 hardware default? */,
		.error_code = PMU_ERROR_TX_POWER,
	},
	.ltc3675_reg = LTC3675_REG_5,
};

static tps54478_pmu_regulator_t PS_FPGA = {
	.pmu_reg = {
		.ops = &tps54478_ops,
		.powered = false,
		.voltage = 1000,
		.error_code = PMU_ERROR_FPGA_POWER,
	},
};

static pmu_regulator_t *boot_order[] = {
	&PS_FPGA.pmu_reg,
	&PS_VDRAM.pmu_reg,
	&PS_PERIPHERALS_1_8.pmu_reg,
	&PS_TX.pmu_reg,
	&PS_PERIPHERALS_3_3.pmu_reg,
};

static pmu_button_t *button;
void pmu_register_button(pmu_button_t *pmu_button)
{
	button = pmu_button;
}

static pmu_charger_t *charger;
void pmu_register_charger(pmu_charger_t *pmu_charger)
{
	charger = pmu_charger;
}

static pmu_gauge_t *gauge;
void pmu_register_gauge(pmu_gauge_t *pmu_gauge)
{
	gauge = pmu_gauge;
}

/**
 * \brief Reads the battery voltage from ADC0
 *
 * Vout = (375k / (274k + 357k)) * Vbat
 * Vbat = (Vout * (274k + 357k)) / 357k
 *
 * ADC = (Vin * 1024) / Vref
 * Vin = (ADC * Vref) / 1024
 * Vref = 3.3V
 * Vbat(mV) = 100 * (((ADC * 3.3) / 1024) * (274k + 357k)) / 357k
 * Vbat(mV) ~= ADC * 5.7
 */
static uint16_t pmu_battery_voltage(void)
{
	uint16_t tmp;

	tmp = adc_single_shot();
	tmp *= 5.6961f;
	return (uint16_t) tmp;
}

static inline bool pmu_battery_present(void)
{
	return (pmu_battery_voltage() > PMU_BAT_MIN_VOLTAGE);
}

static void pmu_reset_fpga(bool delay)
{
	io_clear_pin(PS_POR);
	io_clear_pin(PS_SRST);

	if (delay)
		_delay_ms(PMU_FPGA_RESET_DELAY);

	io_set_pin(PS_POR);
	io_set_pin(PS_SRST);
}

int8_t pmu_init(void)
{
	int8_t ret;
	bool battery_present;

	state = OFF;

	/* make panic button an input */
	io_input_pin(PANICn);
	panic_last = io_test_pin(PANICn);

	/* make the LED outputs */
	io_output_pin(CHARGE);
	io_output_pin(POWER_LED);

	/* initialize the ADC, so we can sense the battery */
	adc_init();

	/* initialize TPS54478 for core power */
	tps54478_init(true);

	/* wiggle USB and FTDI pins */
	io_input_pin(USB_RESETn);
	io_output_pin(FTDI_RESETn);
	io_output_pin(USB_CLK_EN);
	io_input_pin(FTDI_CBUS3);

	/* make OVERTEMP input pin */
	io_input_pin(OVERTEMP);

	/* initialize the charger */
	ret = bq2419x_init();
	if (ret)
		goto fail_bq2419x;

	/* wait a sec */
	_delay_ms(1000);

	/* wdt setup */
	cli();
	WDTCSR |= BIT(WDCE) | BIT(WDE);
	WDTCSR = BIT(WDIE);
	sei();

	/* see if we got a battery */
	battery_present = pmu_battery_present();
	battery_present_last = battery_present;

	if (battery_present) {
		last_full_charge = eeprom_get_last_full();
		ret = ltc294x_init(LTC294X_MODEL_2942);
	}
	if (ret)
		return ret;

	ret = ltc3675_init();
	if (ret)
		goto fail_ltc3675;


	/* need to hold them low until power is stable */
	io_output_pin(PS_POR);
	io_output_pin(PS_SRST);
	io_clear_pin(PS_POR);
	io_clear_pin(PS_SRST);

	/* TODO: Not sure if needed */
	io_input_pin(AVR_RESET);

	/* TODO: This will probably need to change */
	io_input_pin(AVR_IRQ);
	io_set_pin(AVR_IRQ); // enable pull-up ?

	/* configure and enable interrupts */
	interrupt_init();

	/* initialize the timers */
	timer0_init();
	timer1_init();

	state = OFF;

	return 0;

fail_ltc3675:
fail_bq2419x:
	return -1;
}

#define is_off (OFF == state)
#define is_on (ON == state)
#define is_booting (BOOT == state)

static inline int8_t pmu_set_regulator(pmu_regulator_t *reg, bool on)
{
	return reg->ops->set_regulator(reg, on);
}

void pmu_power_on(void)
{
	uint8_t i;
	int8_t ret;
	pmu_regulator_t *reg;

	/* if somehow this gets called twice, bail early on */
	if (is_booting)
		return;
	else if (is_on)
		return;
	else
		state = BOOT;

	/* reset the fpga */
	pmu_reset_fpga(true);
	fpga_init();

	for (i = 0; i < ARRAY_SIZE(boot_order); i++) {
		reg = boot_order[i];
		/* if regulator set a on/off function, call it */
		if (reg->ops->set_regulator) {
			ret = pmu_set_regulator(reg, true);
			if (ret) {
				pmu_error = reg->error_code;
				goto fail_regulators;
			}
		}

		/* if regulator set a set_voltage function, call it */
		if (reg->ops->set_voltage && reg->voltage) {
			ret = reg->ops->set_voltage(reg, reg->voltage);
			if (ret) {
				pmu_error = reg->error_code;
				goto fail_regulators;
			}
		}

		/* if we got here, this means all is well */
		reg->powered = true;
	}

	/* enable the usb clock */
	io_set_pin(USB_CLK_EN);
	_delay_ms(PMU_USB_CLK_WAIT);
	io_set_pin(FTDI_RESETn);
	_delay_ms(PMU_FTDI_WAIT);

	/* power for the fpga should be up now, let it run */
	pmu_reset_fpga(false);

	state = ON;

	return;

fail_regulators:
	/* TODO: Turn of stuff again in reverse order */
	return;
}

static inline enum pmu_status pmu_battery_get_status(pmu_charger_t *pmu_charger)
{
	return pmu_charger->ops->get_battery_status
	? pmu_charger->ops->get_battery_status(pmu_charger) : 0;
}

void pmu_power_down(void)
{
	int8_t i;
	int8_t ret;
	pmu_regulator_t *reg;

	state = SHUTDOWN;

	/* keep zynq in reset,
	 * TODO: do we need to also clear PS_POR? */
	io_clear_pin(PS_SRST);

	/* turn off usb clock */
	io_clear_pin(USB_CLK_EN);

	for (i = ARRAY_SIZE(boot_order) - 1; i >= 0; i--) {
		reg = boot_order[i];
		if (reg->ops->set_regulator) {
			ret = pmu_set_regulator(reg, false);
			if (ret)
				goto fail_regulators;
		}

		/* if we got here, this means regulator is off */
		reg->powered = false;
	}

	state = OFF;

	_delay_ms(1000);

	return;

fail_regulators:
	/* for now set solid red */
	pmu_error = reg->error_code;
}

static inline int8_t pmu_charger_check_events(pmu_charger_t *ch)
{
	return ch->ops->check_events
		? ch->ops->check_events(ch) : 0;
}

static inline int8_t pmu_regulator_check_events(pmu_regulator_t *reg)
{
	return reg->ops->check_events
		? reg->ops->check_events(reg) : 0;
}

static inline uint8_t pmu_button_check_events(pmu_button_t *pmu_button)
{
	return pmu_button->ops->check_events
		? pmu_button->ops->check_events(pmu_button) : 0;
}

static inline uint8_t pmu_charger_get_charge_type(pmu_charger_t *pmu_charger)
{
	return pmu_charger->ops->get_charge_type
		? pmu_charger->ops->get_charge_type(pmu_charger) : 0;
}

static inline uint8_t pmu_charger_get_health(pmu_charger_t *pmu_charger)
{
	return pmu_charger->ops->get_charger_health
	? pmu_charger->ops->get_charger_health(pmu_charger) : 0;
}

static inline uint8_t pmu_battery_get_health(pmu_charger_t *pmu_charger)
{
	return charger->ops->get_battery_health(pmu_charger);
}

static inline uint8_t pmu_battery_get_temp_alert(pmu_charger_t *pmu_charger)
{
	return pmu_charger->ops->get_temp_alert
	? pmu_charger->ops->get_temp_alert(pmu_charger) : 0;
}

static inline bool pmu_charger_get_online(pmu_charger_t *pmu_charger)
{
	return pmu_charger->ops->get_charger_online
	? pmu_charger->ops->get_charger_online(pmu_charger) : 0;
}

static inline bool pmu_battery_get_online(pmu_charger_t *pmu_charger)
{
	return pmu_charger->ops->get_battery_online
	? pmu_charger->ops->get_battery_online(pmu_charger) : 0;
}

static inline uint8_t pmu_gauge_check_events(void)
{
	return gauge->ops->check_events
	? gauge->ops->check_events() : 0;
}

static inline uint16_t pmu_gauge_get_temperature(void)
{
	return gauge->ops->get_temperature
	? gauge->ops->get_temperature() : 0;
}

static inline uint16_t pmu_gauge_get_charge(void)
{
	return gauge->ops->get_charge();
}

static inline void pmu_gauge_set_charge(uint16_t val)
{
	gauge->ops->set_charge(val);
}

static inline uint16_t pmu_gauge_get_voltage(void)
{
	return gauge->ops->get_voltage();
}

static inline void pmu_gauge_set_low_threshold(uint16_t val)
{
	if (gauge->ops->set_low_threshold)
		gauge->ops->set_low_threshold(val);
}

static inline bool pmu_is_charging(void)
{
	if (charger)
		return PMU_STATUS_CHARGING == pmu_battery_get_status(charger);

	return false;
}

static inline bool pmu_is_full(void)
{
	if (charger)
		return PMU_STATUS_FULL == pmu_battery_get_status(charger);

	return false;
}

void pmu_handle_events(void)
{
	uint8_t flags;
	uint16_t val;
	bool battery_present = pmu_battery_present();
	bool is_charging = false;
	bool is_full = false;
	bool overtemp = io_test_pin(OVERTEMP);
	bool panic = io_test_pin(PANICn);

	/* check if someone plugged the battery late,
	 * if so init gauge */
	if (battery_present && !battery_present_last) {
		ltc294x_init(LTC294X_MODEL_2942);
		pmu_gauge_set_charge(charge_on_last_unplug);
	} else if (!battery_present && battery_present_last) {
		gauge = NULL;
		charge_on_last_unplug = pmu_gauge_get_charge();
	}
	battery_present_last = battery_present;

	if (panic != panic_last)
		pmu_power_down();
	panic_last = panic;

	if (overtemp) {
		fpga_set_gauge_status(BIT(6));
		pmu_error = PMU_ERROR_GLOBAL_TEMP;
	}

	if (battery_present) {
		is_charging = pmu_is_charging();
		is_full = pmu_is_full();
	}

	/* resolve errors if we can */
	if (pmu_error != PMU_ERROR_NONE) {
		switch (pmu_error) {
		case PMU_ERROR_BATTERY_LOW:
			if (is_off || is_charging)
				pmu_error = PMU_ERROR_NONE;
			break;
		case PMU_ERROR_CHARGER_TEMP:
			if (!is_charging)
				pmu_error = PMU_ERROR_NONE;
			break;
		case PMU_ERROR_GLOBAL_TEMP:
			if (!overtemp)
				pmu_error = PMU_ERROR_NONE;
			break;
		default:
			break;
		}
	}

	(void) pmu_regulator_check_events(&PS_FPGA.pmu_reg);

	(void) pmu_regulator_check_events(&PS_VDRAM.pmu_reg);

	flags = pmu_button_check_events(button);
	if (is_off && (flags & PMU_BUTTON_EVENT_MASK_WAKEUP))
		pmu_power_on();

	else if (is_on && (flags & PMU_BUTTON_EVENT_MASK_POWERDOWN))
		pmu_power_down();

	/* if no battery present, no point ... */
	if (battery_present) {
		flags = pmu_charger_check_events(charger);

		if (flags != PMU_CHARGER_EVENT_NONE) {
			if ((flags & PMU_CHARGER_EVENT_FAULT_CHANGE)
				|| (flags & PMU_CHARGER_EVENT_STATUS_CHANGE)) {
				uint8_t health = pmu_battery_get_health(charger);
				switch (health) {
				case PMU_HEALTH_OVERHEAT:
					pmu_error = PMU_ERROR_CHARGER_TEMP;
					break;
				default:
					break;
				}
			}

			if ((flags & PMU_CHARGER_EVENT_CHARGE_DONE)) {
				last_full_charge = pmu_gauge_get_charge();
				pmu_gauge_set_low_threshold(last_full_charge / 10);
				eeprom_set_last_full_charge(last_full_charge);
			}
		}

		flags = pmu_gauge_check_events();
		if (flags != PMU_GAUGE_EVENT_NONE) {
			if (flags & PMU_GAUGE_CHARGE_LO) {
				if (!is_charging) {
					fpga_set_gauge_status(BIT(7));
					pmu_error = PMU_ERROR_BATTERY_LOW;
				}
			}

			if (flags & PMU_GAUGE_TEMP_HI) {
				fpga_set_gauge_status(BIT(6));
				pmu_error = PMU_ERROR_GAUGE_TEMP;
			}

			if (flags & PMU_GAUGE_TEMP_LO) {
				fpga_set_gauge_status(BIT(6));
				pmu_error = PMU_ERROR_GAUGE_TEMP;
			}
		}
	}

	/* blink error codes ... */
	switch (pmu_error) {
	case PMU_ERROR_NONE:
		if (is_off) {
			if (is_charging)
				led_set_blink(LED_BLINK_GREEN_SLOW);
			else
				led_set_solid(LED_OFF);
		} else if (is_on) {
			if (is_charging)
				led_set_blink(LED_BLINK_GREEN_FAST);
			else if (is_full || !battery_present)
				led_set_solid(LED_GREEN);
			else if (battery_present)
				led_set_solid(LED_ORANGE);
			else
				led_set_solid(LED_GREEN);
		}
		break;
	case PMU_ERROR_BATTERY_LOW:
		if (!is_charging && is_on)
			led_set_blink(LED_BLINK_ORANGE);
		break;
	default:
		led_set_blink_seq(pmu_error, LED_BLINK_RED_FAST);
		break;
	};

	fpga_set_charger_health(pmu_charger_get_health(charger));
	fpga_set_charger_online(pmu_charger_get_online(charger));
	if (battery_present) {
		fpga_set_charger_charge_type(pmu_charger_get_charge_type(charger));
		fpga_set_battery_voltage(pmu_battery_voltage());
		fpga_set_battery_temp_alert(pmu_battery_get_temp_alert(charger));
		fpga_set_battery_status(pmu_battery_get_status(charger));
		fpga_set_battery_health(pmu_battery_get_health(charger));
		fpga_set_battery_online(pmu_battery_get_online(charger));
		fpga_set_gauge_charge(pmu_gauge_get_charge());
		fpga_set_gauge_charge_last_full(last_full_charge);
		fpga_set_gauge_temp(pmu_gauge_get_temperature());
		fpga_set_gauge_voltage(pmu_gauge_get_voltage());
	}
	if (state != OFF) {
		fpga_sync();
		if (fpga_get_write_charge()) {
			val = fpga_get_gauge_charge();
			pmu_gauge_set_charge(val);
			if (pmu_error == PMU_ERROR_BATTERY_LOW)
				pmu_error = PMU_ERROR_NONE;
		}

		if (fpga_get_shutdown())
			pmu_power_down();

		if (fpga_get_write_settings()) {
			eeprom_set_autoboot(fpga_get_settings() & BIT(0));
			pmu_set_regulator(&PS_TX.pmu_reg, !!(fpga_get_settings() & BIT(1)));
		}
	}
}
