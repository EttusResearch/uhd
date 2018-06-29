//
// Copyright 2012-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_AD9361_CTRL_HPP
#define INCLUDED_AD9361_CTRL_HPP

#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/exception.hpp>
#include <boost/shared_ptr.hpp>
#include <ad9361_device.h>
#include <string>
#include <complex>
#include <uhd/types/filters.hpp>
#include <vector>

namespace uhd { namespace usrp {

/*! AD936x Control Interface
 *
 * This is a convenient way to access the AD936x RF IC.
 * It basically encodes knowledge of register values etc. into
 * accessible API calls.
 *
 * \section ad936x_which The `which` parameter
 *
 * Many function calls require a `which` parameter to select
 * the RF frontend. Valid values for `which` are:
 * - RX1, RX2
 * - TX1, TX2
 *
 * Frontend numbering is as designed by the AD9361.
 */
class ad9361_ctrl : public boost::noncopyable
{
public:
    typedef boost::shared_ptr<ad9361_ctrl> sptr;

    virtual ~ad9361_ctrl(void) {}

    //! make a new codec control object
    static sptr make_spi(
        ad9361_params::sptr client_settings,
        uhd::spi_iface::sptr spi_iface,
        uint32_t slave_num
    );
    //! Get a list of gain names for RX or TX
    static std::vector<std::string> get_gain_names(const std::string &/*which*/)
    {
        return std::vector<std::string>(1, "PGA");
    }

    //! get the gain range for a particular gain element
    static uhd::meta_range_t get_gain_range(const std::string &which)
    {
        if(which[0] == 'R') {
            return uhd::meta_range_t(0.0, 76.0, 1.0);
        } else {
            return uhd::meta_range_t(0.0, 89.75, 0.25);
        }
    }

    //! get the freq range
    static uhd::meta_range_t get_rf_freq_range(void)
    {
        return uhd::meta_range_t(50e6, 6e9);
    }

    //! get the filter range for the frontend which
    static uhd::meta_range_t get_bw_filter_range(void)
    {
        return uhd::meta_range_t(ad9361_device_t::AD9361_MIN_BW, ad9361_device_t::AD9361_MAX_BW);
    }

    //! get the clock rate range for the frontend
    static uhd::meta_range_t get_clock_rate_range(void)
    {
        return uhd::meta_range_t(
                ad9361_device_t::AD9361_MIN_CLOCK_RATE,
                ad9361_device_t::AD9361_MAX_CLOCK_RATE
        );
    }

    //! set the filter bandwidth for the frontend's analog low pass
    virtual double set_bw_filter(const std::string &/*which*/, const double /*bw*/) = 0;

    //! set the gain for a particular gain element
    virtual double set_gain(const std::string &which, const double value) = 0;

    //! Enable or disable the AGC module
    virtual void set_agc(const std::string &which, bool enable) = 0;

    //! configure the AGC module to slow or fast mode
    virtual void set_agc_mode(const std::string &which, const std::string &mode) = 0;

    //! set a new clock rate, return the exact value
    virtual double set_clock_rate(const double rate) = 0;

    //! set which RX and TX chains/antennas are active
    virtual void set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2) = 0;

    //! set which timing mode is used
    virtual void set_timing_mode(const std::string &timing_mode) = 0;

    //! tune the given frontend, return the exact value
    virtual double tune(const std::string &which, const double value) = 0;

    //! set the DC offset for I and Q manually
    void set_dc_offset(const std::string &, const std::complex<double>)
    {
        //This feature should not be used according to Analog Devices
        throw uhd::runtime_error("ad9361_ctrl::set_dc_offset this feature is not supported on this device.");
    }

    //! enable or disable the BB/RF DC tracking feature
    virtual void set_dc_offset_auto(const std::string &which, const bool on) = 0;

    //! set the IQ correction value manually
    void set_iq_balance(const std::string &, const std::complex<double>)
    {
        //This feature should not be used according to Analog Devices
        throw uhd::runtime_error("ad9361_ctrl::set_iq_balance this feature is not supported on this device.");
    }

    //! enable or disable the quadrature calibration
    virtual void set_iq_balance_auto(const std::string &which, const bool on) = 0;

    //! get the current frequency for the given frontend
    virtual double get_freq(const std::string &which) = 0;

    //! turn on/off Catalina's data port loopback
    virtual void data_port_loopback(const bool on) = 0;

    //! read internal RSSI sensor
    virtual sensor_value_t get_rssi(const std::string &which) = 0;

    //! read the internal temp sensor
    virtual sensor_value_t get_temperature() = 0;

    //! List all available filters by name
    virtual std::vector<std::string> get_filter_names(const std::string &which) = 0;

    //! Return a list of all filters
    virtual filter_info_base::sptr get_filter(const std::string &which, const std::string &filter_name) = 0;

    //! Write back a filter
    virtual void set_filter(const std::string &which, const std::string &filter_name, const filter_info_base::sptr) = 0;

    virtual void output_digital_test_tone(bool enb) = 0;
};

}}

#endif /* INCLUDED_AD9361_CTRL_HPP */
