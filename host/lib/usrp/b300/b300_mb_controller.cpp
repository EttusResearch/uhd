//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_mb_controller.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/lmk05318.hpp>
#include <uhdlib/usrp/common/tmp468.hpp>
#include <algorithm>
#include <thread>

using namespace uhd::usrp::b300;

namespace uhd { namespace rfnoc {

/******************************************************************************
 * Structors
 *****************************************************************************/
b300_mb_controller::b300_mb_controller(b300_clock_ctrl::sptr clock_ctrl,
    bar0_regmap_t::sptr bar0_regmap,
    b300_gps_control::sptr gps_ctrl,
    i2c_core_100_wb32::sptr i2c_core,
    const uhd::device_addr_t& dev_args,
    const double master_clock_rate,
    const uint16_t board_rev,
    uhd::usrp::mboard_eeprom_t mb_eeprom)
    : _b300_clock_ctrl(clock_ctrl)
    , _bar0_regmap(bar0_regmap)
    , _b300_gps_ctrl(gps_ctrl)
    , _current_clock_source(B300_DEFAULT_CLOCK_SOURCE) // Clock Ctrl inits to internal
    , _current_time_source(B300_DEFAULT_TIME_SOURCE) // Clock Ctrl inits to internal
    , _gpio_srcs(B300_GPIO_SRC_NUM_PINS, B300_GPIO_SRC_RF0)
    , _board_rev(board_rev)
    , _mb_eeprom(std::move(mb_eeprom))
    , _pcie_max_speed_gtps(0.0)
    , _pcie_neg_speed_gtps(0.0)
    , _pcie_max_width(0)
    , _pcie_neg_width(0)
{
    const auto decode_speed_gtps = [](uint32_t code) {
        switch (code) {
            case 0:
                return 2.5;
            case 1:
                return 5.0;
            case 2:
                return 8.0;
            case 3:
                return 16.0;
            case 4:
                return 32.0;
            case 5:
                return 64.0;
            default:
                return 0.0;
        }
    };
    const auto decode_width_bits = [](uint32_t code) {
        return static_cast<uint16_t>(code);
    };

    const uint32_t pcie_max_speed_code =
        _bar0_regmap->hbtidr_reg.read(bar0_regmap_t::hbtidr_reg_t::HBMAXSPD);
    const uint32_t pcie_neg_speed_code =
        _bar0_regmap->hbtidr_reg.read(bar0_regmap_t::hbtidr_reg_t::HBNEGSPD);
    const uint32_t pcie_max_width_code =
        _bar0_regmap->hbtidr_reg.read(bar0_regmap_t::hbtidr_reg_t::HBMAXWDTH);
    const uint32_t pcie_neg_width_code =
        _bar0_regmap->hbtidr_reg.read(bar0_regmap_t::hbtidr_reg_t::HBNEGWDTH);

    _pcie_max_speed_gtps = decode_speed_gtps(pcie_max_speed_code);
    _pcie_neg_speed_gtps = decode_speed_gtps(pcie_neg_speed_code);
    _pcie_max_width      = decode_width_bits(pcie_max_width_code);
    _pcie_neg_width      = decode_width_bits(pcie_neg_width_code);

    if (_pcie_max_speed_gtps <= 0.0 || _pcie_neg_speed_gtps <= 0.0 || _pcie_max_width == 0
        || _pcie_neg_width == 0) {
        throw uhd::runtime_error(
            "Invalid PCIe link info in HBTIDR register: max_speed_code="
            + std::to_string(pcie_max_speed_code)
            + ", negotiated_speed_code=" + std::to_string(pcie_neg_speed_code)
            + ", max_width_code=" + std::to_string(pcie_max_width_code)
            + ", negotiated_width_code=" + std::to_string(pcie_neg_width_code));
    }

    if (_pcie_neg_speed_gtps < _pcie_max_speed_gtps
        || _pcie_neg_width < _pcie_max_width) {
        UHD_LOG_WARNING("B300",
            "PCIe link negotiated below capability: "
                << "speed " << _pcie_neg_speed_gtps << " GT/s"
                << " (max " << _pcie_max_speed_gtps << " GT/s), "
                << "width x" << _pcie_neg_width << " (max x" << _pcie_max_width << ").");
    }

    _b300_clock_ctrl->init();
    _b300_clock_ctrl->config_lmk04832(master_clock_rate);
    _bar0_regmap->int_pps_divider_reg.write(
        bar0_regmap_t::int_pps_divider_reg_t::INT_PPS_DIV,
        static_cast<uint32_t>(master_clock_rate));

    if (dev_args.has_key("clock_source") || dev_args.has_key("time_source")) {
        set_sync_source(dev_args);
    } else {
        set_sync_source(B300_DEFAULT_CLOCK_SOURCE, B300_DEFAULT_TIME_SOURCE);
    }

    const size_t num_tks = bar0_regmap->num_timekeepers_reg.read(
        bar0_regmap_t::num_timekeepers_reg_t::NUM_TIMEKEEPERS);
    for (size_t i = 0; i < num_tks; i++) {
        register_timekeeper(
            i, std::make_shared<b300_timekeeper>(i, _bar0_regmap, master_clock_rate));
    };

    _temp_sensor = tmp468_iface::make(i2c_core, I2C_TMP_SENSOR);
    // Ideality factor value 1.01 sourced from AMD Kintex 7 Data Sheet
    _temp_sensor->set_ideality_factor(tmp468_iface::REMOTE_SENSOR8, 1.01);

    _power_monitor = ina231_iface::make(i2c_core, I2C_PWR_MONITOR, 0.01, 4);
}

b300_mb_controller::~b300_mb_controller() {}

/******************************************************************************
 * Timekeeper APIs
 *****************************************************************************/
uint64_t b300_mb_controller::b300_timekeeper::get_ticks_now()
{
    uint32_t ticks_lo = _bar0_regmap->tk_ticks_now_lo_reg.read(
        bar0_regmap_t::tk_ticks_now_lo_reg_t::TICKS_NOW_LO);
    uint32_t ticks_hi = _bar0_regmap->tk_ticks_now_hi_reg.read(
        bar0_regmap_t::tk_ticks_now_hi_reg_t::TICKS_NOW_HI);
    return uint64_t(ticks_lo) | (uint64_t(ticks_hi) << 32);
}

uint64_t b300_mb_controller::b300_timekeeper::get_ticks_last_pps()
{
    uint32_t ticks_lo = _bar0_regmap->tk_ticks_pps_lo_reg.read(
        bar0_regmap_t::tk_ticks_pps_lo_reg_t::TICKS_PPS_LO);
    uint32_t ticks_hi = _bar0_regmap->tk_ticks_pps_hi_reg.read(
        bar0_regmap_t::tk_ticks_pps_hi_reg_t::TICKS_PPS_HI);
    return uint64_t(ticks_lo) | (uint64_t(ticks_hi) << 32);
}

void b300_mb_controller::b300_timekeeper::set_ticks_now(const uint64_t ticks)
{
    _bar0_regmap->tk_ticks_ctrl_reg.write(
        bar0_regmap_t::tk_ticks_ctrl_reg_t::TIME_PPS, 0); // Unset TIME_PPS
    _bar0_regmap->tk_ticks_event_lo_reg.write(
        bar0_regmap_t::tk_ticks_event_lo_reg_t::TICKS_EVENT_LO,
        uint32_t(ticks & 0xFFFFFFFF));
    _bar0_regmap->tk_ticks_event_hi_reg.write(
        bar0_regmap_t::tk_ticks_event_hi_reg_t::TICKS_EVENT_HI, uint32_t(ticks >> 32));
    _bar0_regmap->tk_ticks_ctrl_reg.write(
        bar0_regmap_t::tk_ticks_ctrl_reg_t::TIME_NOW, 1); // Set TIME_NOW
}

void b300_mb_controller::b300_timekeeper::set_ticks_next_pps(const uint64_t ticks)
{
    _bar0_regmap->tk_ticks_ctrl_reg.write(
        bar0_regmap_t::tk_ticks_ctrl_reg_t::TIME_NOW, 0); // Unset TIME_NOW
    _bar0_regmap->tk_ticks_event_lo_reg.write(
        bar0_regmap_t::tk_ticks_event_lo_reg_t::TICKS_EVENT_LO,
        uint32_t(ticks & 0xFFFFFFFF));
    _bar0_regmap->tk_ticks_event_hi_reg.write(
        bar0_regmap_t::tk_ticks_event_hi_reg_t::TICKS_EVENT_HI, uint32_t(ticks >> 32));
    _bar0_regmap->tk_ticks_ctrl_reg.write(
        bar0_regmap_t::tk_ticks_ctrl_reg_t::TIME_PPS, 1); // Set TIME_PPS
}

void b300_mb_controller::b300_timekeeper::set_period(const uint64_t period_ns)
{
    _bar0_regmap->tk_ticks_period_lo_reg.write(
        bar0_regmap_t::tk_ticks_period_lo_reg_t::TICKS_PERIOD_LO,
        uint32_t(period_ns & 0xFFFFFFFF));
    _bar0_regmap->tk_ticks_period_hi_reg.write(
        bar0_regmap_t::tk_ticks_period_hi_reg_t::TICKS_PERIOD_HI,
        uint32_t(period_ns >> 32));
}

/******************************************************************************
 * Motherboard Control API
 *****************************************************************************/
std::string b300_mb_controller::get_mboard_name() const
{
    return "B310";
}

void b300_mb_controller::set_time_source(const std::string& source)
{
    auto valid_sync_sources = get_sync_sources();
    auto clock_source       = _current_clock_source;
    std::pair<std::string, std::string> source_pair{clock_source, source};
    if (std::find_if(valid_sync_sources.cbegin(),
            valid_sync_sources.cend(),
            [&source_pair](const device_addr_t& sync_source) {
                return sync_source["clock_source"] == source_pair.first
                       && sync_source["time_source"] == source_pair.second;
            })
        == valid_sync_sources.cend()) {
        if (source == "internal") {
            clock_source = "internal";
        } else if (source == "external") {
            clock_source = "external";
        } else if (source == "gpsdo") {
            clock_source = "gpsdo";
        } else if (source == "sync") {
            clock_source = "sync";
        } else if (source == "sync_gpsdo") {
            clock_source = "sync_gpsdo";
        } else {
            auto valid_sources_vec = get_time_sources();
            std::string valid_sources_str;
            for (size_t i = 0; i < valid_sources_vec.size(); ++i) {
                if (i != 0)
                    valid_sources_str += ", ";
                valid_sources_str += valid_sources_vec[i];
            }
            throw uhd::key_error(std::string("Invalid time source: ") + source
                                 + ". Valid options are: " + valid_sources_str + ".");
        }
        UHD_LOG_WARNING("B300",
            "Clock source " << _current_clock_source
                            << " is an invalid selection with time source " << source
                            << ". Coercing clock source to " << clock_source << ".");
    }
    set_sync_source(clock_source, source);
}

std::string b300_mb_controller::get_time_source() const
{
    return _current_time_source;
}

std::vector<std::string> b300_mb_controller::get_time_sources() const
{
    return {"internal", "external", "gpsdo", "sync", "sync_gpsdo"};
}

void b300_mb_controller::set_clock_source(const std::string& source)
{
    auto valid_sync_sources = get_sync_sources();
    auto time_source        = _current_time_source;
    std::pair<std::string, std::string> source_pair{source, time_source};
    if (std::find_if(valid_sync_sources.cbegin(),
            valid_sync_sources.cend(),
            [&source_pair](const device_addr_t& sync_source) {
                return sync_source["clock_source"] == source_pair.first
                       && sync_source["time_source"] == source_pair.second;
            })
        == valid_sync_sources.cend()) {
        if (source == "internal" || source == "external") {
            time_source = "internal";
        } else if (source == "gpsdo") {
            time_source = "gpsdo";
        } else if (source == "sync") {
            time_source = "sync";
        } else if (source == "sync_gpsdo") {
            time_source = "sync_gpsdo";
        } else {
            auto valid_sources_vec = get_clock_sources();
            std::string valid_sources_str;
            for (size_t i = 0; i < valid_sources_vec.size(); ++i) {
                if (i != 0)
                    valid_sources_str += ", ";
                valid_sources_str += valid_sources_vec[i];
            }
            throw uhd::key_error(std::string("Invalid clock source: ") + source
                                 + ". Valid options are: " + valid_sources_str + ".");
        }
        UHD_LOG_WARNING("B300",
            "Time source " << _current_time_source
                           << " is an invalid selection with clock source " << source
                           << ". Coercing time source to " << time_source << ".");
    }
    set_sync_source(source, time_source);
}

std::string b300_mb_controller::get_clock_source() const
{
    return _current_clock_source;
}

std::vector<std::string> b300_mb_controller::get_clock_sources() const
{
    return {"internal", "external", "gpsdo", "sync", "sync_gpsdo"};
}

void b300_mb_controller::set_sync_source(
    const std::string& clock_source, const std::string& time_source)
{
    auto valid_sync_sources = get_sync_sources();
    std::pair<std::string, std::string> source_pair{clock_source, time_source};
    if (std::find_if(valid_sync_sources.cbegin(),
            valid_sync_sources.cend(),
            [&source_pair](const device_addr_t& sync_source) {
                return sync_source["clock_source"] == source_pair.first
                       && sync_source["time_source"] == source_pair.second;
            })
        == valid_sync_sources.cend()) {
        std::string valid_combinations;
        for (size_t i = 0; i < valid_sync_sources.size(); ++i) {
            if (i != 0) {
                valid_combinations += ", ";
            }
            valid_combinations += valid_sync_sources[i]["clock_source"] + "+"
                                  + valid_sync_sources[i]["time_source"];
        }
        throw uhd::value_error(
            std::string("Invalid sync source combination: clock source ") + clock_source
            + " with time source " + time_source
            + ". Valid combinations are: " + valid_combinations + ".");
    }
    if (clock_source != _current_clock_source) {
        // Internal and GPSDO both go into CLKin0 of the LMK04832, External goes into
        // CLKin1.
        if (clock_source == "external" || clock_source == "sync"
            || clock_source == "sync_gpsdo") {
            if (clock_source == "external") {
                _b300_clock_ctrl->set_ext_clk_rate(10e6);
            } else {
                _b300_clock_ctrl->set_ext_clk_rate(
                    _b300_clock_ctrl->get_master_clock_rate());
            }
            // In this case, we need to switch from CLKin0 to CLKIn1.
            _current_clock_source = clock_source;
            _b300_clock_ctrl->set_lmk04832_clock_in(CLKin1);
        } else if (clock_source == "internal" || clock_source == "gpsdo") {
            if (_current_clock_source == "external" || _current_clock_source == "sync"
                || _current_clock_source == "sync_gpsdo") {
                // If we are switching from External to Internal or GPSDO, we need to
                // switch the CLKin source to CLKin0.
                _current_clock_source = clock_source;
                _b300_clock_ctrl->set_lmk04832_clock_in(
                    (_board_rev == 1) ? CLKin0 : CLKin2);
            } else {
                // We still want to store the correct current clock source even if we
                // don't need to switch the LMK04832 input.
                _current_clock_source = clock_source;
            }

            if (clock_source == "gpsdo") {
                _bar0_regmap->clock_ctrl_reg.write(
                    bar0_regmap_t::clk_ctrl_reg_t::REF_CLK_SRC,
                    bar0_regmap_t::clk_ctrl_reg_t::SRC_GPSDO);
            } else {
                _bar0_regmap->clock_ctrl_reg.write(
                    bar0_regmap_t::clk_ctrl_reg_t::REF_CLK_SRC,
                    bar0_regmap_t::clk_ctrl_reg_t::SRC_INTERNAL);
            }
        }
    }

    if (time_source != _current_time_source) {
        _current_time_source = time_source;
        if (time_source == "internal") {
            _bar0_regmap->clock_ctrl_reg.write(bar0_regmap_t::clk_ctrl_reg_t::PPS_SRC,
                bar0_regmap_t::clk_ctrl_reg_t::PPS_SRC_INT);
        } else {
            _bar0_regmap->clock_ctrl_reg.write(bar0_regmap_t::clk_ctrl_reg_t::PPS_SRC,
                bar0_regmap_t::clk_ctrl_reg_t::PPS_SRC_EXT);
        }
    }

    if ((time_source == "gpsdo" || time_source == "sync_gpsdo")
        && !_b300_gps_ctrl->is_initialized()) {
        _b300_gps_ctrl->initialize();
    } else if ((time_source != "gpsdo" && time_source != "sync_gpsdo")
               && _b300_gps_ctrl->is_initialized()) {
        _b300_gps_ctrl->shutdown();
    }


    if (!_b300_clock_ctrl->wait_for_ref_locked(2000)) {
        throw uhd::runtime_error(
            "Reference Clock PLL failed to lock to " + clock_source + " clock source.");
    }
}

void b300_mb_controller::set_sync_source(const device_addr_t& sync_source)
{
    if (sync_source.has_key("clock_source") && sync_source.has_key("time_source")) {
        set_sync_source(sync_source["clock_source"], sync_source["time_source"]);
    } else if (sync_source.has_key("clock_source")) {
        set_clock_source(sync_source["clock_source"]);
    } else if (sync_source.has_key("time_source")) {
        set_time_source(sync_source["time_source"]);
    } else {
        throw uhd::key_error("Invalid Sync source input, must have at least one of "
                             "clock_source or time_source keys");
    }
}

device_addr_t b300_mb_controller::get_sync_source() const
{
    const std::string clock_source = get_clock_source();
    const std::string time_source  = get_time_source();
    device_addr_t sync_source;
    sync_source["clock_source"] = clock_source;
    sync_source["time_source"]  = time_source;
    return sync_source;
}

std::vector<device_addr_t> b300_mb_controller::get_sync_sources()
{
    const std::vector<std::pair<std::string, std::string>> clock_time_src_pairs = {
        // Clock source, Time source
        {"internal", "internal"},
        {"external", "internal"},
        {"external", "external"},
        {"external", "sync"},
        {"external", "sync_gpsdo"},
        {"gpsdo", "gpsdo"},
        {"sync", "sync"},
        {"sync_gpsdo", "sync_gpsdo"}};

    // Now convert to vector of device_addr_t
    std::vector<device_addr_t> sync_sources;
    for (const auto& ct_pair : clock_time_src_pairs) {
        device_addr_t sync_source;
        sync_source["clock_source"] = ct_pair.first;
        sync_source["time_source"]  = ct_pair.second;
        sync_sources.push_back(sync_source);
    }
    return sync_sources;
}

void b300_mb_controller::set_clock_source_out(const bool enb)
{
    if (!enb) {
        UHD_LOG_WARNING("B300", "Disabling clock source output is not supported.");
    }
}

void b300_mb_controller::set_time_source_out(const bool enb)
{
    if (!enb) {
        UHD_LOG_WARNING("B300", "Disabling time source output is not supported.");
    }
}

sensor_value_t b300_mb_controller::get_sensor(const std::string& name)
{
    if (name == "pcie_max_speed") {
        return sensor_value_t("PCIe Max Speed", _pcie_max_speed_gtps, "GT/s", "%g");
    }
    if (name == "pcie_negotiated_speed") {
        return sensor_value_t(
            "PCIe Negotiated Speed", _pcie_neg_speed_gtps, "GT/s", "%g");
    }
    if (name == "pcie_max_width") {
        return sensor_value_t(
            "PCIe Max Width", std::string("x") + std::to_string(_pcie_max_width), "");
    }
    if (name == "pcie_negotiated_width") {
        return sensor_value_t("PCIe Negotiated Width",
            std::string("x") + std::to_string(_pcie_neg_width),
            "");
    }
    if (name == "ref_locked") {
        if (!_b300_gps_ctrl->is_initialized()) {
            return sensor_value_t(
                "Ref", _b300_clock_ctrl->get_ref_locked(), "locked", "unlocked");
        } else {
            return sensor_value_t("Ref",
                _b300_clock_ctrl->get_ref_locked() && _b300_clock_ctrl->get_dpll_locked(),
                "locked",
                "unlocked");
        }
    }
    if (name == "ref_stable") {
        return sensor_value_t(
            "Ref", _b300_clock_ctrl->get_ref_stable(), "stable", "lost lock");
    }
    if (name == "temp_middle") {
        return sensor_value_t("Temp Middle",
            _temp_sensor->read_temperature(tmp468_iface::LOCAL_SENSOR),
            "degC");
    }
    if (name == "temp_pcie_connector_edge") {
        return sensor_value_t("Temp PCIe Connector Edge",
            _temp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR1),
            "degC");
    }
    if (name == "temp_front_end") {
        return sensor_value_t("Temp Front End",
            _temp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR2),
            "degC");
    }
    if (name == "temp_fpga") {
        return sensor_value_t("Temp FPGA",
            _temp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR8),
            "degC");
    }
    if (name == "input_voltage") {
        return sensor_value_t("Input Voltage", _power_monitor->read_bus_voltage(), "V");
    }
    if (name == "input_power") {
        return sensor_value_t("Input Power", _power_monitor->read_power(), "W");
    }
    if (name == "input_current") {
        return sensor_value_t("Input Current", _power_monitor->read_current(), "A");
    }
    if ((name == "gps_locked") || (name == "gps_time") || (name == "gps_gprmc")
        || (name == "gps_gpgga")) {
        if (!_b300_gps_ctrl->is_initialized()) {
            throw uhd::runtime_error("GPS module not initialized. Set clock_source to "
                                     "gpsdo or sync_gpsdo to initialize.");
        }
        return _b300_gps_ctrl->get_sensor(name);
    }
    if (name == "gps_lmk04832_lock") {
        if (!_b300_gps_ctrl->is_initialized()) {
            throw uhd::runtime_error("GPS module not initialized. Set clock_source to "
                                     "gpsdo or sync_gpsdo to initialize.");
        }
        return sensor_value_t(
            "Ref", _b300_clock_ctrl->get_ref_locked(), "locked", "unlocked");
    }
    if (name == "gps_lmk05318_dpll_freq_lock") {
        if (!_b300_gps_ctrl->is_initialized()) {
            throw uhd::runtime_error("GPS module not initialized. Set clock_source to "
                                     "gpsdo or sync_gpsdo to initialize.");
        }
        return sensor_value_t("LMK05318 DPLL Frequency",
            _b300_clock_ctrl->get_dpll_locked(
                lmk05318_iface::dpll_lock_check_t::FREQ_LOCK),
            "locked",
            "unlocked");
    }
    if (name == "gps_lmk05318_dpll_phase_lock") {
        if (!_b300_gps_ctrl->is_initialized()) {
            throw uhd::runtime_error("GPS module not initialized. Set clock_source to "
                                     "gpsdo or sync_gpsdo to initialize.");
        }
        return sensor_value_t("LMK05318 DPLL Phase",
            _b300_clock_ctrl->get_dpll_locked(
                lmk05318_iface::dpll_lock_check_t::PHASE_LOCK),
            "locked",
            "unlocked");
    }
    // This is a sensor for Mfg to check that the Timepulse from the GPS chip to the
    // LMK05318 is functioning.
    if (name == "lmk05318_priref_valid") {
        return sensor_value_t("LMK05318 Primary Reference Valid",
            _b300_clock_ctrl->validate_lmk05318_priref(),
            "valid",
            "invalid");
    }
    // This is a sensor for Mfg to check that the PPS from the LMK chip is toggling, when
    // there is a GPS lock.
    if (name == "gps_pps_present") {
        return sensor_value_t(
            "GPS PPS Present", _check_gps_pps_present(), "present", "not present");
    }
    throw uhd::key_error(std::string("Invalid sensor name: ") + name);
}

std::vector<std::string> b300_mb_controller::get_sensor_names()
{
    std::vector<std::string> sensors{"pcie_max_speed",
        "pcie_negotiated_speed",
        "pcie_max_width",
        "pcie_negotiated_width",
        "ref_locked",
        "ref_stable",
        "temp_middle",
        "temp_pcie_connector_edge",
        "temp_front_end",
        "temp_fpga",
        "input_voltage",
        "input_power",
        "input_current",
        "lmk05318_priref_valid",
        "gps_pps_present"};
    if (_b300_gps_ctrl->is_initialized()) {
        std::vector<std::string> gps_sensors{"gps_locked",
            "gps_gpgga",
            "gps_time",
            "gps_gprmc",
            "gps_lmk04832_lock",
            "gps_lmk05318_dpll_freq_lock",
            "gps_lmk05318_dpll_phase_lock"};
        sensors.insert(sensors.end(), gps_sensors.begin(), gps_sensors.end());
    }
    return sensors;
}

uhd::usrp::mboard_eeprom_t b300_mb_controller::get_eeprom()
{
    return _mb_eeprom;
}

bool b300_mb_controller::synchronize(std::vector<mb_controller::sptr>& mb_controllers,
    const uhd::time_spec_t& time_spec,
    const bool quiet)
{
    UHD_LOG_INFO("B300",
        "Calling base class synchronize implementation. The B310 multi-device "
        "synchronization procedure must be run at initialization before the "
        "transceiver chips are initialized. To do this, open a multi-device session "
        "with clock_source and time_source set to 'external', 'sync', or "
        "'sync_gpsdo'.");
    return mb_controller::synchronize(mb_controllers, time_spec, quiet);
}

std::vector<std::string> b300_mb_controller::get_gpio_banks() const
{
    return {B300_GPIO_SRC_BANK};
}

std::vector<std::string> b300_mb_controller::get_gpio_srcs(const std::string& bank) const
{
    if (bank == B300_GPIO_SRC_BANK) {
        return {B300_GPIO_SRC_RF0, B300_GPIO_SRC_RF1};
    } else {
        throw uhd::key_error("Invalid GPIO bank: " + bank);
    }
}

std::vector<std::string> b300_mb_controller::get_gpio_src(const std::string& bank)
{
    if (bank == B300_GPIO_SRC_BANK) {
        return _gpio_srcs;
    } else {
        throw uhd::key_error("Invalid GPIO bank: " + bank);
    }
}

uhd::soft_reg_field_t b300_mb_controller::_get_gpio_field(size_t index)
{
    switch (index) {
        case 0:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO0_SRC;
        case 1:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO1_SRC;
        case 2:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO2_SRC;
        case 3:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO3_SRC;
        case 4:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO4_SRC;
        case 5:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO5_SRC;
        case 6:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO6_SRC;
        case 7:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO7_SRC;
        case 8:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO8_SRC;
        case 9:
            return bar0_regmap_t::gpio_ctrl_reg_t::FP_GPIO9_SRC;
        default:
            throw uhd::key_error("Invalid GPIO index: " + std::to_string(index));
    }
}

void b300_mb_controller::set_gpio_src(
    const std::string& bank, const std::vector<std::string>& src)
{
    if (bank == B300_GPIO_SRC_BANK) {
        // Check if src has more elements than _gpio_srcs
        if (src.size() > _gpio_srcs.size()) {
            throw uhd::value_error("Too many sources provided! Provided "
                                   + std::to_string(src.size()) + " sources for "
                                   + std::to_string(_gpio_srcs.size()) + " pins.");
        }

        // Set each element of _gpio_srcs to the corresponding element of src
        for (size_t i = 0; i < src.size(); ++i) {
            _gpio_srcs[i] = src[i];
            auto field    = _get_gpio_field(i);
            _bar0_regmap->gpio_ctrl_reg.set(field,
                src[i] == B300_GPIO_SRC_RF0 ? bar0_regmap_t::gpio_ctrl_reg_t::SRC_CH0
                                            : bar0_regmap_t::gpio_ctrl_reg_t::SRC_CH1);
        }
        _bar0_regmap->gpio_ctrl_reg.flush();
    } else {
        throw uhd::key_error("Invalid GPIO bank: " + bank);
    }
}

b300_clock_ctrl::sptr b300_mb_controller::get_clock_ctrl()
{
    return _b300_clock_ctrl;
}

bool b300_mb_controller::_check_gps_pps_present()
{
    // Check if GPS PPS is present by monitoring the GPS_PPS_MONITOR register for a
    // change, which indicates that the PPS signal is toggling.
    const auto start = std::chrono::steady_clock::now();
    auto prev =
        _bar0_regmap->gps_ctrl_reg.read(bar0_regmap_t::gps_ctrl_reg_t::GPS_PPS_MONITOR);
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        const auto curr = _bar0_regmap->gps_ctrl_reg.read(
            bar0_regmap_t::gps_ctrl_reg_t::GPS_PPS_MONITOR);
        if (curr != prev) {
            return true;
        }
    } while (std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now() - start)
                 .count()
             < 1100);
    return false;
}

void b300_mb_controller::setup_multi_device_sync()
{
    get_timekeeper(0)->set_time_next_pps(uhd::time_spec_t(0.0));
}

void b300_mb_controller::configure_lmk_for_sync()
{
    auto start = std::chrono::steady_clock::now();
    // We need to do the multi-device sync configuration for all devices within PPS
    // pulses, so wait until we have seen the PPS go to 0 meaning that the previous call
    // to set the next PPS to zero has taken effect. If we happen to get to a second
    // device and the time has already ticked over to 1, then throw an error.
    while (get_timekeeper(0)->get_time_last_pps().get_full_secs() > 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
                           .count();
        // Wait a little over a second to give some buffer.
        if (elapsed >= 1100) {
            throw uhd::runtime_error(
                "Timeout waiting for PPS to be set to 0 for multi-device sync. Please "
                "run again or check PPS in source.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    _b300_clock_ctrl->config_lmk04832_for_sync();
}

void b300_mb_controller::finish_multi_device_sync()
{
    _b300_clock_ctrl->finish_lmk04832_sync();
    get_timekeeper(0)->set_time_next_pps(uhd::time_spec_t(0.0));
}

}} // namespace uhd::rfnoc
