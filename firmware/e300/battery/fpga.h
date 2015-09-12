/* USRP E310 Firmware FPGA driver
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
 * \file fpga.h
 * \brief FPGA driver
 */

#ifndef FPGA_H
#define FPGA_H

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief Initialize the fpga internal shadow registers
 *
 */
void fpga_init(void);

/* battery stuff */

/**
 * \brief Initialize the fpga internal shadow registers
 *
 * \param[in] voltage The voltage that will be sent to the FPGA
 *
 */
void fpga_set_battery_voltage(uint16_t voltage);

/**
 * \brief Trigger a battery temp alert in FPGA
 *
 * \param[in] alert The alert flags
 *
 */
void fpga_set_battery_temp_alert(uint8_t alert);

void fpga_set_battery_online(bool online);

bool fpga_get_battery_online(void);

/**
 * \brief Set the battery health that will be sent to the FPGA with next fpga_sync()
 *
 * \param[in] health The battery health
 *
 */
void fpga_set_battery_health(uint8_t health);

/**
 * \brief Set the battery status that will be sent to the FPGA with next fpga_sync()
 *
 * \param[in] status The battery status
 *
 */
void fpga_set_battery_status(uint8_t status);

/* charger stuff */
/**
 * \brief Set the charger status that will be sent to the FPGA with next fpga_sync()
 *
 * \param[in] status The charger status
 *
 */
void fpga_set_charger_status(uint8_t status);

void fpga_set_charger_online(bool online);

void fpga_set_charger_charge_type(uint8_t type);

/**
 * \brief Set the charger health that will be sent to the FPGA with next fpga_sync()
 *
 * \param[in] health The charger health
 *
 */
void fpga_set_charger_health(uint8_t health);

/**
 * \brief Set the gauge charge value that is sent to the FPGA with next fpga_sync()
 *
 * \param[in] charge The accumulated charge value
 *
 */
void fpga_set_gauge_charge(uint16_t charge);

/**
 * \brief Get the gauge charge value that is stored in shadow register
 *
 * \return The accumulated charge value stored in shadow register
 *
 */
uint16_t fpga_get_gauge_charge(void);

/**
 * \brief Set the gauge charge last full value that is sent to the FPGA with next fpga_sync()
 *
 * \param[in] charge The accumulated charge when last charge cycle terminated
 *
 */
void fpga_set_gauge_charge_last_full(uint16_t charge);

/**
 * \brief Set the gauge temperature value that is sent to the FPGA with next fpga_sync()
 *
 * \param[in] temp The temperature as reported by fuel gauge
 *
 */
void fpga_set_gauge_temp(uint16_t temp);

/**
 * \brief Set the gauge voltage value that is sent to the FPGA with next fpga_sync()
 *
 * \param[in] volt The voltage as reported by fuel gauge
 *
 */
void fpga_set_gauge_voltage(uint16_t volt);

/**
 * \brief Set the gauge status value that is sent to the FPGA with next fpga_sync()
 *
 * \param[in] status The status as reported by fuel gauge
 *
 */
void fpga_set_gauge_status(uint8_t status);

/* misc stuff */

/**
 * \brief Get the shutdown flag from the shadow registers
 *
 * \return true if shutdown was requested by FPGA
 *
 */
bool fpga_get_shutdown(void);

/**
 * \brief Get the write charge flag from shadow register
 *
 * \return true if write to charge count was requested by FPGA
 *
 */
bool fpga_get_write_charge(void);

/**
 * \brief Get the settings from shadow register
 *
 * \return value that was requested to be stored in settings reg
 *
 */
uint8_t fpga_get_settings(void);

/**
 * \brief Get the settings write flag from shadow register
 *
 * \return true if write to settings reg was requested by FPGA
 *
 */
bool fpga_get_write_settings(void);

/**
 * \brief Synchronize the shadow registers with the FPGA
 *
 */
void fpga_sync(void);

#endif /* FPGA_H */
