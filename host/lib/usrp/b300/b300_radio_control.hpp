//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "b300_clock_ctrl.hpp"
#include "b300_jesd_core.hpp"
#include "b300_mb_controller.hpp"
#include <uhd/experts/expert_factory.hpp>
#include <uhd/experts/expert_nodes.hpp>
#include <uhdlib/rfnoc/radio_control_impl.hpp>
#include <uhdlib/usrp/common/adrv9032_manager.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <uhdlib/usrp/cores/spi_core_adrv.hpp>

namespace uhd { namespace rfnoc {

static const gain_range_t b300_tx_gain_range(0.0, 41.5, 0.05);
static const gain_range_t b300_rx_gain_range(0.0, 36.0, 0.5);

namespace b300_regs {

static constexpr uint32_t PERIPH_BASE = 0x80000;
constexpr uint32_t RADIO_ADDR(uint32_t x)
{
    return PERIPH_BASE + x;
}

static constexpr uint32_t MISC_OUTPUTS = RADIO_ADDR(0x0000);
static constexpr uint32_t MISC_INPUTS  = RADIO_ADDR(0x0004);

static constexpr uint32_t SR_ADRV_SPI = RADIO_ADDR(0x0100);

static constexpr uint32_t ATR_CHAN_REG_OFFSET = 0x10;
static constexpr uint32_t ATR_READ_OFFSET     = 0x8;
static constexpr uint32_t ATR_LEDS            = RADIO_ADDR(0x0010);
static constexpr uint32_t ATR_FP_GPIO         = RADIO_ADDR(0x0030);
static constexpr uint32_t ATR_PALMA           = RADIO_ADDR(0x0050);
static constexpr uint32_t ATR_RF_PATH         = RADIO_ADDR(0x0070);

static constexpr uint32_t SYSREF = RADIO_ADDR(0x1078);

static constexpr int SR_TXRX_GREEN = (1 << 0);
static constexpr int SR_TXRX_RED   = (1 << 1);
static constexpr int SR_RX_GREEN   = (1 << 4);

// Bitfields for the RF path ATR register
static constexpr int ENABLE_TXRX_TDR = (1 << 0);
static constexpr int BYPASS_RX       = (1 << 1);
// Settings for each bitfield in case of Tx or Rx to Tx/Rx0 port
static constexpr int TRX_PATH_RX = ENABLE_TXRX_TDR;
static constexpr int TRX_PATH_TX = BYPASS_RX;

constexpr size_t JESD_NUM_QPLLS        = 1;
constexpr size_t JESD_NUM_CPLLS        = 2;
constexpr uint8_t JESD_LMFC_DIVIDER    = 32;
constexpr uint8_t JESD_RX_SYSREF_DELAY = 10;
constexpr uint8_t JESD_TX_SYSREF_DELAY = 10;
} // namespace b300_regs

class b300_radio_regmap_t : public uhd::soft_regmap_t
{
public:
    typedef std::shared_ptr<b300_radio_regmap_t> sptr;
    class misc_outs_reg_t : public uhd::soft_reg32_rw_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(ADRV9032_RESET, /*width*/ 1, /*shift*/ 0); //[0]
        UHD_DEFINE_SOFT_REG_FIELD(ADRV9032_TEST_EN, /*width*/ 1, /*shift*/ 1); //[1]

        misc_outs_reg_t() : uhd::soft_reg32_rw_t(b300_regs::MISC_OUTPUTS)
        {
            // Initial values
            set(ADRV9032_RESET, 0);
            set(ADRV9032_TEST_EN, 0);
        }
    } misc_outs_reg;

    class misc_ins_reg_t : public uhd::soft_reg32_ro_t
    {
    public:
        misc_ins_reg_t() : uhd::soft_reg32_ro_t(b300_regs::MISC_INPUTS) {}
    } misc_ins_reg;

    b300_radio_regmap_t() : soft_regmap_t("radio_regmap")
    {
        add_to_map(misc_outs_reg, "misc_outs_reg", PRIVATE);
        add_to_map(misc_ins_reg, "misc_ins_reg", PRIVATE);
    }
};

class b300_radio_control_impl : public radio_control_impl
{
public:
    using sptr = std::shared_ptr<b300_radio_control_impl>;

    /************************************************************************
     * Structors
     ***********************************************************************/
    b300_radio_control_impl(make_args_ptr make_args);
    virtual ~b300_radio_control_impl() = default;

    std::string get_slot_name() const override
    {
        // B300 only has one radio control
        return "A";
    }

    size_t get_chan_from_dboard_fe(const std::string&, uhd::direction_t) const override;
    std::string get_dboard_fe_from_chan(size_t chan, uhd::direction_t) const override;

    /**************************************************************************
     * RF-specific API calls
     *************************************************************************/
    // Setters
    double set_rate(const double rate) override;
    void set_tx_antenna(const std::string& ant, const size_t chan) override;
    void set_rx_antenna(const std::string& ant, const size_t chan) override;
    double set_tx_frequency(const double freq, const size_t chan) override;
    double set_rx_frequency(const double freq, const size_t chan) override;
    double set_tx_gain(const double gain, const size_t chan) override;
    double set_rx_gain(const double gain, const size_t chan) override;

    // Getters
    std::string get_tx_antenna(const size_t chan) const override;
    std::string get_rx_antenna(const size_t chan) const override;
    std::vector<std::string> get_tx_antennas(const size_t chan) const override;
    std::vector<std::string> get_rx_antennas(const size_t chan) const override;
    double get_tx_frequency(const size_t) override;
    double get_rx_frequency(const size_t) override;
    uhd::freq_range_t get_tx_frequency_range(const size_t chan) const override;
    uhd::freq_range_t get_rx_frequency_range(const size_t chan) const override;
    double get_tx_gain(const size_t chan) override;
    double get_rx_gain(const size_t chan) override;
    uhd::gain_range_t get_tx_gain_range(const size_t) const override;
    uhd::gain_range_t get_tx_gain_range(const std::string&, const size_t) const override;
    uhd::gain_range_t get_rx_gain_range(const size_t) const override;
    uhd::gain_range_t get_rx_gain_range(const std::string&, const size_t) const override;
    meta_range_t get_tx_bandwidth_range(const size_t chan) const override;
    double get_tx_bandwidth(const size_t chan) override;
    meta_range_t get_rx_bandwidth_range(const size_t chan) const override;
    double get_rx_bandwidth(const size_t chan) override;

    /**************************************************************************
     * LO Controls
     *************************************************************************/
    std::vector<std::string> get_rx_lo_names(const size_t chan) const override;
    std::vector<std::string> get_rx_lo_sources(
        const std::string& name, const size_t chan) const override;
    freq_range_t get_rx_lo_freq_range(
        const std::string& name, const size_t chan) const override;
    void set_rx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) override;
    std::string get_rx_lo_source(const std::string& name, const size_t chan) override;
    double set_rx_lo_freq(
        double freq, const std::string& name, const size_t chan) override;
    double get_rx_lo_freq(const std::string& name, const size_t chan) override;
    std::vector<std::string> get_tx_lo_names(const size_t chan) const override;
    std::vector<std::string> get_tx_lo_sources(
        const std::string& name, const size_t chan) const override;
    freq_range_t get_tx_lo_freq_range(
        const std::string& name, const size_t chan) override;
    void set_tx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) override;
    std::string get_tx_lo_source(const std::string& name, const size_t chan) override;
    double set_tx_lo_freq(
        const double freq, const std::string& name, const size_t chan) override;
    double get_tx_lo_freq(const std::string& name, const size_t chan) override;

    /**************************************************************************
     * Sensor API
     *************************************************************************/
    std::vector<std::string> get_rx_sensor_names(const size_t chan) const override;
    uhd::sensor_value_t get_rx_sensor(
        const std::string& name, const size_t chan) override;
    std::vector<std::string> get_tx_sensor_names(const size_t chan) const override;
    uhd::sensor_value_t get_tx_sensor(
        const std::string& name, const size_t chan) override;

    /**************************************************************************
     * GPIO Controls
     *************************************************************************/
    std::vector<std::string> get_gpio_banks() const override;
    void set_gpio_attr(
        const std::string& bank, const std::string& attr, const uint32_t value) override;
    uint32_t get_gpio_attr(const std::string& bank, const std::string& attr) override;

    /**************************************************************************
     * Command Time API
     *************************************************************************/
    void set_command_time(uhd::time_spec_t time, const size_t instance) override;

private:
    void _init_experts();
    void _init_prop_tree();
    void _init_rx_frontend_subtree(
        uhd::property_tree::sptr subtree, const size_t chan_idx);
    void _init_tx_frontend_subtree(
        uhd::property_tree::sptr subtree, const size_t chan_idx);
    void _init_power_cal_managers();
    void _jesd_init();
    uint32_t _build_rf1_gpio_mask();
    bool _get_timed_command_enabled() const;

    std::vector<uhd::timed_wb_iface::sptr> _wb_ifaces;
    std::shared_ptr<b300_radio_regmap_t> _regs;
    uhd::usrp::adrv9032_manager::sptr _adrv9032_manager;
    b300_mb_controller::sptr _mb_control;
    uhd::usrp::b300::b300_clock_ctrl::sptr _clock_ctrl;
    uhd::usrp::b300::b300_jesd_core::sptr _jesd_core;
    spi_core_adrv::sptr _spi;
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _leds;
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _adrv9032_gpio;
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _rf_path_atr;
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _fp_gpio;
    std::string _rx_ant;
    std::mutex _cmd_time_mutex;
    uhd::experts::expert_container::sptr _expert_container;
    double _master_clock_rate;
};
}} // namespace uhd::rfnoc
