//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_MULTI_USRP_CLOCK_HPP
#define INCLUDED_UHD_MULTI_USRP_CLOCK_HPP

#include <string>
#include <vector>

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/sensors.hpp>

namespace uhd{ namespace usrp_clock{

/*!
 * The Multi-USRP-Clock device class:
 *
 * This class facilitates ease-of-use for must use-case scenarios when
 * using clock devices with UHD. This class can be used with a
 * single clock device or with multiple clock devices connected to the same
 * host.
 *
 * To create a multi_usrp_clock out of a single USRP Clock:
 *
 * <pre>
 * device_addr_t dev;
 * dev["addr"] = 192.168.10.3;
 * multi_usrp_clock::sptr clock = multi_usrp_clock::make(dev);
 * </pre>
 *
 * To create a multi_usrp_clock out of multiple clock devices:
 *
 * <pre>
 * device_addr_t dev;
 * dev["addr0"] = 192.168.10.3;
 * dev["addr1"] = 192.168.10.4;
 * multi_usrp_clock::sptr clock = multi_usrp_clock::make(dev);
 * </pre>
 */
class UHD_API multi_usrp_clock : boost::noncopyable {
public:
    typedef boost::shared_ptr<multi_usrp_clock> sptr;

    virtual ~multi_usrp_clock(void) = 0;

    /*!
     * Make a new Multi-USRP-Clock from the given device address.
     * \param dev_addr the device address
     * \return a new Multi-USRP-Clock object
     */
    static sptr make(const device_addr_t &dev_addr);

    /*!
     * Return the underlying device.
     * This allows direct access to the EEPROM and sensors.
     * \return the device object within this Multi-USRP-Clock
     */
    virtual device::sptr get_device(void) = 0;

    /*!
     * Get a printable summary for this USRP Clock configuration.
     * \return a printable string
     */
    virtual std::string get_pp_string(void) = 0;

    //! Get the number of USRP Clocks in this configuration.
    virtual size_t get_num_boards(void) = 0;

    //! Get time from device
    virtual uint32_t get_time(size_t board = 0) = 0;

    /*!
     * Get a USRP Clock sensor value.
     * \param name the name of the sensor
     * \param board the board index (0 to M-1)
     * \return a sensor value object
     */
    virtual sensor_value_t get_sensor(const std::string &name, size_t board = 0) = 0;

    /*!
     * Get a list of possible USRP Clock sensor names.
     * \param board the board index (0 to M-1)
     * \return a vector of sensor names
     */
    virtual std::vector<std::string> get_sensor_names(size_t board = 0) = 0;
};

} //namespace
} //namespace

#endif /* INCLUDED_UHD_MULTI_USRP_CLOCK_HPP */
