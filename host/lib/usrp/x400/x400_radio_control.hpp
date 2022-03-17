//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "adc_self_calibration.hpp"
#include "x400_gpio_control.hpp"
#include <uhd/features/spi_getter_iface.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/ranges.hpp>
#include <uhdlib/features/fpga_load_notification_iface.hpp>
#include <uhdlib/rfnoc/radio_control_impl.hpp>
#include <uhdlib/rfnoc/reg_iface_adapter.hpp>
#include <uhdlib/rfnoc/rf_control/dboard_iface.hpp>
#include <uhdlib/usrp/common/mpmd_mb_controller.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <uhdlib/usrp/cores/spi_core_4000.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <stddef.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>


namespace uhd { namespace rfnoc {

namespace x400_regs {

//! Base address for the rf_timing_control module. This controls the NCOs and
// other things in the RFDC.
constexpr uint32_t RFDC_CTRL_BASE = radio_control_impl::regmap::PERIPH_BASE + 0x8000;

constexpr uint32_t DIO_REGMAP_OFFSET = 0x2000;
constexpr uint32_t DIO_WINDOW_OFFSET = 0xC000;

// SPI control registers
constexpr uint32_t SPI_SLAVE_CFG =
    DIO_REGMAP_OFFSET + DIO_WINDOW_OFFSET + radio_control_impl::regmap::PERIPH_BASE;

//! Base address for SPI_TRANSACTION_CONFIG Register
constexpr uint32_t SPI_TRANSACTION_CFG_REG = SPI_SLAVE_CFG + 0x0010;

//! Base address for SPI_TRANSACTION_GO Register
constexpr uint32_t SPI_TRANSACTION_GO_REG = SPI_SLAVE_CFG + 0x0014;

//! Base address for SPI_STATUS Register
constexpr uint32_t SPI_STATUS_REG = SPI_SLAVE_CFG + 0x0018;

//! Base address for SPI_CONTROLLER_INFO Register
constexpr uint32_t SPI_CONTROLLER_INFO_REG = SPI_SLAVE_CFG + 0x001C;

} // namespace x400_regs

class x400_radio_control_impl : public radio_control_impl
{
public:
    using sptr = std::shared_ptr<x400_radio_control_impl>;

    /************************************************************************
     * Structors
     ***********************************************************************/
    x400_radio_control_impl(make_args_ptr make_args);
    virtual ~x400_radio_control_impl() = default;

    std::string get_slot_name() const override
    {
        return _radio_slot;
    }

    size_t get_chan_from_dboard_fe(const std::string&, uhd::direction_t) const override;
    std::string get_dboard_fe_from_chan(size_t chan, uhd::direction_t) const override;

    uhd::eeprom_map_t get_db_eeprom() override;

    // GPIO methods
    std::vector<std::string> get_gpio_banks() const override;
    uint32_t get_gpio_attr(const std::string& bank, const std::string& attr) override;
    void set_gpio_attr(
        const std::string& bank, const std::string& attr, const uint32_t value) override;

    // Shim calls for every method in rf_control_core
    double set_rate(const double rate) override;
    std::string get_tx_antenna(const size_t chan) const override;
    std::vector<std::string> get_tx_antennas(const size_t chan) const override;
    void set_tx_antenna(const std::string& ant, const size_t chan) override;
    std::string get_rx_antenna(const size_t chan) const override;
    std::vector<std::string> get_rx_antennas(const size_t chan) const override;
    void set_rx_antenna(const std::string& ant, const size_t chan) override;
    double get_tx_frequency(const size_t chan) override;
    double set_tx_frequency(const double freq, size_t chan) override;
    void set_tx_tune_args(const uhd::device_addr_t& args, const size_t chan) override;
    uhd::freq_range_t get_tx_frequency_range(const size_t chan) const override;
    double get_rx_frequency(const size_t chan) override;
    double set_rx_frequency(const double freq, const size_t chan) override;
    void set_rx_tune_args(const uhd::device_addr_t& args, const size_t chan) override;
    uhd::freq_range_t get_rx_frequency_range(const size_t chan) const override;
    std::vector<std::string> get_tx_gain_names(const size_t chan) const override;
    uhd::gain_range_t get_tx_gain_range(const size_t chan) const override;
    uhd::gain_range_t get_tx_gain_range(
        const std::string& name, const size_t chan) const override;
    double get_tx_gain(const size_t chan) override;
    double get_tx_gain(const std::string& name, const size_t chan) override;
    double set_tx_gain(const double gain, const size_t chan) override;
    double set_tx_gain(
        const double gain, const std::string& name, const size_t chan) override;
    std::vector<std::string> get_rx_gain_names(const size_t chan) const override;
    uhd::gain_range_t get_rx_gain_range(const size_t chan) const override;
    uhd::gain_range_t get_rx_gain_range(
        const std::string& name, const size_t chan) const override;
    double get_rx_gain(const size_t chan) override;
    double get_rx_gain(const std::string& name, const size_t chan) override;
    double set_rx_gain(const double gain, const size_t chan) override;
    double set_rx_gain(
        const double gain, const std::string& name, const size_t chan) override;
    void set_rx_agc(const bool enable, const size_t chan) override;
    meta_range_t get_tx_bandwidth_range(size_t chan) const override;
    double get_tx_bandwidth(const size_t chan) override;
    double set_tx_bandwidth(const double bandwidth, const size_t chan) override;
    meta_range_t get_rx_bandwidth_range(size_t chan) const override;
    double get_rx_bandwidth(const size_t chan) override;
    double set_rx_bandwidth(const double bandwidth, const size_t chan) override;

    std::vector<std::string> get_rx_lo_names(const size_t chan) const override;
    std::vector<std::string> get_rx_lo_sources(
        const std::string& name, const size_t chan) const override;
    freq_range_t get_rx_lo_freq_range(
        const std::string& name, const size_t chan) const override;
    void set_rx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) override;
    const std::string get_rx_lo_source(
        const std::string& name, const size_t chan) override;
    void set_rx_lo_export_enabled(
        bool enabled, const std::string& name, const size_t chan) override;
    bool get_rx_lo_export_enabled(const std::string& name, const size_t chan) override;
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
    const std::string get_tx_lo_source(
        const std::string& name, const size_t chan) override;
    void set_tx_lo_export_enabled(
        const bool enabled, const std::string& name, const size_t chan) override;
    bool get_tx_lo_export_enabled(const std::string& name, const size_t chan) override;
    double set_tx_lo_freq(
        const double freq, const std::string& name, const size_t chan) override;
    double get_tx_lo_freq(const std::string& name, const size_t chan) override;
    std::vector<std::string> get_rx_sensor_names(size_t chan) const override;
    uhd::sensor_value_t get_rx_sensor(const std::string& name, size_t chan) override;
    std::vector<std::string> get_tx_sensor_names(size_t chan) const override;
    uhd::sensor_value_t get_tx_sensor(const std::string& name, size_t chan) override;

    void set_command_time(uhd::time_spec_t time, const size_t chan) override;

    // Non-API methods
    // This is used for x4xx radio block unit tests
    std::vector<uhd::usrp::pwr_cal_mgr::sptr> get_pwr_mgr(const uhd::direction_t trx);

private:
    //! Locks access to the API
    mutable std::recursive_mutex _lock;

    std::string _get_trx_string(const direction_t dir) const;

    void _validate_master_clock_rate_args();
    void _init_mpm();
    void _init_mpm_sensors(const direction_t dir, const size_t chan_idx);
    void _init_prop_tree();
    fs_path _get_db_fe_path(const size_t chan, const uhd::direction_t dir) const;

    //! Reference to the MB controller
    uhd::rfnoc::mpmd_mb_controller::sptr _mb_control;

    //! Reference to the RPC client
    uhd::usrp::x400_rpc_iface::sptr _rpcc;
    uhd::usrp::dboard_base_rpc_iface::sptr _db_rpcc;

    //! Reference to the MB timekeeper
    uhd::rfnoc::mpmd_mb_controller::mpmd_timekeeper::sptr _x4xx_timekeeper;

    std::string _radio_slot;
    std::string _rpc_prefix;

    //! Reference to this radio block's RFDC control
    x400::rfdc_control::sptr _rfdcc;

    uhd::usrp::x400::x400_dboard_iface::sptr _daughterboard;

    uhd::features::adc_self_calibration_iface::sptr _adc_self_calibration;

    uhd::features::spi_getter_iface::sptr _spi_getter_iface;

    x400::gpio_control::sptr _gpios;

    class fpga_onload : public uhd::features::fpga_load_notification_iface
    {
    public:
        using sptr = std::shared_ptr<fpga_onload>;

        fpga_onload(size_t num_channels,
            uhd::features::adc_self_calibration_iface::sptr adc_self_cal,
            std::string unique_id);

        void onload() override;

    private:
        const size_t _num_channels;
        uhd::features::adc_self_calibration_iface::sptr _adc_self_cal;
        const std::string _unique_id;
        std::string get_unique_id() const
        {
            return _unique_id;
        }
    };

    fpga_onload::sptr _fpga_onload;

    class x400_spi_getter : public uhd::features::spi_getter_iface
    {
    public:
        using sptr = std::shared_ptr<spi_getter_iface>;

        x400_spi_getter(uhd::cores::spi_core_4000::sptr _spi) : _spicore(_spi) {}

        uhd::spi_iface::sptr get_spi_ref(
            const std::vector<uhd::features::spi_slave_config_t>& spi_slave_config)
            const override
        {
            _spicore->set_spi_slave_config(spi_slave_config);
            return _spicore;
        }

    private:
        uhd::cores::spi_core_4000::sptr _spicore;
    };
};


}} // namespace uhd::rfnoc
