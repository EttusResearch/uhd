//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_radio_control.hpp"
#include "b300_clock_ctrl.hpp"
#include "b300_experts.hpp"
#include "b300_mb_controller.hpp"
#include <uhd/rfnoc/registry.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/rfnoc/radio_control_impl.hpp>
#include <uhdlib/rfnoc/reg_iface_adapter.hpp>
#include <uhdlib/usrp/common/adrv9032_ctrl.hpp>
#include <uhdlib/usrp/common/pwr_cal_mgr.hpp>
#include <adi_adrv903x_datainterface.h>
#include <adi_adrv903x_dev_temp_types.h>
#include <chrono>
#include <string>

namespace uhd { namespace rfnoc {

b300_radio_control_impl::b300_radio_control_impl(make_args_ptr make_args)
    : radio_control_impl(std::move(make_args)), _rx_ant("RX1")
{
    _spi = spi_core_adrv::make(
        [this](const uint32_t addr, const std::vector<uint32_t>& data) {
            regs().block_poke32(addr, data, get_command_time(0));
        },
        [this](const uint32_t addr, const size_t length) {
            return regs().block_peek32(addr, length, get_command_time(0));
        },
        [this](const uint32_t addr, const std::vector<uint32_t>& data) {
            regs().burst_poke32(addr, data, get_command_time(0));
        },
        [this](const uint32_t addr, const size_t length) {
            return regs().burst_peek32(addr, length, get_command_time(0));
        },
        b300_regs::SR_ADRV_SPI);

    for (size_t radio_chan = 0; radio_chan < get_num_input_ports(); ++radio_chan) {
        _wb_ifaces.push_back(RFNOC_MAKE_WB_IFACE(0, radio_chan));
        _adrv9032_gpio.emplace_back(usrp::gpio_atr::gpio_atr_3000::make(_wb_ifaces.back(),
            usrp::gpio_atr::gpio_atr_offsets::make_default(
                b300_regs::ATR_PALMA + (radio_chan * b300_regs::ATR_CHAN_REG_OFFSET),
                b300_regs::ATR_PALMA + (radio_chan * b300_regs::ATR_CHAN_REG_OFFSET)
                    + b300_regs::ATR_READ_OFFSET,
                1 /* Stride */)));
        _adrv9032_gpio.back()->set_atr_mode(
            usrp::gpio_atr::MODE_GPIO, usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
        // Start with all channels disabled.
        _adrv9032_gpio.back()->set_gpio_out(0x0);

        _leds.emplace_back(usrp::gpio_atr::gpio_atr_3000::make(_wb_ifaces.back(),
            usrp::gpio_atr::gpio_atr_offsets::make_write_only(
                b300_regs::ATR_LEDS + radio_chan * b300_regs::ATR_CHAN_REG_OFFSET,
                1 /* Stride */)));
        _leds.back()->set_atr_mode(
            usrp::gpio_atr::MODE_ATR, usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
        _leds.back()->set_atr_reg(usrp::gpio_atr::ATR_REG_IDLE, 0x0);
        _leds.back()->set_atr_reg(
            usrp::gpio_atr::ATR_REG_RX_ONLY, b300_regs::SR_RX_GREEN);
        _leds.back()->set_atr_reg(
            usrp::gpio_atr::ATR_REG_TX_ONLY, b300_regs::SR_TXRX_RED);
        _leds.back()->set_atr_reg(usrp::gpio_atr::ATR_REG_FULL_DUPLEX,
            b300_regs::SR_RX_GREEN | b300_regs::SR_TXRX_RED);

        // For default Rx antenna, all ATR states connect the Tx path to the TX/RX0 port
        _rf_path_atr.emplace_back(usrp::gpio_atr::gpio_atr_3000::make(_wb_ifaces.back(),
            usrp::gpio_atr::gpio_atr_offsets::make_write_only(
                b300_regs::ATR_RF_PATH + radio_chan * b300_regs::ATR_CHAN_REG_OFFSET,
                1 /* Stride */)));
        _rf_path_atr.back()->set_atr_mode(
            usrp::gpio_atr::MODE_ATR, usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
        _rf_path_atr.back()->set_atr_reg(
            usrp::gpio_atr::ATR_REG_IDLE, b300_regs::TRX_PATH_TX);
        _rf_path_atr.back()->set_atr_reg(
            usrp::gpio_atr::ATR_REG_RX_ONLY, b300_regs::TRX_PATH_TX);
        _rf_path_atr.back()->set_atr_reg(
            usrp::gpio_atr::ATR_REG_TX_ONLY, b300_regs::TRX_PATH_TX);
        _rf_path_atr.back()->set_atr_reg(
            usrp::gpio_atr::ATR_REG_FULL_DUPLEX, b300_regs::TRX_PATH_TX);

        _fp_gpio.emplace_back(usrp::gpio_atr::gpio_atr_3000::make(_wb_ifaces.front(),
            usrp::gpio_atr::gpio_atr_offsets::make_default(
                b300_regs::ATR_FP_GPIO + radio_chan * b300_regs::ATR_CHAN_REG_OFFSET,
                (b300_regs::ATR_FP_GPIO + radio_chan * b300_regs::ATR_CHAN_REG_OFFSET)
                    + b300_regs::ATR_READ_OFFSET,
                1 /* Stride */)));
    }

    _regs = std::make_shared<b300_radio_regmap_t>();
    _regs->initialize(*(_wb_ifaces.front()), true);

    UHD_ASSERT_THROW(get_mb_controller());
    _mb_control = std::dynamic_pointer_cast<b300_mb_controller>(get_mb_controller());
    UHD_ASSERT_THROW(_mb_control);
    _clock_ctrl = _mb_control->get_clock_ctrl();

    _master_clock_rate = _mb_control->get_clock_ctrl()->get_master_clock_rate();
    UHD_ASSERT_THROW(get_tick_rate() == _master_clock_rate);
    radio_control_impl::set_rate(_master_clock_rate);

    auto block_args = get_block_args();

    _adrv9032_manager = uhd::usrp::adrv9032_manager::make(
        _spi,
        [this](uint8_t value) {
            _regs->misc_outs_reg.write(
                b300_radio_regmap_t::misc_outs_reg_t::ADRV9032_RESET, value);
        },
        _master_clock_rate,
        [this](const size_t channel, const uint32_t value) {
            _adrv9032_gpio[channel]->set_gpio_out(value);
        },
        [this](const uhd::time_spec_t& sleep_time) { regs().sleep(sleep_time); },
        block_args.get("init_cals", "DEFAULT"),
        block_args.get("tracking_cals", "DEFAULT"));

    _jesd_core = std::make_shared<uhd::usrp::b300::b300_jesd_core>(
        [this](uint32_t addr, uint32_t value) { regs().poke32(addr, value); },
        [this](uint32_t addr) { return regs().peek32(addr); },
        b300_regs::JESD_NUM_QPLLS,
        b300_regs::JESD_NUM_CPLLS,
        b300_regs::JESD_LMFC_DIVIDER,
        b300_regs::JESD_RX_SYSREF_DELAY,
        b300_regs::JESD_TX_SYSREF_DELAY);
    _jesd_core->reset();

    // Reset disables the SYSREF sampler, so we reset the LMFC counters before
    // enabling the SYSREF sampler in _jesd_init().
    _jesd_core->reset_lmfc();

    _jesd_init();

    _adrv9032_manager->enable_tracking_cals();

    _expert_container = uhd::experts::expert_factory::create_container("b300_experts");
    _init_prop_tree();
    _init_experts();
    _expert_container->resolve_all();
    _init_power_cal_managers();

    // Properties
    for (auto& samp_rate_prop : _samp_rate_in) {
        set_property(samp_rate_prop.get_id(), get_rate(), samp_rate_prop.get_src_info());
    }
    for (auto& samp_rate_prop : _samp_rate_out) {
        set_property(samp_rate_prop.get_id(), get_rate(), samp_rate_prop.get_src_info());
    }

    // Stop the Radio from from responding to SYSREF before powering down the SYSREF.

    _jesd_core->enable_lmfc(false);

    // Power down the LMK SYSREF since it is no longer needed and can cause RF spurs
    // during operation.
    _clock_ctrl->power_down_lmk04832_sysref();
}

size_t b300_radio_control_impl::get_chan_from_dboard_fe(
    const std::string& fe, const uhd::direction_t) const
{
    if (fe == "0") {
        return 0;
    }
    if (fe == "1") {
        return 1;
    }
    throw uhd::key_error(std::string("[B300] Invalid frontend: ") + fe);
}

std::string b300_radio_control_impl::get_dboard_fe_from_chan(
    const size_t chan, const uhd::direction_t) const
{
    if (chan == 0) {
        return "0";
    }
    if (chan == 1) {
        return "1";
    }
    throw uhd::key_error(std::string("[B300] Invalid channel: ") + std::to_string(chan));
}

/**************************************************************************
 * RF-specific API calls
 *************************************************************************/
double b300_radio_control_impl::set_rate(double rate)
{
    // B300 does not support runtime rate changes
    if (!uhd::math::frequencies_are_equal(rate, get_rate())) {
        RFNOC_LOG_WARNING("Requesting invalid sampling rate from device: "
                          << (rate / 1e6)
                          << " MHz. Actual rate is: " << (get_rate() / 1e6) << " MHz.");
    }
    return get_rate();
}

void b300_radio_control_impl::set_tx_antenna(const std::string& ant, const size_t)
{
    // There is only one option for the Tx antenna, the switch connecting the Tx chain to
    // the TX/RX0 port is controlled by set_rx_antenna. Still check for valid inputs.
    if (ant != "TX/RX0" && ant != "TX/RX") {
        throw uhd::value_error("Invalid TX antenna: " + ant);
    }
}

void b300_radio_control_impl::set_rx_antenna(const std::string& ant, const size_t chan)
{
    // Accept the legacy antenna names "TX/RX" and "RX2" for compatibility with code for
    // older devices and coerce to the correct names.
    if (ant == "TX/RX0" || ant == "TX/RX") {
        _rf_path_atr[chan]->set_atr_reg(
            usrp::gpio_atr::ATR_REG_RX_ONLY, b300_regs::TRX_PATH_RX);
        _leds[chan]->set_atr_reg(
            usrp::gpio_atr::ATR_REG_RX_ONLY, b300_regs::SR_TXRX_GREEN);
        _rx_ant = "TX/RX0";
    } else if (ant == "RX1" || ant == "RX2") {
        _rf_path_atr[chan]->set_atr_reg(
            usrp::gpio_atr::ATR_REG_RX_ONLY, b300_regs::TRX_PATH_TX);
        _leds[chan]->set_atr_reg(usrp::gpio_atr::ATR_REG_RX_ONLY, b300_regs::SR_RX_GREEN);
        _rx_ant = "RX1";
    } else {
        throw uhd::value_error("Invalid RX antenna: " + ant);
    }
}

double b300_radio_control_impl::set_tx_frequency(const double freq, const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "tx_frontends" / chan / "freq" / "value")
        .set(freq)
        .get();
}

double b300_radio_control_impl::set_rx_frequency(const double freq, const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "rx_frontends" / chan / "freq" / "value")
        .set(freq)
        .get();
}

double b300_radio_control_impl::set_tx_gain(const double gain, const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "tx_frontends" / chan / "gains" / "all" / "value")
        .set(gain)
        .get();
}

double b300_radio_control_impl::set_rx_gain(const double gain, const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "rx_frontends" / chan / "gains" / "all" / "value")
        .set(gain)
        .get();
}

std::string b300_radio_control_impl::get_tx_antenna(const size_t chan) const
{
    return get_tree()
        ->access<std::string>(DB_PATH / "tx_frontends" / chan / "antenna" / "value")
        .get();
}

std::string b300_radio_control_impl::get_rx_antenna(const size_t chan) const
{
    return get_tree()
        ->access<std::string>(DB_PATH / "rx_frontends" / chan / "antenna" / "value")
        .get();
}

std::vector<std::string> b300_radio_control_impl::get_tx_antennas(const size_t chan) const
{
    return get_tree()
        ->access<std::vector<std::string>>(
            DB_PATH / "tx_frontends" / chan / "antenna" / "options")
        .get();
}

std::vector<std::string> b300_radio_control_impl::get_rx_antennas(const size_t chan) const
{
    return get_tree()
        ->access<std::vector<std::string>>(
            DB_PATH / "rx_frontends" / chan / "antenna" / "options")
        .get();
}

double b300_radio_control_impl::get_tx_frequency(const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "tx_frontends" / chan / "freq" / "value")
        .get();
}

double b300_radio_control_impl::get_rx_frequency(const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "rx_frontends" / chan / "freq" / "value")
        .get();
}

uhd::freq_range_t b300_radio_control_impl::get_tx_frequency_range(const size_t) const
{
    return uhd::usrp::adrv9032_freq_range;
}

uhd::freq_range_t b300_radio_control_impl::get_rx_frequency_range(const size_t) const
{
    return uhd::usrp::adrv9032_freq_range;
}

double b300_radio_control_impl::get_tx_gain(const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "tx_frontends" / chan / "gains" / "all" / "value")
        .get();
}

double b300_radio_control_impl::get_rx_gain(const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "rx_frontends" / chan / "gains" / "all" / "value")
        .get();
}

uhd::gain_range_t b300_radio_control_impl::get_tx_gain_range(const size_t) const
{
    return b300_tx_gain_range;
}

uhd::gain_range_t b300_radio_control_impl::get_tx_gain_range(
    const std::string&, const size_t) const
{
    return b300_tx_gain_range;
}

uhd::gain_range_t b300_radio_control_impl::get_rx_gain_range(const size_t) const
{
    return b300_rx_gain_range;
}

uhd::gain_range_t b300_radio_control_impl::get_rx_gain_range(
    const std::string&, const size_t) const
{
    return b300_rx_gain_range;
}

meta_range_t b300_radio_control_impl::get_tx_bandwidth_range(const size_t) const
{
    // TODO: Currently hardcoded - there doesn't appear to be a direct bandwidth query
    // function available in the Palma API, have user story to figure this out (AZDO
    // 3023560)
    return meta_range_t(100e6, 100e6);
}

double b300_radio_control_impl::get_tx_bandwidth(const size_t)
{
    // TODO: Currently hardcoded - there doesn't appear to be a direct bandwidth query
    // function available in the Palma API, have user story to figure this out (AZDO
    // 3023560)
    return 100e6;
}
meta_range_t b300_radio_control_impl::get_rx_bandwidth_range(const size_t) const
{
    // TODO: Currently hardcoded - there doesn't appear to be a direct bandwidth query
    // function available in the Palma API, have user story to figure this out (AZDO
    // 3023560)
    return meta_range_t(100e6, 100e6);
}

double b300_radio_control_impl::get_rx_bandwidth(const size_t)
{
    // TODO: Currently hardcoded - there doesn't appear to be a direct bandwidth query
    // function available in the Palma API, have user story to figure this out (AZDO
    // 3023560)
    return 100e6;
}

/**************************************************************************
 * LO Controls
 *************************************************************************/
std::vector<std::string> b300_radio_control_impl::get_rx_lo_names(const size_t) const
{
    return {"RFLO", "NCO"};
}

std::vector<std::string> b300_radio_control_impl::get_rx_lo_sources(
    const std::string& name, const size_t) const
{
    if (name == "RFLO") {
        return {"LO0", "LO1"};
    } else if (name == "NCO") {
        return {"NCO"};
    } else if (name == "all") {
        // RFLO is the only LO that can have a different source, so if the user uses the
        // special value ALL_LOS, then return the RFLO options.
        return {"LO0", "LO1"};
    } else {
        throw uhd::value_error("Invalid RX LO name: " + name);
    }
}

freq_range_t b300_radio_control_impl::get_rx_lo_freq_range(
    const std::string& name, const size_t) const
{
    if (name == "RFLO") {
        return uhd::usrp::adrv9032_lo_freq_range;
    } else if (name == "NCO") {
        // TODO (AzDo 3375444): Call into manager class for NCO frequency range?
        return freq_range_t(0, 100e6);
    } else {
        throw uhd::value_error("Invalid RX LO name: " + name);
    }
}

void b300_radio_control_impl::set_rx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    std::lock_guard<std::mutex> lock(_cmd_time_mutex);
    if (name == "all" || name == "RFLO") {
        // RFLO is the only LO that can have a different source, so if the user uses the
        // special value ALL_LOS, then set the RFLO source
        get_tree()
            ->access<std::string>(DB_PATH / "rx_frontends" / chan / "RFLO" / "source")
            .set(src);
    } else if (name == "NCO") {
        // For other LO names, which is just NCO, treat this as a noop since the NCO can't
        // actually have different sources.
    } else {
        throw uhd::value_error("Invalid RX LO name: " + name);
    }
}

std::string b300_radio_control_impl::get_rx_lo_source(
    const std::string& name, const size_t chan)
{
    std::lock_guard<std::mutex> lock(_cmd_time_mutex);
    if (name == "all" || name == "RFLO") {
        // RFLO is the only LO that can have a different source, so if the user uses the
        // special value ALL_LOS, then get the RFLO source
        return get_tree()
            ->access<std::string>(DB_PATH / "rx_frontends" / chan / "RFLO" / "source")
            .get();
    } else if (name == "NCO") {
        // NCO is the only other option, it can't have other sources
        return "NCO";
    } else {
        throw uhd::value_error("Invalid RX LO name: " + name);
    }
}

double b300_radio_control_impl::set_rx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "rx_frontends" / chan / name / "freq" / "value")
        .set(freq)
        .get();
}

double b300_radio_control_impl::get_rx_lo_freq(const std::string& name, const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "rx_frontends" / chan / name / "freq" / "value")
        .get();
}

std::vector<std::string> b300_radio_control_impl::get_tx_lo_names(const size_t) const
{
    return {"RFLO", "NCO"};
}

std::vector<std::string> b300_radio_control_impl::get_tx_lo_sources(
    const std::string& name, const size_t) const
{
    if (name == "RFLO") {
        return {"LO0", "LO1"};
    } else if (name == "NCO") {
        return {"NCO"};
    } else if (name == "all") {
        // RFLO is the only LO that can have a different source, so if the user uses the
        // special value ALL_LOS, then return the RFLO options.
        return {"LO0", "LO1"};
    } else {
        throw uhd::value_error("Invalid TX LO name: " + name);
    }
}

freq_range_t b300_radio_control_impl::get_tx_lo_freq_range(
    const std::string& name, const size_t)
{
    if (name == "RFLO") {
        return uhd::usrp::adrv9032_lo_freq_range;
    } else if (name == "NCO") {
        // TODO (AzDo 3375444): Call into manager class for NCO frequency range?
        return freq_range_t(0, 100e6);
    } else {
        throw uhd::value_error("Invalid TX LO name: " + name);
    }
}

void b300_radio_control_impl::set_tx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    std::lock_guard<std::mutex> lock(_cmd_time_mutex);
    if (name == "all" || name == "RFLO") {
        // RFLO is the only LO that can have a different source, so if the user uses the
        // special value ALL_LOS, then set the RFLO source
        get_tree()
            ->access<std::string>(DB_PATH / "tx_frontends" / chan / "RFLO" / "source")
            .set(src);
    } else if (name == "NCO") {
        // For other LO names, which is just NCO, treat this as a noop since the NCO can't
        // actually have different sources.
    } else {
        throw uhd::value_error("Invalid TX LO name: " + name);
    }
}

std::string b300_radio_control_impl::get_tx_lo_source(
    const std::string& name, const size_t chan)
{
    std::lock_guard<std::mutex> lock(_cmd_time_mutex);
    if (name == "all" || name == "RFLO") {
        // RFLO is the only LO that can have a different source, so if the user uses the
        // special value ALL_LOS, then get the RFLO source
        return get_tree()
            ->access<std::string>(DB_PATH / "tx_frontends" / chan / "RFLO" / "source")
            .get();
    } else if (name == "NCO") {
        // NCO is the only other option, it can't have other sources
        return "NCO";
    } else {
        throw uhd::value_error("Invalid TX LO name: " + name);
    }
}

double b300_radio_control_impl::set_tx_lo_freq(
    const double freq, const std::string& name, const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "tx_frontends" / chan / name / "freq" / "value")
        .set(freq)
        .get();
}

double b300_radio_control_impl::get_tx_lo_freq(const std::string& name, const size_t chan)
{
    return get_tree()
        ->access<double>(DB_PATH / "tx_frontends" / chan / name / "freq" / "value")
        .get();
}

std::vector<std::string> b300_radio_control_impl::get_rx_sensor_names(const size_t) const
{
    return {"lo_locked", "temp_rfic"};
}

uhd::sensor_value_t b300_radio_control_impl::get_rx_sensor(
    const std::string& name, const size_t chan)
{
    std::lock_guard<std::mutex> lock(_cmd_time_mutex);
    if (name == "lo_locked") {
        bool locked = _adrv9032_manager->get_pll_locked(RX_DIRECTION, chan);
        return sensor_value_t("LO", locked, "locked", "unlocked");
    } else if (name == "temp_rfic") {
        return sensor_value_t("Temp RFIC",
            static_cast<double>(_adrv9032_manager->get_average_temperature()),
            "degC");
    } else {
        throw uhd::key_error("Invalid RX sensor name: " + name);
    }
}

std::vector<std::string> b300_radio_control_impl::get_tx_sensor_names(const size_t) const
{
    return {"lo_locked", "temp_rfic"};
}

uhd::sensor_value_t b300_radio_control_impl::get_tx_sensor(
    const std::string& name, const size_t chan)
{
    std::lock_guard<std::mutex> lock(_cmd_time_mutex);
    if (name == "lo_locked") {
        bool locked = _adrv9032_manager->get_pll_locked(TX_DIRECTION, chan);
        return sensor_value_t("LO", locked, "locked", "unlocked");
    } else if (name == "temp_rfic") {
        return sensor_value_t("Temp RFIC",
            static_cast<double>(_adrv9032_manager->get_average_temperature()),
            "degC");
    } else {
        throw uhd::key_error("Invalid TX sensor name: " + name);
    }
}

void b300_radio_control_impl::_init_experts()
{
    for (size_t chan_idx = 0; chan_idx < get_num_output_ports(); ++chan_idx) {
        const fs_path fe_path = fs_path("rx_frontends") / chan_idx;
        uhd::experts::expert_factory::add_worker_node<
            uhd::usrp::b300::b300_desired_channel_frequency_expert>(_expert_container,
            _expert_container->node_retriever(),
            fe_path,
            _master_clock_rate);
        uhd::experts::expert_factory::add_worker_node<
            uhd::usrp::b300::b300_rflo_channel_frequency_expert>(_expert_container,
            _expert_container->node_retriever(),
            fe_path,
            _master_clock_rate);
        uhd::experts::expert_factory::add_worker_node<
            uhd::usrp::b300::b300_nco_programming_expert>(_expert_container,
            _expert_container->node_retriever(),
            fe_path,
            RX_DIRECTION,
            [this, chan_idx](const double freq) {
                std::lock_guard<std::mutex> lock(_cmd_time_mutex);
                auto coerced_freq = _adrv9032_manager->set_lo_freq(
                    RX_DIRECTION, freq, "NCO", chan_idx, _get_timed_command_enabled());
                if (_get_timed_command_enabled()) {
                    regs().sleep(uhd::usrp::ADRV9032_CMD_SLEEP_TIME);
                }
                return coerced_freq;
            });
        uhd::experts::expert_factory::add_worker_node<
            uhd::usrp::b300::b300_coerced_channel_frequency_expert>(
            _expert_container, _expert_container->node_retriever(), fe_path);
    }
    for (size_t chan_idx = 0; chan_idx < get_num_input_ports(); ++chan_idx) {
        const fs_path fe_path = fs_path("tx_frontends") / chan_idx;
        uhd::experts::expert_factory::add_worker_node<
            uhd::usrp::b300::b300_desired_channel_frequency_expert>(_expert_container,
            _expert_container->node_retriever(),
            fe_path,
            _master_clock_rate);
        uhd::experts::expert_factory::add_worker_node<
            uhd::usrp::b300::b300_rflo_channel_frequency_expert>(_expert_container,
            _expert_container->node_retriever(),
            fe_path,
            _master_clock_rate);
        uhd::experts::expert_factory::add_worker_node<
            uhd::usrp::b300::b300_nco_programming_expert>(_expert_container,
            _expert_container->node_retriever(),
            fe_path,
            TX_DIRECTION,
            [this, chan_idx](const double freq) {
                std::lock_guard<std::mutex> lock(_cmd_time_mutex);
                auto coerced_freq = _adrv9032_manager->set_lo_freq(
                    TX_DIRECTION, freq, "NCO", chan_idx, _get_timed_command_enabled());
                if (_get_timed_command_enabled()) {
                    regs().sleep(uhd::usrp::ADRV9032_CMD_SLEEP_TIME);
                }
                return coerced_freq;
            });
        uhd::experts::expert_factory::add_worker_node<
            uhd::usrp::b300::b300_coerced_channel_frequency_expert>(
            _expert_container, _expert_container->node_retriever(), fe_path);
    }
    uhd::experts::expert_factory::add_worker_node<
        uhd::usrp::b300::b300_rflo_programming_expert>(_expert_container,
        _expert_container->node_retriever(),
        "LO0",
        [this](const uhd::direction_t dir, const size_t chan, const double freq) {
            std::lock_guard<std::mutex> lock(_cmd_time_mutex);
            auto coerced_freq = _adrv9032_manager->set_lo_freq(
                dir, freq, "RFLO", chan, _get_timed_command_enabled());
            if (_get_timed_command_enabled()) {
                regs().sleep(uhd::usrp::ADRV9032_CMD_SLEEP_TIME);
            }
            return coerced_freq;
        });
    uhd::experts::expert_factory::add_worker_node<
        uhd::usrp::b300::b300_rflo_programming_expert>(_expert_container,
        _expert_container->node_retriever(),
        "LO1",
        [this](const uhd::direction_t dir, const size_t chan, const double freq) {
            std::lock_guard<std::mutex> lock(_cmd_time_mutex);
            auto coerced_freq = _adrv9032_manager->set_lo_freq(
                dir, freq, "RFLO", chan, _get_timed_command_enabled());
            if (_get_timed_command_enabled()) {
                regs().sleep(uhd::usrp::ADRV9032_CMD_SLEEP_TIME);
            }
            return coerced_freq;
        });
}

void b300_radio_control_impl::_init_prop_tree()
{
    get_tree()
        ->subtree(DB_PATH)
        ->create<std::string>("fw_version")
        .set(_adrv9032_manager->get_firmware_version());
    for (size_t chan_idx = 0; chan_idx < get_num_output_ports(); ++chan_idx) {
        _init_rx_frontend_subtree(get_tree()->subtree(DB_PATH), chan_idx);
    }
    for (size_t chan_idx = 0; chan_idx < get_num_input_ports(); ++chan_idx) {
        _init_tx_frontend_subtree(get_tree()->subtree(DB_PATH), chan_idx);
    }
    const auto lo0_freq = _adrv9032_manager->get_lo_freq(RX_DIRECTION, "RFLO", 0);
    const auto lo1_freq = _adrv9032_manager->get_lo_freq(TX_DIRECTION, "RFLO", 0);
    uhd::experts::expert_factory::add_data_node<double>(
        _expert_container, "LO0_freq", lo0_freq, uhd::experts::AUTO_RESOLVE_ON_WRITE);
    uhd::experts::expert_factory::add_data_node<double>(
        _expert_container, "LO1_freq", lo1_freq, uhd::experts::AUTO_RESOLVE_ON_WRITE);
}

void b300_radio_control_impl::_init_rx_frontend_subtree(
    uhd::property_tree::sptr subtree, const size_t chan_idx)
{
    const fs_path fe_path = fs_path("rx_frontends") / chan_idx;
    subtree->create<std::string>(fe_path / "name")
        .set(get_fe_name(chan_idx, RX_DIRECTION));
    subtree->create<std::string>(fe_path / "connection").set("IQ");
    // Antenna
    subtree->create<std::string>(fe_path / "antenna" / "value")
        .add_coerced_subscriber([this, chan_idx](const std::string& ant) {
            this->set_rx_antenna(ant, chan_idx);
        })
        .set_publisher([this, chan_idx]() { return _rx_ant; })
        .set("RX1");
    subtree->create<std::vector<std::string>>(fe_path / "antenna" / "options")
        .set_publisher([]() {
            return std::vector<std::string>{"TX/RX0", "RX1"};
        })
        .add_coerced_subscriber([](const std::vector<std::string>&) {
            throw uhd::runtime_error("Attempting to update antenna options!");
        });
    // Frequency
    uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
        subtree,
        fe_path / "freq" / "value",
        fe_path / "freq" / "desired",
        fe_path / "freq" / "coerced",
        _adrv9032_manager->get_frequency(RX_DIRECTION, chan_idx),
        uhd::experts::AUTO_RESOLVE_ON_WRITE);
    subtree->create<freq_range_t>(fe_path / "freq" / "range")
        .set_publisher(
            [this, chan_idx]() { return this->get_rx_frequency_range(chan_idx); });
    for (const auto& lo_name : get_rx_lo_names(chan_idx)) {
        uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
            subtree,
            fe_path / lo_name / "freq" / "value",
            fe_path / lo_name / "freq" / "desired",
            fe_path / lo_name / "freq" / "coerced",
            _adrv9032_manager->get_lo_freq(RX_DIRECTION, lo_name, chan_idx),
            uhd::experts::AUTO_RESOLVE_ON_WRITE);
        subtree->create<freq_range_t>(fe_path / lo_name / "freq" / "range")
            .set_publisher([this, chan_idx, lo_name]() {
                return this->get_rx_lo_freq_range(lo_name, chan_idx);
            });
    }
    uhd::experts::expert_factory::add_prop_node<std::string>(_expert_container,
        subtree,
        fe_path / "RFLO" / "source",
        fe_path / "RFLO" / "source",
        _adrv9032_manager->get_lo_source(RX_DIRECTION, "RFLO", chan_idx),
        uhd::experts::AUTO_RESOLVE_ON_WRITE)
        .set_coercer([this, chan_idx](const std::string& src) {
            _adrv9032_manager->set_lo_source(RX_DIRECTION, src, "RFLO", chan_idx);
            return src;
        });
    // Gain
    subtree->create<double>(fe_path / "gains" / "all" / "value")
        .set_coercer([this, chan_idx](const double gain) {
            std::lock_guard<std::mutex> lock(_cmd_time_mutex);
            double coerced_gain = b300_rx_gain_range.clip(gain);
            _adrv9032_manager->set_rx_gain(coerced_gain, chan_idx);
            return coerced_gain;
        })
        .set(0.0);
    subtree->create<meta_range_t>(fe_path / "gains" / "all" / "range")
        .set(b300_rx_gain_range);
    // Bandwidth - fixed, so set range to a single value
    auto rx_bandwidth = get_rx_bandwidth(chan_idx);
    subtree->create<meta_range_t>(fe_path / "bandwidth" / "range")
        .set(meta_range_t(rx_bandwidth, rx_bandwidth));
    subtree->create<double>(fe_path / "bandwidth" / "value")
        .set_coercer([this, rx_bandwidth](const double) { return rx_bandwidth; })
        .set_publisher([this, chan_idx]() { return this->get_rx_bandwidth(chan_idx); })
        .set(rx_bandwidth);

    for (const std::string& sensor_name : get_rx_sensor_names(chan_idx)) {
        subtree->create<sensor_value_t>(fe_path / "sensors" / sensor_name)
            .set_publisher([this, sensor_name, chan_idx]() {
                return get_rx_sensor(sensor_name, chan_idx);
            });
    }
}

void b300_radio_control_impl::_init_tx_frontend_subtree(
    uhd::property_tree::sptr subtree, const size_t chan_idx)
{
    const fs_path fe_path = fs_path("tx_frontends") / chan_idx;
    subtree->create<std::string>(fe_path / "name")
        .set(get_fe_name(chan_idx, TX_DIRECTION));
    subtree->create<std::string>(fe_path / "connection").set("IQ");
    // Antenna
    subtree->create<std::string>(fe_path / "antenna" / "value")
        .add_coerced_subscriber([this, chan_idx](const std::string& ant) {
            this->set_tx_antenna(ant, chan_idx);
        })
        .set_publisher([]() { return "TX/RX0"; });
    subtree->create<std::vector<std::string>>(fe_path / "antenna" / "options")
        .set_publisher([]() { return std::vector<std::string>{"TX/RX0"}; })
        .add_coerced_subscriber([](const std::vector<std::string>&) {
            throw uhd::runtime_error("Attempting to update antenna options!");
        });
    // Frequency
    uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
        subtree,
        fe_path / "freq" / "value",
        fe_path / "freq" / "desired",
        fe_path / "freq" / "coerced",
        _adrv9032_manager->get_frequency(TX_DIRECTION, chan_idx),
        uhd::experts::AUTO_RESOLVE_ON_WRITE);
    subtree->create<freq_range_t>(fe_path / "freq" / "range")
        .set_publisher(
            [this, chan_idx]() { return this->get_tx_frequency_range(chan_idx); });
    for (const auto& lo_name : get_tx_lo_names(chan_idx)) {
        uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
            subtree,
            fe_path / lo_name / "freq" / "value",
            fe_path / lo_name / "freq" / "desired",
            fe_path / lo_name / "freq" / "coerced",
            _adrv9032_manager->get_lo_freq(TX_DIRECTION, lo_name, chan_idx),
            uhd::experts::AUTO_RESOLVE_ON_WRITE);
        subtree->create<freq_range_t>(fe_path / lo_name / "freq" / "range")
            .set_publisher([this, chan_idx, lo_name]() {
                return this->get_tx_lo_freq_range(lo_name, chan_idx);
            });
    }
    uhd::experts::expert_factory::add_prop_node<std::string>(_expert_container,
        subtree,
        fe_path / "RFLO" / "source",
        fe_path / "RFLO" / "source",
        _adrv9032_manager->get_lo_source(TX_DIRECTION, "RFLO", chan_idx),
        uhd::experts::AUTO_RESOLVE_ON_WRITE)
        .set_coercer([this, chan_idx](const std::string& src) {
            _adrv9032_manager->set_lo_source(TX_DIRECTION, src, "RFLO", chan_idx);
            return src;
        });
    // Gain
    subtree->create<double>(fe_path / "gains" / "all" / "value")
        .set_coercer([this, chan_idx](const double gain) {
            std::lock_guard<std::mutex> lock(_cmd_time_mutex);
            double coerced_gain = b300_tx_gain_range.clip(gain);
            _adrv9032_manager->set_tx_gain(coerced_gain, chan_idx);
            return coerced_gain;
        })
        .set(0.0);
    subtree->create<meta_range_t>(fe_path / "gains" / "all" / "range")
        .set(b300_tx_gain_range);
    // Bandwidth - fixed, so set range to a single value
    auto tx_bandwidth = get_tx_bandwidth(chan_idx);
    subtree->create<meta_range_t>(fe_path / "bandwidth" / "range")
        .set(meta_range_t(tx_bandwidth, tx_bandwidth));
    subtree->create<double>(fe_path / "bandwidth" / "value")
        .set_coercer([this, tx_bandwidth](const double) { return tx_bandwidth; })
        .set_publisher([this, chan_idx]() { return this->get_tx_bandwidth(chan_idx); })
        .set(tx_bandwidth);

    for (const std::string& sensor_name : get_tx_sensor_names(chan_idx)) {
        subtree->create<sensor_value_t>(fe_path / "sensors" / sensor_name)
            .set_publisher([this, sensor_name, chan_idx]() {
                return get_tx_sensor(sensor_name, chan_idx);
            });
    }
}

void b300_radio_control_impl::_init_power_cal_managers()
{
    const auto eeprom = _mb_control->get_eeprom();
    if (!eeprom.has_key("serial")) {
        RFNOC_LOG_WARNING(
            "EEPROM does not contain a serial number, cannot initialize power "
            "calibration manager");
        return;
    }
    const std::string module_serial = eeprom["serial"];

    const size_t num_tx_ports = get_num_input_ports();
    const size_t num_rx_ports = get_num_output_ports();
    _tx_pwr_mgr.resize(num_tx_ports);
    _rx_pwr_mgr.resize(num_rx_ports);

    for (size_t chan = 0; chan < num_tx_ports; chan++) {
        // Create a gain group for this.
        auto gain_group = uhd::gain_group::make();
        gain_group->register_fcns("hw",
            {[this, chan]() { return get_tx_gain_range(chan); },
                [this, chan]() { return get_tx_gain(chan); },
                [this, chan](const double gain) { set_tx_gain(gain, chan); }});
        _tx_pwr_mgr.at(chan) = uhd::usrp::pwr_cal_mgr::make(
            module_serial + "_" + get_fe_name(chan, TX_DIRECTION),
            "B3XX-CAL-TX",
            [this, chan]() { return get_tx_frequency(chan); },
            [this, chan]() -> std::string {
                const std::string ant = get_tx_antenna(chan);
                return "b3xx_pwr_ch" + std::to_string(chan) + "_tx_"
                       + uhd::usrp::pwr_cal_mgr::sanitize_antenna_name(ant);
            },
            gain_group);
        // Every time we retune, we need to re-set the power level if we're in power
        // tracking mode.
        get_tree()
            ->access<double>(DB_PATH / "tx_frontends" / chan / "freq" / "value")
            .add_coerced_subscriber(
                [this, chan](const double) { _tx_pwr_mgr.at(chan)->update_power(); });
    }

    for (size_t chan = 0; chan < num_rx_ports; chan++) {
        // Create a gain group for this.
        auto gain_group = uhd::gain_group::make();
        gain_group->register_fcns("hw",
            {[this, chan]() { return get_rx_gain_range(chan); },
                [this, chan]() { return get_rx_gain(chan); },
                [this, chan](const double gain) { set_rx_gain(gain, chan); }});
        _rx_pwr_mgr.at(chan) = uhd::usrp::pwr_cal_mgr::make(
            module_serial + "_" + get_fe_name(chan, RX_DIRECTION),
            "B3XX-CAL-RX",
            [this, chan]() { return get_rx_frequency(chan); },
            [this, chan]() -> std::string {
                const std::string ant = get_rx_antenna(chan);
                return "b3xx_pwr_ch" + std::to_string(chan) + "_rx_"
                       + uhd::usrp::pwr_cal_mgr::sanitize_antenna_name(ant);
            },
            gain_group);
        // Every time we retune, we need to re-set the power level if we're in power
        // tracking mode
        get_tree()
            ->access<double>(DB_PATH / "rx_frontends" / chan / "freq" / "value")
            .add_coerced_subscriber(
                [this, chan](const double) { _rx_pwr_mgr.at(chan)->update_power(); });
    }
}

bool b300_radio_control_impl::_get_timed_command_enabled() const
{
    return get_command_time(0) != time_spec_t::ASAP;
}

/**************************************************************************
 * GPIO Controls
 *************************************************************************/
std::vector<std::string> b300_radio_control_impl::get_gpio_banks() const
{
    return {B300_GPIO_SRC_BANK};
}

uint32_t b300_radio_control_impl::_build_rf1_gpio_mask()
{
    auto srcs = _mb_control->get_gpio_src(B300_GPIO_SRC_BANK);
    // Create bitmask for RF1 sources
    uint32_t rf1_mask = 0;
    for (size_t i = 0; i < srcs.size(); ++i) {
        if (srcs[i] == B300_GPIO_SRC_RF1) {
            rf1_mask |= (1U << i);
        }
    }
    return rf1_mask;
}

void b300_radio_control_impl::set_gpio_attr(
    const std::string& bank, const std::string& attr, const uint32_t value)
{
    if (bank == B300_GPIO_SRC_BANK) {
        uint32_t rf1_mask = _build_rf1_gpio_mask();
        _fp_gpio[0]->set_gpio_attr(
            usrp::gpio_atr::gpio_attr_rev_map.at(attr), value & ~rf1_mask);
        _fp_gpio[1]->set_gpio_attr(
            usrp::gpio_atr::gpio_attr_rev_map.at(attr), value & rf1_mask);
    } else {
        throw uhd::key_error("Invalid GPIO bank: " + bank);
    }
}

uint32_t b300_radio_control_impl::get_gpio_attr(
    const std::string& bank, const std::string& attr)
{
    if (bank == B300_GPIO_SRC_BANK) {
        uint32_t rf1_mask = _build_rf1_gpio_mask();
        uint32_t rf0_vals =
            _fp_gpio[0]->get_attr_reg(usrp::gpio_atr::gpio_attr_rev_map.at(attr));
        uint32_t rf1_vals =
            _fp_gpio[1]->get_attr_reg(usrp::gpio_atr::gpio_attr_rev_map.at(attr));
        return (rf0_vals & ~rf1_mask) | (rf1_vals & rf1_mask);
    } else {
        throw uhd::key_error("Invalid GPIO bank: " + bank);
    }
}

/**************************************************************************
 * Command Time API
 *************************************************************************/
void b300_radio_control_impl::set_command_time(
    uhd::time_spec_t time, const size_t instance)
{
    std::lock_guard<std::mutex> lock(_cmd_time_mutex);
    radio_control_impl::set_command_time(time, instance);
}

void b300_radio_control_impl::_jesd_init()
{
    _jesd_core->init();
    bool jesd_initialized = false;
    std::string jesd_error_string;
    // We have seen this fail as many as 12 times before passing, give some cushion to
    // that.
    const size_t max_attempts = 25;
    for (size_t i = 0; i < max_attempts; ++i) {
        try {
            _adrv9032_manager->jesd_deframer_sysref_request_enable(
                ADI_ADRV903X_DEFRAMER_0, false);
            _adrv9032_manager->jesd_framer_sysref_request_enable(
                ADI_ADRV903X_FRAMER_0, false);
            // Currently LMK04832 is configured for continuous SYSREF
            _adrv9032_manager->jesd_deframer_link_state_enable(
                ADI_ADRV903X_DEFRAMER_0, false);
            _adrv9032_manager->jesd_framer_link_state_enable(
                ADI_ADRV903X_FRAMER_0, false);
            _adrv9032_manager->reset_jesd_serializer();
            // Setup Framers in the FPGA
            _jesd_core->init_framer(true);
            _adrv9032_manager->jesd_run_deframer_init_cals(ADI_ADRV903X_DEFRAMER_0);
            _adrv9032_manager->jesd_deframer_link_state_enable(
                ADI_ADRV903X_DEFRAMER_0, true);
            _adrv9032_manager->jesd_deframer_sysref_request_enable(
                ADI_ADRV903X_DEFRAMER_0, true);
            // Send SysRef Pulses (Currently continuous, so noop)
            _adrv9032_manager->jesd_deframer_sysref_request_enable(
                ADI_ADRV903X_DEFRAMER_0, false);
            _adrv9032_manager->jesd_framer_link_state_enable(ADI_ADRV903X_FRAMER_0, true);
            _adrv9032_manager->jesd_framer_sysref_request_enable(
                ADI_ADRV903X_FRAMER_0, true);
            _jesd_core->enable_lmfc(true);
            // Send SysRef Pulses (Currently continuous, so noop)
            _adrv9032_manager->jesd_framer_sysref_request_enable(
                ADI_ADRV903X_FRAMER_0, false);
            _jesd_core->init_deframer(false);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (!_jesd_core->get_framer_status()) {
                throw uhd::runtime_error("JESD204b FPGA Core Framer is not synced!");
            }

            UHD_LOG_DEBUG("B300",
                "SYNCB ADRV9032 Status Check: "
                    << static_cast<int>(_adrv9032_manager->jesd_get_framer_sync_status(
                           ADI_ADRV903X_FRAMER_0))
                    << "  , With SYNCB Mode: "
                    << static_cast<int>(_adrv9032_manager->jesd_get_framer_sync_mode(
                           ADI_ADRV903X_FRAMER_0)));

            if (!_jesd_core->get_deframer_status()) {
                // Set SYNCB to 1 (SPI_MODE) to help with debugging
                _adrv9032_manager->jesd_set_framer_sync_mode(ADI_ADRV903X_FRAMER_0, 1);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                // Set SYNCb mode back to Normal, to allow ILA to complete.
                _adrv9032_manager->jesd_set_framer_sync_mode(ADI_ADRV903X_FRAMER_0, 0);
                throw uhd::runtime_error("JESD204b FPGA Core Deframer is not synced!");
            }

            if (!_adrv9032_manager->jesd_get_framer_status(ADI_ADRV903X_FRAMER_0)) {
                throw uhd::runtime_error("JESD204b ADRV9032 Framer error!");
            }
            if (!_adrv9032_manager->jesd_get_deframer_status(ADI_ADRV903X_DEFRAMER_0)) {
                throw uhd::runtime_error("JESD204b ADRV9032 Deframer Lane not up!");
            }
            jesd_initialized = true;
            break;
        } catch (const uhd::runtime_error& e) {
            jesd_error_string = e.what();
            UHD_LOG_DEBUG("B300",
                "JESD204b Initialization attempt " << (i + 1)
                                                   << " failed: " << jesd_error_string);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    if (!jesd_initialized) {
        throw uhd::runtime_error("JESD204b Initialization failed after "
                                 + std::to_string(max_attempts)
                                 + " attempts: " + jesd_error_string);
    }

    _adrv9032_manager->jesd_deframer_error_clear(ADI_ADRV903X_DEFRAMER_0);

    // Link Initialization and Debugging on Dev Guide Page 81 (if needed)
}

UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(
    b300_radio_control, RADIO_BLOCK, B310, "Radio", true, "radio_clk", "radio_clk")
}} // namespace uhd::rfnoc
