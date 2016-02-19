/* USRP E310 FPGA driver
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

#include "eeprom.h"
#include "fpga.h"
#include "spi.h"
#include "mcu_settings.h"
#include "utils.h"
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>

typedef struct fpga_tx_mem_map0 {
	uint16_t battery_voltage;
	uint8_t battery_status;
	uint8_t charger_status;
	uint8_t unused[2];
	uint8_t version;
	uint8_t type;
} fpga_tx_mem_map0_t;

typedef struct fpga_tx_mem_map1 {
	uint8_t status;
	uint16_t voltage;
	uint16_t temp;
	uint16_t charge;
	uint8_t type;
} fpga_tx_mem_map1_t;

typedef struct fpga_tx_mem_map2 {
	uint8_t unused[4];
	uint8_t settings;
	uint16_t charge_last_full;
	uint8_t type;
} fpga_tx_mem_map2_t;

typedef struct fpga_rx_mem_map0 {
	uint8_t unused[2];
	uint16_t value;
	uint8_t reg;
	uint8_t os_status;
} fpga_rx_mem_map0_t;

typedef struct fpga_rx_mem_map1 {
	uint8_t unused[3];
	uint16_t value;
	uint8_t reg;
} fpga_rx_mem_map1_t;

typedef struct fpga_rx_mem_map {
	uint8_t valid;
	union {
		fpga_rx_mem_map0_t map0;
		fpga_rx_mem_map1_t map1;
	};
	uint8_t type;
} fpga_rx_mem_map_t;

static bool shutdown = false;
static bool write_charge = false;
static bool write_settings = false;

static volatile fpga_tx_mem_map0_t fpga_tx0;
static volatile fpga_tx_mem_map1_t fpga_tx1;
static volatile fpga_tx_mem_map2_t fpga_tx2;
static volatile fpga_rx_mem_map_t fpga_rx;

/* battery status */
static const uint8_t BATTERY_TEMP_ALERT_MASK = BIT(7) | BIT(6);
static const uint8_t BATTERY_TEMP_ALERT_SHIFT = 6;
static const uint8_t BATTERY_ONLINE_MASK = BIT(5);
static const uint8_t BATTERY_ONLINE_SHIFT = 5;
static const uint8_t BATTERY_HEALTH_MASK = BIT(4) | BIT(3) | BIT(2);
static const uint8_t BATTERY_HEALTH_SHIFT = 2;
static const uint8_t BATTERY_STATUS_MASK = BIT(1) | BIT(0);
static const uint8_t BATTERY_STATUS_SHIFT = 0;

/* charger_status */
static const uint8_t CHARGER_HEALTH_MASK = BIT(5) | BIT(4);
static const uint8_t CHARGER_HEALTH_SHIFT = 4;
static const uint8_t CHARGER_ONLINE_MASK = BIT(3);
static const uint8_t CHARGER_ONLINE_SHIFT = 3;
/* BIT(2) is unused */
static const uint8_t CHARGER_CHARGE_TYPE_MASK = BIT(1) | BIT(0);
static const uint8_t CHARGER_CHARGE_TYPE_SHIFT = 0;


void fpga_set_battery_voltage(uint16_t voltage)
{
	fpga_tx0.battery_voltage = voltage;
}

void fpga_set_battery_temp_alert(uint8_t alert)
{
	uint8_t status = fpga_tx0.battery_status;

	status &= ~BATTERY_TEMP_ALERT_MASK;
	status |= alert << BATTERY_TEMP_ALERT_SHIFT;

	fpga_tx0.battery_status = status;
}

void fpga_set_battery_online(bool online)
{
	uint8_t status = fpga_tx0.battery_status;

	status &= ~BATTERY_ONLINE_MASK;
	status |= (online ? 1 : 0) << BATTERY_ONLINE_SHIFT;

	fpga_tx0.battery_status = status;
}

void fpga_set_battery_health(uint8_t health)
{
	uint8_t status = fpga_tx0.battery_status;

	status &= ~BATTERY_HEALTH_MASK;
	status |= health << BATTERY_HEALTH_SHIFT;

	fpga_tx0.battery_status = status;
}

void fpga_set_battery_status(uint8_t st)
{
	uint8_t status = fpga_tx0.battery_status;

	status &= ~BATTERY_STATUS_MASK;
	status |= st << BATTERY_STATUS_SHIFT;

	fpga_tx0.battery_status = status;
}

void fpga_set_charger_health(uint8_t health)
{
	uint8_t status = fpga_tx0.charger_status;

	status &= ~CHARGER_HEALTH_MASK;
	status |= health << CHARGER_HEALTH_SHIFT;

	fpga_tx0.charger_status = status;
}

void fpga_set_charger_online(bool online)
{
	uint8_t status = fpga_tx0.charger_status;

	status &= ~CHARGER_ONLINE_MASK;
	status |= (online ? 1 : 0) << CHARGER_ONLINE_SHIFT;

	fpga_tx0.charger_status = status;
}

void fpga_set_charger_charge_type(uint8_t type)
{
	uint8_t status = fpga_tx0.charger_status;

	status &= ~CHARGER_CHARGE_TYPE_MASK;
	status |= type << CHARGER_CHARGE_TYPE_SHIFT;

	fpga_tx0.charger_status = status;
}

void fpga_set_gauge_charge(uint16_t charge)
{
	fpga_tx1.charge = charge;
}

uint16_t fpga_get_gauge_charge(void)
{
	return fpga_tx1.charge;
}

bool fpga_get_write_charge(void)
{
	bool ret = false;

	if (write_charge) {
		ret = write_charge;
		write_charge = false;
	}

	return ret;
}

uint8_t fpga_get_settings(void)
{
	return fpga_tx2.settings;
}

bool fpga_get_write_settings(void)
{
	bool ret = false;

	if (write_settings) {
		ret = write_settings;
		write_settings = false;
	}

	return ret;
}

void fpga_set_gauge_charge_last_full(uint16_t charge)
{
	fpga_tx2.charge_last_full = charge;
}

void fpga_set_gauge_temp(uint16_t temp)
{
	fpga_tx1.temp = temp;
}

void fpga_set_gauge_voltage(uint16_t volt)
{
	fpga_tx1.voltage = volt;
}

void fpga_set_gauge_status(uint8_t status)
{
	fpga_tx1.status = status;
}

bool fpga_get_shutdown(void)
{
	return shutdown;
}

void fpga_init(void)
{
	memset((void *) &fpga_tx0, 0, sizeof(fpga_tx0));
	memset((void *) &fpga_tx1, 0, sizeof(fpga_tx1));
	memset((void *) &fpga_tx2, 0, sizeof(fpga_tx2));
	fpga_tx0.type = 0;
	fpga_tx0.version = VERSION_MAJ << 4 | VERSION_MIN;

	fpga_tx1.type = 1;
	fpga_tx2.type = 2;

	/* get autoboot value from eeprom, keep TX reg on */
	fpga_tx2.settings = BIT(1);
	fpga_tx2.settings |= eeprom_get_autoboot() ? BIT(0) : 0x0;

	memset((void *) &fpga_rx, 0, sizeof(fpga_rx));

	shutdown = false;
}

void fpga_handle_write(uint8_t reg, uint16_t value)
{
	if (reg == 0x10) {
		fpga_tx1.charge = value;
		write_charge = true;
	} else if (reg == 0x14) {
		fpga_tx1.status = value;
	} else if (reg == 0x1c) {
		fpga_tx2.settings = (uint8_t) value;
		write_settings = true;
	}
}

void fpga_sync(void)
{
	fpga_rx_mem_map_t rx;

	spi_transact_buf((uint8_t *) &fpga_tx0, (uint8_t *) &rx, 8);
	if (rx.valid) {
		if (rx.type == 0 && rx.map0.os_status == 0x7a)
			shutdown = true;
		else if (rx.type == 1)
			fpga_handle_write(rx.map1.reg, rx.map1.value);
	}
	spi_transact_buf((uint8_t *) &fpga_tx1, (uint8_t *) &rx, 8);
	if (rx.valid) {
		if (rx.type == 0 && rx.map0.os_status == 0x7a)
			shutdown = true;
		else if (rx.type == 1)
			fpga_handle_write(rx.map1.reg, rx.map1.value);
	}
	spi_transact_buf((uint8_t *) &fpga_tx2, (uint8_t *) &rx, 8);
	if (rx.valid) {
		if (rx.type == 0 && rx.map0.os_status == 0x7a)
			shutdown = true;
		else if (rx.type == 1)
			fpga_handle_write(rx.map1.reg, rx.map1.value);
	}
}
