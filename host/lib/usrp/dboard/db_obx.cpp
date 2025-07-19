//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "db_obx.hpp"
#include "obx/obx_cpld_ctrl.hpp"
#include "obx/obx_expert.hpp"
#include "obx/obx_gpio_ctrl.hpp"
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/usrp/common/max287x.hpp>
#include <uhdlib/usrp/common/tmp468.hpp>
#include <cmath>

namespace uhd { namespace usrp { namespace dboard { namespace obx {

obx_xcvr::obx_xcvr(ctor_args_t args) : xcvr_dboard_base(args), _db_iface(get_iface())
{
    _initialize_ref_clocks();
    _gpio = std::make_shared<obx_gpio_ctrl>(_db_iface);
    _cpld = std::make_shared<obx_cpld_ctrl>(_db_iface, _gpio);

    const size_t db_temp_addr = _db_iface->get_special_props().mangle_i2c_addrs ? 0x49
                                                                                : 0x48;

    _temp_sensor = tmp468_iface::make(_db_iface, db_temp_addr);
    _initialize_los();
    _expert_container = uhd::experts::expert_factory::create_container("obx_expert");
    _initialize_property_tree();
    _initialize_experts();
    _expert_container->resolve_all();
    _add_register_abstractions();
}

obx_xcvr::~obx_xcvr() {}

void obx_xcvr::_initialize_ref_clocks()
{
    // Enable the reference clocks that we need
    uint32_t pfd_freq_max   = 50e6;
    _rx_target_pfd_freq     = pfd_freq_max;
    _tx_target_pfd_freq     = pfd_freq_max;
    bool can_set_clock_rate = true;
    // set dboard clock rates to as close to the max PFD freq as possible while making
    // sure the master clock rate is integer-divisible by the chosen rate.
    if (_db_iface->get_clock_rate(dboard_iface::UNIT_RX) > pfd_freq_max) {
        std::vector<double> rates = _db_iface->get_clock_rates(dboard_iface::UNIT_RX);
        double master_clock_rate  = _db_iface->get_codec_rate(dboard_iface::UNIT_RX);
        double highest_rate       = 0.0;
        for (double rate : rates) {
            if (rate <= pfd_freq_max and rate > highest_rate
                and uhd::math::fp_compare::freq_compare_epsilon(
                        std::fmod(master_clock_rate, rate))
                        == 0.0)
                highest_rate = rate;
        }
        try {
            _db_iface->set_clock_rate(dboard_iface::UNIT_RX, highest_rate);
            _db_iface->lock_clock_rate(dboard_iface::UNIT_RX);
        } catch (const uhd::runtime_error&) {
            UHD_LOG_WARNING("OBX", "Unable to set dboard clock rate - phase will vary");
            can_set_clock_rate = false;
        }
        _rx_target_pfd_freq = highest_rate;
    }
    if (can_set_clock_rate
        and _db_iface->get_clock_rate(dboard_iface::UNIT_TX) > pfd_freq_max) {
        std::vector<double> rates = _db_iface->get_clock_rates(dboard_iface::UNIT_TX);
        double master_clock_rate  = _db_iface->get_codec_rate(dboard_iface::UNIT_TX);
        double highest_rate       = 0.0;
        for (double rate : rates) {
            if (rate <= pfd_freq_max and rate > highest_rate
                and uhd::math::fp_compare::freq_compare_epsilon(
                        std::fmod(master_clock_rate, rate))
                        == 0.0)
                highest_rate = rate;
        }
        try {
            _db_iface->set_clock_rate(dboard_iface::UNIT_TX, highest_rate);
            _db_iface->lock_clock_rate(dboard_iface::UNIT_TX);
        } catch (const uhd::runtime_error&) {
            UHD_LOG_WARNING("OBX", "Unable to set dboard clock rate - phase will vary");
        }
        _tx_target_pfd_freq = highest_rate;
    }
    _db_iface->set_clock_enabled(dboard_iface::UNIT_TX, true);
    _db_iface->set_clock_enabled(dboard_iface::UNIT_RX, true);
}

void obx_xcvr::_initialize_los()
{
    _txlo1 = max287x_iface::make<max2871>(
        [this](const std::vector<uint32_t>& regs) { _cpld->write_lo_regs(TXLO1, regs); });
    _txlo2 = max287x_iface::make<max2871>(
        [this](const std::vector<uint32_t>& regs) { _cpld->write_lo_regs(TXLO2, regs); });
    _rxlo1 = max287x_iface::make<max2871>(
        [this](const std::vector<uint32_t>& regs) { _cpld->write_lo_regs(RXLO1, regs); });
    _rxlo2 = max287x_iface::make<max2871>(
        [this](const std::vector<uint32_t>& regs) { _cpld->write_lo_regs(RXLO2, regs); });
    std::vector<max287x_iface::sptr> los{_txlo1, _txlo2, _rxlo1, _rxlo2};
    for (max287x_iface::sptr lo : los) {
        lo->set_auto_retune(false);
        lo->set_charge_pump_current(max287x_iface::CHARGE_PUMP_CURRENT_5_12MA);
        lo->set_muxout_mode(max287x_iface::MUXOUT_SYNC);
        lo->set_ld_pin_mode(max287x_iface::LD_PIN_MODE_DLD);
    }
}

void obx_xcvr::_initialize_property_tree()
{
    ////////////////////////////////////////////////////////////////////
    // Register power save properties
    ////////////////////////////////////////////////////////////////////
    get_rx_subtree()
        ->create<std::vector<std::string>>("power_mode/options")
        .set(obx_power_modes);
    uhd::experts::expert_factory::add_prop_node<std::string>(_expert_container,
        get_rx_subtree(),
        "power_mode/value",
        "power_mode",
        "performance",
        uhd::experts::AUTO_RESOLVE_ON_WRITE);
    uhd::experts::expert_factory::add_data_node<power_mode_t>(
        _expert_container, "coerced_power_mode", PERFORMANCE);
    get_rx_subtree()
        ->create<std::vector<std::string>>("xcvr_mode/options")
        .set(obx_xcvr_modes);
    uhd::experts::expert_factory::add_prop_node<std::string>(_expert_container,
        get_rx_subtree(),
        "xcvr_mode/value",
        "xcvr_mode",
        "FDX",
        uhd::experts::AUTO_RESOLVE_ON_WRITE);
    uhd::experts::expert_factory::add_data_node<xcvr_mode_t>(
        _expert_container, "coerced_xcvr_mode", FDX);
    get_rx_subtree()
        ->create<std::vector<std::string>>("temp_comp_mode/options")
        .set(obx_temp_comp_modes);
    uhd::experts::expert_factory::add_prop_node<std::string>(_expert_container,
        get_rx_subtree(),
        "temp_comp_mode/value",
        "temp_comp_mode",
        "disabled",
        uhd::experts::AUTO_RESOLVE_ON_WRITE);
    get_tx_subtree()
        ->create<std::vector<std::string>>("power_mode/options")
        .set(obx_power_modes);
    get_tx_subtree()
        ->create<std::string>("power_mode/value")
        .add_coerced_subscriber([this](std::string mode) {
            get_rx_subtree()->access<std::string>("power_mode/value").set(mode);
        })
        .set_publisher([this]() {
            return get_rx_subtree()->access<std::string>("power_mode/value").get();
        });
    get_tx_subtree()
        ->create<std::vector<std::string>>("xcvr_mode/options")
        .set(obx_xcvr_modes);
    get_tx_subtree()
        ->create<std::string>("xcvr_mode/value")
        .add_coerced_subscriber([this](std::string mode) {
            get_rx_subtree()->access<std::string>("xcvr_mode/value").set(mode);
        })
        .set_publisher([this]() {
            return get_rx_subtree()->access<std::string>("xcvr_mode/value").get();
        });
    get_tx_subtree()
        ->create<std::vector<std::string>>("temp_comp_mode/options")
        .set(obx_temp_comp_modes);
    get_tx_subtree()
        ->create<std::string>("temp_comp_mode/value")
        .add_coerced_subscriber([this](std::string mode) {
            get_rx_subtree()->access<std::string>("temp_comp_mode/value").set(mode);
        })
        .set_publisher([this]() {
            return get_rx_subtree()->access<std::string>("temp_comp_mode/value").get();
        });

    ////////////////////////////////////////////////////////////////////
    // Register TX properties
    ////////////////////////////////////////////////////////////////////
    get_tx_subtree()->create<std::string>("name").set("OBX TX");
    uhd::experts::expert_factory::add_prop_node<device_addr_t>(
        _expert_container, get_tx_subtree(), "tune_args", "tune_args", device_addr_t());
    get_tx_subtree()->create<sensor_value_t>("sensors/lo_locked").set_publisher([this]() {
        return sensor_value_t(
            "TXLO", _gpio->get_field(TX_LO_LOCKED) != 0, "locked", "unlocked");
    });
    get_tx_subtree()->create<sensor_value_t>("sensors/temp_top").set_publisher([this]() {
        return sensor_value_t("Temp Top",
            _temp_sensor->read_temperature(tmp468_iface::LOCAL_SENSOR),
            "degC");
    });
    get_tx_subtree()
        ->create<sensor_value_t>("sensors/temp_bottom")
        .set_publisher([this]() {
            return sensor_value_t("Temp Bottom",
                _temp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR1),
                "degC");
        });
    uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
        get_tx_subtree(),
        "gains/PGA0/value",
        "tx_gain/desired",
        "tx_gain/coerced",
        0,
        uhd::experts::AUTO_RESOLVE_ON_WRITE);
    get_tx_subtree()->create<meta_range_t>("gains/PGA0/range").set(obx_gain_range);
    get_tx_subtree()->create<meta_range_t>("freq/range").set(obx_freq_range);
    uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
        get_tx_subtree(),
        "freq/value",
        "tx_freq/desired",
        "tx_freq/coerced",
        obx_freq_range.start(),
        uhd::experts::AUTO_RESOLVE_ON_WRITE)
        .set_coercer([this](const double) {
            // Dummy Coercer that just gets the result of the expert coercion, but we need
            // to have a coercer set so that coerced_subscribers are called
            return get_tx_subtree()->access<double>("freq/value").get();
        });
    get_tx_subtree()
        ->create<std::vector<std::string>>("antenna/options")
        .set(obx_tx_antennas);
    uhd::experts::expert_factory::add_prop_node<std::string>(_expert_container,
        get_tx_subtree(),
        "antenna/value",
        "tx_antenna",
        obx_tx_antennas.at(0),
        uhd::experts::AUTO_RESOLVE_ON_WRITE);
    get_tx_subtree()->create<std::string>("connection").set("QI");
    get_tx_subtree()->create<bool>("enabled").set(true); // always enabled
    get_tx_subtree()->create<bool>("use_lo_offset").set(false);
    get_tx_subtree()->create<meta_range_t>("bandwidth/range").set(obx_bw_range);
    get_tx_subtree()
        ->create<double>("bandwidth/value")
        .set_coercer([this](const double bandwidth) {
            return get_tx_subtree()
                ->access<meta_range_t>("bandwidth/range")
                .get()
                .clip(bandwidth);
        })
        .set(obx_bw_range.start());
    uhd::experts::expert_factory::add_prop_node<int64_t>(
        _expert_container, get_tx_subtree(), "sync_delay", "tx_sync_delay", 0);
    get_tx_subtree()
        ->create<bool>("calibrate_vco_map")
        .add_coerced_subscriber(
            [this](const bool) { _calibrate_vco_maps(TX_DIRECTION); });
    get_tx_subtree()
        ->create<std::map<uint8_t, uhd::range_t>>("LO1/vco_map")
        .add_coerced_subscriber(
            [this](std::map<uint8_t, uhd::range_t> map) { _txlo1->set_vco_map(map); })
        .set_publisher([this]() { return _txlo1->get_vco_map(); });
    get_tx_subtree()
        ->create<std::map<uint8_t, uhd::range_t>>("LO2/vco_map")
        .add_coerced_subscriber(
            [this](std::map<uint8_t, uhd::range_t> map) { _txlo2->set_vco_map(map); })
        .set_publisher([this]() { return _txlo2->get_vco_map(); });
    // For OBX, it was seen that iq balance became worse at higher gains, which caused
    // correction values to be chosen that actually worsened the iq imbalance when
    // running the device at mid-level gains. Choosing a mid-level gain instead, while
    // running the utilities, caused better correction values to be chosen for the
    // full gain range, though the values were not quite as optimal for higher gains.
    get_tx_subtree()->create<double>("default_cal_gain").set(15);

    ////////////////////////////////////////////////////////////////////
    // Register RX properties
    ////////////////////////////////////////////////////////////////////
    get_rx_subtree()->create<std::string>("name").set("OBX RX");
    get_rx_subtree()->create<device_addr_t>("tune_args").set(device_addr_t());
    get_rx_subtree()->create<sensor_value_t>("sensors/lo_locked").set_publisher([this]() {
        return sensor_value_t(
            "RXLO", _gpio->get_field(RX_LO_LOCKED) != 0, "locked", "unlocked");
    });
    get_rx_subtree()->create<sensor_value_t>("sensors/temp_top").set_publisher([this]() {
        return sensor_value_t("Temp Top",
            _temp_sensor->read_temperature(tmp468_iface::LOCAL_SENSOR),
            "degC");
    });
    get_rx_subtree()
        ->create<sensor_value_t>("sensors/temp_bottom")
        .set_publisher([this]() {
            return sensor_value_t("Temp Bottom",
                _temp_sensor->read_temperature(tmp468_iface::REMOTE_SENSOR1),
                "degC");
        });
    uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
        get_rx_subtree(),
        "gains/PGA0/value",
        "rx_gain/desired",
        "rx_gain/coerced",
        0,
        uhd::experts::AUTO_RESOLVE_ON_WRITE);
    get_rx_subtree()->create<meta_range_t>("gains/PGA0/range").set(obx_gain_range);
    get_rx_subtree()->create<meta_range_t>("freq/range").set(obx_freq_range);
    uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
        get_rx_subtree(),
        "freq/value",
        "rx_freq/desired",
        "rx_freq/coerced",
        obx_freq_range.start(),
        uhd::experts::AUTO_RESOLVE_ON_WRITE)
        .set_coercer([this](const double) {
            // Dummy coercer that just gets the result of the expert coercion, but we need
            // to have a coercer set so that coerced_subscribers are called
            return get_rx_subtree()->access<double>("freq/value").get();
        });
    get_rx_subtree()
        ->create<std::vector<std::string>>("antenna/options")
        .set(obx_rx_antennas);
    uhd::experts::expert_factory::add_prop_node<std::string>(_expert_container,
        get_rx_subtree(),
        "antenna/value",
        "rx_antenna",
        "RX2",
        uhd::experts::AUTO_RESOLVE_ON_WRITE);
    get_rx_subtree()->create<std::string>("connection").set("IQ");
    get_rx_subtree()->create<bool>("enabled").set(true); // always enabled
    get_rx_subtree()->create<bool>("use_lo_offset").set(false);
    get_rx_subtree()->create<meta_range_t>("bandwidth/range").set(obx_bw_range);
    get_rx_subtree()
        ->create<double>("bandwidth/value")
        .set_coercer([this](const double bandwidth) {
            return get_rx_subtree()
                ->access<meta_range_t>("bandwidth/range")
                .get()
                .clip(bandwidth);
        })
        .set(obx_bw_range.start());
    uhd::experts::expert_factory::add_prop_node<int64_t>(
        _expert_container, get_rx_subtree(), "sync_delay", "rx_sync_delay", 0);
    get_rx_subtree()
        ->create<bool>("calibrate_vco_map")
        .add_coerced_subscriber(
            [this](const bool) { _calibrate_vco_maps(RX_DIRECTION); });
    get_rx_subtree()
        ->create<std::map<uint8_t, uhd::range_t>>("LO1/vco_map")
        .add_coerced_subscriber(
            [this](std::map<uint8_t, uhd::range_t> map) { _rxlo1->set_vco_map(map); })
        .set_publisher([this]() { return _rxlo1->get_vco_map(); });
    get_rx_subtree()
        ->create<std::map<uint8_t, uhd::range_t>>("LO2/vco_map")
        .add_coerced_subscriber(
            [this](std::map<uint8_t, uhd::range_t> map) { _rxlo2->set_vco_map(map); })
        .set_publisher([this]() { return _rxlo2->get_vco_map(); });
}

void obx_xcvr::_initialize_experts()
{
    uhd::experts::expert_factory::add_worker_node<obx_tx_frequency_expert>(
        _expert_container,
        _expert_container->node_retriever(),
        _db_iface,
        _cpld,
        _gpio,
        _txlo1,
        _txlo2,
        _tx_target_pfd_freq);
    uhd::experts::expert_factory::add_worker_node<obx_rx_frequency_expert>(
        _expert_container,
        _expert_container->node_retriever(),
        _db_iface,
        _cpld,
        _gpio,
        _rxlo1,
        _rxlo2,
        _rx_target_pfd_freq);
    uhd::experts::expert_factory::add_worker_node<obx_tx_frontend_expert>(
        _expert_container, _expert_container->node_retriever(), _db_iface);
    uhd::experts::expert_factory::add_worker_node<obx_rx_frontend_expert>(
        _expert_container, _expert_container->node_retriever(), _db_iface);
    uhd::experts::expert_factory::add_worker_node<obx_tx_antenna_expert>(
        _expert_container, _expert_container->node_retriever(), _cpld);
    uhd::experts::expert_factory::add_worker_node<obx_rx_antenna_expert>(
        _expert_container, _expert_container->node_retriever(), _cpld, _gpio);
    uhd::experts::expert_factory::add_worker_node<obx_tx_gain_expert>(
        _expert_container, _expert_container->node_retriever(), _gpio);
    uhd::experts::expert_factory::add_worker_node<obx_rx_gain_expert>(
        _expert_container, _expert_container->node_retriever(), _gpio);
    uhd::experts::expert_factory::add_worker_node<obx_xcvr_mode_expert>(
        _expert_container, _expert_container->node_retriever(), _cpld);
    uhd::experts::expert_factory::add_worker_node<obx_temp_comp_mode_expert>(
        _expert_container,
        _expert_container->node_retriever(),
        _txlo1,
        _txlo2,
        _rxlo1,
        _rxlo2);
    uhd::experts::expert_factory::add_worker_node<obx_power_mode_expert>(
        _expert_container, _expert_container->node_retriever(), _db_iface, _cpld);
}

void obx_xcvr::_add_register_abstractions()
{
    _db_iface->define_custom_register_space(
        0x80000000,
        5,
        [this](uint32_t addr, uint32_t value) {
            _txlo1->set_register(uint8_t(addr - 0x80000000), 0xFFFFFFFF, value, true);
        },
        [this](
            uint32_t addr) { return _txlo1->get_register(uint8_t(addr - 0x80000000)); });
    _db_iface->define_custom_register_space(
        0x81000000,
        5,
        [this](uint32_t addr, uint32_t value) {
            _txlo2->set_register(uint8_t(addr - 0x81000000), 0xFFFFFFFF, value, true);
        },
        [this](
            uint32_t addr) { return _txlo2->get_register(uint8_t(addr - 0x81000000)); });
    _db_iface->define_custom_register_space(
        0x82000000,
        5,
        [this](uint32_t addr, uint32_t value) {
            _rxlo1->set_register(uint8_t(addr - 0x82000000), 0xFFFFFFFF, value, true);
        },
        [this](
            uint32_t addr) { return _rxlo1->get_register(uint8_t(addr - 0x82000000)); });
    _db_iface->define_custom_register_space(
        0x83000000,
        5,
        [this](uint32_t addr, uint32_t value) {
            _rxlo2->set_register(uint8_t(addr - 0x83000000), 0xFFFFFFFF, value, true);
        },
        [this](
            uint32_t addr) { return _rxlo2->get_register(uint8_t(addr - 0x83000000)); });
    _db_iface->define_custom_register_space(
        0x84000000,
        32,
        [this](uint32_t addr, uint32_t value) {
            _cpld->set_field(obx_tx_cpld_field_id_t(addr - 0x84000000), value);
            _cpld->write();
        },
        [this](uint32_t) { return _cpld->get_tx_value(); });
    _db_iface->define_custom_register_space(
        0x85000000,
        32,
        [this](uint32_t addr, uint32_t value) {
            _cpld->set_field(obx_rx_cpld_field_id_t(addr - 0x85000000), value);
            _cpld->write();
        },
        [this](uint32_t) { return _cpld->get_rx_value(); });
}

void obx_xcvr::_calibrate_vco_maps(uhd::direction_t dir)
{
    if (dir == TX_DIRECTION) {
        double orig_freq = get_tx_subtree()->access<double>("freq/value").get();
        double ref_freq  = _db_iface->get_clock_rate(dboard_iface::UNIT_TX);

        // Set to mid band frequency (so lock detect will be LO1 only)
        get_tx_subtree()->access<double>("freq/value").set(6000e6);
        // Calibrate LO1
        _txlo1->calibrate_vco_map(
            [this]() { return _gpio->get_field(TX_LO_LOCKED) != 0; },
            ref_freq,
            _tx_target_pfd_freq);
        // Set to low band frequency (so LO1 will be fixed and lock detect will be LO1
        // & LO2)
        get_tx_subtree()->access<double>("freq/value").set(400e6);
        // Calibrate LO2
        _txlo2->calibrate_vco_map(
            [this]() { return _gpio->get_field(TX_LO_LOCKED) != 0; },
            ref_freq,
            _tx_target_pfd_freq);
        // Restore to original frequency
        get_tx_subtree()->access<double>("freq/value").set(orig_freq);
    } else {
        double orig_freq = get_rx_subtree()->access<double>("freq/value").get();
        double ref_freq  = _db_iface->get_clock_rate(dboard_iface::UNIT_RX);

        // Set to mid band frequency (so lock detect will be LO1 only)
        get_rx_subtree()->access<double>("freq/value").set(6000e6);
        // Calibrate LO1
        _rxlo1->calibrate_vco_map(
            [this]() { return _gpio->get_field(RX_LO_LOCKED) != 0; },
            ref_freq,
            _rx_target_pfd_freq);
        // Set to low band frequency (so LO1 will be fixed and lock detect will be LO1
        // & LO2)
        get_rx_subtree()->access<double>("freq/value").set(400e6);
        // Calibrate LO2
        _rxlo2->calibrate_vco_map(
            [this]() { return _gpio->get_field(RX_LO_LOCKED) != 0; },
            ref_freq,
            _rx_target_pfd_freq);
        // Restore to original frequency
        get_rx_subtree()->access<double>("freq/value").set(orig_freq);
    }
}

/***********************************************************************
 * Register the OBX dboard
 **********************************************************************/
static dboard_base::sptr make_obx(dboard_base::ctor_args_t args)
{
    return dboard_base::sptr(new obx_xcvr(args));
}

UHD_STATIC_BLOCK(reg_obx_dboards)
{
    dboard_manager::register_dboard(OBX_RX_ID, OBX_TX_ID, &make_obx, "OBX");
}

}}}} // namespace uhd::usrp::dboard::obx
