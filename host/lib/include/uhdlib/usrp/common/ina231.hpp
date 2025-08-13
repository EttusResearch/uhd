//
// Copyright 2025 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/serial.hpp>
#include <map>
#include <memory>

class ina231_iface
{
public:
    using sptr = std::shared_ptr<ina231_iface>;

    virtual ~ina231_iface() = default;

    /*!
     * Makes an interface for communicating with a ina231 output current, voltage and
     * power monitor. Takes the shunt resistance and maximum current as parameters to
     * configure the calibration register that needs to be set before reading power or
     * current.
     *
     * \param i2c The I2C interface to use for communicating with the sensor
     * \param chip_addr The I2C address of the ina231 chip
     * \param shunt_resistance The shunt resistance in ohms
     * \param max_current The maximum expected current in amps
     */
    static sptr make(uhd::i2c_iface::sptr i2c,
        const uint16_t chip_addr,
        const double shunt_resistance,
        const double max_current);

    /*!
     * Read the most recent bus voltage reading. Returns the averaged value if averaging
     * is enabled.
     *
     * \return bus voltage reading in volts
     */
    virtual double read_bus_voltage() = 0;

    /*!
     * Read the Power register. Returns the averaged value if averaging is enabled.
     *
     * \return power reading in watts
     */
    virtual double read_power() = 0;

    /*!
     * Read the Current register. Returns the averaged value if averaging is enabled.
     *
     * \return current reading in amps
     */
    virtual double read_current() = 0;
};
