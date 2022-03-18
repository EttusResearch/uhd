//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/mpmd_mb_controller.hpp>

using namespace uhd::rfnoc;
using namespace uhd;

namespace {
//! Default timeout value for RPC calls that we know can take long (ms)
constexpr size_t MPMD_DEFAULT_LONG_TIMEOUT = 30000; // ms
} // namespace

mpmd_mb_controller::fpga_onload::fpga_onload()
{}

void mpmd_mb_controller::fpga_onload::onload()
{
    for (auto& cb : _cbs)
    {
        if (auto spt = cb.lock())
        {
            spt->onload();
        }
    }
}

void mpmd_mb_controller::fpga_onload::request_cb(uhd::features::fpga_load_notification_iface::sptr handler)
{
    _cbs.emplace_back(handler);
}

mpmd_mb_controller::ref_clk_calibration::ref_clk_calibration(uhd::usrp::mpmd_rpc_iface::sptr rpcc)
    : _rpcc(rpcc)
{}

void mpmd_mb_controller::ref_clk_calibration::set_ref_clk_tuning_word(uint32_t tuning_word)
{
    _rpcc->set_ref_clk_tuning_word(tuning_word);
}

uint32_t mpmd_mb_controller::ref_clk_calibration::get_ref_clk_tuning_word()
{
    return _rpcc->get_ref_clk_tuning_word();
}

void mpmd_mb_controller::ref_clk_calibration::store_ref_clk_tuning_word(uint32_t tuning_word)
{
    _rpcc->store_ref_clk_tuning_word(tuning_word);
}

mpmd_mb_controller::trig_io_mode::trig_io_mode(uhd::usrp::mpmd_rpc_iface::sptr rpcc)
    : _rpcc(rpcc)
{
}

void mpmd_mb_controller::trig_io_mode::set_trig_io_mode(const uhd::trig_io_mode_t mode)
{
    switch (mode) {
        case uhd::trig_io_mode_t::PPS_OUTPUT:
            _rpcc->set_trigger_io("pps_output");
            break;
        case uhd::trig_io_mode_t::INPUT:
            _rpcc->set_trigger_io("input");
            break;
        case uhd::trig_io_mode_t::OFF:
            _rpcc->set_trigger_io("off");
            break;
        default:
            throw uhd::value_error("set_trig_io_mode: Requested mode is invalid.");
    }
}

mpmd_mb_controller::gpio_power::gpio_power(
    uhd::usrp::dio_rpc_iface::sptr rpcc, const std::vector<std::string>& ports)
    : _rpcc(rpcc), _ports(ports)
{}

std::vector<std::string> mpmd_mb_controller::gpio_power::get_supported_voltages(
    const std::string& port) const
{
    return _rpcc->dio_get_supported_voltage_levels(port);
}

void mpmd_mb_controller::gpio_power::set_port_voltage(
    const std::string& port, const std::string& voltage)
{
    _rpcc->dio_set_voltage_level(port, voltage);
}

std::string mpmd_mb_controller::gpio_power::get_port_voltage(
    const std::string& port) const
{
    return _rpcc->dio_get_voltage_level(port);
}

void mpmd_mb_controller::gpio_power::set_external_power(
    const std::string& port, bool enable)
{
    _rpcc->dio_set_external_power(port, enable);
}

std::string mpmd_mb_controller::gpio_power::get_external_power_status(
    const std::string& port) const
{
    return _rpcc->dio_get_external_power_state(port);
}

mpmd_mb_controller::mpmd_mb_controller(
    uhd::usrp::mpmd_rpc_iface::sptr rpcc, uhd::device_addr_t device_info)
    : _rpc(rpcc), _device_info(device_info)
{
    const size_t num_tks = _rpc->get_num_timekeepers();
    for (size_t tk_idx = 0; tk_idx < num_tks; tk_idx++) {
        register_timekeeper(tk_idx, std::make_shared<mpmd_timekeeper>(tk_idx, _rpc));
    }

    // Enumerate sensors
    auto sensor_list = _rpc->get_mb_sensors();
    UHD_LOG_DEBUG("MPMD", "Found " << sensor_list.size() << " motherboard sensors.");
    _sensor_names.insert(sensor_list.cbegin(), sensor_list.cend());

    // Enumerate GPIO banks that are under mb_controller control
    _gpio_banks = _rpc->get_gpio_banks();
    for (const auto& bank : _gpio_banks) {
        _gpio_srcs.insert({bank, _rpc->get_gpio_srcs(bank)});
    }

    _fpga_onload = std::make_shared<fpga_onload>();
    register_feature(_fpga_onload);

    if (_rpc->supports_feature("ref_clk_calibration")) {
        _ref_clk_cal = std::make_shared<ref_clk_calibration>(_rpc);
        register_feature(_ref_clk_cal);
    }

    if (_rpc->supports_feature("trig_io_mode")) {
        _trig_io_mode = std::make_shared<trig_io_mode>(_rpc);
        register_feature(_trig_io_mode);
    }

    if (_rpc->supports_feature("gpio_power")) {
        _gpio_power = std::make_shared<gpio_power>(
            std::make_shared<uhd::usrp::dio_rpc>(get_rpc_client()), _gpio_banks);
        register_feature(_gpio_power);
    }
}

/******************************************************************************
 * Timekeeper API
 *****************************************************************************/
uint64_t mpmd_mb_controller::mpmd_timekeeper::get_ticks_now()
{
    return _rpc->get_timekeeper_time(_tk_idx, false);
}

uint64_t mpmd_mb_controller::mpmd_timekeeper::get_ticks_last_pps()
{
    return _rpc->get_timekeeper_time(_tk_idx, true);
}

void mpmd_mb_controller::mpmd_timekeeper::set_ticks_now(const uint64_t ticks)
{
    _rpc->set_timekeeper_time(_tk_idx, ticks, false);
}

void mpmd_mb_controller::mpmd_timekeeper::set_ticks_next_pps(const uint64_t ticks)
{
    _rpc->set_timekeeper_time(_tk_idx, ticks, true);
}

void mpmd_mb_controller::mpmd_timekeeper::set_period(const uint64_t period_ns)
{
    _rpc->set_tick_period(_tk_idx, period_ns);
}

void mpmd_mb_controller::mpmd_timekeeper::update_tick_rate(const double tick_rate)
{
    set_tick_rate(tick_rate);
}

/******************************************************************************
 * Motherboard Control API (see mb_controller.hpp)
 *****************************************************************************/
std::string mpmd_mb_controller::get_mboard_name() const
{
    return _device_info.get("product", "UNKNOWN");
}

void mpmd_mb_controller::set_time_source(const std::string& source)
{
    _rpc->get_raw_rpc_client()->notify_with_token(MPMD_DEFAULT_LONG_TIMEOUT, "set_time_source", source);
    if (!_sync_source_updaters.empty()) {
        mb_controller::sync_source_t sync_source;
        sync_source["time_source"] = source;
        for (const auto& updater : _sync_source_updaters) {
            updater(sync_source);
        }
    }
}

std::string mpmd_mb_controller::get_time_source() const
{
    return _rpc->get_time_source();
}

std::vector<std::string> mpmd_mb_controller::get_time_sources() const
{
    return _rpc->get_time_sources();
}

void mpmd_mb_controller::set_clock_source(const std::string& source)
{
    _rpc->get_raw_rpc_client()->notify_with_token(MPMD_DEFAULT_LONG_TIMEOUT, "set_clock_source", source);
    if (!_sync_source_updaters.empty()) {
        mb_controller::sync_source_t sync_source;
        sync_source["clock_source"] = source;
        for (const auto& updater : _sync_source_updaters) {
            updater(sync_source);
        }
    }
}

std::string mpmd_mb_controller::get_clock_source() const
{
    return _rpc->get_clock_source();
}

std::vector<std::string> mpmd_mb_controller::get_clock_sources() const
{
    return _rpc->get_clock_sources();
}

void mpmd_mb_controller::set_sync_source(
    const std::string& clock_source, const std::string& time_source)
{
    uhd::device_addr_t sync_source;
    sync_source["clock_source"] = clock_source;
    sync_source["time_source"]  = time_source;
    set_sync_source(sync_source);
}

void mpmd_mb_controller::set_sync_source(const device_addr_t& sync_source)
{
    std::map<std::string, std::string> sync_source_map;
    for (const auto& key : sync_source.keys()) {
        sync_source_map[key] = sync_source.get(key);
    }
    _rpc->get_raw_rpc_client()->notify_with_token(
        MPMD_DEFAULT_LONG_TIMEOUT, "set_sync_source", sync_source_map);
    if (!_sync_source_updaters.empty()) {
        for (const auto& updater : _sync_source_updaters) {
            updater(sync_source);
        }
    }
}

device_addr_t mpmd_mb_controller::get_sync_source() const
{
    const auto sync_source_map = _rpc->get_sync_source();
    return device_addr_t(sync_source_map);
}

std::vector<device_addr_t> mpmd_mb_controller::get_sync_sources()
{
    std::vector<device_addr_t> result;
    const auto sync_sources = _rpc->get_sync_sources();
    for (auto& sync_source : sync_sources) {
        result.push_back(device_addr_t(sync_source));
    }

    return result;
}

void mpmd_mb_controller::set_clock_source_out(const bool enb)
{
    _rpc->set_clock_source_out(enb);
}

void mpmd_mb_controller::set_time_source_out(const bool enb)
{
    if (_rpc->supports_feature("time_export"))
    {
        _rpc->set_trigger_io(enb ? "pps_output" : "off");
    }
    else
    {
        throw uhd::not_implemented_error(
            "set_time_source_out() not implemented on this device!");
    }
}

sensor_value_t mpmd_mb_controller::get_sensor(const std::string& name)
{
    if (!_sensor_names.count(name)) {
        throw uhd::key_error(std::string("Invalid motherboard sensor name: ") + name);
    }
    return sensor_value_t(_rpc->get_mb_sensor(name));
}

std::vector<std::string> mpmd_mb_controller::get_sensor_names()
{
    std::vector<std::string> sensor_names(_sensor_names.cbegin(), _sensor_names.cend());
    return sensor_names;
}

uhd::usrp::mboard_eeprom_t mpmd_mb_controller::get_eeprom()
{
    auto mb_eeprom = _rpc->get_mb_eeprom();
    uhd::usrp::mboard_eeprom_t mb_eeprom_dict(mb_eeprom.cbegin(), mb_eeprom.cend());
    return mb_eeprom_dict;
}

std::vector<std::string> mpmd_mb_controller::get_gpio_banks() const
{
    return _gpio_banks;
}

std::vector<std::string> mpmd_mb_controller::get_gpio_srcs(const std::string& bank) const
{
    if (!_gpio_srcs.count(bank)) {
        UHD_LOG_ERROR("MPMD", "Invalid GPIO bank: `" << bank << "'");
        throw uhd::key_error(std::string("Invalid GPIO bank: ") + bank);
    }
    return _gpio_srcs.at(bank);
}

std::vector<std::string> mpmd_mb_controller::get_gpio_src(const std::string& bank)
{
    if (!_gpio_srcs.count(bank)) {
        UHD_LOG_ERROR("MPMD", "Invalid GPIO bank: `" << bank << "'");
        throw uhd::key_error(std::string("Invalid GPIO bank: ") + bank);
    }
    if (_current_gpio_src.count(bank)) {
        return _current_gpio_src[bank];
    } else {
        return _rpc->get_gpio_src(bank);
    }
}

void mpmd_mb_controller::set_gpio_src(
    const std::string& bank, const std::vector<std::string>& src)
{
    if (!_gpio_srcs.count(bank)) {
        UHD_LOG_ERROR("MPMD", "Invalid GPIO bank: `" << bank << "'");
        throw uhd::key_error(std::string("Invalid GPIO bank: ") + bank);
    }
    _rpc->set_gpio_src(bank, src);
    _current_gpio_src[bank] = src;
}

void mpmd_mb_controller::register_sync_source_updater(
    mb_controller::sync_source_updater_t callback_f)
{
    _sync_source_updaters.push_back(callback_f);
}
