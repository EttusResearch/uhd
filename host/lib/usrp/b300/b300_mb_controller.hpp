//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "b300_clock_ctrl.hpp"
#include "b300_gps_control.hpp"
#include "b300_mb_eeprom.hpp"
#include "b300_regs.hpp"
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhdlib/features/discoverable_feature_registry.hpp>
#include <uhdlib/usrp/common/ina231.hpp>
#include <uhdlib/usrp/common/tmp468.hpp>
#include <uhdlib/usrp/cores/i2c_core_100_wb32.hpp>
#include <string>

namespace uhd { namespace rfnoc {

constexpr char B300_GPIO_SRC_BANK[]     = "FP0";
constexpr char B300_GPIO_SRC_RF0[]      = "RF0";
constexpr char B300_GPIO_SRC_RF1[]      = "RF1";
constexpr size_t B300_GPIO_SRC_NUM_PINS = 10;

class b300_mb_controller : public mb_controller,
                           public ::uhd::features::discoverable_feature_registry
{
public:
    /**************************************************************************
     * Structors
     *************************************************************************/
    b300_mb_controller(uhd::usrp::b300::b300_clock_ctrl::sptr clock_ctrl,
        uhd::usrp::b300::bar0_regmap_t::sptr bar0_regmap,
        uhd::usrp::b300::b300_gps_control::sptr gps_ctrl,
        i2c_core_100_wb32::sptr i2c_core,
        const uhd::device_addr_t& dev_args,
        const double master_clock_rate,
        const uint16_t board_rev,
        uhd::usrp::mboard_eeprom_t mb_eeprom);

    ~b300_mb_controller() override;

    using sptr = std::shared_ptr<b300_mb_controller>;

    /**************************************************************************
     * Timekeeper API
     *************************************************************************/
    class b300_timekeeper : public mb_controller::timekeeper
    {
    public:
        b300_timekeeper(size_t tk_idx,
            uhd::usrp::b300::bar0_regmap_t::sptr bar0_regmap,
            double tick_rate)
            : _tk_idx(tk_idx), _bar0_regmap(bar0_regmap), _tick_rate(tick_rate)
        {
            set_tick_rate(tick_rate);
        }

        uint64_t get_ticks_now() override;
        uint64_t get_ticks_last_pps() override;
        void set_ticks_now(const uint64_t ticks) override;
        void set_ticks_next_pps(const uint64_t ticks) override;
        void set_period(const uint64_t period_ns) override;

    private:
        size_t _tk_idx;
        uhd::usrp::b300::bar0_regmap_t::sptr _bar0_regmap;
        double _tick_rate;
    };

    /**************************************************************************
     * Motherboard Control API
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
    std::vector<std::string> get_gpio_srcs(const std::string& bank) const override;
    std::vector<std::string> get_gpio_src(const std::string& bank) override;
    void set_gpio_src(
        const std::string& bank, const std::vector<std::string>& src) override;

    uhd::usrp::b300::b300_clock_ctrl::sptr get_clock_ctrl();
    void setup_multi_device_sync();
    void configure_lmk_for_sync();
    void finish_multi_device_sync();

private:
    uhd::soft_reg_field_t _get_gpio_field(size_t index);
    bool _check_gps_pps_present();

    uhd::usrp::b300::b300_clock_ctrl::sptr _b300_clock_ctrl;
    uhd::usrp::b300::bar0_regmap_t::sptr _bar0_regmap;
    uhd::usrp::b300::b300_gps_control::sptr _b300_gps_ctrl;
    std::string _current_clock_source;
    std::string _current_time_source;
    tmp468_iface::sptr _temp_sensor;
    ina231_iface::sptr _power_monitor;
    std::vector<std::string> _gpio_srcs;
    uint16_t _board_rev;
    uhd::usrp::mboard_eeprom_t _mb_eeprom;
    double _pcie_max_speed_gtps;
    double _pcie_neg_speed_gtps;
    // PCIe link widths expressed as lane counts (e.g., 1, 4, 8 for x1/x4/x8).
    uint16_t _pcie_max_width;
    uint16_t _pcie_neg_width;
};

}} // namespace uhd::rfnoc
