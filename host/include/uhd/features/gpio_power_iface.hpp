//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/features/discoverable_feature.hpp>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace features {

/*! Interface to provide access to configuring the voltage level and external
 * power export configurations of GPIO banks.
 */
class UHD_API gpio_power_iface : public discoverable_feature
{
public:
    using sptr = std::shared_ptr<gpio_power_iface>;

    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::GPIO_POWER;
    }

    std::string get_feature_name() const
    {
        return "GPIO power configuration";
    }

    virtual ~gpio_power_iface() = default;

    /*! Return what I/O voltage levels are supported on the given port. Voltages
     * use a V in place of the decimal point - typical values include "1V8" for
     * 1.8 volts, "3V3" for 3.3 volts, et cetera.
     *
     * \param port Port to list supported voltages for.
     * \return A vector of supported voltage levels.
     */
    virtual std::vector<std::string> get_supported_voltages(
        const std::string& port) const = 0;

    /*! Set the I/O voltage on the given port to the given value.
     *
     * \param port Port to set the voltage on.
     * \param voltage Voltage level to set. See \ref get_supported_voltages for a list
     * of supported voltages.
     */
    virtual void set_port_voltage(
        const std::string& port, const std::string& voltage) = 0;

    /*! Gets the current I/O voltage level that the port is set to.
     *
     * \param port Port to retrieve the voltage level for
     * \return The voltage level for the given port
     */
    virtual std::string get_port_voltage(const std::string& port) const = 0;

    /*! Enables or disables the power supply exposed through the GPIO ports. The
     * GPIO lines will function without external power enabled - the power supply
     * is simply to provide a means for external circuitry to receive power from
     * the device. See your device's documentation for more information on the
     * voltage and current specifications of the external power supply.
     *
     * \param port The port to export power through.
     * \param enable true to enable power, false to disable.
     */
    virtual void set_external_power(const std::string& port, bool enable) = 0;

    /*! Retrieve the status of the external power on the given port. The status
     * can be one of three values:
     * "OFF" - External power is disabled (the default)
     * "ON" - External power is on and functioning normally
     * "FAULT" - The external power supply has encountered a fault condition.
     *
     * \param port Port to retrieve status from
     */
    virtual std::string get_external_power_status(const std::string& port) const = 0;
};

}} // namespace uhd::features
