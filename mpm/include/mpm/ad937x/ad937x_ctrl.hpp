//
// Copyright 2017 Ettus Research (National Instruments)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include "ad937x_ctrl_types.hpp"

#include <mpm/exception.hpp>
#include <mpm/spi/spi_iface.hpp>

#include <uhd/types/direction.hpp>
#include <uhd/types/ranges.hpp>

#include <boost/noncopyable.hpp>

#include <memory>
#include <functional>
#include <set>
#include <mutex>

namespace mpm { namespace chips {

/*! AD937x Control Interface
*
* A sane API for configuring AD937x chips.
*
* \section ad937x_which The `which` parameter
*
* Many function calls require a `which` string parameter to select
* the RF frontend. Valid values for `which` are:
* - RX1, RX2
* - TX1, TX2
*
* Frontend numbering is as designed by the AD9371.
*
* While all functions that use `which` specify an individual channel,
* certain functions affect more than one channel due to the limitations of
* the AD9371.
*/
class ad937x_ctrl : public boost::noncopyable
{
public:
    typedef std::shared_ptr<ad937x_ctrl> sptr;
    /*! \brief make a new AD9371 ctrl object using the specified SPI iface
     *
     * \param spi_mutex a mutex that will be locked whenever the SPI iface is to be used
     * \param iface the spi_iface for accessing the AD9371
     */
    static sptr make(
        std::shared_ptr<std::mutex> spi_mutex,
        mpm::types::regs_iface::sptr iface,
        mpm::ad937x::gpio::gain_pins_t gain_pins);
    virtual ~ad937x_ctrl(void) {}

    virtual void begin_initialization() = 0;
    virtual void finish_initialization() = 0;
    virtual void start_jesd_rx() = 0;
    virtual void start_jesd_tx() = 0;
    virtual uint8_t get_multichip_sync_status() = 0;
    virtual uint8_t get_framer_status() = 0;
    virtual uint8_t get_deframer_status() = 0;

    virtual uint8_t get_deframer_irq() = 0;
    virtual uint16_t get_ilas_config_match() = 0;
    virtual void enable_jesd_loopback(uint8_t enable) = 0;

    //! get the RF frequency range for the AD9371
    static uhd::meta_range_t get_rf_freq_range(void);

    //! get the BW filter range for the AD9371
    static uhd::meta_range_t get_bw_filter_range(void);

    //! get valid clock rates for the AD9371
    static std::vector<double> get_clock_rates(void);

    //! get the gain range for a frontend (RX, TX)
    static uhd::meta_range_t get_gain_range(const std::string &which);

    //! read the product ID from the device
    virtual uint8_t get_product_id() = 0;

    //! read the product revision from the device
    virtual uint8_t get_device_rev() = 0;

    /*! \brief get the API version from the software driver
     *
     * \return a version string in the format "SILICON.MAJOR.MINOR.BUILD"
     */
    virtual std::string get_api_version() = 0;

    /*! \brief get the currently loaded ARM version from the device
     *
     * \return a version string in the format "MAJOR.MINOR.RC"
     */
    virtual std::string get_arm_version() = 0;

    //! set the BW filter for the frontend which
    virtual double set_bw_filter(const std::string &which, double value) = 0;

    /*! \brief set the gain for the frontend which
     *
     * \param which frontend string
     * \param value target gain value
     * \return actual gain value
     */
    virtual double set_gain(const std::string &which, double value) = 0;

    /*! \brief set the agc mode for all RX channels
     *
     * \param which frontend string
     * \param mode requested mode (automatic, manual, hybrid)
     */
    virtual void set_agc_mode(const std::string &which, const std::string &mode) = 0;

    /*! \brief set the clock rate for the device
     *
     * \param value requested clock rate
     * \return actual clock rate
     */
    virtual double set_clock_rate(double value) = 0;

    //! enable the frontend which
    virtual void enable_channel(const std::string &which, bool enable) = 0;

    /*! \brief set the RF frequency for the direction specified in which
     * Sets the RF frequency.  This is a per direction setting.
     * \param which frontend string to specify direction to tune
     * \param value target frequency
     * \return actual frequency
     */
    virtual double set_freq(const std::string &which, double value) = 0;

    /*! \brief get the RF frequency for the direction specified in which
     *
     * Returns the RF frequency.  This is a per direction setting.
     * \param which frontend string to specify direction to get
     * \return actual frequency
     */
    virtual double get_freq(const std::string &which) = 0;

    //! set the FIR filter for the frontend which
    virtual void set_fir(const std::string &which, int8_t gain, const std::vector<int16_t> & fir) = 0;

    //! get the FIR filter for the frontend which
    virtual std::vector<int16_t> get_fir(const std::string &which, int8_t &gain) = 0;

    // TODO: update docstring with temperature unit and calibration information
    //! get the device temperature
    virtual int16_t get_temperature() = 0;

    //! enable or disable gain ctrl pins for one channel
    virtual void set_enable_gain_pins(const std::string &which, bool enable) = 0;

    //! set step sizes for gain ctrl pins for one channel
    virtual void set_gain_pin_step_sizes(const std::string &which, double inc_step, double dec_step) = 0;
};

}}; /* namespace mpm::chips */

#ifdef LIBMPM_PYTHON
void export_mykonos(){
    LIBMPM_BOOST_PREAMBLE("ad937x")
    using namespace mpm::chips;
    bp::class_<ad937x_ctrl, boost::noncopyable, std::shared_ptr<ad937x_ctrl> >("ad937x_ctrl", bp::no_init)
        .def("begin_initialization", &ad937x_ctrl::begin_initialization)
        .def("finish_initialization", &ad937x_ctrl::finish_initialization)
        .def("start_jesd_rx", &ad937x_ctrl::start_jesd_rx)
        .def("start_jesd_tx", &ad937x_ctrl::start_jesd_tx)
        .def("get_multichip_sync_status", &ad937x_ctrl::get_multichip_sync_status)
        .def("get_framer_status", &ad937x_ctrl::get_framer_status)
        .def("get_deframer_status", &ad937x_ctrl::get_deframer_status)
        //.def("get_deframer_irq", &ad937x_ctrl::get_deframer_irq)
        .def("get_ilas_config_match", &ad937x_ctrl::get_ilas_config_match)
        .def("enable_jesd_loopback", &ad937x_ctrl::enable_jesd_loopback)
        .def("get_rf_freq_range", &ad937x_ctrl::get_rf_freq_range)
        .def("get_bw_filter_range", &ad937x_ctrl::get_bw_filter_range)
        .def("get_clock_rates", &ad937x_ctrl::get_clock_rates)
        .def("get_gain_range", &ad937x_ctrl::get_gain_range)
        .def("get_product_id", &ad937x_ctrl::get_product_id)
        .def("get_device_rev", &ad937x_ctrl::get_device_rev)
        .def("get_api_version", &ad937x_ctrl::get_api_version)
        .def("get_arm_version", &ad937x_ctrl::get_arm_version)
        .def("set_bw_filter", &ad937x_ctrl::set_bw_filter)
        .def("set_gain", &ad937x_ctrl::set_gain)
        .def("set_agc_mode", &ad937x_ctrl::set_agc_mode)
        .def("set_clock_rate", &ad937x_ctrl::set_clock_rate)
        .def("enable_channel", &ad937x_ctrl::enable_channel)
        .def("set_freq", &ad937x_ctrl::set_freq)
        .def("get_freq", &ad937x_ctrl::get_freq)
        .def("set_fir", &ad937x_ctrl::set_fir)
        .def("get_fir", &ad937x_ctrl::get_fir)
        .def("get_temperature", &ad937x_ctrl::get_temperature)
        ;
}
#endif

