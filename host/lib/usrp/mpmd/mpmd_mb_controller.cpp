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

mpmd_mb_controller::mpmd_mb_controller(
    uhd::rpc_client::sptr rpcc, uhd::device_addr_t device_info)
    : _rpc(rpcc), _device_info(device_info)
{
    const size_t num_tks = _rpc->request_with_token<size_t>("get_num_timekeepers");
    for (size_t tk_idx = 0; tk_idx < num_tks; tk_idx++) {
        register_timekeeper(tk_idx, std::make_shared<mpmd_timekeeper>(tk_idx, _rpc));
    }

    // Enumerate sensors
    auto sensor_list =
        _rpc->request_with_token<std::vector<std::string>>("get_mb_sensors");
    UHD_LOG_DEBUG("MPMD", "Found " << sensor_list.size() << " motherboard sensors.");
    _sensor_names.insert(sensor_list.cbegin(), sensor_list.cend());

    // Enumerate GPIO banks that are under mb_controller control
    _gpio_banks = _rpc->request_with_token<std::vector<std::string>>("get_gpio_banks");
    for (const auto& bank : _gpio_banks) {
        _gpio_srcs.insert({bank,
            _rpc->request_with_token<std::vector<std::string>>("get_gpio_srcs", bank)});
    }
}

/******************************************************************************
 * Timekeeper API
 *****************************************************************************/
uint64_t mpmd_mb_controller::mpmd_timekeeper::get_ticks_now()
{
    return _rpc->request_with_token<uint64_t>("get_timekeeper_time", _tk_idx, false);
}

uint64_t mpmd_mb_controller::mpmd_timekeeper::get_ticks_last_pps()
{
    return _rpc->request_with_token<uint64_t>("get_timekeeper_time", _tk_idx, true);
}

void mpmd_mb_controller::mpmd_timekeeper::set_ticks_now(const uint64_t ticks)
{
    _rpc->notify_with_token("set_timekeeper_time", _tk_idx, ticks, false);
}

void mpmd_mb_controller::mpmd_timekeeper::set_ticks_next_pps(const uint64_t ticks)
{
    _rpc->notify_with_token("set_timekeeper_time", _tk_idx, ticks, true);
}

void mpmd_mb_controller::mpmd_timekeeper::set_period(const uint64_t period_ns)
{
    _rpc->notify_with_token("set_tick_period", _tk_idx, period_ns);
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
    _rpc->notify_with_token(MPMD_DEFAULT_LONG_TIMEOUT, "set_time_source", source);
}

std::string mpmd_mb_controller::get_time_source() const
{
    return _rpc->request_with_token<std::string>("get_time_source");
}

std::vector<std::string> mpmd_mb_controller::get_time_sources() const
{
    return _rpc->request_with_token<std::vector<std::string>>("get_time_sources");
}

void mpmd_mb_controller::set_clock_source(const std::string& source)
{
    _rpc->notify_with_token(MPMD_DEFAULT_LONG_TIMEOUT, "set_clock_source", source);
}

std::string mpmd_mb_controller::get_clock_source() const
{
    return _rpc->request_with_token<std::string>("get_clock_source");
}

std::vector<std::string> mpmd_mb_controller::get_clock_sources() const
{
    return _rpc->request_with_token<std::vector<std::string>>("get_clock_sources");
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
    _rpc->notify_with_token(
        MPMD_DEFAULT_LONG_TIMEOUT, "set_clock_source", sync_source_map);
}

device_addr_t mpmd_mb_controller::get_sync_source() const
{
    const auto sync_source_map =
        _rpc->request_with_token<std::map<std::string, std::string>>("get_sync_source");
    return device_addr_t(sync_source_map);
}

std::vector<device_addr_t> mpmd_mb_controller::get_sync_sources()
{
    std::vector<device_addr_t> result;
    const auto sync_sources =
        _rpc->request_with_token<std::vector<std::map<std::string, std::string>>>(
            "get_sync_sources");
    for (auto& sync_source : sync_sources) {
        result.push_back(device_addr_t(sync_source));
    }

    return result;
}

void mpmd_mb_controller::set_clock_source_out(const bool /*enb*/)
{
    throw uhd::not_implemented_error(
        "set_clock_source_out() not implemented on this device!");
}

void mpmd_mb_controller::set_time_source_out(const bool /*enb*/)
{
    throw uhd::not_implemented_error(
        "set_time_source_out() not implemented on this device!");
}

sensor_value_t mpmd_mb_controller::get_sensor(const std::string& name)
{
    if (!_sensor_names.count(name)) {
        throw uhd::key_error(std::string("Invalid motherboard sensor name: ") + name);
    }
    return sensor_value_t(
        _rpc->request_with_token<sensor_value_t::sensor_map_t>("get_mb_sensor", name));
}

std::vector<std::string> mpmd_mb_controller::get_sensor_names()
{
    std::vector<std::string> sensor_names(_sensor_names.cbegin(), _sensor_names.cend());
    return sensor_names;
}

uhd::usrp::mboard_eeprom_t mpmd_mb_controller::get_eeprom()
{
    auto mb_eeprom =
        _rpc->request_with_token<std::map<std::string, std::string>>("get_mb_eeprom");
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
    return _rpc->request_with_token<std::vector<std::string>>("get_gpio_src", bank);
}

void mpmd_mb_controller::set_gpio_src(
    const std::string& bank, const std::vector<std::string>& src)
{
    if (!_gpio_srcs.count(bank)) {
        UHD_LOG_ERROR("MPMD", "Invalid GPIO bank: `" << bank << "'");
        throw uhd::key_error(std::string("Invalid GPIO bank: ") + bank);
    }
    _rpc->notify_with_token("set_gpio_src", bank, src);
}
