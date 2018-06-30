//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
#include <future>

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

    static const uint32_t TX_BB_FILTER;
    static const uint32_t ADC_TUNER;
    static const uint32_t TIA_3DB_CORNER;
    static const uint32_t DC_OFFSET;
    static const uint32_t TX_ATTENUATION_DELAY;
    static const uint32_t RX_GAIN_DELAY;
    static const uint32_t FLASH_CAL;
    static const uint32_t PATH_DELAY;
    static const uint32_t TX_LO_LEAKAGE_INTERNAL;
    static const uint32_t TX_LO_LEAKAGE_EXTERNAL ;
    static const uint32_t TX_QEC_INIT;
    static const uint32_t LOOPBACK_RX_LO_DELAY;
    static const uint32_t LOOPBACK_RX_RX_QEC_INIT;
    static const uint32_t RX_LO_DELAY;
    static const uint32_t RX_QEC_INIT;
    static const uint32_t DPD_INIT;
    static const uint32_t CLGC_INIT;
    static const uint32_t VSWR_INIT;
    static const uint32_t TRACK_RX1_QEC;
    static const uint32_t TRACK_RX2_QEC;
    static const uint32_t TRACK_ORX1_QEC;
    static const uint32_t TRACK_ORX2_QEC;
    static const uint32_t TRACK_TX1_LOL;
    static const uint32_t TRACK_TX2_LOL;
    static const uint32_t TRACK_TX1_QEC;
    static const uint32_t TRACK_TX2_QEC;
    static const uint32_t TRACK_TX1_DPD;
    static const uint32_t TRACK_TX2_DPD;
    static const uint32_t TRACK_TX1_CLGC;
    static const uint32_t TRACK_TX2_CLGC;
    static const uint32_t TRACK_TX1_VSWR;
    static const uint32_t TRACK_TX2_VSWR;
    static const uint32_t TRACK_ORX1_QEC_SNLO;
    static const uint32_t TRACK_ORX2_QEC_SNLO;
    static const uint32_t TRACK_SRX_QEC;
    static const uint32_t DEFAULT_INIT_CALS_MASKS;
    static const uint32_t DEFAULT_TRACKING_CALS_MASKS;
    static const uint32_t DEFAULT_INIT_CALS_TIMEOUT;

    // Async call handles
    std::future<void> handle_finish_initialization;
    std::future<void> handle_setup_cal;

    /*! \brief make a new AD9371 ctrl object using the specified SPI iface
     *
     * \param spi_mutex a mutex that will be locked whenever the SPI iface is to be used
     * \param iface the spi_iface for accessing the AD9371
     * \param gain_pins a struct defining the usage of gain pins by this device
     */
    static sptr make(
        std::shared_ptr<std::mutex> spi_mutex,
        const size_t deserializer_lane_xbar,
        mpm::types::regs_iface::sptr iface,
        mpm::ad937x::gpio::gain_pins_t gain_pins);
    virtual ~ad937x_ctrl(void) {}

    //! initializes the AD9371, checks basic functionality, and prepares the chip to receive a SYSREF pulse
    virtual void begin_initialization() = 0;

    //! finishes initialization of the AD9371 by loading the ARM binary and setting a default RF configuration
    virtual void finish_initialization() = 0;

    /*! \setup initialization and tracking calibration
    *
    *\param init_cals_mask bit masking field for init calibration default to 0x4DFF
    * NOTE: this init cals mask need to be at least 0x4F.
    *\param tracking_cals_mask bit masking field for tracking calibration default to 0xC3
    *\param timeout init calibration timeout. default to 10s
    */
    virtual void setup_cal(
            const uint32_t init_cals_mask,
            const uint32_t tracking_cals_mask,
            const uint32_t timeout
    ) = 0;

    //! set LO source
    virtual std::string set_lo_source(
            const std::string &which,
            const std::string &source
    ) = 0;

    //! get LO source
    virtual std::string get_lo_source(const std::string &which) = 0;

    //! resets and start the JESD deframer (JESD Rx, for RF Tx)
    virtual void start_jesd_rx() = 0;

    //! starts the JESD framer (JESD Tx, for RF Rx)
    virtual void start_jesd_tx() = 0;

    //! moves the AD9371 to a running state, RF inputs/outputs will be unmuted
    virtual void start_radio() = 0;

    //! moves the AD9371 to an idle state, RF inputs/outputs will be muted
    virtual void stop_radio() = 0;

    //! get the multichip sync status byte, see AD9371 data sheet for more information
    virtual uint8_t get_multichip_sync_status() = 0;

    //! get the JESD framer status byte, see AD9371 data sheet for more information
    virtual uint8_t get_framer_status() = 0;

    //! get the JESD deframer status byte, see AD9371 data sheet for more information
    virtual uint8_t get_deframer_status() = 0;

    //! get the ilas config status bytes, see AD9371 data sheet for more information
    virtual uint16_t get_ilas_config_match() = 0;

    //! enable or disable JESD loopback, when enabled JESD Rx will be directly connected to JESD Tx
    virtual void enable_jesd_loopback(const uint8_t enable) = 0;

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
    virtual double set_bw_filter(
            const std::string &which,
            const double value
    ) = 0;

    /*! \brief set the gain for the frontend which
     *
     * \param which frontend string
     * \param value target gain value
     * \return actual gain value
     */
    virtual double set_gain(const std::string &which, const double value) = 0;

    /*! \brief get the gain for the frontend which
    *
    * \param which frontend string
    * \return actual gain value
    */
    virtual double get_gain(const std::string &which) = 0;

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
    virtual void enable_channel(const std::string &which, const bool enable) = 0;

    /*! \brief set the RF frequency for the direction specified in which
     * Sets the RF frequency.  This is a per direction setting.
     * \param which frontend string to specify direction to tune
     * \param value target frequency
     * \param wait_for_lock wait after tuning for the PLL to lock
     * \return actual frequency
     */
    virtual double set_freq(
            const std::string &which,
            const double value,
            const bool wait_for_lock
    ) = 0;

    /*! \brief get the RF frequency for the direction specified in which
     *
     * Returns the RF frequency.  This is a per direction setting.
     * \param which frontend string to specify direction to get
     * \return actual frequency
     */
    virtual double get_freq(const std::string &which) = 0;

    /*! \brief Returns the LO lock status
     *
     * Note there's only one LO per direction, so the channel doesn't really
     * matter here.
     * This does not check the PLL lock status for the main clock, the sniffer,
     * or the CAL PLL.
     */
    virtual bool get_lo_locked(const std::string &which) = 0;

    //! set master clock rate
    virtual void set_master_clock_rate(const double rate) = 0;

    //! set the FIR filter for the frontend which
    virtual void set_fir(
            const std::string &which,
            const int8_t gain,
            const std::vector<int16_t> & fir
    ) = 0;

    //! get the FIR filter for the frontend which
    virtual std::vector<int16_t> get_fir(
            const std::string &which,
            int8_t &gain
    ) = 0;

    // TODO: update docstring with temperature unit and calibration information
    //! get the device temperature
    virtual int16_t get_temperature() = 0;

    //! enable or disable gain ctrl pins for one channel
    virtual void set_enable_gain_pins(
            const std::string &which,
            const bool enable
    ) = 0;

    //! set step sizes for gain ctrl pins for one channel
    virtual void set_gain_pin_step_sizes(
            const std::string &which,
            double inc_step,
            double dec_step
    ) = 0;

    //! Direct register read access
    virtual uint8_t peek8(const uint32_t addr) = 0;

    //! Direct register write access
    virtual void poke8(const uint32_t addr, const uint8_t val) = 0;
};

}}; /* namespace mpm::chips */

#ifdef LIBMPM_PYTHON
void export_mykonos(){
    LIBMPM_BOOST_PREAMBLE("ad937x")
    using namespace mpm::chips;
    bp::class_<ad937x_ctrl, boost::noncopyable, std::shared_ptr<ad937x_ctrl>>("ad937x_ctrl", bp::no_init)
        .def("set_master_clock_rate", &ad937x_ctrl::set_master_clock_rate)
        .def("begin_initialization", &ad937x_ctrl::begin_initialization)
        .def("async__finish_initialization", +[](
                ad937x_ctrl& self
            ){
                self.handle_finish_initialization = std::async(std::launch::async,
                    &ad937x_ctrl::finish_initialization,
                    &self
                );
        })
        .def("await__finish_initialization", +[](
                ad937x_ctrl& self
            )->bool{
                if (self.handle_finish_initialization.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
                    self.handle_finish_initialization.get();
                    return true;
                }
                return false;
        })
        .def("set_lo_source", &ad937x_ctrl::set_lo_source)
        .def("get_lo_source", &ad937x_ctrl::get_lo_source)
        .def("async__setup_cal", +[](
                ad937x_ctrl& self,
                const uint32_t init_cals_mask,
                const uint32_t timeout,
                const uint32_t tracking_cals_mask
            ){
                self.handle_setup_cal = std::async(std::launch::async,
                    &ad937x_ctrl::setup_cal,
                    &self,
                    init_cals_mask,
                    timeout,
                    tracking_cals_mask
                );
        })
        .def("await__setup_cal", +[](
                ad937x_ctrl& self
            )->bool{
                if (self.handle_setup_cal.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
                    self.handle_setup_cal.get();
                    return true;
                }
                return false;
        })
        .def("start_jesd_rx", &ad937x_ctrl::start_jesd_rx)
        .def("start_jesd_tx", &ad937x_ctrl::start_jesd_tx)
        .def("start_radio", &ad937x_ctrl::start_radio)
        .def("stop_radio", &ad937x_ctrl::stop_radio)
        .def("get_multichip_sync_status", &ad937x_ctrl::get_multichip_sync_status)
        .def("get_framer_status", &ad937x_ctrl::get_framer_status)
        .def("get_deframer_status", &ad937x_ctrl::get_deframer_status)
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
        .def("get_gain", &ad937x_ctrl::get_gain)
        .def("set_agc_mode", &ad937x_ctrl::set_agc_mode)
        .def("set_clock_rate", &ad937x_ctrl::set_clock_rate)
        .def("enable_channel", &ad937x_ctrl::enable_channel)
        .def("set_freq", &ad937x_ctrl::set_freq)
        .def("get_freq", &ad937x_ctrl::get_freq)
        .def("get_lo_locked", &ad937x_ctrl::get_lo_locked)
        .def("set_fir", &ad937x_ctrl::set_fir)
        .def("get_fir", &ad937x_ctrl::get_fir)
        .def("get_temperature", &ad937x_ctrl::get_temperature)
        .def_readonly("TX_BB_FILTER", &ad937x_ctrl::TX_BB_FILTER)
        .def_readonly("ADC_TUNER", &ad937x_ctrl::ADC_TUNER)
        .def_readonly("TIA_3DB_CORNER", &ad937x_ctrl::TIA_3DB_CORNER)
        .def_readonly("DC_OFFSET", &ad937x_ctrl::DC_OFFSET)
        .def_readonly("TX_ATTENUATION_DELAY", &ad937x_ctrl::TX_ATTENUATION_DELAY)
        .def_readonly("RX_GAIN_DELAY", &ad937x_ctrl::RX_GAIN_DELAY)
        .def_readonly("FLASH_CAL", &ad937x_ctrl::FLASH_CAL)
        .def_readonly("PATH_DELAY", &ad937x_ctrl::PATH_DELAY)
        .def_readonly("TX_LO_LEAKAGE_INTERNAL", &ad937x_ctrl::TX_LO_LEAKAGE_INTERNAL)
        .def_readonly("TX_LO_LEAKAGE_EXTERNAL", &ad937x_ctrl::TX_LO_LEAKAGE_EXTERNAL)
        .def_readonly("TX_QEC_INIT", &ad937x_ctrl::TX_QEC_INIT)
        .def_readonly("LOOPBACK_RX_LO_DELAY", &ad937x_ctrl::LOOPBACK_RX_LO_DELAY)
        .def_readonly("LOOPBACK_RX_RX_QEC_INIT", &ad937x_ctrl::LOOPBACK_RX_RX_QEC_INIT)
        .def_readonly("RX_LO_DELAY", &ad937x_ctrl::RX_LO_DELAY)
        .def_readonly("RX_QEC_INIT", &ad937x_ctrl::RX_QEC_INIT)
        .def_readonly("DPD_INIT", &ad937x_ctrl::DPD_INIT)
        .def_readonly("CLGC_INIT", &ad937x_ctrl::CLGC_INIT)
        .def_readonly("VSWR_INIT", &ad937x_ctrl::VSWR_INIT)
        .def_readonly("TRACK_RX1_QEC", &ad937x_ctrl::TRACK_RX1_QEC)
        .def_readonly("TRACK_RX2_QEC", &ad937x_ctrl::TRACK_RX2_QEC)
        .def_readonly("TRACK_ORX1_QEC", &ad937x_ctrl::TRACK_ORX1_QEC)
        .def_readonly("TRACK_ORX2_QEC", &ad937x_ctrl::TRACK_ORX2_QEC)
        .def_readonly("TRACK_TX1_LOL", &ad937x_ctrl::TRACK_TX1_LOL)
        .def_readonly("TRACK_TX2_LOL", &ad937x_ctrl::TRACK_TX2_LOL)
        .def_readonly("TRACK_TX1_QEC", &ad937x_ctrl::TRACK_TX1_QEC)
        .def_readonly("TRACK_TX2_QEC", &ad937x_ctrl::TRACK_TX2_QEC)
        .def_readonly("TRACK_TX1_DPD", &ad937x_ctrl::TRACK_TX1_DPD)
        .def_readonly("TRACK_TX2_DPD", &ad937x_ctrl::TRACK_TX2_DPD)
        .def_readonly("TRACK_TX1_CLGC", &ad937x_ctrl::TRACK_TX1_CLGC)
        .def_readonly("TRACK_TX2_CLGC", &ad937x_ctrl::TRACK_TX2_CLGC)
        .def_readonly("TRACK_TX1_VSWR", &ad937x_ctrl::TRACK_TX1_VSWR)
        .def_readonly("TRACK_TX2_VSWR", &ad937x_ctrl::TRACK_TX2_VSWR)
        .def_readonly("TRACK_ORX1_QEC_SNLO", &ad937x_ctrl::TRACK_ORX1_QEC_SNLO)
        .def_readonly("TRACK_ORX2_QEC_SNLO", &ad937x_ctrl::TRACK_ORX2_QEC_SNLO)
        .def_readonly("TRACK_SRX_QEC", &ad937x_ctrl::TRACK_SRX_QEC)
        .def_readonly("DEFAULT_INIT_CALS_MASKS", &ad937x_ctrl::DEFAULT_INIT_CALS_MASKS)
        .def_readonly("DEFAULT_TRACKING_CALS_MASKS", &ad937x_ctrl::DEFAULT_TRACKING_CALS_MASKS)
        .def_readonly("DEFAULT_INIT_CALS_TIMEOUT", &ad937x_ctrl::DEFAULT_INIT_CALS_TIMEOUT)
        ;
}
#endif

