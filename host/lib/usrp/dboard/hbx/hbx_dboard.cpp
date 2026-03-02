//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/direction.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/scope_exit.hpp>
#include <uhdlib/usrp/common/apply_corrections.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_dboard.hpp>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <thread>

using namespace std::chrono_literals;

namespace uhd { namespace usrp { namespace hbx {

/******************************************************************************
 * Structors
 *****************************************************************************/
hbx_dboard_impl::hbx_dboard_impl(register_iface& reg_iface,
    const size_t reg_base_address,
    time_accessor_fn_type&& time_accessor,
    const size_t db_idx,
    const std::string& radio_slot,
    const std::string& rpc_prefix,
    const std::string& unique_id,
    uhd::usrp::x400_rpc_iface::sptr mb_rpcc,
    uhd::usrp::hbx_rpc_iface::sptr rpcc,
    uhd::rfnoc::x400::rfdc_control::sptr rfdcc,
    uhd::property_tree::sptr tree,
    const bool ignore_cal_file,
    const double mcr)
    : nameless_gain_mixin([this](const uhd::direction_t trx, size_t) {
        const auto gain_profile = trx == TX_DIRECTION
                                      ? _tx_gain_profile_api->get_gain_profile(0)
                                      : _rx_gain_profile_api->get_gain_profile(0);
        if (gain_profile == HBX_GAIN_PROFILE_MANUAL) {
            const std::string err_msg =
                "When using 'manual' gain mode, a gain name is required!";
            RFNOC_LOG_ERROR(err_msg);
            throw uhd::runtime_error(err_msg);
        }
        return HBX_GAIN_STAGE_ALL;
    })
    , _unique_id(unique_id)
    , _reg_iface(reg_iface)
    , _reg_base_address(reg_base_address)
    , _time_accessor(time_accessor)
    , _radio_slot(radio_slot)
    , _db_idx(db_idx)
    , _rpc_prefix(rpc_prefix)
    , _mb_rpcc(mb_rpcc)
    , _rpcc(rpcc)
    , _rfdcc(rfdcc)
    , _tree(tree)
    , _rfdc_rate(_rpcc->get_dboard_sample_rate())
    , _prc_rate(_rpcc->get_dboard_prc_rate())
    , _ignore_cal_file(ignore_cal_file)
    , _mcr(mcr)
{
    RFNOC_LOG_TRACE("Entering hbx_dboard_impl ctor...");
    RFNOC_LOG_TRACE("Radio slot: " << _radio_slot);

    _rx_antenna = std::make_shared<uhd::rfnoc::rf_control::enumerated_antenna>(
        tree,
        [this](size_t chan) {
            return this->_get_frontend_path(RX_DIRECTION, chan) / "antenna" / "value";
        },
        RX_ANTENNAS,
        RX_ANTENNA_NAME_COMPAT_MAP);
    _tx_antenna = std::make_shared<uhd::rfnoc::rf_control::enumerated_antenna>(
        tree,
        [this](size_t chan) {
            return this->_get_frontend_path(TX_DIRECTION, chan) / "antenna" / "value";
        },
        TX_ANTENNAS,
        TX_ANTENNA_NAME_COMPAT_MAP);

    _tx_gain_profile_api = std::make_shared<rf_control::enumerated_gain_profile>(
        HBX_GAIN_PROFILES, HBX_GAIN_PROFILE_DEFAULT, get_num_tx_channels());
    _rx_gain_profile_api = std::make_shared<rf_control::enumerated_gain_profile>(
        HBX_GAIN_PROFILES, HBX_GAIN_PROFILE_DEFAULT, get_num_rx_channels());

    _expert_container =
        uhd::experts::expert_factory::create_container("hbx_radio_" + _radio_slot);
    uhd::eeprom_map_t eeprom_map = get_db_eeprom();
    /* The cal serial is the DB serial plus the FE name */
    _db_serial = std::string(eeprom_map["serial"].begin(), eeprom_map["serial"].end());
    if (_db_serial.empty()) {
        RFNOC_LOG_WARNING("Daughterboard serial number is empty! Unable to load "
                          "calibration data even if available.");
    }

    _init_hbx_cpld();
    _init_admv();
    _rx_lo_ctrl = _init_lo_ctrl(RX_DIRECTION);
    _tx_lo_ctrl = _init_lo_ctrl(TX_DIRECTION);
    _init_demod();
    _rx_lo_pd = _init_lo_pd(RX_DIRECTION);
    _tx_lo_pd = _init_lo_pd(TX_DIRECTION);
    // Prop tree requires the initialization of certain peripherals
    _init_prop_tree();
    _expert_container->resolve_all();
}

hbx_dboard_impl::~hbx_dboard_impl()
{
    RFNOC_LOG_TRACE("hbx_dboard::dtor()");
}

void hbx_dboard_impl::deinit()
{
    _wb_ifaces.clear();
}

void hbx_dboard_impl::set_command_time(uhd::time_spec_t time, const size_t chan)
{
    // When the command time gets updated, import it into the expert graph
    get_tree()
        ->access<time_spec_t>(_get_frontend_path(RX_DIRECTION, chan) / "time/cmd")
        .set(time);
}

std::string hbx_dboard_impl::get_unique_id() const
{
    return _unique_id;
}

/******************************************************************************
 * API Calls
 *****************************************************************************/

std::string hbx_dboard_impl::get_rx_antenna(const size_t chan) const
{
    return _rx_antenna->get_antenna(chan);
}

double hbx_dboard_impl::set_tx_frequency(const double req_freq, const size_t chan)
{
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    _tree->access<double>(fe_path / "freq").set(req_freq);

    // Our power manager sets a new gain value via the API, based on its new calculations.
    // Since the expert nodes are protected by a mutex, it will hang if we try to call
    // update_power() from inside the expert resolve methods (resolve() -> update_power()
    // -> set_tx_gain -> resolve())

    if (_tx_gain_profile_api->get_gain_profile(0) == HBX_GAIN_PROFILE_DEFAULT) {
        _tx_pwr_mgr.at(0)->update_power();
    } else if (_tx_pwr_mgr.at(0)->get_tracking_mode()
               == uhd::usrp::pwr_cal_mgr::tracking_mode::TRACK_POWER) {
        // If the power manager is in TRACK_GAIN mode, update_power() won't do anything.
        // If we are in TRACK_POWER mode, however, but not in the default gain profile, we
        // will warn.
        RFNOC_LOG_WARNING("Not updating TX power, because power manager is only "
                          "available in the default gain profile.");
    }

    auto ret_val = _tree->access<double>(fe_path / "freq").get();

    if (!_ignore_cal_file) {
        apply_wideband_tx_iq_dc_corrections(
            _tree, _db_serial, _mcr, chan, fe_path, ret_val);
    }

    return ret_val;
}

double hbx_dboard_impl::set_rx_frequency(const double req_freq, const size_t chan)
{
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    _tree->access<double>(fe_path / "freq").set(req_freq);

    // Our power manager sets a new gain value via the API, based on its new calculations.
    // Since the expert nodes are protected by a mutex, it will hang if we try to call
    // update_power() from inside the expert resolve methods (resolve() -> update_power()
    // -> set_rx_gain -> resolve())
    if (_rx_gain_profile_api->get_gain_profile(0) == HBX_GAIN_PROFILE_DEFAULT) {
        _rx_pwr_mgr.at(0)->update_power();
    } else if (_rx_pwr_mgr.at(0)->get_tracking_mode()
               == uhd::usrp::pwr_cal_mgr::tracking_mode::TRACK_POWER) {
        // If the power manager is in TRACK_GAIN mode, update_power() won't do anything.
        // If we are in TRACK_POWER mode, however, but not in the default gain profile, we
        // will warn.
        RFNOC_LOG_WARNING("Not updating RX power, because power manager is only "
                          "available in the default gain profile.");
    }

    auto ret_val = _tree->access<double>(fe_path / "freq").get();

    if (!_ignore_cal_file) {
        apply_wideband_rx_iq_dc_corrections(
            _tree, _db_serial, _mcr, chan, fe_path, ret_val);
    }

    return ret_val;
}

double hbx_dboard_impl::set_tx_bandwidth(const double bandwidth, const size_t chan)
{
    const double bw = get_tx_bandwidth(chan);
    if (!uhd::math::frequencies_are_equal(bandwidth, bw)) {
        RFNOC_LOG_WARNING("Invalid analog bandwidth: " << (bandwidth / 1e6) << " MHz.");
    }
    return bw;
}

double hbx_dboard_impl::get_tx_bandwidth(size_t chan)
{
    return _tree
        ->access<double>(_get_frontend_path(TX_DIRECTION, chan) / "bandwidth/value")
        .get();
}

double hbx_dboard_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    const double bw = get_rx_bandwidth(chan);
    if (!uhd::math::frequencies_are_equal(bandwidth, bw)) {
        RFNOC_LOG_WARNING("Invalid analog bandwidth: " << (bandwidth / 1e6) << " MHz.");
    }
    return bw;
}

double hbx_dboard_impl::get_rx_bandwidth(size_t chan)
{
    return _tree
        ->access<double>(_get_frontend_path(RX_DIRECTION, chan) / "bandwidth/value")
        .get();
}

double hbx_dboard_impl::get_tx_frequency(size_t chan)
{
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);
    return _tree->access<double>(fe_path / "freq").get();
}

double hbx_dboard_impl::get_rx_frequency(size_t chan)
{
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);
    return _tree->access<double>(fe_path / "freq").get();
}

double hbx_dboard_impl::set_tx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE("set_tx_lo_freq(freq=" << freq << ", name=" << lo_name
                                           << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);
    return _tree->access<double>(fe_path / "los" / lo_name / "freq" / "value")
        .set(freq)
        .get();
}

double hbx_dboard_impl::get_tx_lo_freq(const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE("get_tx_lo_freq(name=" << lo_name << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    return _tree->access<double>(fe_path / "los" / lo_name / "freq" / "value").get();
}

double hbx_dboard_impl::set_rx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE("set_rx_lo_freq(freq=" << freq << ", name=" << lo_name
                                           << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    return _tree->access<double>(fe_path / "los" / lo_name / "freq" / "value")
        .set(freq)
        .get();
}

double hbx_dboard_impl::get_rx_lo_freq(const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE("get_rx_lo_freq(name=" << lo_name << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    return _tree->access<double>(fe_path / "los" / lo_name / "freq" / "value").get();
}

void hbx_dboard_impl::set_rx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE("set_rx_lo_source(src=" << src << ", name=" << lo_name
                                            << ", chan=" << chan << ")");
    if (!uhd::has(LO_SOURCE_ALL, src)) {
        throw uhd::value_error("Invalid LO source: " + src);
    }
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    _tree->access<bool>(fe_path / "los" / lo_name / "import")
        .set(src != LO_SOURCE_INT[0]);
}

void hbx_dboard_impl::set_tx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE("set_tx_lo_source(src=" << src << ", name=" << lo_name
                                            << ", chan=" << chan << ")");
    if (!uhd::has(LO_SOURCE_ALL, src)) {
        throw uhd::value_error("Invalid LO source: " + src);
    }
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    _tree->access<bool>(fe_path / "los" / lo_name / "import")
        .set(src != LO_SOURCE_INT[0]);
}

std::string hbx_dboard_impl::get_rx_lo_source(const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    const fs_path fe_path     = _get_frontend_path(RX_DIRECTION, chan);
    const bool lo_source =
        _tree->access<bool>(fe_path / "los" / lo_name / "import").get();
    return LO_SOURCE_ALL[lo_source];
}

std::string hbx_dboard_impl::get_tx_lo_source(const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    const fs_path fe_path     = _get_frontend_path(TX_DIRECTION, chan);
    const bool lo_source =
        _tree->access<bool>(fe_path / "los" / lo_name / "import").get();
    return LO_SOURCE_ALL[lo_source];
}

freq_range_t hbx_dboard_impl::_get_lo_freq_range(const std::string& name) const
{
    if (name == RFDC_NCO) {
        return freq_range_t(0.0, _rfdc_rate);
    }
    return freq_range_t(LMX2572_MIN_FREQ, LMX2572_MAX_FREQ);
}

void hbx_dboard_impl::set_tx_lo_export_enabled(
    const bool enabled, const std::string& name, const size_t chan)
{
    if (name == RFDC_NCO && enabled) {
        throw uhd::value_error("RFDC NCO LO cannot be exported");
    }
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE("set_tx_lo_export_enabled(enabled=" << enabled << ", name=" << lo_name
                                                        << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    _tree->access<bool>(fe_path / "los" / lo_name / "export").set(enabled);
}

void hbx_dboard_impl::set_rx_lo_export_enabled(
    const bool enabled, const std::string& name, const size_t chan)
{
    if (name == RFDC_NCO && enabled) {
        throw uhd::value_error("RFDC NCO LO cannot be exported");
    }
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE("set_rx_lo_export_enabled(enabled=" << enabled << ", name=" << lo_name
                                                        << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    _tree->access<bool>(fe_path / "los" / lo_name / "export").set(enabled);
}
bool hbx_dboard_impl::get_rx_lo_export_enabled(const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE(
        "get_rx_lo_export_enabled(name=" << lo_name << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    return _tree->access<bool>(fe_path / "los" / lo_name / "export").get();
}
bool hbx_dboard_impl::get_tx_lo_export_enabled(const std::string& name, const size_t chan)
{
    const std::string lo_name = _validate_lo_name(name);
    RFNOC_LOG_TRACE(
        "get_tx_lo_export_enabled(name=" << lo_name << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    return _tree->access<bool>(fe_path / "los" / lo_name / "export").get();
}

uhd::gain_range_t hbx_dboard_impl::get_tx_gain_range(
    const std::string& name_, const size_t chan) const
{
    // We have to accept the empty string for "all", because that's widely used
    // (e.g. by multi_usrp)
    const std::string name   = name_.empty() ? HBX_GAIN_STAGE_ALL : name_;
    const fs_path gains_path = _get_frontend_path(TX_DIRECTION, chan) / "gains";
    const auto gain_profile  = _tx_gain_profile_api->get_gain_profile(0);
    if (!_tree->exists(gains_path / name / "value")) {
        throw uhd::value_error(
            std::string("get_tx_gain_range(): Unknown gain name '") + name + "'!");
    }
    if (name == HBX_GAIN_STAGE_ALL) {
        return HBX_TX_GAIN_RANGE;
    } else if (name == HBX_GAIN_STAGE_RF) {
        return uhd::gain_range_t(0.0, RF_DSA_MAX_ATTENUATION, 2.0);
    } else if (name == HBX_GAIN_STAGE_LO) {
        return uhd::gain_range_t(0.0, LO_LF_DSA_MAX_ATTENUATION, 1.0);
    } else if (name == HBX_GAIN_STAGE_LO_PWR_INT || name == HBX_GAIN_STAGE_LO_PWR_EXT) {
        return uhd::gain_range_t(0.0, LO_MAX_PWR, 1.0);
    } else if (name == HBX_GAIN_STAGE_LF_DSA1 || name == HBX_GAIN_STAGE_LF_DSA2) {
        return uhd::gain_range_t(0.0, LO_LF_DSA_MAX_ATTENUATION, 1.0);
    } else if (name == HBX_GAIN_STAGE_ADMV_DSA1 || name == HBX_GAIN_STAGE_ADMV_DSA2) {
        return uhd::gain_range_t(0.0, ADMV_DSA_MAX_ATTENUATION, 1.0);
    } else if (name == HBX_GAIN_STAGE_ADMV_DSA_ALL) {
        // This is the maximum
        return uhd::gain_range_t(0.0, ADMV_DSA_MAX_ATTENUATION * 2, 1.0);
    } else
        return uhd::gain_range_t(0.0, 0.0, 1.0);
}

uhd::gain_range_t hbx_dboard_impl::get_rx_gain_range(
    const std::string& name_, const size_t chan) const
{
    // We have to accept the empty string for "all", because that's widely used
    // (e.g. by multi_usrp)
    const std::string name   = name_.empty() ? HBX_GAIN_STAGE_ALL : name_;
    const fs_path gains_path = _get_frontend_path(RX_DIRECTION, chan) / "gains";
    const auto gain_profile  = _rx_gain_profile_api->get_gain_profile(0);

    if (!_tree->exists(gains_path / name / "value")) {
        throw uhd::value_error(
            std::string("get_rx_gain_range(): Unknown gain name '") + name + "'!");
    }
    if (name == HBX_GAIN_STAGE_ALL) {
        return HBX_RX_GAIN_RANGE;
    } else if (name == HBX_GAIN_STAGE_RF) {
        return uhd::gain_range_t(0.0, RF_DSA_MAX_ATTENUATION, 2.0);
    } else if (name == HBX_GAIN_STAGE_LO || name == HBX_GAIN_STAGE_LF_DSA1
               || name == HBX_GAIN_STAGE_LF_DSA2) {
        return uhd::gain_range_t(0.0, LO_LF_DSA_MAX_ATTENUATION, 1.0);
    } else if (name == HBX_GAIN_STAGE_LO_PWR_INT || name == HBX_GAIN_STAGE_LO_PWR_EXT) {
        return uhd::gain_range_t(0.0, LO_MAX_PWR, 1.0);
    } else if (name == HBX_GAIN_STAGE_ADMV_DSA2) {
        return uhd::gain_range_t(0.0, ADMV1420_DSA2_MAX_ATTENUATION, 6.0);
    } else if (name == HBX_GAIN_STAGE_ADMV_DSA2 || name == HBX_GAIN_STAGE_ADMV_DSA3
               || name == HBX_GAIN_STAGE_ADMV_DSA4 || name == HBX_GAIN_STAGE_ADMV_DSA5) {
        return uhd::gain_range_t(0.0, ADMV_DSA_MAX_ATTENUATION, 1.0);
    } else if (name == HBX_GAIN_STAGE_ADMV_DSA_ALL) {
        // Get the maximum attenuation for the currently active band
        return uhd::gain_range_t(0.0, _admv1420->get_max_compound_dsa(), 1.0);
    } else
        return uhd::gain_range_t(0.0, 0.0, 1.0);
}

double hbx_dboard_impl::set_tx_gain(
    const double gain, const std::string& name_, const size_t chan)
{
    // MultiUSRP uses an empty name for all, so we have to handle this:
    const std::string name  = name_.empty() ? HBX_GAIN_STAGE_ALL : name_;
    const auto gain_profile = _tx_gain_profile_api->get_gain_profile(0);

    // Default gain profile: Only 'all' is allowed
    if (gain_profile == HBX_GAIN_PROFILE_DEFAULT
        && !uhd::has(HBX_TX_GAIN_STAGES_DEFAULT, name)) {
        throw uhd::value_error("Invalid TX gain stage for default gain profile: " + name);
    }

    if (gain_profile == HBX_GAIN_PROFILE_MANUAL
        && !uhd::has(HBX_TX_GAIN_STAGES_MANUAL, name)) {
        throw uhd::value_error("Invalid TX gain stage for manual gain profile: " + name);
    }
    auto _range        = get_tx_gain_range(name, chan);
    auto _gain_coerced = _range.clip(gain, true);
    RFNOC_LOG_TRACE("set_tx_gain(gain=" << _gain_coerced << ", name=" << name
                                        << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    return _tree->access<double>(fe_path / "gains" / name / "value")
        .set(_gain_coerced)
        .get();
}

double hbx_dboard_impl::set_rx_gain(
    const double gain, const std::string& name_, const size_t chan)
{
    // MultiUSRP uses an empty name for all, so we have to handle this:
    const std::string name  = name_.empty() ? HBX_GAIN_STAGE_ALL : name_;
    const auto gain_profile = _rx_gain_profile_api->get_gain_profile(0);

    // Default gain profile: Only 'all' is allowed
    if (gain_profile == HBX_GAIN_PROFILE_DEFAULT
        && !uhd::has(HBX_RX_GAIN_STAGES_DEFAULT, name)) {
        throw uhd::value_error("Invalid RX gain stage for default gain profile: " + name);
    }

    if (gain_profile == HBX_GAIN_PROFILE_MANUAL
        && !uhd::has(HBX_RX_GAIN_STAGES_MANUAL, name)) {
        throw uhd::value_error("Invalid RX gain stage for manual gain profile: " + name);
    }
    auto _range        = get_rx_gain_range(name, chan);
    auto _gain_coerced = _range.clip(gain, true);
    RFNOC_LOG_TRACE("set_rx_gain(gain=" << _gain_coerced << ", name=" << name
                                        << ", chan=" << chan << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    return _tree->access<double>(fe_path / "gains" / name / "value")
        .set(_gain_coerced)
        .get();
}

double hbx_dboard_impl::get_tx_gain(const std::string& name_, const size_t chan)
{
    const std::string name   = name_.empty() ? HBX_GAIN_STAGE_ALL : name_;
    const fs_path gains_path = _get_frontend_path(TX_DIRECTION, chan) / "gains";
    const auto gain_profile  = _tx_gain_profile_api->get_gain_profile(0);
    // Like in ZBX, the overall gain can only be read back reliably in default gain
    // profile. However since this is a common diagnostic for many applications, we only
    // warn and don't throw here.
    if (name == HBX_GAIN_STAGE_ALL && gain_profile != HBX_GAIN_PROFILE_DEFAULT) {
        RFNOC_LOG_WARNING(
            "Reading the overall TX gain is only reliable in the default gain profile!");
    }

    if (!_tree->exists(gains_path / name / "value")) {
        RFNOC_LOG_ERROR("get_tx_gain(): Invalid gain name `" << name << "'");
        throw uhd::key_error(std::string("get_tx_gain(): Invalid gain name: ") + name);
    }

    return _tree->access<double>(gains_path / name / "value").get();
}
double hbx_dboard_impl::get_rx_gain(const std::string& name_, const size_t chan)
{
    const std::string name   = name_.empty() ? HBX_GAIN_STAGE_ALL : name_;
    const fs_path gains_path = _get_frontend_path(RX_DIRECTION, chan) / "gains";
    const auto gain_profile  = _rx_gain_profile_api->get_gain_profile(0);

    // Like in ZBX, the overall gain can only be read back reliably in default gain
    // profile. However since this is a common diagnostic for many applications, we only
    // warn and don't throw here.
    if (name == HBX_GAIN_STAGE_ALL && gain_profile != HBX_GAIN_PROFILE_DEFAULT) {
        RFNOC_LOG_WARNING(
            "Reading the overall RX gain is only reliable in the default gain profile!");
    }

    if (!_tree->exists(gains_path / name / "value")) {
        RFNOC_LOG_ERROR("get_rx_gain(): Invalid gain name `" << name << "'");
        throw uhd::key_error(std::string("get_rx_gain(): Invalid gain name: ") + name);
    }

    return _tree->access<double>(gains_path / name / "value").get();
}

std::vector<std::string> hbx_dboard_impl::get_tx_gain_names(const size_t) const
{
    const std::string gain_profile = _tx_gain_profile_api->get_gain_profile(0);

    if (gain_profile == HBX_GAIN_PROFILE_DEFAULT) {
        return HBX_TX_GAIN_STAGES_DEFAULT;
    }
    return HBX_TX_GAIN_STAGES_MANUAL;
}

std::vector<std::string> hbx_dboard_impl::get_rx_gain_names(const size_t) const
{
    const std::string gain_profile = _rx_gain_profile_api->get_gain_profile(0);

    if (gain_profile == HBX_GAIN_PROFILE_DEFAULT) {
        return HBX_RX_GAIN_STAGES_DEFAULT;
    }
    return HBX_RX_GAIN_STAGES_MANUAL;
}

/******************************************************************************
 * EEPROM API
 *****************************************************************************/
eeprom_map_t hbx_dboard_impl::get_db_eeprom()
{
    return _mb_rpcc->get_db_eeprom(_db_idx);
}

size_t hbx_dboard_impl::get_chan_from_dboard_fe(
    const std::string& fe, const uhd::direction_t dir) const
{
    char* end;
    errno           = 0;
    uint32_t fe_num = std::strtol(fe.c_str(), &end, 0);
    const auto max_num_chans =
        (dir == TX_DIRECTION ? get_num_tx_channels() : get_num_rx_channels());
    if (errno == 0 && strlen(end) == 0 && fe_num < max_num_chans) {
        return fe_num;
    } else {
        throw uhd::key_error(std::string("[X400] Invalid frontend: ") + fe);
    }
}

std::string hbx_dboard_impl::get_dboard_fe_from_chan(
    const size_t chan, const uhd::direction_t dir) const
{
    const auto max_num_chans =
        (dir == TX_DIRECTION ? get_num_tx_channels() : get_num_rx_channels());
    if (chan < max_num_chans) {
        return std::to_string(chan);
    } else {
        throw uhd::lookup_error(
            std::string("[X400] Invalid channel: ") + std::to_string(chan));
    }
}
/*********************************************************************
 * ADC Self Cal API
 **********************************************************************/
bool hbx_dboard_impl::select_adc_self_cal_gain(size_t chan, size_t mode)
{
    constexpr double min_gain = 10.0;
    // Typical values observed are around 30-35 dB. So give it some margin, but still
    // allow it to fail if something is wrong with the loopback path.
    constexpr double max_gain = 40.0;

    bool found_gain = false;

    // First set TX so that we are at about 0 dBm
    this->set_tx_gain(46, chan);

    // Then increase RX gain until the threshold value is reached.
    for (double i = min_gain; i <= max_gain; i += 1.0) {
        this->set_rx_gain(i, chan);

        // Wait for it to settle
        constexpr auto settle_time = 10ms;
        std::this_thread::sleep_for(settle_time);
        try {
            const bool threshold_status =
                _mb_rpcc->get_threshold_status(_db_idx, chan, mode, 0);
            if (threshold_status) {
                found_gain = true;
                break;
            }
        } catch (uhd::runtime_error&) {
            // Catch & eat this error, the user has a 5.0 FPGA and so can't auto-gain
            return false;
        }
    }
    return found_gain;
}

void hbx_dboard_impl::setup_adc_self_cal()
{
    // To perform self-cal on HBX, set the ATR mode to SW and switch to the TRX state
    // (3) manually, so all switches and DSAs are set properly.
    this->_cpld->set_atr_mode(hbx_cpld_ctrl::atr_t::SW_DEFINED);
    this->_cpld->set_atr_state(ATR_ADDR_XX);
}

void hbx_dboard_impl::finalize_adc_self_cal()
{
    this->_cpld->set_atr_state(ATR_ADDR_0X);
    this->_cpld->set_atr_mode(hbx_cpld_ctrl::atr_t::CLASSIC_ATR);
}

/*********************************************************************
 *   Private misc/calculative helper functions
 **********************************************************************/

fs_path hbx_dboard_impl::_get_frontend_path(
    const direction_t dir, const size_t chan_idx) const
{
    UHD_ASSERT_THROW(chan_idx < HBX_MAX_NUM_CHANS);
    const std::string frontend = dir == TX_DIRECTION ? "tx_frontends" : "rx_frontends";
    return fs_path("dboard") / frontend / chan_idx;
}

std::vector<uhd::usrp::pwr_cal_mgr::sptr>& hbx_dboard_impl::get_pwr_mgr(
    uhd::direction_t trx)
{
    switch (trx) {
        case uhd::RX_DIRECTION:
            return _rx_pwr_mgr;
        case uhd::TX_DIRECTION:
            return _tx_pwr_mgr;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

std::string hbx_dboard_impl::_validate_lo_name(const std::string& name) const
{
    const std::string lo_name = (name == GENERIC_LO) ? HBX_LO : name;
    // To be perfectly right, we would have to know the direction and channel of the LO to
    // validate it, but since in HBX, RX and TX are the same in terms of LO options, we
    // allow ourselves the shortcut to always query TX channel 0.
    uhd::assert_has(get_tx_lo_names(0), lo_name, "Invalid LO name: " + lo_name);
    return lo_name;
}

}}} // namespace uhd::usrp::hbx
