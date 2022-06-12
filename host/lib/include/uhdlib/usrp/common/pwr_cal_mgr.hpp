//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/property_tree.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/gain_group.hpp>
#include <functional>
#include <memory>
#include <string>

namespace uhd { namespace usrp {

/*! Power Calibration Manager: Helper class to set track power vs. gain settings
 *
 * This class can be plugged into USRP device drivers (or daughterboard drivers).
 * It will manage loading the calibration data, and translating power settings
 * into gain settings, and vice versa (i.e. read the power from a gain setting).
 *
 * This class mostly communicates with the device driver via callbacks. This
 * allows separating the power control from the device driver without having to
 * implement any device-specific code in this manager.
 *
 * The actual control of gains is also done via callbacks, but they are
 * enapsulated into a gain group. The gain group must separate hardware gains
 * from other gains that can be used for more fine-grained correction; the
 * existing gain groups in device drivers may or may not be adequate.
 *
 * For example, the X3x0 has two gain stages on the RX side, there's an ADC gain
 * as well as the analog gain of the daughterboards. The calibration is performed
 * over the entire gain range, including the ADC gain, which means that the
 * hardware gains *include* the ADC gain and must therefore not be a separate gain
 * stage for this class.
 */
class pwr_cal_mgr
{
public:
    using sptr            = std::shared_ptr<pwr_cal_mgr>;
    using get_double_type = std::function<double(void)>;
    using get_str_type    = std::function<std::string(void)>;
    //! The current power/gain tracking mode. When the device does something
    // that will cause the output power to change, we have an option of keeping
    // the gain constant, or keeping the power constant.
    enum class tracking_mode {
        TRACK_GAIN, //!< In this mode, we keep the gain after retuning.
        TRACK_POWER //!< In this mode, we keep the power after retuning
    };

    //! Helper: Sanitize antenna names (e.g. TX/RX -> tx_rx)
    //
    // Note: Argument is not const std::string&; we make use of C++'s short
    // string optimization for an easier implementation
    static std::string sanitize_antenna_name(std::string antenna_name);

    //! Helper: Check if an antenna name is valid for calibration (e.g., CAL and
    // LOCAL are not)
    static bool is_valid_antenna(const std::string& antenna);

    /*! Factory
     *
     * \param serial The serial string used for the cal database lookup
     * \param log_id A string used for logging
     * \param get_freq A function object to read back the current frequency from the
     *                 underlying device
     * \param get_key A function object to read back the current calibraton key
     *                from the underlying device
     * \param gain_group A gain group. The first gain stage (with the
     *                   higher priority) must be the same gain as used in the
     *                   calibration data; usually, it's the overall hardware
     *                   gain. The second is any ancillary gain that can be used
     *                   for correction. This could be a baseband gain.
     */
    static sptr make(const std::string& serial,
        const std::string& log_id,
        get_double_type&& get_freq,
        get_str_type&& get_key,
        uhd::gain_group::sptr gain_group);

    virtual ~pwr_cal_mgr() = default;

    //! Update the gain group (see make());
    //
    // Not thread-safe: Don't call at the same time as set_power()
    virtual void set_gain_group(uhd::gain_group::sptr gain_group) = 0;

    //! Return true if there is power cal data for the currently selected port
    virtual bool has_power_data() = 0;

    //! Add a property tree node (ref_power/range and ref_power/value)
    //
    // For non-RFNoC devices (e.g. B200), this must be called in order to
    // expose the power APIs to multi_usrp.
    virtual void populate_subtree(uhd::property_tree::sptr subtree) = 0;

    /*! Set the power to \p power_dbm
     *
     * This function will coerce both the valid power, and the resulting gain
     * value, and will return the actual, current power.
     *
     * Note: Calling this will automatically set the tracking mode to TRACK_POWER.
     */
    virtual double set_power(const double power_dbm) = 0;

    //! Return the current power
    virtual double get_power() = 0;

    /*! Reapply the power setting from the last set_power() call
     *
     * This is useful for reapplying settings, e.g., after tuning. It can be
     * set as a callback or subscriber to events that require a power reset.
     * This will do nothing if the current tracking mode is set to
     * tracking_mode::KEEP_GAIN
     */
    virtual void update_power() = 0;

    /*! Return the valid range of powers
     *
     * This will query the device state and return currently valid settings.
     */
    virtual uhd::meta_range_t get_power_range() = 0;

    //! Update the temperature in Celsius
    //
    // Because reading the temperature can be an invasive operation, we leave it
    // up to the device implementation to update the temperature at sensible
    // intervals instead of using a callback.
    virtual void set_temperature(const int temp_C) = 0;

    /*! Set the current power tracking mode
     */
    virtual void set_tracking_mode(const tracking_mode) = 0;

    /*! Get the current power tracking mode
     */
    virtual tracking_mode get_tracking_mode() = 0;

    //! Return the currently active calibration data key
    virtual std::string get_key() = 0;

    //! Return the calibration serial
    virtual std::string get_serial() const = 0;

    //! Update serial
    //
    // This may be called for example when the hardware is hot-pluggable, or
    // if the calibration key changes at runtime.
    // Calling it will not only set the serial number, but will also force a
    // reload of the calibration data. The existing calibration data is
    // discarded.
    virtual void set_serial(const std::string& serial) = 0;
};

}} // namespace uhd::usrp
