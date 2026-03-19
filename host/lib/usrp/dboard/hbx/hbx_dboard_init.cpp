//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/experts/expert_container.hpp>
#include <uhd/experts/expert_factory.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/reg_iface_adapter.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_constants.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_dboard.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_expert.hpp>
#include <boost/algorithm/string.hpp>
#include <numeric>
#include <sstream>
#include <vector>

using namespace uhd;
using namespace uhd::experts;
using namespace uhd::rfnoc;

namespace uhd { namespace usrp { namespace hbx {

void hbx_dboard_impl::_init_hbx_cpld()
{
    RFNOC_LOG_TRACE("Initializing HBX CTRL");
    _cpld = std::make_shared<hbx_cpld_ctrl>(
        [this](
            const uint32_t addr, const uint32_t data, const hbx_cpld_ctrl::chan_t chan) {
            const auto time_spec = (chan == hbx_cpld_ctrl::NO_CHAN) ? time_spec_t::ASAP
                                                                    : _time_accessor(0);
            _reg_iface.poke32(_reg_base_address + addr, data, time_spec);
        },
        [this](const uint32_t addr) {
            // We don't do timed peeks, so no chan parameter here.
            return _reg_iface.peek32(_reg_base_address + addr);
        },
        [this](const uhd::time_spec_t& sleep_time) { _reg_iface.sleep(sleep_time); },
        get_unique_id() + "::HBX_CPLD");
    UHD_ASSERT_THROW(_cpld);
}

void hbx_dboard_impl::_init_admv()
{
    RFNOC_LOG_TRACE("Initializing ADMV1320");
    _admv1320 = std::make_shared<hbx_admv1320_ctrl>(
        _cpld->get_addr("TX_ADMV_SPI_INFO"),
        [this](
            const uint32_t addr, const uint32_t data, const hbx_cpld_ctrl::chan_t chan) {
            const auto time_spec = (chan == hbx_cpld_ctrl::NO_CHAN) ? time_spec_t::ASAP
                                                                    : _time_accessor(0);
            _reg_iface.poke32(_reg_base_address + addr, data, time_spec);
        },
        [this](
            const uint32_t addr) { return _reg_iface.peek32(_reg_base_address + addr); },
        [this](const hbx_cpld_ctrl::init_t step) {
            _cpld->mixer_init_callback(step, TX_DIRECTION);
        },
        _unique_id + "::ADMV1320");
    UHD_ASSERT_THROW(_admv1320);

    RFNOC_LOG_TRACE("Initializing ADMV1420");
    _admv1420 = std::make_shared<hbx_admv1420_ctrl>(
        _cpld->get_addr("RX_ADMV_SPI_INFO"),
        [this](
            const uint32_t addr, const uint32_t data, const hbx_cpld_ctrl::chan_t chan) {
            const auto time_spec = (chan == hbx_cpld_ctrl::NO_CHAN) ? time_spec_t::ASAP
                                                                    : _time_accessor(0);
            _reg_iface.poke32(_reg_base_address + addr, data, time_spec);
        },
        [this](const uint32_t addr) {
            // We don't do timed peeks, so no chan parameter here.
            return _reg_iface.peek32(_reg_base_address + addr);
        },
        [this](const hbx_cpld_ctrl::init_t step) {
            _cpld->mixer_init_callback(step, RX_DIRECTION);
        },
        _unique_id + "::ADMV1420");
}

std::shared_ptr<hbx_lo_ctrl> hbx_dboard_impl::_init_lo_ctrl(const uhd::direction_t trx)
{
    std::shared_ptr<hbx_lo_ctrl> lo_ctrl = std::make_shared<hbx_lo_ctrl>(
        trx,
        trx == RX_DIRECTION ? _cpld->get_addr("RX_LO_SPI_INFO")
                            : _cpld->get_addr("TX_LO_SPI_INFO"),
        [this](
            const uint32_t addr, const uint32_t data, const hbx_cpld_ctrl::chan_t chan) {
            const auto time_spec = (chan == hbx_cpld_ctrl::NO_CHAN) ? time_spec_t::ASAP
                                                                    : _time_accessor(0);
            _reg_iface.poke32(_reg_base_address + addr, data, time_spec);
        },
        [this](const uint32_t addr) {
            // We don't do timed peeks, so no chan parameter here.
            return _reg_iface.peek32(_reg_base_address + addr);
        },
        _prc_rate,
        [this] { return _time_accessor(0); });
    return lo_ctrl;
}

std::shared_ptr<hbx_lo_pd> hbx_dboard_impl::_init_lo_pd(const direction_t trx)
{
    RFNOC_LOG_TRACE("Initializing LO power detectors");

    return std::make_shared<hbx_lo_pd>(
        trx == RX_DIRECTION ? _cpld->get_addr("RX_LO_PD_ADC_SPI_INFO")
                            : _cpld->get_addr("TX_LO_PD_ADC_SPI_INFO"),
        [this](
            const uint32_t addr, const uint32_t data, const hbx_cpld_ctrl::chan_t chan) {
            const auto time_spec = (chan == hbx_cpld_ctrl::NO_CHAN) ? time_spec_t::ASAP
                                                                    : _time_accessor(0);
            _reg_iface.poke32(_reg_base_address + addr, data, time_spec);
        },
        [this](
            const uint32_t addr) { return _reg_iface.peek32(_reg_base_address + addr); });
}

void hbx_dboard_impl::_init_demod()
{
    _demod_ctrl = std::make_shared<hbx_demod_ctrl>(
        _cpld->get_addr("IQ_DEMOD_SPI_INFO"),
        [this](
            const uint32_t addr, const uint32_t data, const hbx_cpld_ctrl::chan_t chan) {
            const auto time_spec = (chan == hbx_cpld_ctrl::NO_CHAN) ? time_spec_t::ASAP
                                                                    : _time_accessor(0);
            _reg_iface.poke32(_reg_base_address + addr, data, time_spec);
        },
        [this](const uint32_t addr) {
            // We don't do timed peeks, so no chan parameter here.
            return _reg_iface.peek32(_reg_base_address + addr);
        });
}

void hbx_dboard_impl::_init_prop_tree()
{
    auto subtree = get_tree()->subtree(fs_path("dboard"));

    // Construct RX frontend
    const fs_path rx_fe_path = fs_path("rx_frontends") / 0;

    // Command time needs to be shadowed into the property tree so we can use
    // it in the expert graph. TX and RX share the command time, so we could
    // put it onto its own sub-tree, or copy the property between TX and RX.
    // With respect to TwinRX and trying to keep the tree lean and browsable,
    // we compromise and put the command time onto the RX frontend path, even
    // though it's also valid for TX.
    // This data node will be used for scheduling the other experts:
    expert_factory::add_data_node<time_spec_t>(
        _expert_container, rx_fe_path / "time/fe", time_spec_t(0.0));
    // This prop node will be used to import the command time into the
    // graph:
    expert_factory::add_prop_node<time_spec_t>(
        _expert_container, subtree, rx_fe_path / "time/cmd", time_spec_t(0.0));

    _init_frontend_subtree(subtree, RX_DIRECTION, rx_fe_path);

    // The time nodes get connected with one scheduling expert per channel:
    expert_factory::add_worker_node<hbx_scheduling_expert>(
        _expert_container, _expert_container->node_retriever(), rx_fe_path);

    // Construct TX frontend
    // Note: the TX frontend uses the RX property tree, this must
    // be constructed after the RX frontend
    const fs_path tx_fe_path = fs_path("tx_frontends") / 0;
    _init_frontend_subtree(subtree, TX_DIRECTION, tx_fe_path);

    // Now add the sync worker:
    UHD_ASSERT_THROW(get_num_rx_channels() == get_num_tx_channels());
    expert_factory::add_worker_node<hbx_sync_expert>(_expert_container,
        _expert_container->node_retriever(),
        fs_path("tx_frontends"),
        fs_path("rx_frontends"),
        _rfdcc);

    subtree->create<eeprom_map_t>("eeprom")
        .add_coerced_subscriber([](const eeprom_map_t&) {
            throw uhd::runtime_error("Attempting to update daughterboard eeprom!");
        })
        .set_publisher([this]() { return get_db_eeprom(); });
}

void hbx_dboard_impl::_init_frontend_subtree(
    uhd::property_tree::sptr subtree, const uhd::direction_t trx, const fs_path fe_path)
{
    static constexpr char HBX_FE_NAME[] = "HBX";

    RFNOC_LOG_TRACE("Adding non-RFNoC block properties to prop tree path " << fe_path);
    // Standard attributes
    subtree->create<std::string>(fe_path / "name").set(HBX_FE_NAME);
    subtree->create<std::string>(fe_path / "connection").set("IQ");

    _init_frequency_prop_tree(subtree, _expert_container, trx, fe_path);
    _init_gain_prop_tree(subtree, _expert_container, trx, fe_path);
    _init_antenna_prop_tree(subtree, _expert_container, trx, fe_path);
    _init_lo_prop_tree(subtree, _expert_container, trx, fe_path);
    _init_iq_dc_prop_tree(subtree, _expert_container, trx, fe_path);
    _init_experts(subtree, _expert_container, trx, fe_path);
}

uhd::usrp::pwr_cal_mgr::sptr hbx_dboard_impl::_init_power_cal(
    uhd::property_tree::sptr subtree, const uhd::direction_t trx, const fs_path fe_path)
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
        {[this, trx]() {
             return trx == TX_DIRECTION ? get_tx_gain_range(0) : get_rx_gain_range(0);
         },
            [this, trx]() {
                return trx == TX_DIRECTION ? get_tx_gain(HBX_GAIN_STAGE_ALL, 0)
                                           : get_rx_gain(HBX_GAIN_STAGE_ALL, 0);
            },
            [this, trx](const double gain) {
                trx == TX_DIRECTION ? this->set_tx_gain(gain, 0)
                                    : this->set_rx_gain(gain, 0);
            }},
        10 /* High priority */);

    return uhd::usrp::pwr_cal_mgr::make(
        cal_serial,
        "X400-CAL-" + DIR,
        [this, trx]() {
            return trx == TX_DIRECTION ? get_tx_frequency(0) : get_rx_frequency(0);
        },
        [trx_str = (trx == TX_DIRECTION ? "tx" : "rx"),
            fe_path,
            subtree]() -> std::string {
            const std::string antenna = pwr_cal_mgr::sanitize_antenna_name(
                subtree->access<std::string>(fe_path / "antenna/value").get());
            // Adding "_0_" as we only have channel 0 but keep the naming format aligned
            // with others devices.
            return std::string("x4xx_pwr_hbx_") + trx_str + "_0_" + antenna;
        },
        ggroup);
}

void hbx_dboard_impl::_init_experts(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
    const fs_path fe_path)
{
    RFNOC_LOG_TRACE(fe_path + ", Creating experts...");

    get_pwr_mgr(trx).insert(
        get_pwr_mgr(trx).begin(), _init_power_cal(subtree, trx, fe_path));

    for (auto lo_select : HBX_LOS) {
        if (lo_select == RFDC_NCO) {
            RFNOC_LOG_TRACE("Creating expert for RFDC NCO");
            expert_factory::add_worker_node<hbx_rfdc_freq_expert>(
                expert, expert->node_retriever(), fe_path, trx, _db_idx, _mb_rpcc);
        } else {
            RFNOC_LOG_TRACE("Creating expert for LO " << lo_select);
            // We don't distinguish between RX and TX here, because the lo_ctrl is
            // different already and the fe_path takes care of sorting it into the correct
            // expert.
            expert_factory::add_worker_node<hbx_lo_expert>(expert,
                expert->node_retriever(),
                fe_path,
                trx,
                lo_select,
                trx == TX_DIRECTION ? _tx_lo_ctrl : _rx_lo_ctrl,
                _cpld);
        }
    }

    if (trx == TX_DIRECTION) {
        expert_factory::add_worker_node<hbx_tx_programming_expert>(
            expert, expert->node_retriever(), fe_path, _cpld);
        expert_factory::add_worker_node<hbx_tx_band_expert>(
            expert, expert->node_retriever(), fe_path, _rpcc, _cpld, _admv1320);
        expert_factory::add_worker_node<hbx_tx_gain_programming_expert>(
            expert, expert->node_retriever(), fe_path, _cpld, _admv1320, _tx_lo_ctrl);
        expert_factory::add_worker_node<hbx_tx_gain_expert>(
            expert, expert->node_retriever(), fe_path, get_pwr_mgr(trx).at(0));
    } else {
        expert_factory::add_worker_node<hbx_rx_programming_expert>(
            expert, expert->node_retriever(), fe_path, _cpld);

        expert_factory::add_worker_node<hbx_rx_band_expert>(expert,
            expert->node_retriever(),
            fe_path,
            _rpcc,
            _cpld,
            _demod_ctrl,
            _admv1420);

        expert_factory::add_worker_node<hbx_rx_gain_programming_expert>(
            expert, expert->node_retriever(), fe_path, _cpld, _admv1420, _rx_lo_ctrl);
        expert_factory::add_worker_node<hbx_rx_gain_expert>(
            expert, expert->node_retriever(), fe_path, get_pwr_mgr(trx).at(0));
    }
    if (!_ignore_cal_file) {
        // By not initializing this when ignoring the cal file we don't pick new
        // coefficients and thus won't write anything.
        expert_factory::add_worker_node<hbx_iq_dc_coeffs_expert>(
            expert, expert->node_retriever(), fe_path, trx, subtree, _db_serial, _mcr);
    }
    // This expert needs to be added even when ignoring the cal file, so that the default
    // coefficients get written during init.
    expert_factory::add_worker_node<hbx_iq_dc_correction_expert>(
        expert,
        expert->node_retriever(),
        fe_path,
        trx,
        trx == direction_t::RX_DIRECTION ? _rfdc_dc_offset
                                         : std::complex<double>{0.0, 0.0},
        [this, trx](const uint32_t addr, const uint32_t data) {
            _reg_iface.poke32(
                _reg_base_address + RF_CORE_ADDR_OFFSET + addr, data, _time_accessor(0));
        },
        [this, trx](const uint32_t addr) {
            // We don't do timed peeks, so no chan parameter here.
            return _reg_iface.peek32(_reg_base_address + RF_CORE_ADDR_OFFSET + addr);
        });

    RFNOC_LOG_TRACE(fe_path + ", Experts created");
}

void hbx_dboard_impl::_init_frequency_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
    const fs_path fe_path)
{
    expert_factory::add_dual_prop_node<double>(
        expert, subtree, fe_path / "freq", HBX_DEFAULT_FREQ, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_dual_prop_node<double>(
        expert, subtree, fe_path / "if_freq", 0.0, AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_dual_prop_node<double>(expert,
        subtree,
        fe_path / "los" / RFDC_NCO / "freq" / "value",
        _mb_rpcc->rfdc_get_nco_freq(trx == TX_DIRECTION ? "tx" : "rx",
            _db_idx,
            0,
            static_cast<size_t>(uhd::usrp::x400::ch_mode::REAL)),
        AUTO_RESOLVE_ON_WRITE);

    subtree->create<double>(fe_path / "bandwidth" / "value")
        .set(HBX_DEFAULT_BANDWIDTH)
        .set_coercer([](const double) { return HBX_DEFAULT_BANDWIDTH; });
    subtree->create<meta_range_t>(fe_path / "bandwidth" / "range")
        .set(meta_range_t(HBX_DEFAULT_BANDWIDTH, HBX_DEFAULT_BANDWIDTH))
        .set_coercer([](const meta_range_t&) {
            return meta_range_t(HBX_DEFAULT_BANDWIDTH, HBX_DEFAULT_BANDWIDTH);
        });
    subtree->create<meta_range_t>(fe_path / "freq" / "range")
        .set(HBX_FREQ_RANGE)
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update freq range!");
        });
}

void hbx_dboard_impl::_init_gain_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
    const fs_path fe_path)
{
    // First, overall gain nodes
    const auto gain_base_path = fe_path / "gains";
    expert_factory::add_dual_prop_node<double>(expert,
        subtree,
        gain_base_path / HBX_GAIN_STAGE_ALL / "value",
        trx == TX_DIRECTION ? TX_MIN_GAIN : RX_MIN_GAIN,
        AUTO_RESOLVE_ON_WRITE);
    subtree->create<meta_range_t>(fe_path / "gains" / "all" / "range")
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update gain range!");
        })
        .set_publisher([this, trx]() {
            return (trx == TX_DIRECTION) ? this->get_tx_gain_range(0)
                                         : this->get_rx_gain_range(0);
        });
    // Individual DSAs/gains
    if (trx == TX_DIRECTION) {
        // ADMV DSAs
        for (const auto dsa : {HBX_GAIN_STAGE_ADMV_DSA1, HBX_GAIN_STAGE_ADMV_DSA2}) {
            const auto gain_path = gain_base_path / dsa;
            expert_factory::add_dual_prop_node<double>(
                expert, subtree, gain_path / "value", TX_MIN_GAIN, AUTO_RESOLVE_ON_WRITE);
            subtree->create<meta_range_t>(gain_path / "range")
                .set(uhd::meta_range_t(0, ADMV_DSA_MAX_ATTENUATION, 1.0));
        }
        // ADMV compound DSA (ADMV_DSA1 + ADMV_DSA2)
        auto gain_path = gain_base_path / HBX_GAIN_STAGE_ADMV_DSA_ALL;
        expert_factory::add_dual_prop_node<double>(
            expert, subtree, gain_path / "value", TX_MIN_GAIN, AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, ADMV_DSA_MAX_ATTENUATION * 2, 1.0));

        // LO DSA
        gain_path = gain_base_path / HBX_GAIN_STAGE_LO;
        expert_factory::add_dual_prop_node<double>(expert,
            subtree,
            gain_path / "value",
            LO_DSA_DEFAULT_GAIN,
            AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, LO_LF_DSA_MAX_ATTENUATION, 1.0));

        // RF DSA
        gain_path = gain_base_path / HBX_GAIN_STAGE_RF;
        expert_factory::add_dual_prop_node<double>(
            expert, subtree, gain_path / "value", TX_MIN_GAIN, AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, RF_DSA_MAX_ATTENUATION, 1.0));

        // LO power port A
        gain_path = gain_base_path / HBX_GAIN_STAGE_LO_PWR_INT;
        expert_factory::add_dual_prop_node<double>(
            expert, subtree, gain_path / "value", LO_MAX_PWR, AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, LO_MAX_PWR, 1.0));

        // LO power port B
        gain_path = gain_base_path / HBX_GAIN_STAGE_LO_PWR_EXT;
        expert_factory::add_dual_prop_node<double>(
            expert, subtree, gain_path / "value", 0, AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, LO_MAX_PWR, 1.0));
    } else {
        // RX
        // ADMV DSAs (not DSA2 as this is special)
        for (const auto dsa : {HBX_GAIN_STAGE_ADMV_DSA3,
                 HBX_GAIN_STAGE_ADMV_DSA4,
                 HBX_GAIN_STAGE_ADMV_DSA5}) {
            const auto gain_path = gain_base_path / dsa;
            expert_factory::add_dual_prop_node<double>(
                expert, subtree, gain_path / "value", RX_MIN_GAIN, AUTO_RESOLVE_ON_WRITE);
            subtree->create<meta_range_t>(gain_path / "range")
                .set(uhd::meta_range_t(0, ADMV_DSA_MAX_ATTENUATION, 1.0));
        }
        // ADMV DSA2
        auto gain_path = gain_base_path / HBX_GAIN_STAGE_ADMV_DSA2;
        expert_factory::add_dual_prop_node<double>(
            expert, subtree, gain_path / "value", RX_MIN_GAIN, AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, ADMV1420_DSA2_MAX_ATTENUATION, 1.0));


        // ADMV Compound DSA (max: DSA3 + DSA4 + DSA5)
        gain_path = gain_base_path / HBX_GAIN_STAGE_ADMV_DSA_ALL;
        expert_factory::add_dual_prop_node<double>(
            expert, subtree, gain_path / "value", RX_MIN_GAIN, AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, ADMV_DSA_MAX_ATTENUATION * 3, 1.0));

        // LO DSA and LF DSAs (max attenuation: 31 dB)
        for (const auto dsa :
            {HBX_GAIN_STAGE_LO, HBX_GAIN_STAGE_LF_DSA1, HBX_GAIN_STAGE_LF_DSA2}) {
            const auto gain_path = gain_base_path / dsa;
            expert_factory::add_dual_prop_node<double>(expert,
                subtree,
                gain_path / "value",
                dsa == HBX_GAIN_STAGE_LO ? LO_DSA_DEFAULT_GAIN : RX_MIN_GAIN,
                AUTO_RESOLVE_ON_WRITE);
            subtree->create<meta_range_t>(gain_path / "range")
                .set(uhd::meta_range_t(0, LO_LF_DSA_MAX_ATTENUATION, 1.0));
        }

        // RF DSA
        gain_path = gain_base_path / HBX_GAIN_STAGE_RF;
        expert_factory::add_dual_prop_node<double>(
            expert, subtree, gain_path / "value", RX_MIN_GAIN, AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, RF_DSA_MAX_ATTENUATION, 1.0));

        // LO power port A
        gain_path = gain_base_path / HBX_GAIN_STAGE_LO_PWR_INT;
        expert_factory::add_dual_prop_node<double>(
            expert, subtree, gain_path / "value", LO_MAX_PWR, AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, LO_MAX_PWR, 1.0));

        // LO power port B
        gain_path = gain_base_path / HBX_GAIN_STAGE_LO_PWR_EXT;
        expert_factory::add_dual_prop_node<double>(
            expert, subtree, gain_path / "value", 0, AUTO_RESOLVE_ON_WRITE);
        subtree->create<meta_range_t>(gain_path / "range")
            .set(uhd::meta_range_t(0, LO_MAX_PWR, 1.0));
    }

    const uhd::fs_path gain_profile_path = gain_base_path / "all" / "profile";
    expert_factory::add_prop_node<std::string>(expert,
        subtree,
        gain_profile_path,
        HBX_GAIN_PROFILE_DEFAULT,
        AUTO_RESOLVE_ON_WRITE);
    auto& gain_profile           = (trx == TX_DIRECTION) ? _tx_gain_profile_api
                                                         : _rx_gain_profile_api;
    auto gain_profile_subscriber = [this, trx](const std::string& profile, const size_t) {
        // Upon changing the gain profile, we need to import the new value into the
        // property tree.
        const auto path = fs_path("dboard")
                          / (trx == TX_DIRECTION ? "tx_frontends" : "rx_frontends") / "0"
                          / "gains" / "all" / "profile";
        get_tree()->access<std::string>(path).set(profile);
    };

    gain_profile->add_subscriber(std::move(gain_profile_subscriber));
}

void hbx_dboard_impl::_init_antenna_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
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
        .set(trx == TX_DIRECTION ? get_tx_antennas(0) : get_rx_antennas(0))
        .add_coerced_subscriber([](const std::vector<std::string>&) {
            throw uhd::runtime_error("Attempting to update antenna options!");
        });
}

void hbx_dboard_impl::_init_lo_prop_tree(uhd::property_tree::sptr subtree,
    expert_container::sptr expert,
    const uhd::direction_t trx,
    const fs_path fe_path)
{
    for (auto lo : HBX_LOS) {
        // Property for LO export
        expert_factory::add_prop_node<bool>(expert,
            subtree,
            fe_path / "los" / lo / "export",
            false,
            AUTO_RESOLVE_ON_WRITE);
        // Property for LO import
        expert_factory::add_prop_node<bool>(expert,
            subtree,
            fe_path / "los" / lo / "import",
            false,
            AUTO_RESOLVE_ON_WRITE);
    }
    // Property for LO frequency (desired and coerced)
    expert_factory::add_dual_prop_node<double>(expert,
        subtree,
        fe_path / "los" / HBX_LO / "freq" / "value",
        LMX2572_DEFAULT_FREQ,
        AUTO_RESOLVE_ON_WRITE);


    subtree->create<meta_range_t>(fe_path / "los" / HBX_LO / "freq/range")
        .set_publisher([this]() { return this->_get_lo_freq_range(HBX_LO); })
        .add_coerced_subscriber([](const meta_range_t&) {
            throw uhd::runtime_error("Attempting to update LO freq range!");
        });

    subtree->create<sensor_value_t>(fe_path / "sensors" / "lo_locked")
        .add_coerced_subscriber([](const sensor_value_t&) {
            throw uhd::runtime_error("Attempting to write to sensor!");
        })
        .set_publisher([this, trx]() {
            auto lo_ctrl_map = std::map<uhd::direction_t, std::shared_ptr<hbx_lo_ctrl>>{
                {RX_DIRECTION, this->_rx_lo_ctrl},
                {TX_DIRECTION, this->_tx_lo_ctrl},
            };
            auto lo_ctrl = lo_ctrl_map.at(trx);
            return sensor_value_t(
                "all_los", lo_ctrl_map.at(trx)->get_lock_status(), "locked", "unlocked");
        });
    subtree->create<sensor_value_t>(fe_path / "sensors" / "nco_locked")
        .add_coerced_subscriber([](const sensor_value_t&) {
            throw uhd::runtime_error("Attempting to write to sensor!");
        })
        .set_publisher([this]() {
            return sensor_value_t(
                RFDC_NCO, this->_rfdcc->get_nco_reset_done(), "locked", "unlocked");
        });
    subtree->create<sensor_value_t>(fe_path / "sensors" / "lo_pd")
        .add_coerced_subscriber([](const sensor_value_t&) {
            throw uhd::runtime_error("Attempting to write to sensor!");
        })
        .set_publisher([this, trx]() {
            const auto fe_path_local = _get_frontend_path(trx, 0);
            const auto lo_freq_path  = fe_path_local / "los" / HBX_LO / "freq" / "value";
            auto lo_pd_ctrl          = (trx == RX_DIRECTION) ? _rx_lo_pd : _tx_lo_pd;

            hbx_cpld_ctrl::pd_read_fct_t read_fn = [this, lo_pd_ctrl, lo_freq_path]() {
                const double lo_freq = get_tree()->access<double>(lo_freq_path).get();
                return static_cast<double>(lo_pd_ctrl->read_data_dbm(lo_freq));
            };

            const bool lo_imported =
                get_tree()->access<bool>(fe_path_local / "los" / HBX_LO / "import").get();
            if (lo_imported) {
                UHD_LOG_WARNING(get_unique_id(),
                    "LO power detector reading may be inaccurate when using an external "
                    "LO, as the actual LO frequency is unknown.");
            }
            return sensor_value_t(LO_PD, _cpld->read_lo_pd_val(trx, read_fn), "dBm");
        });
}

void hbx_dboard_impl::_init_iq_dc_prop_tree(uhd::property_tree::sptr subtree,
    uhd::experts::expert_container::sptr expert,
    const uhd::direction_t trx,
    const fs_path fe_path)
{
    expert_factory::add_dual_prop_node<iq_dc_cal_coeffs_t>(expert,
        subtree,
        fe_path / "iq_balance/coeffs/value",
        IQ_DC_DEFAULT_VALUES,
        AUTO_RESOLVE_ON_WRITE);

    // Make the DC offset calibration values from the converter available in the property
    // tree so they can be read by the correction utility.
    if (trx == RX_DIRECTION) {
        // This is only relevant for RX as the DACs don't do an own DC offset calibration.
        const auto i_coeffs = (_mb_rpcc->get_cal_coefs(0, _db_idx, 1, 1))[0];
        const auto q_coeffs = (_mb_rpcc->get_cal_coefs(0, _db_idx, 1, 2))[0];
        // The cal coeffs are 8 values, but we have only 4 sub-ADCs, therefore the average
        // must be taken from 4 coeffs and not from the length of the vector.
        const auto i_avg = std::accumulate(i_coeffs.begin(), i_coeffs.end(), 0.0) / 4;
        const auto q_avg = std::accumulate(q_coeffs.begin(), q_coeffs.end(), 0.0) / 4;

        _rfdc_dc_offset = {i_avg, q_avg};
        expert_factory::add_prop_node<double>(expert,
            subtree,
            fe_path / "dc_offset/i_conv_cal/value",
            _rfdc_dc_offset.real(),
            AUTO_RESOLVE_ON_WRITE);
        expert_factory::add_prop_node<double>(expert,
            subtree,
            fe_path / "dc_offset/q_conv_cal/value",
            _rfdc_dc_offset.imag(),
            AUTO_RESOLVE_ON_WRITE);
    }
}
}}} // namespace uhd::usrp::hbx
