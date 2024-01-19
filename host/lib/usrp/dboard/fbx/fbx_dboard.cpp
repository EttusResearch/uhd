//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/direction.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/dboard/fbx/fbx_dboard.hpp>
#include <cstdlib>
#include <sstream>

namespace uhd { namespace usrp { namespace fbx {

/******************************************************************************
 * Structors
 *****************************************************************************/
fbx_dboard_impl::fbx_dboard_impl(register_iface& reg_iface,
    const size_t reg_base_address,
    time_accessor_fn_type&& time_accessor,
    const size_t db_idx,
    const std::string& radio_slot,
    const size_t num_tx_chans,
    const size_t num_rx_chans,
    const std::string& rpc_prefix,
    const std::string& unique_id,
    uhd::usrp::x400_rpc_iface::sptr mb_rpcc,
    uhd::usrp::fbx_rpc_iface::sptr rpcc,
    uhd::rfnoc::x400::rfdc_control::sptr rfdcc,
    uhd::property_tree::sptr tree)
    : nameless_gain_mixin([](const uhd::direction_t, size_t) { return ""; })
    , _unique_id(unique_id)
    , _num_tx_chans(num_tx_chans)
    , _num_rx_chans(num_rx_chans)
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
{
    RFNOC_LOG_TRACE("Entering fbx_dboard_impl ctor...");
    RFNOC_LOG_TRACE("Radio slot: " << _radio_slot);
    UHD_ASSERT_THROW(_num_rx_chans <= FBX_MAX_NUM_CHANS);
    UHD_ASSERT_THROW(_num_tx_chans <= FBX_MAX_NUM_CHANS);

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
        FBX_GAIN_PROFILES, FBX_GAIN_PROFILE_DEFAULT, get_num_tx_channels());
    _rx_gain_profile_api = std::make_shared<rf_control::enumerated_gain_profile>(
        FBX_GAIN_PROFILES, FBX_GAIN_PROFILE_DEFAULT, get_num_rx_channels());

    _expert_container =
        uhd::experts::expert_factory::create_container("fbx_radio_" + _radio_slot);
    _init_fbx_ctrl();
    // Prop tree requires the initialization of certain peripherals
    _init_prop_tree();
    _expert_container->resolve_all();
}

fbx_dboard_impl::~fbx_dboard_impl()
{
    RFNOC_LOG_TRACE("fbx_dboard::dtor()");
}

void fbx_dboard_impl::deinit()
{
    _wb_ifaces.clear();
}

void fbx_dboard_impl::set_command_time(uhd::time_spec_t time, const size_t chan)
{
    // When the command time gets updated, import it into the expert graph
    get_tree()
        ->access<time_spec_t>(fs_path("dboard") / "rx_frontends" / chan / "time/cmd")
        .set(time);
}

std::string fbx_dboard_impl::get_unique_id() const
{
    return _unique_id;
}

/******************************************************************************
 * API Calls
 *****************************************************************************/

std::string fbx_dboard_impl::get_rx_antenna(const size_t chan) const
{
    std::string ant = _rx_antenna->get_antenna(chan);
    if (ant == ANTENNA_SYNC_INT || ant == ANTENNA_SYNC_EXT) {
        return _fbx_ctrl->get_rx_sync_switch_state();
    }
    return _rx_antenna->get_antenna(chan);
}

double fbx_dboard_impl::set_tx_frequency(const double req_freq, const size_t chan)
{
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    _tree->access<double>(fe_path / "freq").set(req_freq);

    return _tree->access<double>(fe_path / "freq").get();
}

double fbx_dboard_impl::set_rx_frequency(const double req_freq, const size_t chan)
{
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    _tree->access<double>(fe_path / "freq").set(req_freq);

    return _tree->access<double>(fe_path / "freq").get();
}

double fbx_dboard_impl::set_tx_bandwidth(const double bandwidth, const size_t chan)
{
    const double bw = get_tx_bandwidth(chan);
    if (!uhd::math::frequencies_are_equal(bandwidth, bw)) {
        RFNOC_LOG_WARNING("Invalid analog bandwidth: " << (bandwidth / 1e6) << " MHz.");
    }
    return bw;
}

double fbx_dboard_impl::get_tx_bandwidth(size_t chan)
{
    return _tree
        ->access<double>(_get_frontend_path(TX_DIRECTION, chan) / "bandwidth/value")
        .get();
}

double fbx_dboard_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    const double bw = get_rx_bandwidth(chan);
    if (!uhd::math::frequencies_are_equal(bandwidth, bw)) {
        RFNOC_LOG_WARNING("Invalid analog bandwidth: " << (bandwidth / 1e6) << " MHz.");
    }
    return bw;
}

double fbx_dboard_impl::get_rx_bandwidth(size_t chan)
{
    return _tree
        ->access<double>(_get_frontend_path(RX_DIRECTION, chan) / "bandwidth/value")
        .get();
}

double fbx_dboard_impl::get_tx_frequency(size_t chan)
{
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);
    return _tree->access<double>(fe_path / "freq").get();
}

double fbx_dboard_impl::get_rx_frequency(size_t chan)
{
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);
    return _tree->access<double>(fe_path / "freq").get();
}


fs_path fbx_dboard_impl::_get_frontend_path(
    const direction_t dir, const size_t chan_idx) const
{
    UHD_ASSERT_THROW(
        chan_idx < (dir == TX_DIRECTION ? get_num_tx_channels() : get_num_rx_channels()));
    const std::string frontend = dir == TX_DIRECTION ? "tx_frontends" : "rx_frontends";
    return fs_path("dboard") / frontend / chan_idx;
}

double fbx_dboard_impl::set_tx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    _validate_lo_name(name, "set_tx_lo_freq");

    RFNOC_LOG_TRACE("set_tx_lo_freq(freq=" << freq << ", name=" << name << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    return _tree->access<double>(fe_path / "los" / name / "freq" / "value")
        .set(freq)
        .get();
}

double fbx_dboard_impl::get_tx_lo_freq(const std::string& name, const size_t chan)
{
    _validate_lo_name(name, "get_tx_lo_freq");

    RFNOC_LOG_TRACE("get_tx_lo_freq(name=" << name << ")");
    const fs_path fe_path = _get_frontend_path(TX_DIRECTION, chan);

    return _tree->access<double>(fe_path / "los" / name / "freq" / "value").get();
}

double fbx_dboard_impl::set_rx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    _validate_lo_name(name, "set_rx_lo_freq");

    RFNOC_LOG_TRACE("set_rx_lo_freq(freq=" << freq << ", name=" << name << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    return _tree->access<double>(fe_path / "los" / name / "freq" / "value")
        .set(freq)
        .get();
}

double fbx_dboard_impl::get_rx_lo_freq(const std::string& name, size_t chan)
{
    _validate_lo_name(name, "get_rx_lo_freq");

    RFNOC_LOG_TRACE("get_rx_lo_freq(name=" << name << ")");
    const fs_path fe_path = _get_frontend_path(RX_DIRECTION, chan);

    return _tree->access<double>(fe_path / "los" / name / "freq" / "value").get();
}

freq_range_t fbx_dboard_impl::_get_lo_freq_range(
    const std::string& name, const size_t /*chan*/) const
{
    _validate_lo_name(name, "_get_lo_freq_range");
    return freq_range_t{0.0, _rfdc_rate};
}

void fbx_dboard_impl::_validate_lo_name(
    const std::string& name, const std::string& caller) const
{
    if (name != RFDC_NCO) {
        throw uhd::value_error("Invalid LO name: " + name + " called from " + caller);
    }
}

/******************************************************************************
 * EEPROM API
 *****************************************************************************/
eeprom_map_t fbx_dboard_impl::get_db_eeprom()
{
    return _mb_rpcc->get_db_eeprom(_db_idx);
}

size_t fbx_dboard_impl::get_chan_from_dboard_fe(
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

std::string fbx_dboard_impl::get_dboard_fe_from_chan(
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
bool fbx_dboard_impl::select_adc_self_cal_gain(size_t chan)
{
    return _mb_rpcc->get_threshold_status(_db_idx, chan, 0);
}

}}} // namespace uhd::usrp::fbx
