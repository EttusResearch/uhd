//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/cal/container.hpp>
#include <uhd/config.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/optional.hpp>
#include <map>

namespace uhd { namespace usrp { namespace cal {

/*! Class that stores power levels (in dBm) at various gains, frequencies, and
 * temperatures.
 *
 * This container class is suitable for all cases where devices want to store a
 * gain-to-power mapping from a single, overall gain value.
 *
 * The underlying data structure stores the power level for every gain/frequency
 * point that is part of this data set. It can also do a reverse look-up of gain
 * values for a given power value.
 *
 * The interpolation algorithms assume a monotonic gain/power profile, i.e.,
 * f(gain) = power is monotonically increasing. If the power is not monotonically
 * increasing, the interpolation algorithms still work, but get_gain() might
 * produce a greater than necessary interpolation error. For that case, this
 * class provides get_gain_coerced(), which helps with both coercion of the
 * interpolated gain into a gain value that can be used at the call site, as
 * well as minimizing the effect on the application using this container.
 *
 * All interpolation algorithms first interpolate temperature by finding the
 * nearest available temperature data. For example, if the data set includes
 * calibration data for 40C and 50C, and the actual temperature is measured at
 * 48C, then the data set for 50C is used, and the data set for 40C is not
 * considered at all.
 * Within a data set, frequency and gain are interpolated in two dimensions (the
 * same is true for frequency and power for get_gain() and get_gain_coerced())
 * using a bilinear interpolation.
 */
class UHD_API pwr_cal : public container
{
public:
    using sptr = std::shared_ptr<pwr_cal>;

    /*! Add or update a power level data point
     *
     * Note: Power measurements can only be written atomically. It is not
     * possible to add individual gain/power points using this method.
     *
     * \param gain_power_map A mapping gain -> power (dB -> dBm) for all
     *                       measured gain points for this frequency.
     * \param min_power The minimum available power for this frequency.
     * \param max_power The maximum available power for this frequency.
     * \param freq The frequency at which this power level was measured
     * \param temperature The temperature at which this power level was measured,
     *                    in Celsius. This parameter is optional, the return
     *                    value for get_temperature() is used if no temperature
     *                    is provided here.
     */
    virtual void add_power_table(const std::map<double, double>& gain_power_map,
        const double min_power,
        const double max_power,
        const double freq,
        const boost::optional<int> temperature = boost::none) = 0;

    /*! Clear all stored values
     *
     * Note: All of the getters will throw a uhd::assertion_error if called after
     * clearing the data.
     */
    virtual void clear() = 0;

    /*! Set the current default temperature
     *
     * Some of the API calls have a optional temperature argument. However, in
     * practice, it is often useful to set an ambient temperature once, or only
     * when it has changed, and then use a default temperature instead of
     * providing the temperature on every call. This sets the default
     * temperature.
     */
    virtual void set_temperature(const int temperature) = 0;

    //! Return the current default temperature
    virtual int get_temperature() const = 0;

    /*! Set the reference gain point, i.e., the gain value where by definition
     * the difference between linearized power and measured power is zero.
     *
     * Currently unused.
     */
    virtual void set_ref_gain(const double gain) = 0;

    /*! Return the current reference gain point.
     *
     * Currently unused.
     */
    virtual double get_ref_gain() const = 0;

    /*! Return the min and max power available for this frequency
     */
    virtual uhd::meta_range_t get_power_limits(const double freq,
        const boost::optional<int> temperature = boost::none) const = 0;

    /*! Returns the power at a gain value.
     *
     * This will interpolate from the given data set to obtain a power value if
     * gain and frequency are not exactly within the given data set.
     *
     * \param gain The gain at which we are checking the power
     * \param freq The frequency at which we are checking the power
     * \param temperature The temperature at which we are checking the power. If
     *                    none is given, uses the current default temperature
     *                    (see set_temperature()).
     */
    virtual double get_power(const double gain,
        const double freq,
        const boost::optional<int> temperature = boost::none) const = 0;

    /*! Look up a gain value from a power value.
     *
     * This is the reverse function to get_power(). Like get_power(), it will
     * interpolate in two dimensions (linearly) if the requested power value is
     * not part of the measurement data.
     *
     * Note: \p power_dbm is coerced into the available power range using
     * get_power_limits().
     *
     * \param power_dbm The power (in dBm) at which we are getting the gain value for
     * \param freq The frequency at which we are finding the gain
     * \param temperature The temperature at which we are finding the gain. If
     *                    none is given, uses the current default temperature
     *                    (see set_temperature()).
     */
    virtual double get_gain(const double power_dbm,
        const double freq,
        const boost::optional<int> temperature = boost::none) const = 0;

    //! Factory for new cal data sets
    static sptr make(
        const std::string& name, const std::string& serial, const uint64_t timestamp);

    //! Default factory
    static sptr make();
};

}}} // namespace uhd::usrp::cal
