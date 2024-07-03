//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/database.hpp>
#include <uhd/exception.hpp>
#include <uhd/experts/expert_container.hpp>
#include <uhd/experts/expert_factory.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/property_tree.ipp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/reg_iface_adapter.hpp>
#include <uhdlib/usrp/dboard/fbx/fbx_constants.hpp>
#include <uhdlib/usrp/dboard/fbx/fbx_dboard.hpp>
#include <uhdlib/usrp/dboard/fbx/fbx_expert.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <vector>

using namespace uhd;
using namespace uhd::experts;
using namespace uhd::rfnoc;

// ostream << operator overloads for our enum classes, so that property nodes of that type
// can be added to our expert graph
namespace uhd { namespace usrp { namespace fbx {

void fbx_dboard_impl::_init_fbx_ctrl()
{
    RFNOC_LOG_TRACE("Initializing FBX CTRL");
    _fbx_ctrl = std::make_shared<fbx_ctrl>(
        [this](const uint32_t addr, const uint32_t data, const fbx_ctrl::chan_t chan) {
            const auto time_spec = (chan == fbx_ctrl::NO_CHAN) ? time_spec_t::ASAP
                                   : (chan == fbx_ctrl::CHAN1) ? _time_accessor(1)
                                                               : _time_accessor(0);
            _regs.poke32(_reg_base_address + addr, data, time_spec);
        },
        [this](const uint32_t addr) {
            // We don't do timed peeks, so no chan parameter here.
            return _regs.peek32(_reg_base_address + addr);
        },
        [this](const uhd::time_spec_t& sleep_time) { _regs.sleep(sleep_time); },
        get_unique_id() + "::FBX_CTRL");
    UHD_ASSERT_THROW(_fbx_ctrl);
}

void fbx_dboard_impl::_init_prop_tree()
{
    auto subtree = get_tree()->subtree(fs_path("dboard"));

    // Construct RX frontend
    for (size_t chan_idx = 0; chan_idx < get_num_rx_channels(); chan_idx++) {
        const fs_path fe_path = fs_path("rx_frontends") / chan_idx;

        // Command time needs to be shadowed into the property tree so we can use
        // it in the expert graph. TX and RX share the command time, so we could
        // put it onto its own sub-tree, or copy the property between TX and RX.
        // With respect to TwinRX and trying to keep the tree lean and browsable,
        // we compromise and put the command time onto the RX frontend path, even
        // though it's also valid for TX.
        // This data node will be used for scheduling the other experts:
        expert_factory::add_data_node<time_spec_t>(
            _expert_container, fe_path / "time/fe", time_spec_t(0.0));
        // This prop node will be used to import the command time into the
        // graph:
        expert_factory::add_prop_node<time_spec_t>(
            _expert_container, subtree, fe_path / "time/cmd", time_spec_t(0.0));

        _init_frontend_subtree(subtree, RX_DIRECTION, chan_idx, fe_path);

        // The time nodes get connected with one scheduling expert per channel:
        expert_factory::add_worker_node<fbx_scheduling_expert>(
            _expert_container, _expert_container->node_retriever(), fe_path);
    }

    // Construct TX frontend
    // Note: the TX frontend uses the RX property tree, this must
    // be constructed after the RX frontend
    for (size_t chan_idx = 0; chan_idx < get_num_tx_channels(); chan_idx++) {
        const fs_path fe_path = fs_path("tx_frontends") / chan_idx;
        _init_frontend_subtree(subtree, TX_DIRECTION, chan_idx, fe_path);
    }

    // Now add the sync worker:
    UHD_ASSERT_THROW(get_num_rx_channels() == get_num_tx_channels());
    expert_factory::add_worker_node<fbx_sync_expert>(_expert_container,
        _expert_container->node_retriever(),
        get_num_rx_channels(),
        fs_path("tx_frontends"),
        fs_path("rx_frontends"),
        _rfdcc);

    subtree->create<eeprom_map_t>("eeprom")
        .add_coerced_subscriber([](const eeprom_map_t&) {
            throw uhd::runtime_error("Attempting to update daughterboard eeprom!");
        })
        .set_publisher([this]() { return get_db_eeprom(); });
}

void fbx_dboard_impl::_init_frontend_subtree(uhd::property_tree::sptr subtree,
    const uhd::direction_t trx,
    const size_t chan_idx,
    const fs_path fe_path)
{
    static constexpr char FBX_FE_NAME[] = "FBX";

    RFNOC_LOG_TRACE("Adding non-RFNoC block properties for channel "
                    << chan_idx << " to prop tree path " << fe_path);
    // Standard attributes
    subtree->create<std::string>(fe_path / "name").set(FBX_FE_NAME);
    subtree->create<std::string>(fe_path / "connection").set("IQ");

    _init_frequency_prop_tree(subtree, _expert_container, trx, chan_idx, fe_path);
    _init_gain_prop_tree(subtree, _expert_container, fe_path);
    _init_antenna_prop_tree(subtree, _expert_container, trx, chan_idx, fe_path);
    _init_lo_prop_tree(subtree, trx, chan_idx, fe_path);

    _init_experts(_expert_container, trx, chan_idx, fe_path);
}


void fbx_dboard_impl::_init_experts(expert_container::sptr expert,
    const uhd::direction_t trx,
    const size_t chan_idx,
    const fs_path fe_path)
{
    RFNOC_LOG_TRACE(fe_path + ", Creating experts...");

    if (trx == TX_DIRECTION) {
        expert_factory::add_worker_node<fbx_tx_programming_expert>(expert,
            expert->node_retriever(),
            fe_path,
            fs_path("rx_frontends") / chan_idx,
            chan_idx,
            _fbx_ctrl);

    } else {
        expert_factory::add_worker_node<fbx_rx_programming_expert>(
            expert, expert->node_retriever(), fe_path, chan_idx, _fbx_ctrl);
    }

    expert_factory::add_worker_node<fbx_band_inversion_expert>(
        expert, expert->node_retriever(), fe_path, trx, chan_idx, _rfdc_rate, _rpcc);

    expert_factory::add_worker_node<fbx_rfdc_freq_expert>(expert,
        expert->node_retriever(),
        fe_path,
        trx,
        chan_idx,
        _rfdc_rate,
        _rpc_prefix,
        _db_idx,
        _mb_rpcc);

    RFNOC_LOG_TRACE(fe_path + ", Experts created");
}

void fbx_dboard_impl::_init_frequency_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
    const size_t chan_idx,
    const fs_path fe_path)
{
    expert_factory::add_dual_prop_node<double>(
        expert, subtree, fe_path / "freq", FBX_DEFAULT_FREQ, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_dual_prop_node<double>(
        expert, subtree, fe_path / "if_freq", 0.0, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_dual_prop_node<double>(expert,
        subtree,
        fe_path / "los" / RFDC_NCO / "freq" / "value",
        // Initialize with current value
        _mb_rpcc->rfdc_get_nco_freq(trx == TX_DIRECTION ? "tx" : "rx", _db_idx, chan_idx),
        AUTO_RESOLVE_ON_WRITE);

    subtree->create<double>(fe_path / "bandwidth" / "value")
        .set(FBX_DEFAULT_BANDWIDTH)
        .set_coercer([](const double) { return FBX_DEFAULT_BANDWIDTH; });
    subtree->create<meta_range_t>(fe_path / "bandwidth" / "range")
        .set({FBX_DEFAULT_BANDWIDTH, FBX_DEFAULT_BANDWIDTH})
        .set_coercer([](const meta_range_t&) {
            return meta_range_t(FBX_DEFAULT_BANDWIDTH, FBX_DEFAULT_BANDWIDTH);
        });
    subtree->create<meta_range_t>(fe_path / "freq" / "range")
        .set(FBX_FREQ_RANGE)
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update freq range!");
        });
}

void fbx_dboard_impl::_init_gain_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const fs_path fe_path)
{
    // To satisfy RFNoC we need to create the path, although we're not going to use it.
    const auto gain_base_path = fe_path / "gains";
    expert_factory::add_dual_prop_node<double>(
        expert, subtree, gain_base_path / "all" / "value", 0, AUTO_RESOLVE_ON_WRITE);
    subtree->create<meta_range_t>(fe_path / "gains" / "all" / "range")
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update gain range!");
        })
        .set_publisher([]() {
            const auto empty_range = uhd::meta_range_t(0, 0, 0);
            return empty_range;
        });
}

void fbx_dboard_impl::_init_antenna_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
    const size_t chan_idx,
    const fs_path fe_path)
{
    const std::string default_ant = trx == TX_DIRECTION ? DEFAULT_TX_ANTENNA
                                                        : DEFAULT_RX_ANTENNA;
    expert_factory::add_prop_node<std::string>(expert,
        subtree,
        fe_path / "antenna" / "value",
        default_ant,
        AUTO_RESOLVE_ON_WRITE);
    subtree->access<std::string>(fe_path / "antenna" / "value")
        .set_coercer([trx](const std::string& ant_name) {
            const auto ant_map = trx == TX_DIRECTION ? TX_ANTENNA_NAME_COMPAT_MAP
                                                     : RX_ANTENNA_NAME_COMPAT_MAP;
            return ant_map.count(ant_name) ? ant_map.at(ant_name) : ant_name;
        });
    subtree->create<std::vector<std::string>>(fe_path / "antenna" / "options")
        .set(trx == TX_DIRECTION ? get_tx_antennas(chan_idx) : get_rx_antennas(chan_idx))
        .add_coerced_subscriber([](const std::vector<std::string>&) {
            throw uhd::runtime_error("Attempting to update antenna options!");
        });
}

void fbx_dboard_impl::_init_lo_prop_tree(uhd::property_tree::sptr subtree,
    const uhd::direction_t,
    const size_t,
    const fs_path fe_path)
{
    // LO lock sensor
    // as there are no LOs on the boards we can assume lo_locked to be
    // always true
    subtree->create<sensor_value_t>(fe_path / "sensors" / "lo_locked")
        .set(sensor_value_t("all_los", false, "locked", "unlocked"))
        .add_coerced_subscriber([](const sensor_value_t&) {
            throw uhd::runtime_error("Attempting to write to sensor!");
        })
        .set_publisher(
            []() { return sensor_value_t("all_los", true, "locked", "unlocked"); });
    subtree->create<sensor_value_t>(fe_path / "sensors" / "nco_locked")
        .add_coerced_subscriber([](const sensor_value_t&) {
            throw uhd::runtime_error("Attempting to write to sensor!");
        })
        .set_publisher([this]() {
            return sensor_value_t(
                RFDC_NCO, this->_rfdcc->get_nco_reset_done(), "locked", "unlocked");
        });
}
}}} // namespace uhd::usrp::fbx
