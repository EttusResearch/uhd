//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_dboard.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <cstdlib>
#include <sstream>

namespace uhd { namespace usrp { namespace zbx {

/******************************************************************************
 * Structors
 *****************************************************************************/
zbx_dboard_impl::zbx_dboard_impl(register_iface& reg_iface,
    const size_t reg_base_address,
    time_accessor_fn_type&& time_accessor,
    const size_t db_idx,
    const std::string& radio_slot,
    const std::string& rpc_prefix,
    const std::string& unique_id,
    uhd::usrp::x400_rpc_iface::sptr mb_rpcc,
    uhd::usrp::zbx_rpc_iface::sptr rpcc,
    uhd::rfnoc::x400::rfdc_control::sptr rfdcc,
    uhd::property_tree::sptr tree)
    : _unique_id(unique_id)
    , _regs(reg_iface)
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
{
    RFNOC_LOG_TRACE("Entering zbx_dboard_impl ctor...");
    RFNOC_LOG_TRACE("Radio slot: " << _radio_slot);

    _tx_gain_profile_api = std::make_shared<rf_control::enumerated_gain_profile>(
        ZBX_GAIN_PROFILES, ZBX_GAIN_PROFILE_DEFAULT, ZBX_NUM_CHANS);
    _rx_gain_profile_api = std::make_shared<rf_control::enumerated_gain_profile>(
        ZBX_GAIN_PROFILES, ZBX_GAIN_PROFILE_DEFAULT, ZBX_NUM_CHANS);

    _expert_container =
        uhd::experts::expert_factory::create_container("zbx_radio_" + _radio_slot);
    _init_cpld();
    _init_peripherals();
    // Prop tree requires the initialization of certain peripherals
    _init_prop_tree();
    _expert_container->resolve_all();
}

zbx_dboard_impl::~zbx_dboard_impl()
{
    RFNOC_LOG_TRACE("zbx_dboard::dtor() ");
}

void zbx_dboard_impl::deinit()
{
    _wb_ifaces.clear();
}

void zbx_dboard_impl::set_command_time(uhd::time_spec_t time, const size_t chan)
{
    // When the command time gets updated, import it into the expert graph
    get_tree()
        ->access<time_spec_t>(fs_path("dboard") / "rx_frontends" / chan / "time/cmd")
        .set(time);
}

std::string zbx_dboard_impl::get_unique_id() const
{
    return _unique_id;
}


/******************************************************************************
 * API Calls
 *****************************************************************************/
void zbx_dboard_impl::set_tx_antenna(const std::string& ant, const size_t chan)
{
    RFNOC_LOG_TRACE("Setting TX antenna to " << ant << " for chan " << chan);
    if (!TX_ANTENNA_NAME_COMPAT_MAP.count(ant)) {
        assert_has(TX_ANTENNAS, ant, "tx antenna");
    }
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    _tree->access<std::string>(fe_path / "antenna" / "value").set(ant);
}

void zbx_dboard_impl::set_rx_antenna(const std::string& ant, const size_t chan)
{
    RFNOC_LOG_TRACE("Setting RX antenna to " << ant << " for chan " << chan);
    if (!RX_ANTENNA_NAME_COMPAT_MAP.count(ant)) {
        assert_has(RX_ANTENNAS, ant, "rx antenna");
    }

    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    _tree->access<std::string>(fe_path / "antenna" / "value").set(ant);
}

double zbx_dboard_impl::set_tx_frequency(const double req_freq, const size_t chan)
{
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    _tree->access<double>(fe_path / "freq").set(req_freq);

    // Our power manager sets a new gain value via the API, based on its new calculations.
    // Since the expert nodes are protected by a mutex, it will hang if we try to call
    // update_power() from inside the expert resolve methods (resolve() -> update_power()
    // -> set_tx_gain -> resolve())
    _tx_pwr_mgr.at(chan)->update_power();

    return _tree->access<double>(fe_path / "freq").get();
}

double zbx_dboard_impl::set_rx_frequency(const double req_freq, const size_t chan)
{
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    _tree->access<double>(fe_path / "freq").set(req_freq);

    // Our power manager sets a new gain value via the API, based on its new calculations.
    // Since the expert nodes are protected by a mutex, it will hang if we try to call
    // update_power() from inside the expert resolve methods (resolve() -> update_power()
    // -> set_rx_gain -> resolve())
    _rx_pwr_mgr.at(chan)->update_power();

    return _tree->access<double>(fe_path / "freq").get();
}

double zbx_dboard_impl::set_tx_bandwidth(const double bandwidth, const size_t chan)
{
    const double bw = get_tx_bandwidth(chan);
    if (!uhd::math::frequencies_are_equal(bandwidth, bw)) {
        RFNOC_LOG_WARNING("Invalid analog bandwidth: " << (bandwidth / 1e6) << " MHz.");
    }
    return bw;
}

double zbx_dboard_impl::get_tx_bandwidth(size_t chan)
{
    return _tree
        ->access<double>(_get_frontend_path(TX_DIRECTION, chan) / "bandwidth/value")
        .get();
}

double zbx_dboard_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    const double bw = get_rx_bandwidth(chan);
    if (!uhd::math::frequencies_are_equal(bandwidth, bw)) {
        RFNOC_LOG_WARNING("Invalid analog bandwidth: " << (bandwidth / 1e6) << " MHz.");
    }
    return bw;
}

double zbx_dboard_impl::get_rx_bandwidth(size_t chan)
{
    return _tree
        ->access<double>(_get_frontend_path(RX_DIRECTION, chan) / "bandwidth/value")
        .get();
}

double zbx_dboard_impl::set_tx_gain(
    const double gain, const std::string& name_, const size_t chan)
{
    // We have to accept the empty string for "all", because that's widely used
    // (e.g. by multi_usrp)
    const std::string name   = name_.empty() ? ZBX_GAIN_STAGE_ALL : name_;
    const fs_path gains_path = _get_frontend_path(TX_DIRECTION, chan) / "gains";
    const auto gain_profile  = _tx_gain_profile_api->get_gain_profile(chan);
    // Default gain profile: Setting anything other than 'all' is forbidden
    if (gain_profile == ZBX_GAIN_PROFILE_DEFAULT && name != ZBX_GAIN_STAGE_ALL) {
        throw uhd::key_error("Invalid gain name for gain profile 'default': " + name);
    }
    // Also, when the gain name is all, we have to be in default mode.
    if (gain_profile != ZBX_GAIN_PROFILE_DEFAULT && name == ZBX_GAIN_STAGE_ALL) {
        throw uhd::key_error(
            "Setting overall gain is only valid in gain profile 'default'!");
    }
    // The combination of the no-ATR profile, and any gain name other than 'table'
    // is not valid.
    if (gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR && name != ZBX_GAIN_STAGE_TABLE) {
        throw uhd::key_error("set_tx_gain(): Invalid combination of gain profile "
                             + gain_profile + " and gain name " + name);
    }
    // First, we handle the 'table' gain name. It's handled a bit differently
    // than the rest.
    if (name == ZBX_GAIN_STAGE_TABLE) {
        static const uhd::meta_range_t table_range(0, 255, 1);
        const uint8_t table_idx = uhd::narrow<uint8_t>(table_range.clip(gain, true));
        if (gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR) {
            _cpld->set_sw_config(chan, zbx_cpld_ctrl::atr_mode_target::DSA, table_idx);
            return static_cast<double>(table_idx);
        }
        if (gain_profile == ZBX_GAIN_PROFILE_MANUAL
            || gain_profile == ZBX_GAIN_PROFILE_CPLD) {
            _cpld->set_tx_gain_switches(chan, ATR_ADDR_TX, table_idx);
            _cpld->set_tx_gain_switches(chan, ATR_ADDR_XX, table_idx);
            return static_cast<double>(table_idx);
        }
        // That covers all the gain profiles for gain name 'table'.
        UHD_THROW_INVALID_CODE_PATH();
    }
    // Sanity check key. Note we do this after the previous gain stage, because
    // it's not a property node.
    if (!_tree->exists(gains_path / name)) {
        throw uhd::key_error("Invalid TX gain stage: " + name);
    }
    // This leaves directly setting either the DSAs or the amplifier. This is
    // possible in both the manual and CPLD gain profiles.
    return _tree->access<double>(gains_path / name / "value").set(gain).get();
}

double zbx_dboard_impl::set_rx_gain(
    const double gain, const std::string& name_, const size_t chan)
{
    // We have to accept the empty string for "all", because that's widely used
    // (e.g. by multi_usrp).
    const std::string name   = name_.empty() ? ZBX_GAIN_STAGE_ALL : name_;
    const fs_path gains_path = _get_frontend_path(RX_DIRECTION, chan) / "gains";
    const auto gain_profile  = _rx_gain_profile_api->get_gain_profile(chan);

    // Default gain profile: Setting anything other than ZBX_GAIN_STAGE_ALL is forbidden
    if (gain_profile == ZBX_GAIN_PROFILE_DEFAULT && name != ZBX_GAIN_STAGE_ALL) {
        throw uhd::key_error("Invalid gain name for gain profile 'default': " + name);
    }
    // Also, when the gain name is all, we have to be in default mode.
    if (gain_profile != ZBX_GAIN_PROFILE_DEFAULT && name == ZBX_GAIN_STAGE_ALL) {
        throw uhd::key_error(
            "Setting overall gain is only valid in gain profile 'default'!");
    }
    // The combination of the no-ATR profile, and any gain name other than 'table'
    // is not valid.
    if (gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR && name != ZBX_GAIN_STAGE_TABLE) {
        throw uhd::key_error("set_rx_gain(): Invalid combination of gain profile "
                             + gain_profile + " and gain name " + name);
    }
    // First, we handle the 'table' gain name. It's a bit different from the
    // rest.
    if (name == ZBX_GAIN_STAGE_TABLE) {
        static const uhd::meta_range_t table_range(0, 255, 1);
        const uint8_t table_idx = uhd::narrow<uint8_t>(table_range.clip(gain, true));
        if (gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR) {
            _cpld->set_sw_config(chan, zbx_cpld_ctrl::atr_mode_target::DSA, table_idx);
            return static_cast<double>(table_idx);
        }
        if (gain_profile == ZBX_GAIN_PROFILE_MANUAL
            || gain_profile == ZBX_GAIN_PROFILE_CPLD) {
            _cpld->set_rx_gain_switches(chan, ATR_ADDR_RX, table_idx);
            _cpld->set_rx_gain_switches(chan, ATR_ADDR_XX, table_idx);
            return static_cast<double>(table_idx);
        }
        // That covers all the gain profiles for gain name 'table'.
        UHD_THROW_INVALID_CODE_PATH();
    }
    // Sanity check key. Note we do this after the previous gain stage, because
    // it's not a property node.
    if (!_tree->exists(gains_path / name / "value")) {
        throw uhd::key_error("Invalid RX gain stage: " + name);
    }
    return _tree->access<double>(gains_path / name / "value").set(gain).get();
}

double zbx_dboard_impl::set_tx_gain(const double gain, const size_t chan)
{
    const auto gain_profile = _tx_gain_profile_api->get_gain_profile(chan);
    if (gain_profile == ZBX_GAIN_PROFILE_MANUAL) {
        const std::string err_msg = "When using 'manual' gain mode, calling "
                                    "set_tx_gain() without a gain name is not allowed!";
        RFNOC_LOG_ERROR(err_msg);
        throw uhd::runtime_error(err_msg);
    }
    if (gain_profile == ZBX_GAIN_PROFILE_CPLD
        || gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR) {
        return set_tx_gain(gain, ZBX_GAIN_STAGE_TABLE, chan);
    }
    return set_tx_gain(gain, ZBX_GAIN_STAGE_ALL, chan);
}

double zbx_dboard_impl::set_rx_gain(const double gain, const size_t chan)
{
    const auto gain_profile = _rx_gain_profile_api->get_gain_profile(chan);
    if (gain_profile == ZBX_GAIN_PROFILE_MANUAL) {
        const std::string err_msg = "When using 'manual' gain mode, calling "
                                    "set_rx_gain() without a gain name is not allowed!";
        RFNOC_LOG_ERROR(err_msg);
        throw uhd::runtime_error(err_msg);
    }
    if (gain_profile == ZBX_GAIN_PROFILE_CPLD
        || gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR) {
        return set_rx_gain(gain, ZBX_GAIN_STAGE_TABLE, chan);
    }
    return set_rx_gain(gain, ZBX_GAIN_STAGE_ALL, chan);
}

double zbx_dboard_impl::get_tx_gain(const size_t chan)
{
    const auto gain_profile = _tx_gain_profile_api->get_gain_profile(chan);
    if (gain_profile == ZBX_GAIN_PROFILE_CPLD
        || gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR) {
        return get_tx_gain(ZBX_GAIN_STAGE_TABLE, chan);
    }
    if (gain_profile == ZBX_GAIN_PROFILE_DEFAULT) {
        return get_tx_gain(ZBX_GAIN_STAGE_ALL, chan);
    }
    throw uhd::runtime_error(
        "get_tx_gain(): When in 'manual' gain profile, a gain name is required!");
}

double zbx_dboard_impl::get_rx_gain(const size_t chan)
{
    const auto gain_profile = _rx_gain_profile_api->get_gain_profile(chan);
    if (gain_profile == ZBX_GAIN_PROFILE_CPLD
        || gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR) {
        return get_rx_gain(ZBX_GAIN_STAGE_TABLE, chan);
    }
    if (gain_profile == ZBX_GAIN_PROFILE_DEFAULT) {
        return get_rx_gain(ZBX_GAIN_STAGE_ALL, chan);
    }
    throw uhd::runtime_error(
        "get_rx_gain(): When in 'manual' gain profile, a gain name is required!");
}

double zbx_dboard_impl::get_tx_gain(const std::string& name_, const size_t chan)
{
    // We have to accept the empty string for "all", because that's widely used
    // (e.g. by multi_usrp)
    const std::string name   = name_.empty() ? ZBX_GAIN_STAGE_ALL : name_;
    const fs_path gains_path = _get_frontend_path(TX_DIRECTION, chan) / "gains";
    const auto gain_profile  = _tx_gain_profile_api->get_gain_profile(chan);
    // Overall gain: Only reliable in 'default' mode. We warn, not throw, in
    // the other modes. That's because reading back the overall gain is common
    // diagnostic for many existing applications.
    if (name == ZBX_GAIN_STAGE_ALL && gain_profile != ZBX_GAIN_PROFILE_DEFAULT) {
        RFNOC_LOG_WARNING("get_tx_gain(): Trying to read back overall gain in "
                          "non-default gain profile is undefined.");
    }
    // Table gain: Returns the current DSA table index.
    if (name == ZBX_GAIN_STAGE_TABLE) {
        return static_cast<double>(
            _cpld->get_current_config(chan, zbx_cpld_ctrl::atr_mode_target::DSA));
    }
    // Otherwise: DSA or amp. Sanity check key is valid. Because the table gain
    // is not a property tree node, this check comes after the previous if-clause.
    if (!_tree->exists(gains_path / name / "value")) {
        RFNOC_LOG_ERROR("get_tx_gain(): Invalid gain name `" << name << "'");
        throw uhd::key_error(std::string("get_tx_gain(): Invalid gain name: ") + name);
    }
    // We're not yet done: If we're in CPLD/table profiles, we peek the current
    // DSA settings and apply them to the local cache.
    // Note: This means we have a different behaviour between directly accessing
    // the prop tree, or accessing the C++ API.
    if ((name == ZBX_GAIN_STAGE_DSA1 || name == ZBX_GAIN_STAGE_DSA2)
        && (gain_profile == ZBX_GAIN_PROFILE_CPLD
            || gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR)) {
        const uint8_t idx =
            (gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR)
                ? _cpld->get_current_config(chan, zbx_cpld_ctrl::atr_mode_target::DSA)
                : ATR_ADDR_TX;
        constexpr bool update_cache = true; // Make sure to peek the actual value
        const auto dsa = (name == ZBX_GAIN_STAGE_DSA1) ? zbx_cpld_ctrl::dsa_type::DSA1
                                                       : zbx_cpld_ctrl::dsa_type::DSA2;
        const uint8_t dsa_val = _cpld->get_tx_dsa(chan, idx, dsa, update_cache);
        // Update the tree because we're good citizens, and if we switch the
        // gain profile from 'table' to 'manual', we want everything to be
        // consistent. This will not cause a poke to the CPLD, b/c the experts
        // won't write gains in this gain profile.
        // Note that the other DSA values in the tree are not updated automatically,
        // which is why we can't write DSA values to the CPLD in this mode. If
        // we want to allow writing DSA values in this mode, we need to update
        // everything here, or put some more cleverness into the programming
        // expert.
        _tree->access<double>(gains_path / name / "value")
            .set(ZBX_TX_DSA_MAX_ATT - dsa_val);
    }
    // Now return the value from the tree
    return _tree->access<double>(gains_path / name / "value").get();
}

double zbx_dboard_impl::get_rx_gain(const std::string& name_, const size_t chan)
{
    // We have to accept the empty string for "all", because that's widely used
    // (e.g. by multi_usrp)
    const std::string name   = name_.empty() ? ZBX_GAIN_STAGE_ALL : name_;
    const fs_path gains_path = _get_frontend_path(RX_DIRECTION, chan) / "gains";
    const auto gain_profile = _rx_gain_profile_api->get_gain_profile(chan);
    // Overall gain: Only reliable in 'default' mode. We warn, not throw, in
    // the other modes. That's because reading back the overall gain is common
    // diagnostic for many existing applications.
    if (name == ZBX_GAIN_STAGE_ALL && gain_profile != ZBX_GAIN_PROFILE_DEFAULT) {
        RFNOC_LOG_WARNING("get_rx_gain(): Trying to read back overall gain in "
                          "non-default gain profile is undefined.");
    }
    // Table gain: Returns the current DSA table index.
    if (name == ZBX_GAIN_STAGE_TABLE) {
        return static_cast<double>(
            _cpld->get_current_config(chan, zbx_cpld_ctrl::atr_mode_target::DSA));
    }
    // Otherwise: DSA. Sanity check key is valid. Because the table gain is not
    // a property tree node, this check comes after the previous if-clause.
    if (!_tree->exists(gains_path / name / "value")) {
        RFNOC_LOG_ERROR("get_rx_gain(): Invalid gain name `" << name << "'");
        throw uhd::key_error(std::string("get_rx_gain(): Invalid gain name: ") + name);
    }
    // We're not yet done: If we're in CPLD/table profiles, we peek the current
    // DSA settings and apply them to the local cache.
    // Note: This means we have a different behaviour between directly accessing
    // the prop tree, or accessing the C++ API.
    if (gain_profile == ZBX_GAIN_PROFILE_CPLD
        || gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR) {
        const uint8_t idx =
            (gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR)
                ? _cpld->get_current_config(chan, zbx_cpld_ctrl::atr_mode_target::DSA)
                : ATR_ADDR_RX;
        constexpr bool update_cache = true; // Make sure to peek the actual value
        static const std::map<std::string, zbx_cpld_ctrl::dsa_type> dsa_map{
            {ZBX_GAIN_STAGE_DSA1, zbx_cpld_ctrl::dsa_type::DSA1},
            {ZBX_GAIN_STAGE_DSA2, zbx_cpld_ctrl::dsa_type::DSA2},
            {ZBX_GAIN_STAGE_DSA3A, zbx_cpld_ctrl::dsa_type::DSA3A},
            {ZBX_GAIN_STAGE_DSA3B, zbx_cpld_ctrl::dsa_type::DSA3B},
        };
        const auto dsa        = dsa_map.at(name);
        const uint8_t dsa_val = _cpld->get_rx_dsa(chan, idx, dsa, update_cache);
        // Update the tree because we're good citizens, and if we switch the
        // gain profile from 'table' to 'manual', we want everything to be
        // consistent. This will not cause a poke to the CPLD, b/c the experts
        // won't write gains in this gain profile.
        // Note that the other DSA values in the tree are not updated automatically,
        // which is why we can't write DSA values to the CPLD in this profile. If
        // we want to allow writing DSA values in this profile, we need to update
        // everything here, or put some more cleverness into the programming
        // expert.
        _tree->access<double>(gains_path / name / "value")
            .set(static_cast<double>(ZBX_RX_DSA_MAX_ATT - dsa_val));
    }
    return _tree->access<double>(gains_path / name / "value").get();
}

std::vector<std::string> zbx_dboard_impl::get_tx_gain_names(const size_t chan) const
{
    UHD_ASSERT_THROW(chan < ZBX_NUM_CHANS);
    const std::string gain_profile = _tx_gain_profile_api->get_gain_profile(chan);

    if (gain_profile == ZBX_GAIN_PROFILE_DEFAULT) {
        return {ZBX_GAIN_STAGE_ALL};
    }
    if (gain_profile == ZBX_GAIN_PROFILE_CPLD
        || gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR) {
        return {ZBX_GAIN_STAGE_TABLE};
    }
    return ZBX_TX_GAIN_STAGES;
}

std::vector<std::string> zbx_dboard_impl::get_rx_gain_names(const size_t chan) const
{
    UHD_ASSERT_THROW(chan < ZBX_NUM_CHANS);
    const std::string gain_profile = _rx_gain_profile_api->get_gain_profile(chan);

    if (gain_profile == ZBX_GAIN_PROFILE_DEFAULT) {
        return {ZBX_GAIN_STAGE_ALL};
    }
    if (gain_profile == ZBX_GAIN_PROFILE_CPLD
        || gain_profile == ZBX_GAIN_PROFILE_CPLD_NOATR) {
        return {ZBX_GAIN_STAGE_TABLE};
    }
    return ZBX_RX_GAIN_STAGES;
}

const std::string zbx_dboard_impl::get_tx_lo_source(
    const std::string& name, const size_t chan)
{
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);
    if (!_tree->exists(fe_path / "ch" / name)) {
        throw uhd::value_error("get_tx_lo_source(): Invalid LO name: " + name);
    }

    const zbx_lo_source_t lo_source =
        _tree->access<zbx_lo_source_t>(fe_path / "ch" / name / "source").get();
    return lo_source == zbx_lo_source_t::internal ? "internal" : "external";
}

const std::string zbx_dboard_impl::get_rx_lo_source(
    const std::string& name, const size_t chan)
{
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);
    if (!_tree->exists(fe_path / "ch" / name)) {
        throw uhd::value_error("get_rx_lo_source(): Invalid LO name: " + name);
    }

    const zbx_lo_source_t lo_source =
        _tree->access<zbx_lo_source_t>(fe_path / "ch" / name / "source").get();
    return lo_source == zbx_lo_source_t::internal ? "internal" : "external";
}

void zbx_dboard_impl::set_rx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_rx_lo_source(name=" << name << ", src=" << src << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);
    if (!_tree->exists(fe_path / "ch" / name)) {
        throw uhd::value_error("set_rx_lo_source(): Invalid LO name: " + name);
    }

    _tree->access<zbx_lo_source_t>(fe_path / "ch" / name / "source")
        .set(src == "internal" ? zbx_lo_source_t::internal : zbx_lo_source_t::external);
}

void zbx_dboard_impl::set_tx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_lo_source(name=" << name << ", src=" << src << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);
    if (!_tree->exists(fe_path / "ch" / name)) {
        throw uhd::value_error("set_tx_lo_source(): Invalid LO name: " + name);
    }

    _tree->access<zbx_lo_source_t>(fe_path / "ch" / name / "source")
        .set(src == "internal" ? zbx_lo_source_t::internal : zbx_lo_source_t::external);
}

double zbx_dboard_impl::set_tx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_lo_freq(freq=" << freq << ", name=" << name << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);
    assert_has(ZBX_LOS, name);

    return _tree->access<double>(fe_path / "los" / name / "freq" / "value").set(freq).get();
}

double zbx_dboard_impl::get_tx_lo_freq(const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_tx_lo_freq(name=" << name << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);
    assert_has(ZBX_LOS, name);

    return _tree->access<double>(fe_path / "los" / name / "freq" / "value").get();
}

freq_range_t zbx_dboard_impl::_get_lo_freq_range(
    const std::string& name, const size_t /*chan*/) const
{
    if (name == ZBX_LO1 || name == ZBX_LO2) {
        // Note this doesn't include the LO step size. The LO step size is only
        // used when the LO frequencies are automatically calculated (which is
        // the normal use case). When setting LO frequencies manually, it is
        // possible to set LOs to values outside of the step size.
        return freq_range_t{LMX2572_MIN_FREQ, LMX2572_MAX_FREQ};
    }
    if (name == RFDC_NCO) {
        // It might make sense to constrain the possible NCO values more, since
        // the bandpass filters for IF2 only allow a certain range. Note that LO1
        // and LO2 freq ranges are also constrained by their analog filters.
        // But in principle, this is the range for the NCO... so why not.
        return freq_range_t{0.0, _rfdc_rate};
    }
    throw uhd::value_error("Invalid LO name: " + name);
}

double zbx_dboard_impl::set_rx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_rx_lo_freq(freq=" << freq << ", name=" << name << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);
    assert_has(ZBX_LOS, name);

    return _tree->access<double>(fe_path / "los" / name / "freq" / "value")
        .set(freq)
        .get();
}

double zbx_dboard_impl::get_rx_lo_freq(const std::string& name, size_t chan)
{
    RFNOC_LOG_TRACE("get_rx_lo_freq(name=" << name << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);
    assert_has(ZBX_LOS, name);

    return _tree->access<double>(fe_path / "los" / name / "freq" / "value").get();
}

std::string zbx_dboard_impl::get_tx_antenna(size_t chan) const
{
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);
    return _tree->access<std::string>(fe_path / "antenna" / "value").get();
}

std::string zbx_dboard_impl::get_rx_antenna(size_t chan) const
{
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);
    return _tree->access<std::string>(fe_path / "antenna" / "value").get();
}

double zbx_dboard_impl::get_tx_frequency(size_t chan)
{
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);
    return _tree->access<double>(fe_path / "freq").get();
}

double zbx_dboard_impl::get_rx_frequency(size_t chan)
{
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);
    return _tree->access<double>(fe_path / "freq").get();
}

void zbx_dboard_impl::set_tx_tune_args(const uhd::device_addr_t&, const size_t)
{
    RFNOC_LOG_TRACE("tune_args not supported by this radio.");
}

void zbx_dboard_impl::set_rx_tune_args(const uhd::device_addr_t&, const size_t)
{
    RFNOC_LOG_TRACE("tune_args not supported by this radio.");
}

void zbx_dboard_impl::set_rx_agc(const bool, const size_t)
{
    throw uhd::not_implemented_error("set_rx_agc() is not supported on this radio!");
}

uhd::gain_range_t zbx_dboard_impl::get_tx_gain_range(
    const std::string& name, const size_t chan) const
{
    // We have to accept the empty string for "all", because that's widely used
    // (e.g. by multi_usrp)
    if (!name.empty() && name != ZBX_GAIN_STAGE_ALL) {
        throw uhd::value_error(
            std::string("get_tx_gain_range(): Unknown gain name '") + name + "'!");
    }
    return get_tx_gain_range(chan);
}

uhd::gain_range_t zbx_dboard_impl::get_rx_gain_range(
    const std::string& name, const size_t chan) const
{
    // We have to accept the empty string for "all", because that's widely used
    // (e.g. by multi_usrp)
    if (!name.empty() && name != ZBX_GAIN_STAGE_ALL) {
        throw uhd::value_error(
            std::string("get_rx_gain_range(): Unknown gain name '") + name + "'!");
    }
    return get_rx_gain_range(chan);
}

void zbx_dboard_impl::set_rx_lo_export_enabled(bool, const std::string&, const size_t)
{
    throw uhd::not_implemented_error(
        "set_rx_lo_export_enabled is not supported on this radio");
}

bool zbx_dboard_impl::get_rx_lo_export_enabled(const std::string&, const size_t)
{
    return false;
}

void zbx_dboard_impl::set_tx_lo_export_enabled(bool, const std::string&, const size_t)
{
    throw uhd::not_implemented_error(
        "set_rx_lo_export_enabled is not supported on this radio");
}

bool zbx_dboard_impl::get_tx_lo_export_enabled(const std::string&, const size_t)
{
    return false;
}

/******************************************************************************
 * EEPROM API
 *****************************************************************************/
eeprom_map_t zbx_dboard_impl::get_db_eeprom()
{
    return _mb_rpcc->get_db_eeprom(_db_idx);
}

size_t zbx_dboard_impl::get_chan_from_dboard_fe(
    const std::string& fe, const uhd::direction_t) const
{
    if (fe == "0") {
        return 0;
    }
    if (fe == "1") {
        return 1;
    }
    throw uhd::key_error(std::string("[X400] Invalid frontend: ") + fe);
}

std::string zbx_dboard_impl::get_dboard_fe_from_chan(
    const size_t chan, const uhd::direction_t) const
{
    if (chan == 0) {
        return "0";
    }
    if (chan == 1) {
        return "1";
    }
    throw uhd::lookup_error(
        std::string("[X400] Invalid channel: ") + std::to_string(chan));
}

/*********************************************************************
 *   Private misc/calculative helper functions
 **********************************************************************/

bool zbx_dboard_impl::_get_all_los_locked(const direction_t dir, const size_t chan)
{
    const fs_path fe_path = _get_frontend_path(dir, chan);

    const bool is_lo1_enabled = _tree->access<bool>(fe_path / ZBX_LO1 / "enabled").get();
    const bool is_lo1_locked =
        _lo_ctrl_map.at(zbx_lo_ctrl::lo_string_to_enum(dir, chan, ZBX_LO1))
            ->get_lock_status();
    // LO2 is always enabled via center frequency tuning, but users may manually disable
    // it
    const bool is_lo2_enabled = _tree->access<bool>(fe_path / ZBX_LO2 / "enabled").get();
    const bool is_lo2_locked =
        _lo_ctrl_map.at(zbx_lo_ctrl::lo_string_to_enum(dir, chan, ZBX_LO2))
            ->get_lock_status();
    // We only care about the lock status if it's enabled (lowband center frequency)
    // That means we have set it to true if is_lo[1,2]_enabled is *false*, but check for
    // the lock if is_lo[1,2]_enabled is *true*
    return (!is_lo1_enabled || is_lo1_locked) && (!is_lo2_enabled || is_lo2_locked);
}

fs_path zbx_dboard_impl::_get_frontend_path(
    const direction_t dir, const size_t chan_idx) const
{
    UHD_ASSERT_THROW(chan_idx < ZBX_NUM_CHANS);
    const std::string frontend = dir == TX_DIRECTION ? "tx_frontends" : "rx_frontends";
    return fs_path("dboard") / frontend / chan_idx;
}

std::vector<uhd::usrp::pwr_cal_mgr::sptr>& zbx_dboard_impl::get_pwr_mgr(
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

}}} // namespace uhd::usrp::zbx
