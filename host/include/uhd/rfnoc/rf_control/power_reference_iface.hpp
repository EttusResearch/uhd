//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/direction.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/assert_has.hpp>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace rf_control {

/*! Interface for reference power API commands
 *
 * This interface contains all methods related to the reference power API,
 * and is usually only accessed directly via the radio_control class.
 */
class power_reference_iface
{
public:
    using sptr = std::shared_ptr<power_reference_iface>;

    virtual ~power_reference_iface() = default;

    /*! Return true if this channel has a reference power API enabled
     *
     * Many devices either don't have a built-in reference power API, or they
     * require calibration data for it to work. This means that it is not clear,
     * even when the device type is known, if a device supports setting a power
     * reference level. Use this method to query the availability of
     * set_rx_power_reference() and get_rx_power_reference(), which will throw
     * a uhd::not_implemented_error or uhd::runtime_error if they cannot be used.
     *
     * See \ref page_power for more information, or query the specific device's
     * manual page to see if a power API is available, and how to enable it.
     *
     * \param chan The channel for which this feature is queried
     *
     * \returns true if this channel has an RX power API available
     */
    virtual bool has_rx_power_reference(const size_t chan = 0) = 0;

    /*! Set the reference RX power level for a given channel
     *
     * Note: This functionality is not supported for most devices, and will
     * cause a uhd::not_implemented_error exception to be thrown on devices that
     * do not have this functionality.
     *
     * For more information on how to use this API, see \ref page_power.
     *
     * \param power_dbm The reference power level in dBm
     * \param chan The channel for which this setting applies
     *
     * \throws uhd::not_implemented_error if this functionality does not exist
     *         for this device
     */
    virtual void set_rx_power_reference(
        const double power_dbm, const size_t chan = 0) = 0;

    /*! Return the actual reference RX power level.
     *
     * Note: This functionality is not supported for most devices, and will
     * cause a uhd::not_implemented_error exception to be thrown on devices that
     * do not have this functionality.
     *
     * For more information on how to use this API, see \ref page_power.
     *
     * \param chan The channel for which this setting is queried
     * \throws uhd::not_implemented_error if this functionality does not exist
     *         for this device
     */
    virtual double get_rx_power_reference(const size_t chan = 0) = 0;

    /*! Return the keys by which the power calibration data is referenced for this
     * channel.
     *
     * The first entry is the key, the second the serial. These are the same
     * arguments that can be used for uhd::usrp::cal::database::read_cal_data()
     * and friends. See also \ref cal_db_serial.
     *
     * Note that the key can change at runtime, e.g., when the antenna port is
     * switched.
     *
     * The difference between this and has_rx_power_reference() is that the
     * latter requires both device support as well as calibration data, whereas
     * this function will never throw, and will always return a non-empty vector
     * if device support is there, even if the device does not have calbration
     * data loaded.
     *
     * \returns an empty vector if no power calibration is supported, or a
     *          vector of length 2 with key and serial if it does.
     */
    virtual std::vector<std::string> get_rx_power_ref_keys(const size_t chan = 0) = 0;

    /*! Return the available RX power range given the current configuration
     *
     * This will return the range of available power levels given the current
     * frequency, gain profile, antenna, and whatever other settings may affect
     * the available power ranges. Note that the available power range may
     * change frequently, so don't assume an immutable range.
     *
     * \param chan The channel index
     */
    virtual meta_range_t get_rx_power_range(const size_t chan) = 0;

    /*! Return true if this channel has a reference power API enabled
     *
     * Many devices either don't have a built-in reference power API, or they
     * require calibration data for it to work. This means that it is not clear,
     * even when the device type is known, if a device supports setting a power
     * reference level. Use this method to query the availability of
     * set_tx_power_reference() and get_tx_power_reference(), which will throw
     * a uhd::not_implemented_error or uhd::runtime_error if they cannot be used.
     *
     * See \ref page_power for more information, or query the specific device's
     * manual page to see if a power API is available, and how to enable it.
     *
     * \param chan The channel for which this feature is queried
     *
     * \returns true if this channel has a TX power API available
     */
    virtual bool has_tx_power_reference(const size_t chan = 0) = 0;

    /*! Set the reference TX power level for a given channel
     *
     * Note: This functionality is not supported for most devices, and will
     * cause a uhd::not_implemented_error exception to be thrown on devices that
     * do not have this functionality.
     *
     * For more information on how to use this API, see \ref page_power.
     *
     * \param power_dbm The reference power level in dBm
     * \param chan The channel for which this setting applies
     *
     * \throws uhd::not_implemented_error if this functionality does not exist
     *         for this device
     */
    virtual void set_tx_power_reference(
        const double power_dbm, const size_t chan = 0) = 0;

    /*! Return the actual reference TX power level.
     *
     * Note: This functionality is not supported for most devices, and will
     * cause a uhd::not_implemented_error exception to be thrown on devices that
     * do not have this functionality.
     *
     * For more information on how to use this API, see \ref page_power.
     *
     * \param chan The channel for which this setting is queried
     * \throws uhd::not_implemented_error if this functionality does not exist
     *         for this device
     */
    virtual double get_tx_power_reference(const size_t chan = 0) = 0;

    /*! Return the keys by which the power calibration data is referenced for this
     * channel.
     *
     * The first entry is the key, the second the serial. These are the same
     * arguments that can be used for uhd::usrp::cal::database::read_cal_data()
     * and friends. See also \ref cal_db_serial.
     *
     * Note that the key can change at runtime, e.g., when the antenna port is
     * switched.
     *
     * The difference between this and has_tx_power_reference() is that the
     * latter requires both device support as well as calibration data, whereas
     * this function will never throw, and will always return a non-empty vector
     * if device support is there, even if the device does not have calbration
     * data loaded.
     *
     * \returns an empty vector if no power calibration is supported, or a
     *          vector of length 2 with key and serial if it does.
     */
    virtual std::vector<std::string> get_tx_power_ref_keys(const size_t chan = 0) = 0;

    /*! Return the available TX power range given the current configuration
     *
     * This will return the range of available power levels given the current
     * frequency, gain profile, antenna, and whatever other settings may affect
     * the available power ranges. Note that the available power range may
     * change frequently, so don't assume an immutable range.
     *
     * \param chan The channel index
     */
    virtual meta_range_t get_tx_power_range(const size_t chan) = 0;

};

}}} // namespace uhd::rfnoc::rf_control
