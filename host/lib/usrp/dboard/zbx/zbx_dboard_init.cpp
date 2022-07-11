//
// Copyright 2020 Ettus Research, a National Instruments Brand
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
#include <uhdlib/usrp/dboard/zbx/zbx_constants.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_dboard.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_expert.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <vector>

using namespace uhd;
using namespace uhd::experts;
using namespace uhd::rfnoc;

// ostream << operator overloads for our enum classes, so that property nodes of that type
// can be added to our expert graph
namespace uhd { namespace usrp { namespace zbx {

std::ostream& operator<<(
    std::ostream& os, const ::uhd::usrp::zbx::zbx_lo_source_t& lo_source)
{
    switch (lo_source) {
        case ::uhd::usrp::zbx::zbx_lo_source_t::internal:
            os << "internal";
            return os;
        case ::uhd::usrp::zbx::zbx_lo_source_t::external:
            os << "external";
            return os;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

std::ostream& operator<<(
    std::ostream& os, const ::uhd::usrp::zbx::zbx_cpld_ctrl::atr_mode& atr)
{
    switch (atr) {
        case ::uhd::usrp::zbx::zbx_cpld_ctrl::atr_mode::SW_DEFINED:
            os << "SW_DEFINED";
            return os;
        case ::uhd::usrp::zbx::zbx_cpld_ctrl::atr_mode::CLASSIC_ATR:
            os << "CLASSIC ATR";
            return os;
        case ::uhd::usrp::zbx::zbx_cpld_ctrl::atr_mode::FPGA_STATE:
            os << "FPGA_STATE";
            return os;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

std::ostream& operator<<(
    std::ostream& os, const std::vector<::uhd::usrp::zbx::tune_map_item_t>& tune_map)
{
    os << "Tune map with " << tune_map.size() << " entries";
    return os;
}

void zbx_dboard_impl::_init_cpld()
{
    // CPLD
    RFNOC_LOG_TRACE("Initializing CPLD...");
    _cpld = std::make_shared<zbx_cpld_ctrl>(
        [this](
            const uint32_t addr, const uint32_t data, const zbx_cpld_ctrl::chan_t chan) {
            const auto time_spec = (chan == zbx_cpld_ctrl::NO_CHAN) ? time_spec_t::ASAP
                                   : (chan == zbx_cpld_ctrl::CHAN1) ? _time_accessor(1)
                                                                    : _time_accessor(0);
            _regs.poke32(_reg_base_address + addr, data, time_spec);
        },
        [this](const uint32_t addr) {
            // We don't do timed peeks, so no chan parameter here.
            return _regs.peek32(_reg_base_address + addr);
        },
        [this](const uhd::time_spec_t& sleep_time) { _regs.sleep(sleep_time); },
        get_unique_id() + "::CPLD");
    UHD_ASSERT_THROW(_cpld);
    // We don't have access to the scratch register, so we use the config
    // registers to test communication. This also does some basic sanity check
    // of the CPLDs logic.
    RFNOC_LOG_TRACE("Testing CPLD communication...");
    const uint32_t random_value = static_cast<uint32_t>(time(NULL));
    _cpld->set_scratch(random_value);
    UHD_ASSERT_THROW(_cpld->get_scratch() == random_value);
    // Now go to classic ATR mode
    RFNOC_LOG_TRACE("CPLD communication good. Switching to classic ATR mode.");
    for (size_t i = 0; i < ZBX_NUM_CHANS; ++i) {
        _cpld->set_atr_mode(
            i, zbx_cpld_ctrl::atr_mode_target::DSA, zbx_cpld_ctrl::atr_mode::CLASSIC_ATR);
        _cpld->set_atr_mode(i,
            zbx_cpld_ctrl::atr_mode_target::PATH_LED,
            zbx_cpld_ctrl::atr_mode::CLASSIC_ATR);
    }
}

void zbx_dboard_impl::_init_peripherals()
{
    RFNOC_LOG_TRACE("Initializing peripherals...");
    // Load DSA cal data (rx and tx)
    constexpr char dsa_step_filename_tx[] = "zbx_dsa_tx";
    constexpr char dsa_step_filename_rx[] = "zbx_dsa_rx";
    uhd::eeprom_map_t eeprom_map          = get_db_eeprom();
    const std::string db_serial(eeprom_map["serial"].begin(), eeprom_map["serial"].end());
    if (uhd::usrp::cal::database::has_cal_data(
            dsa_step_filename_tx, db_serial, uhd::usrp::cal::source::ANY)) {
        RFNOC_LOG_TRACE("load binary TX DSA steps from database...");
        const auto tx_dsa_data = uhd::usrp::cal::database::read_cal_data(
            dsa_step_filename_tx, db_serial, uhd::usrp::cal::source::ANY);
        RFNOC_LOG_TRACE("create TX DSA object...");
        _tx_dsa_cal = uhd::usrp::cal::zbx_tx_dsa_cal::make();
        RFNOC_LOG_TRACE("store deserialized TX DSA data into object...");
        _tx_dsa_cal->deserialize(tx_dsa_data);
    } else {
        RFNOC_LOG_ERROR("Could not find TX DSA cal data!");
        throw uhd::runtime_error("Could not find TX DSA cal data!");
    }
    if (uhd::usrp::cal::database::has_cal_data(
            dsa_step_filename_rx, db_serial, uhd::usrp::cal::source::ANY)) {
        // read binary blob without knowledge about content
        RFNOC_LOG_TRACE("load binary RX DSA steps from database...");
        const auto rx_dsa_data = uhd::usrp::cal::database::read_cal_data(
            dsa_step_filename_rx, db_serial, uhd::usrp::cal::source::ANY);

        RFNOC_LOG_TRACE("create RX DSA object...");
        _rx_dsa_cal = uhd::usrp::cal::zbx_rx_dsa_cal::make();

        RFNOC_LOG_TRACE("store deserialized RX DSA data into object...");
        _rx_dsa_cal->deserialize(rx_dsa_data);
    } else {
        RFNOC_LOG_ERROR("Could not find RX DSA cal data!");
        throw uhd::runtime_error("Could not find RX DSA cal data!");
    }
}

void zbx_dboard_impl::_init_prop_tree()
{
    auto subtree = get_tree()->subtree(fs_path("dboard"));

    // Construct RX frontend
    for (size_t chan_idx = 0; chan_idx < ZBX_NUM_CHANS; chan_idx++) {
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
        expert_factory::add_worker_node<zbx_scheduling_expert>(
            _expert_container, _expert_container->node_retriever(), fe_path);
    }

    // Construct TX frontend
    // Note: the TX frontend uses the RX property tree, this must
    // be constructed after the RX frontend
    for (size_t chan_idx = 0; chan_idx < ZBX_NUM_CHANS; chan_idx++) {
        const fs_path fe_path = fs_path("tx_frontends") / chan_idx;
        _init_frontend_subtree(subtree, TX_DIRECTION, chan_idx, fe_path);
    }

    // Now add the sync worker:
    expert_factory::add_worker_node<zbx_sync_expert>(_expert_container,
        _expert_container->node_retriever(),
        fs_path("tx_frontends"),
        fs_path("rx_frontends"),
        _rfdcc,
        _cpld);

    subtree->create<eeprom_map_t>("eeprom")
        .add_coerced_subscriber([](const eeprom_map_t&) {
            throw uhd::runtime_error("Attempting to update daughterboard eeprom!");
        })
        .set_publisher([this]() { return get_db_eeprom(); });
}

void zbx_dboard_impl::_init_frontend_subtree(uhd::property_tree::sptr subtree,
    const uhd::direction_t trx,
    const size_t chan_idx,
    const fs_path fe_path)
{
    static constexpr char ZBX_FE_NAME[] = "ZBX";

    RFNOC_LOG_TRACE("Adding non-RFNoC block properties for channel "
                    << chan_idx << " to prop tree path " << fe_path);
    // Standard attributes
    subtree->create<std::string>(fe_path / "name").set(ZBX_FE_NAME);
    subtree->create<std::string>(fe_path / "connection").set("IQ");

    _init_frequency_prop_tree(subtree, _expert_container, fe_path);
    _init_gain_prop_tree(subtree, _expert_container, trx, chan_idx, fe_path);
    _init_antenna_prop_tree(subtree, _expert_container, trx, chan_idx, fe_path);
    _init_lo_prop_tree(subtree, _expert_container, trx, chan_idx, fe_path);
    _init_programming_prop_tree(subtree, _expert_container, fe_path);
    _init_experts(subtree, _expert_container, trx, chan_idx, fe_path);
}


uhd::usrp::pwr_cal_mgr::sptr zbx_dboard_impl::_init_power_cal(
    uhd::property_tree::sptr subtree,
    const uhd::direction_t trx,
    const size_t chan_idx,
    const fs_path fe_path)
{
    const std::string DIR = (trx == TX_DIRECTION) ? "TX" : "RX";

    uhd::eeprom_map_t eeprom_map = get_db_eeprom();
    /* The cal serial is the DB serial plus the FE name */
    const std::string db_serial(eeprom_map["serial"].begin(), eeprom_map["serial"].end());
    const std::string cal_serial =
        db_serial + "#" + subtree->access<std::string>(fe_path / "name").get();
    /* Now create a gain group for this. */
    /* _?x_gain_groups won't work, because it doesn't group the */
    /* gains the way we want them to be grouped. */
    auto ggroup = uhd::gain_group::make();
    ggroup->register_fcns(HW_GAIN_STAGE,
        {[this, trx, chan_idx]() {
             return trx == TX_DIRECTION ? get_tx_gain_range(chan_idx)
                                        : get_rx_gain_range(chan_idx);
         },
            [this, trx, chan_idx]() {
                return trx == TX_DIRECTION ? get_tx_gain(ZBX_GAIN_STAGE_ALL, chan_idx)
                                           : get_rx_gain(ZBX_GAIN_STAGE_ALL, chan_idx);
            },
            [this, trx, chan_idx](const double gain) {
                trx == TX_DIRECTION ? this->set_tx_gain(gain, chan_idx)
                                    : this->set_rx_gain(gain, chan_idx);
            }},
        10 /* High priority */);
    /* If we had a digital (baseband) gain, we would register it here,*/
    /* so that the power manager would know to use it as a */
    /* backup gain stage. */
    /* Note that such a baseband gain might not be available */
    /* on the LV version. */
    return uhd::usrp::pwr_cal_mgr::make(
        cal_serial,
        "X400-CAL-" + DIR,
        [this, trx, chan_idx]() {
            return trx == TX_DIRECTION ? get_tx_frequency(chan_idx)
                                       : get_rx_frequency(chan_idx);
        },
        [trx_str = (trx == TX_DIRECTION ? "tx" : "rx"),
            fe_path,
            subtree,
            chan_str = std::to_string(chan_idx)]() -> std::string {
            const std::string antenna = pwr_cal_mgr::sanitize_antenna_name(
                subtree->access<std::string>(fe_path / "antenna/value").get());
            // The lookup key for X410 + ZBX shall start with x4xx_pwr_zbx.
            // Should we rev the ZBX in a way that would make generic cal data
            // unsuitable between revs, then we need to check the rev (or PID)
            // here and generate a different key prefix (e.g. x4xx_pwr_zbxD_ or
            // something like that).
            return std::string("x4xx_pwr_zbx_") + trx_str + "_" + chan_str + "_"
                   + antenna;
        },
        ggroup);
}

void zbx_dboard_impl::_init_experts(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
    const size_t chan_idx,
    const fs_path fe_path)
{
    RFNOC_LOG_TRACE(fe_path + ", Creating experts...");

    get_pwr_mgr(trx).insert(get_pwr_mgr(trx).begin() + chan_idx,
        _init_power_cal(subtree, trx, chan_idx, fe_path));

    // NOTE: THE ORDER OF EXPERT INITIALIZATION MATTERS
    //    After construction, all nodes (properties and experts) are marked dirty. Any
    //    subsequent calls to the container will trigger a resolve_all(), in which case
    //    the nodes are all resolved in REVERSE ORDER of construction, like a stack. With
    //    that in mind, we have to initialize the experts in line with that reverse order,
    //    because some experts rely on each other's construction/resolution to avoid
    //    errors (e.g., gain expert's dsa_cal is dependant on frequency be's coerced
    //    frequency, which is nan on dual_prop_node construction) After construction and
    //    subsequent resolution, the nodes will follow simple topological ruling as long
    //    as we only change one property at a time.

    // The current order should be:
    // Frequency FE Expert -> LO Expert(s) -> MPM Expert -> Frequency BE Expert -> Gain
    // Expert -> Programming Expert

    if (trx == TX_DIRECTION) {
        expert_factory::add_worker_node<zbx_tx_programming_expert>(expert,
            expert->node_retriever(),
            fe_path,
            fs_path("rx_frontends") / chan_idx,
            chan_idx,
            _tx_dsa_cal,
            _cpld);

        expert_factory::add_worker_node<zbx_tx_gain_expert>(expert,
            expert->node_retriever(),
            fe_path,
            chan_idx,
            get_pwr_mgr(trx).at(chan_idx),
            _tx_dsa_cal);
    } else {
        expert_factory::add_worker_node<zbx_rx_programming_expert>(
            expert, expert->node_retriever(), fe_path, chan_idx, _rx_dsa_cal, _cpld);

        expert_factory::add_worker_node<zbx_rx_gain_expert>(expert,
            expert->node_retriever(),
            fe_path,
            get_pwr_mgr(trx).at(chan_idx),
            _rx_dsa_cal);
    }

    expert_factory::add_worker_node<zbx_freq_be_expert>(
        expert, expert->node_retriever(), fe_path);

    expert_factory::add_worker_node<zbx_band_inversion_expert>(
        expert, expert->node_retriever(), fe_path, trx, chan_idx, _rpcc);


    // Initialize our LO Control Experts
    for (auto lo_select : ZBX_LOS) {
        if (lo_select == RFDC_NCO) {
            expert_factory::add_worker_node<zbx_rfdc_freq_expert>(expert,
                expert->node_retriever(),
                fe_path,
                trx,
                chan_idx,
                _rpc_prefix,
                _db_idx,
                _mb_rpcc);
        } else {
            const zbx_lo_t lo = zbx_lo_ctrl::lo_string_to_enum(trx, chan_idx, lo_select);
            std::shared_ptr<zbx_lo_ctrl> lo_ctrl = std::make_shared<zbx_lo_ctrl>(
                lo,
                [this, lo](const uint32_t addr, const uint16_t data) {
                    _cpld->lo_poke16(lo, addr, data);
                },
                [this, lo](const uint32_t addr) { return _cpld->lo_peek16(lo, addr); },
                [this](const uhd::time_spec_t& sleep_time) { _regs.sleep(sleep_time); },
                LMX2572_DEFAULT_FREQ,
                _prc_rate,
                false);
            expert_factory::add_worker_node<zbx_lo_expert>(
                expert, expert->node_retriever(), fe_path, lo_select, lo_ctrl);
            _lo_ctrl_map.insert({lo, lo_ctrl});
        }
    }

    const double lo_step_size = _prc_rate / ZBX_RELATIVE_LO_STEP_SIZE;
    RFNOC_LOG_DEBUG("LO step size: " << (lo_step_size / 1e6) << " MHz.")
    expert_factory::add_worker_node<zbx_freq_fe_expert>(expert,
        expert->node_retriever(),
        fe_path,
        trx,
        chan_idx,
        _rfdc_rate,
        lo_step_size);
    RFNOC_LOG_TRACE(fe_path + ", Experts created");
}

void zbx_dboard_impl::_init_frequency_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const fs_path fe_path)
{
    expert_factory::add_dual_prop_node<double>(
        expert, subtree, fe_path / "freq", ZBX_DEFAULT_FREQ, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_dual_prop_node<double>(
        expert, subtree, fe_path / "if_freq", 0.0, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_data_node<bool>(expert, fe_path / "is_highband", false);
    expert_factory::add_data_node<int>(
        expert, fe_path / "mixer1_m", 0, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_data_node<int>(
        expert, fe_path / "mixer1_n", 0, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_data_node<int>(
        expert, fe_path / "mixer2_m", 0, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_data_node<int>(
        expert, fe_path / "mixer2_n", 0, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_data_node<bool>(
        expert, fe_path / "band_inverted", false, AUTO_RESOLVE_ON_WRITE);

    subtree->create<double>(fe_path / "bandwidth" / "value")
        .set(ZBX_DEFAULT_BANDWIDTH)
        .set_coercer([](const double) { return ZBX_DEFAULT_BANDWIDTH; });
    subtree->create<meta_range_t>(fe_path / "bandwidth" / "range")
        .set({ZBX_DEFAULT_BANDWIDTH, ZBX_DEFAULT_BANDWIDTH})
        .set_coercer([](const meta_range_t&) {
            return meta_range_t(ZBX_DEFAULT_BANDWIDTH, ZBX_DEFAULT_BANDWIDTH);
        });
    subtree->create<meta_range_t>(fe_path / "freq" / "range")
        .set(ZBX_FREQ_RANGE)
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update freq range!");
        });
}

void zbx_dboard_impl::_init_gain_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
    const size_t chan_idx,
    const fs_path fe_path)
{
    // First, overall gain nodes
    const auto gain_base_path = fe_path / "gains";
    expert_factory::add_dual_prop_node<double>(expert,
        subtree,
        gain_base_path / ZBX_GAIN_STAGE_ALL / "value",
        trx == TX_DIRECTION ? TX_MIN_GAIN : RX_MIN_GAIN,
        AUTO_RESOLVE_ON_WRITE);
    subtree->create<meta_range_t>(fe_path / "gains" / "all" / "range")
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update gain range!");
        })
        .set_publisher([this, trx, chan_idx]() {
            return (trx == TX_DIRECTION) ? this->get_tx_gain_range(chan_idx)
                                         : this->get_rx_gain_range(chan_idx);
        });
    // Then, individual DSA/amp gain nodes
    if (trx == TX_DIRECTION) {
        // DSAs
        for (const auto dsa : {ZBX_GAIN_STAGE_DSA1, ZBX_GAIN_STAGE_DSA2}) {
            const auto gain_path = gain_base_path / dsa;
            expert_factory::add_dual_prop_node<double>(
                expert, subtree, gain_path / "value", 0, AUTO_RESOLVE_ON_WRITE);
            subtree->create<meta_range_t>(gain_path / "range")
                .set(uhd::meta_range_t(0, ZBX_TX_DSA_MAX_ATT, 1.0));
            expert_factory::add_worker_node<zbx_gain_coercer_expert>(_expert_container,
                _expert_container->node_retriever(),
                gain_path / "value",
                uhd::meta_range_t(0, ZBX_TX_DSA_MAX_ATT, 1.0));
        }
        // Amp
        const auto amp_path = gain_base_path / ZBX_GAIN_STAGE_AMP;
        expert_factory::add_dual_prop_node<double>(expert,
            subtree,
            amp_path / "value",
            ZBX_TX_LOWBAND_GAIN,
            AUTO_RESOLVE_ON_WRITE);
        uhd::meta_range_t amp_gain_range;
        for (const auto tx_gain_pair : ZBX_TX_GAIN_AMP_MAP) {
            amp_gain_range.push_back(uhd::range_t(tx_gain_pair.first));
        }
        subtree->create<meta_range_t>(amp_path / "range").set(amp_gain_range);
        expert_factory::add_worker_node<zbx_gain_coercer_expert>(_expert_container,
            _expert_container->node_retriever(),
            amp_path / "value",
            amp_gain_range);
    } else {
        // RX only has DSAs
        for (const auto dsa : {ZBX_GAIN_STAGE_DSA1,
                 ZBX_GAIN_STAGE_DSA2,
                 ZBX_GAIN_STAGE_DSA3A,
                 ZBX_GAIN_STAGE_DSA3B}) {
            const auto gain_path = gain_base_path / dsa;
            expert_factory::add_dual_prop_node<double>(
                expert, subtree, gain_path / "value", 0, AUTO_RESOLVE_ON_WRITE);
            subtree->create<meta_range_t>(gain_path / "range")
                .set(uhd::meta_range_t(0, ZBX_RX_DSA_MAX_ATT, 1.0));
            expert_factory::add_worker_node<zbx_gain_coercer_expert>(_expert_container,
                _expert_container->node_retriever(),
                gain_path / "value",
                uhd::meta_range_t(0, ZBX_RX_DSA_MAX_ATT, 1.0));
        }
    }

    const uhd::fs_path gain_profile_path = gain_base_path / "all" / "profile";
    expert_factory::add_prop_node<std::string>(expert,
        subtree,
        gain_profile_path,
        ZBX_GAIN_PROFILE_DEFAULT,
        AUTO_RESOLVE_ON_WRITE);
    auto& gain_profile           = (trx == TX_DIRECTION) ? _tx_gain_profile_api
                                                         : _rx_gain_profile_api;
    auto& other_dir_gp           = (trx == TX_DIRECTION) ? _rx_gain_profile_api
                                                         : _tx_gain_profile_api;
    auto gain_profile_subscriber = [this, other_dir_gp, trx](
                                       const std::string& profile, const size_t chan) {
        // Upon changing the gain profile, we need to import the new value into
        // the property tree.
        const auto path = fs_path("dboard")
                          / (trx == TX_DIRECTION ? "tx_frontends" : "rx_frontends") / chan
                          / "gains" / "all" / "profile";
        get_tree()->access<std::string>(path).set(profile);
        // The CPLD does not have the option to have different ATR modes for RX
        // and TX (it does have different modes for channel 0 and 1 though).
        // This means we have to match up the gain profiles between RX and TX.
        // The ZBX_GAIN_PROFILE_CPLD_NOATR profile uses the SW_DEFINED mode,
        // and all the others use CLASSIC_ATR. So either both match
        // ZBX_GAIN_PROFILE_CPLD_NOATR, or none do.
        // This will not cause a loop, because the other_dir_gp will already
        // match this one by the time we call it.
        if ((profile == ZBX_GAIN_PROFILE_CPLD_NOATR
                && other_dir_gp->get_gain_profile(chan) != ZBX_GAIN_PROFILE_CPLD_NOATR)
            || (profile != ZBX_GAIN_PROFILE_CPLD_NOATR
                && other_dir_gp->get_gain_profile(chan) == ZBX_GAIN_PROFILE_CPLD_NOATR)) {
            RFNOC_LOG_DEBUG("Channel " << chan << ": Setting gain profile to `" << profile
                                       << "' for both TX and RX.");
            other_dir_gp->set_gain_profile(profile, chan);
        }
    };

    gain_profile->add_subscriber(std::move(gain_profile_subscriber));
}

void zbx_dboard_impl::_init_antenna_prop_tree(uhd::property_tree::sptr subtree,
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

void zbx_dboard_impl::_init_programming_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const fs_path fe_path)
{
    expert_factory::add_prop_node<int>(
        expert, subtree, fe_path / "rf" / "filter", 1, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_prop_node<int>(
        expert, subtree, fe_path / "if1" / "filter", 1, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_prop_node<int>(
        expert, subtree, fe_path / "if2" / "filter", 1, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_prop_node<zbx_cpld_ctrl::atr_mode>(expert,
        subtree,
        fe_path / "atr_mode",
        zbx_cpld_ctrl::atr_mode::CLASSIC_ATR,
        AUTO_RESOLVE_ON_WRITE);
}

void zbx_dboard_impl::_init_lo_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
    const size_t chan_idx,
    const fs_path fe_path)
{
    // Tuning table
    expert_factory::add_prop_node<std::vector<tune_map_item_t>>(expert,
        subtree,
        fe_path / "tune_table",
        trx == RX_DIRECTION ? rx_tune_map : tx_tune_map,
        AUTO_RESOLVE_ON_WRITE);

    // Analog LO Specific
    for (const std::string lo : {ZBX_LO1, ZBX_LO2}) {
        expert_factory::add_prop_node<zbx_lo_source_t>(expert,
            subtree,
            fe_path / "ch" / lo / "source",
            ZBX_DEFAULT_LO_SOURCE,
            AUTO_RESOLVE_ON_WRITE);
        expert_factory::add_prop_node<bool>(
            expert, subtree, fe_path / lo / "enabled", false, AUTO_RESOLVE_ON_WRITE);
        expert_factory::add_prop_node<bool>(
            expert, subtree, fe_path / lo / "test_mode", false, AUTO_RESOLVE_ON_WRITE);
        expert_factory::add_dual_prop_node<double>(expert,
            subtree,
            fe_path / "los" / lo / "freq" / "value",
            LMX2572_DEFAULT_FREQ,
            AUTO_RESOLVE_ON_WRITE);

        subtree->create<meta_range_t>(fe_path / "los" / lo / "freq/range")
            .set_publisher(
                [this, lo, chan_idx]() { return this->_get_lo_freq_range(lo, chan_idx); })
            .add_coerced_subscriber([](const meta_range_t&) {
                throw uhd::runtime_error("Attempting to update freq range!");
            });
        subtree->create<std::vector<std::string>>(fe_path / "los" / lo / "source/options")
            .set_publisher([this, lo, trx, chan_idx]() {
                return trx == TX_DIRECTION ? this->get_tx_lo_sources(lo, chan_idx)
                                           : this->get_rx_lo_sources(lo, chan_idx);
            })
            .add_coerced_subscriber([](const std::vector<std::string>&) {
                throw uhd::runtime_error("Attempting to update LO source options!");
            });

        subtree
            ->create<sensor_value_t>(
                fe_path / "sensors" / boost::algorithm::to_lower_copy(lo) + "_locked")
            .add_coerced_subscriber([](const sensor_value_t&) {
                throw uhd::runtime_error("Attempting to write to sensor!");
            })
            .set_publisher([this, lo, trx, chan_idx]() {
                return sensor_value_t(lo,
                    this->_lo_ctrl_map
                        .at(zbx_lo_ctrl::lo_string_to_enum(trx, chan_idx, lo))
                        ->get_lock_status(),
                    "locked",
                    "unlocked");
            });
    }

    // The NCO gets a sub-node called 'reset'. It is read/write: Write will
    // perform a reset, and read will return the reset status. The latter is
    // also returned in the 'locked' sensor for the NCO, but the 'nco_locked'
    // sensor node is read-only, and returns a sensor_value_t (not a bool).
    // This node is primarily used for debugging, but can also serve as a manual
    // reset line for the NCOs.
    const auto nco = (trx == TX_DIRECTION)
                         ? (chan_idx == 0 ? rfdc_control::rfdc_type::TX0
                                          : rfdc_control::rfdc_type::TX1)
                         : (chan_idx == 0 ? rfdc_control::rfdc_type::RX0
                                          : rfdc_control::rfdc_type::RX1);
    subtree->create<bool>(fe_path / "los" / RFDC_NCO / "reset")
        .set_publisher([this]() { return this->_rfdcc->get_nco_reset_done(); })
        .add_coerced_subscriber([this, nco, chan_idx](const bool&) {
            RFNOC_LOG_TRACE("Resetting NCO " << size_t(nco) << ", chan " << chan_idx);
            this->_rfdcc->reset_ncos({nco}, this->_time_accessor(chan_idx));
        });

    expert_factory::add_dual_prop_node<double>(expert,
        subtree,
        fe_path / "los" / RFDC_NCO / "freq" / "value",
        // Initialize with current value
        _mb_rpcc->rfdc_get_nco_freq(trx == TX_DIRECTION ? "tx" : "rx", _db_idx, chan_idx),
        AUTO_RESOLVE_ON_WRITE);

    expert_factory::add_prop_node<zbx_lo_source_t>(expert,
        subtree,
        fe_path / "ch" / RFDC_NCO / "source",
        ZBX_DEFAULT_LO_SOURCE,
        AUTO_RESOLVE_ON_WRITE);

    // LO lock sensor
    // We can't make this its own property value because it has to have access to two
    // containers (two instances of zbx lo expert)
    subtree->create<sensor_value_t>(fe_path / "sensors" / "lo_locked")
        .set(sensor_value_t("all_los", false, "locked", "unlocked"))
        .add_coerced_subscriber([](const sensor_value_t&) {
            throw uhd::runtime_error("Attempting to write to sensor!");
        })
        .set_publisher([this, trx, chan_idx]() {
            return sensor_value_t("all_los",
                this->_get_all_los_locked(trx, chan_idx),
                "locked",
                "unlocked");
        });
    subtree->create<sensor_value_t>(fe_path / "sensors" / "nco_locked")
        .add_coerced_subscriber([](const sensor_value_t&) {
            throw uhd::runtime_error("Attempting to write to sensor!");
        })
        .set_publisher([this]() {
            return sensor_value_t(
                RFDC_NCO, this->_rfdcc->get_nco_reset_done(), "locked", "unlocked");
        });
}
}}} // namespace uhd::usrp::zbx
