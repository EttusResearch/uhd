//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/features/gpio_power_iface.hpp>
#include <uhd/features/ref_clk_calibration_iface.hpp>
#include <uhd/features/trig_io_mode_iface.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhdlib/features/discoverable_feature_registry.hpp>
#include <uhdlib/features/fpga_load_notification_iface.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <map>
#include <memory>

namespace uhd { namespace rfnoc {

/*! MPM-Specific version of the mb_controller
 *
 * Reminder: There is one of these per motherboard.
 *
 * This motherboard controller abstracts out a bunch of RPC calls.
 */
class mpmd_mb_controller : public mb_controller,
                           public ::uhd::features::discoverable_feature_registry
{
public:
    using sptr = std::shared_ptr<mpmd_mb_controller>;

    mpmd_mb_controller(uhd::usrp::mpmd_rpc_iface::sptr rpcc, uhd::device_addr_t device_info);

    //! Return reference to the RPC client
    uhd::rpc_client::sptr get_rpc_client()
    {
        return _rpc->get_raw_rpc_client();
    }

    template<typename T>
    std::shared_ptr<T> dynamic_cast_rpc_as()
    {
        return std::dynamic_pointer_cast<T>(_rpc);
    }

    /**************************************************************************
     * Timekeeper API
     *************************************************************************/
    //! MPM-specific version of the timekeeper controls
    //
    // MPM devices talk to MPM via RPC to control the timekeeper
    class mpmd_timekeeper : public mb_controller::timekeeper
    {
    public:
        using sptr = std::shared_ptr<mpmd_timekeeper>;

        mpmd_timekeeper(const size_t tk_idx, uhd::usrp::mpmd_rpc_iface::sptr rpc_client)
            : _tk_idx(tk_idx), _rpc(rpc_client)
        {
            // nop
        }

        uint64_t get_ticks_now() override;
        uint64_t get_ticks_last_pps() override;
        void set_ticks_now(const uint64_t ticks) override;
        void set_ticks_next_pps(const uint64_t ticks) override;
        void set_period(const uint64_t period_ns) override;

        /*! Update the tick rate
         *  Note: This is separate from set_tick_rate because the latter is
         *  protected, and we need to implement mpmd-specific functionality here
         */
        void update_tick_rate(const double tick_rate);

    private:
        const size_t _tk_idx;
        uhd::usrp::mpmd_rpc_iface::sptr _rpc;
    };

    /**************************************************************************
     * Motherboard Control API (see mb_controller.hpp)
     *************************************************************************/
    std::string get_mboard_name() const override;
    void set_time_source(const std::string& source) override;
    std::string get_time_source() const override;
    std::vector<std::string> get_time_sources() const override;
    void set_clock_source(const std::string& source) override;
    std::string get_clock_source() const override;
    std::vector<std::string> get_clock_sources() const override;
    void set_sync_source(
        const std::string& clock_source, const std::string& time_source) override;
    void set_sync_source(const uhd::device_addr_t& sync_source) override;
    uhd::device_addr_t get_sync_source() const override;
    std::vector<uhd::device_addr_t> get_sync_sources() override;
    void set_clock_source_out(const bool enb) override;
    void set_time_source_out(const bool enb) override;
    uhd::sensor_value_t get_sensor(const std::string& name) override;
    std::vector<std::string> get_sensor_names() override;
    uhd::usrp::mboard_eeprom_t get_eeprom() override;
    std::vector<std::string> get_gpio_banks() const override;
    std::vector<std::string> get_gpio_srcs(const std::string& bank) const override;
    std::vector<std::string> get_gpio_src(const std::string& bank) override;
    void set_gpio_src(
        const std::string& bank, const std::vector<std::string>& src) override;
    void register_sync_source_updater(
        mb_controller::sync_source_updater_t callback_f) override;

private:
    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Reference to RPC interface
    mutable uhd::usrp::mpmd_rpc_iface::sptr _rpc;

    uhd::device_addr_t _device_info;

    //! List of MB sensor names
    std::unordered_set<std::string> _sensor_names;

    //! Cache of available GPIO sources
    std::vector<std::string> _gpio_banks;
    std::unordered_map<std::string, std::vector<std::string>> _gpio_srcs;

    //! Cache of currently set GPIO sources
    std::unordered_map<std::string, std::vector<std::string>> _current_gpio_src;

    std::vector<mb_controller::sync_source_updater_t> _sync_source_updaters;

public:
    /*! When the FPGA is reloaded, pass the notification to every Radio block
     *  Public to allow other classes to register for notifications.
     */
    class fpga_onload : public uhd::features::fpga_load_notification_iface {
    public:
        using sptr = std::shared_ptr<fpga_onload>;

        fpga_onload();

        void onload() override;

        void request_cb(uhd::features::fpga_load_notification_iface::sptr handler);

    private:
        std::vector<std::weak_ptr<uhd::features::fpga_load_notification_iface>> _cbs;
    };

    //! Class to expose the ref_clk_calibration discoverable feature functions.
    class ref_clk_calibration : public uhd::features::ref_clk_calibration_iface {
    public:
        using sptr = std::shared_ptr<ref_clk_calibration>;

        ref_clk_calibration(uhd::usrp::mpmd_rpc_iface::sptr rpcc);

        void set_ref_clk_tuning_word(uint32_t tuning_word) override;
        uint32_t get_ref_clk_tuning_word() override;
        void store_ref_clk_tuning_word(uint32_t tuning_word) override;

    private:
        uhd::usrp::mpmd_rpc_iface::sptr _rpcc;
    };

    class trig_io_mode : public uhd::features::trig_io_mode_iface
    {
    public:
        using sptr = std::shared_ptr<trig_io_mode>;

        trig_io_mode(uhd::usrp::mpmd_rpc_iface::sptr rpcc);

        void set_trig_io_mode(const uhd::trig_io_mode_t mode) override;

    private:
        uhd::usrp::mpmd_rpc_iface::sptr _rpcc;
    };

    class gpio_power : public uhd::features::gpio_power_iface
    {
    public:
        using sptr = std::shared_ptr<gpio_power>;

        gpio_power(uhd::usrp::dio_rpc_iface::sptr rpcc, const std::vector<std::string>& ports);

        std::vector<std::string> get_supported_voltages(const std::string& port) const override;
        void set_port_voltage(const std::string& port, const std::string& voltage) override;
        std::string get_port_voltage(const std::string& port) const override;
        void set_external_power(const std::string& port, bool enable) override;
        std::string get_external_power_status(const std::string& port) const override;

    private:
        uhd::usrp::dio_rpc_iface::sptr _rpcc;
        const std::vector<std::string> _ports;
    };

    fpga_onload::sptr _fpga_onload;
    ref_clk_calibration::sptr _ref_clk_cal;
    trig_io_mode::sptr _trig_io_mode;
    gpio_power::sptr _gpio_power;
};

}} // namespace uhd::rfnoc
