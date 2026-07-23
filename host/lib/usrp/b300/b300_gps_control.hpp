//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/byte_vector.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <functional>
#include <memory>

namespace uhd { namespace usrp { namespace b300 {

class b300_gps_control : uhd::noncopyable
{
public:
    //! Write functor
    using write_fn_t = std::function<void(uint32_t, uint32_t)>;
    //! Read functor
    using read_fn_t = std::function<uint32_t(uint32_t)>;
    //! Power-on functor
    using power_on_fn_t = std::function<void(bool)>;

    using sptr = std::shared_ptr<b300_gps_control>;

    virtual ~b300_gps_control(void) = default;

    static sptr make(
        write_fn_t&& poke32, read_fn_t&& peek32, power_on_fn_t&& power_on_fn);

    /*! Initialize the GPS control.
     * This will power on the GPS chip and configure it with the correct setting via UBLOX
     * commands.
     * \throw uhd::runtime_error if any of the UBLOX commands fail.
     */
    virtual void initialize(void) = 0;

    /*! Powers off the GPS chip.
     */
    virtual void shutdown(void) = 0;

    /*! Returns whether the GPS chip has been powered on and initialized, and thus is
     * ready for other communication.
     * \return true if GPS has been initialized, false otherwise.
     */
    virtual bool is_initialized(void) const = 0;

    /*! Retrieve the named sensor
     * \param key the name of the sensor to retrieve
     * \return the sensor value
     */
    virtual uhd::sensor_value_t get_sensor(std::string key) = 0;
};

}}} // namespace uhd::usrp::b300
