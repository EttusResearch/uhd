//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/features/discoverable_feature.hpp>
#include <uhd/types/sensors.hpp>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace features {

/*! Interface to provide access to configuring the voltage level and external
 * power export configurations of GPIO banks.
 */
class UHD_API gps_iface : public discoverable_feature
{
public:
    using sptr = std::shared_ptr<gps_iface>;

    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::GPS;
    }

    std::string get_feature_name() const
    {
        return "GPS Interface";
    }

    virtual ~gps_iface() = default;

    /*! Retrieve the list of sensors this GPS object provides
     */
    virtual std::vector<std::string> get_sensors(void) = 0;

    /*! Retrieve the sensor value by name
     *
     * This will return the same sensors as the motherboard sensors, but only
     * the ones that are relevant to the GPS.
     */
    virtual uhd::sensor_value_t get_sensor(const std::string& key) = 0;
};

}} // namespace uhd::features
