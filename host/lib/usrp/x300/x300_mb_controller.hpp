//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_X300_MB_CONTROLLER_HPP
#define INCLUDED_LIBUHD_X300_MB_CONTROLLER_HPP

#include "x300_clock_ctrl.hpp"
#include "x300_device_args.hpp"
#include "x300_radio_mbc_iface.hpp"
#include "x300_regs.hpp"
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhdlib/features/discoverable_feature_registry.hpp>
#include <unordered_set>
#include <string>

namespace uhd { namespace rfnoc {

/*! X300-Specific version of the mb_controller
 *
 * Reminder: There is one of these per motherboard.
 *
 * The X300 motherboard controller is responsible for:
 * - Controlling the timekeeper
 * - Controlling all time- and clock-related settings
 * - Initialize and hold the GPS control
 */
class x300_mb_controller : public mb_controller,
                           public ::uhd::features::discoverable_feature_registry
{
public:
    /**************************************************************************
     * Structors
     *************************************************************************/
    x300_mb_controller(const size_t hw_rev,
        const std::string product_name,
        uhd::i2c_iface::sptr zpu_i2c,
        uhd::wb_iface::sptr zpu_ctrl,
        x300_clock_ctrl::sptr clock_ctrl,
        uhd::usrp::mboard_eeprom_t mb_eeprom,
        uhd::usrp::x300::x300_device_args_t args);

    ~x300_mb_controller() override;

    /**************************************************************************
     * X300-Specific APIs
     *************************************************************************/
    //! Return reference to the ZPU-owned I2C controller
    uhd::i2c_iface::sptr get_zpu_i2c()
    {
        return _zpu_i2c;
    }

    //! Reference to the ZPU peek/poke interface
    uhd::wb_iface::sptr get_zpu_ctrl()
    {
        return _zpu_ctrl;
    }

    //! Return reference to LMK clock controller
    x300_clock_ctrl::sptr get_clock_ctrl()
    {
        return _clock_ctrl;
    }

    void register_reset_codec_cb(std::function<void(void)>&& reset_cb)
    {
        _reset_cbs.push_back(std::move(reset_cb));
    }

    void set_initialization_done()
    {
        _initialization_done = true;
    }

    void register_radio(uhd::usrp::x300::x300_radio_mbc_iface* radio)
    {
        _radio_refs.push_back(radio);
    }

    /**************************************************************************
     * Timekeeper API
     *************************************************************************/
    //! X300-specific version of the timekeeper controls
    //
    // The X300 controls timekeepers via the ZPU
    class x300_timekeeper : public mb_controller::timekeeper
    {
    public:
        x300_timekeeper(
            const size_t tk_idx, uhd::wb_iface::sptr zpu_ctrl, const double tick_rate)
            : _tk_idx(tk_idx), _zpu_ctrl(zpu_ctrl)
        {
            set_tick_rate(tick_rate);
        }
        uint64_t get_ticks_now() override;
        uint64_t get_ticks_last_pps() override;
        void set_ticks_now(const uint64_t ticks) override;
        void set_ticks_next_pps(const uint64_t ticks) override;
        void set_period(const uint64_t period_ns) override;

    private:
        uint32_t get_tk_addr(const uint32_t tk_addr);
        const size_t _tk_idx;
        uhd::wb_iface::sptr _zpu_ctrl;
    }; /* x300_timekeeper */

    /**************************************************************************
     * Motherboard Control API (see mb_controller.hpp)
     *************************************************************************/
    void init() override;
    std::string get_mboard_name() const override;
    void set_time_source(const std::string& source) override;
    std::string get_time_source() const override;
    std::vector<std::string> get_time_sources() const override;
    void set_clock_source(const std::string& source) override;
    std::string get_clock_source() const override;
    std::vector<std::string> get_clock_sources() const override;
    void set_sync_source(
        const std::string& clock_source, const std::string& time_source) override;
    void set_sync_source(const device_addr_t& sync_source) override;
    device_addr_t get_sync_source() const override;
    std::vector<device_addr_t> get_sync_sources() override;
    void set_clock_source_out(const bool enb) override;
    void set_time_source_out(const bool enb) override;
    sensor_value_t get_sensor(const std::string& name) override;
    std::vector<std::string> get_sensor_names() override;
    uhd::usrp::mboard_eeprom_t get_eeprom() override;
    bool synchronize(std::vector<mb_controller::sptr>& mb_controllers,
        const uhd::time_spec_t& time_spec = uhd::time_spec_t(0.0),
        const bool quiet                  = false) override;
    std::vector<std::string> get_gpio_banks() const override;
    std::vector<std::string> get_gpio_srcs(const std::string&) const override;
    std::vector<std::string> get_gpio_src(const std::string&) override;
    void set_gpio_src(const std::string&, const std::vector<std::string>&) override;

private:
    //! Return a string X300::MB_CTRL#N
    std::string get_unique_id();

    //! Init GPS
    void init_gps();

    //! Reset all registered DACs and ADCs
    void reset_codecs();

    //! Wait until reference clock locks, or a timeout occurs
    //
    // \returns lock status
    bool wait_for_clk_locked(uint32_t which, double timeout);

    //! Returns true if a PPS signal is detected
    bool is_pps_present();

    //! Return LMK lock status
    bool get_ref_locked();

    /*! Calibrate the ADC transfer delay
     *
     * This will try various clock delay settings to the ADC, and pick the one
     * with the best BER performance.
     */
    void self_cal_adc_xfer_delay(const bool apply_delay);

    void extended_adc_test(double duration_s);

    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Hardware revision
    const size_t _hw_rev;

    //! Product name (X310, X300)
    const std::string _product_name;

    //! Reference to the ZPU-owned I2C controller
    uhd::i2c_iface::sptr _zpu_i2c;

    //! Reference to the ZPU peek/poke interface
    uhd::wb_iface::sptr _zpu_ctrl;

    //! Reference to LMK clock controller
    x300_clock_ctrl::sptr _clock_ctrl;

    //! State of the MB EEPROM
    uhd::usrp::mboard_eeprom_t _mb_eeprom;

    //! Copy of the device args
    uhd::usrp::x300::x300_device_args_t _args;

    //! Reference to clock control register
    uhd::usrp::x300::fw_regmap_t::sptr _fw_regmap;

    //! Reference to GPS control
    uhd::gps_ctrl::sptr _gps;

    //! Reference to all callbacks to reset the ADCs/DACs
    std::vector<std::function<void(void)>> _reset_cbs;

    //! Current clock source (external, internal, gpsdo)
    std::string _current_refclk_src;

    //! Current time source (external, internal, gpsdo)
    std::string _current_time_src;

    //! Reference to radios on this motherboard
    std::vector<uhd::usrp::x300::x300_radio_mbc_iface*> _radio_refs;

    //! List of available sensors
    std::unordered_set<std::string> _sensors{"ref_locked", "temp_fpga"};

    //! Flag to tell us if initialization is complete. Some functions behave
    // differently after initialization.
    bool _initialization_done = false;
};

}} // namespace uhd::rfnoc

#endif /* INCLUDED_LIBUHD_X300_MB_CONTROLLER_HPP */
