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

/**
 * \file pmu.h
 * \brief Power Management Unit (PMU) functionality
 */
#ifndef PMU_H
#define PMU_H

#include <stdbool.h>
#include "interrupt.h"
#include "utils.h"

/**
 * \brief Initialize the Power Management Unit
 * \return 0 on success, negative error code on error
 */
int8_t pmu_init(void);

typedef struct pmu_regulator pmu_regulator_t;

typedef struct pmu_regulator_ops {
	int8_t (*set_voltage)(pmu_regulator_t *, uint16_t);
	int8_t (*set_regulator)(pmu_regulator_t *, bool);
	int8_t (*check_events)(pmu_regulator_t *);
} pmu_regulator_ops_t;

struct pmu_regulator {
	const pmu_regulator_ops_t *ops;
	const uint16_t voltage;
	bool powered;
	uint8_t error_code;
};

/**
 * \brief Event loop that handles all the various events
 */
void pmu_handle_events(void);

extern irqreturn_t pmu_fpga_irq_handler(void);
extern irqreturn_t pmu_led_timer_comp_a_irq_handler(void);
extern irqreturn_t pmu_led_timer_comp_b_irq_handler(void);
extern irqreturn_t pmu_wdt_handler(void);

/**
 * \brief Turn on the regulators and powerup ARM and FPGA
 */
void pmu_power_on(void);

enum pmu_health {
	PMU_HEALTH_GOOD,
	PMU_HEALTH_UNSPEC_FAIL,
	PMU_HEALTH_OVERVOLTAGE,
	PMU_HEALTH_OVERHEAT,
	PMU_HEALTH_COLD,
	PMU_HEALTH_SAFETY_TIMER_EXPIRE,
	PMU_HEALTH_UNKNOWN
};

enum pmu_charge_type {
	PMU_CHARGE_TYPE_NONE,
	PMU_CHARGE_TYPE_TRICKLE,
	PMU_CHARGE_TYPE_FAST,
};

enum pmu_status {
	PMU_STATUS_NOT_CHARGING,
	PMU_STATUS_CHARGING,
	PMU_STATUS_FULL,
	PMU_STATUS_DISCHARGING
};

typedef struct pmu_charger pmu_charger_t;

enum pmu_charger_event_mask {
	PMU_CHARGER_EVENT_NONE = 0x00,
	PMU_CHARGER_EVENT_STATUS_CHANGE = BIT(0),
	PMU_CHARGER_EVENT_FAULT_CHANGE = BIT(1),
	PMU_CHARGER_EVENT_CHARGE_DONE = BIT(2)
};

typedef struct pmu_charger_ops {
	int8_t (*set_charger_voltage)(pmu_charger_t *, uint16_t);
	int8_t (*set_charger_current)(pmu_charger_t *, uint16_t);

	uint8_t (*get_temp_alert)(pmu_charger_t *);
	int8_t (*set_temp_alert)(pmu_charger_t *, uint8_t);

	enum pmu_charge_type (*get_charge_type)(pmu_charger_t *);
	int8_t (*set_charge_type)(pmu_charger_t *, enum pmu_charge_type);
	enum pmu_health (*get_charger_health)(pmu_charger_t *);
	bool (*get_charger_online)(pmu_charger_t *);

	enum pmu_health (*get_battery_health)(pmu_charger_t *);
	enum pmu_status (*get_battery_status)(pmu_charger_t *);
	bool (*get_battery_online)(pmu_charger_t *);

	uint8_t (*check_events)(pmu_charger_t *);
} pmu_charger_ops_t;

struct pmu_charger {
	const pmu_charger_ops_t *ops;
};

/**
 * \brief Set the PMU's charger to the given one
 * \param[in] pmu_charger Pointer to the implementation's pmu_charger
 */
void pmu_register_charger(pmu_charger_t *pmu_charger);

typedef struct pmu_button pmu_button_t;

enum pmu_button_event_mask {
	PMU_BUTTON_EVENT_MASK_PRESS	=	0x01,
	PMU_BUTTON_EVENT_MASK_RELEASE	=	0x02,
	PMU_BUTTON_EVENT_MASK_POWERDOWN	=	0x04,
	PMU_BUTTON_EVENT_MASK_WAKEUP	=	0x08,
};

typedef struct pmu_button_ops {
	uint8_t (*check_events)(pmu_button_t *);
} pmu_button_ops_t;

struct pmu_button {
	const pmu_button_ops_t *ops;
};

/**
 * \brief Set the PMU's button to the given one
 * \param[in] pmu_button Pointer to the implementation's pmu_button
 */
void pmu_register_button(pmu_button_t *pmu_button);

typedef struct pmu_gauge pmu_gauge_t;

enum pmu_gauge_event_mask {
	PMU_GAUGE_EVENT_NONE = 0x00,
	PMU_GAUGE_CHARGE_HI = 0x01,
	PMU_GAUGE_CHARGE_LO = 0x02,
	PMU_GAUGE_TEMP_HI = 0x04,
	PMU_GAUGE_TEMP_LO = 0x08,
	PMU_GAUGE_VOLT_LO = 0x10,
	PMU_GAUGE_VOLT_HI = 0x20,
};

typedef struct pmu_gauge_ops {
	uint8_t (*check_events)(void);
	uint16_t (*get_charge)(void);
	void (*set_charge)(uint16_t val);
	void (*set_low_threshold)(uint16_t val);
	uint16_t (*get_temperature)(void);
	uint16_t (*get_voltage)(void);
} pmu_gauge_ops_t;

struct pmu_gauge {
	const pmu_gauge_ops_t *ops;
};
/**
 * \brief Set the PMU's power gauge to the given one
 * \param[in] pmu_gauge Pointer to the implementation's pmu_gauge
 */
void pmu_register_gauge(pmu_gauge_t *pmu_gauge);

#endif /* PMU_H */
