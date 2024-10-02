//
// Copyright 2025 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/serial.hpp>
#include <map>
#include <memory>

class tmp468_iface
{
public:
    using sptr = std::shared_ptr<tmp468_iface>;

    virtual ~tmp468_iface() = default;

    /*!
     * Makes an interface for communicating with a tmp468 temperature sensor.
     *
     * \param i2c The I2C interface to use for communicating with the sensor
     * \param chip_addr The I2C address of the tmp468 chip
     */
    static sptr make(uhd::i2c_iface::sptr i2c, uint16_t chip_addr);

    //! Enum for the different sensors on the tmp468 chip
    enum tmp_sensor_t : uint8_t {
        LOCAL_SENSOR   = 0,
        REMOTE_SENSOR1 = 1,
        REMOTE_SENSOR2 = 2,
        REMOTE_SENSOR3 = 3,
        REMOTE_SENSOR4 = 4,
        REMOTE_SENSOR5 = 5,
        REMOTE_SENSOR6 = 6,
        REMOTE_SENSOR7 = 7,
        REMOTE_SENSOR8 = 8,
    };

    /*!
     * Performs the read on the temperature sensor to get the binary representation
     * and then converts it to a temperature in degrees Celsius based on the procedure
     * outlined in the datasheet.
     *
     * \param addr The specific sensor on the tmp468 chip to read from. 0 for local sensor
     *             and 1-8 for the remote sensors if atttached.
     * \return temperature reading in degrees Celsius
     */
    virtual double read_temperature(const tmp_sensor_t sensor) = 0;

    /*!
     * Sets the ideality factor for the specified remote sensor.
     *
     * \param addr The specific sensor on the tmp468 chip to set the ideality factor for.
     *             1-8 for the remote sensors if attached.
     * \param ideality_factor The ideality factor for the remote sensor.
     */
    virtual void set_ideality_factor(
        const tmp_sensor_t sensor, const double ideality_factor) = 0;
};
