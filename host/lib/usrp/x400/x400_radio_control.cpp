//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x400_radio_control.hpp"
#include "x400_gpio_control.hpp"
#include <uhd/rfnoc/registry.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/rfnoc/reg_iface_adapter.hpp>
#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <uhdlib/usrp/cores/spi_core_4000.hpp>
#include <uhdlib/usrp/dboard/debug_dboard.hpp>
#include <uhdlib/usrp/dboard/null_dboard.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_dboard.hpp>

namespace uhd { namespace rfnoc {

x400_radio_control_impl::fpga_onload::fpga_onload(size_t num_channels,
    uhd::features::adc_self_calibration_iface::sptr adc_self_cal,
    std::string unique_id)
    : _num_channels(num_channels), _adc_self_cal(adc_self_cal), _unique_id(unique_id)
{
}

void x400_radio_control_impl::fpga_onload::onload()
{
    for (size_t channel = 0; channel < _num_channels; channel++) {
        if (_adc_self_cal) {
            try {
                _adc_self_cal->run(channel);
            } catch (uhd::runtime_error& e) {
                RFNOC_LOG_WARNING("Failure while running self cal on channel "
                                  << channel << ": " << e.what());
            }
        }
    }
}

x400_radio_control_impl::x400_radio_control_impl(make_args_ptr make_args)
    : radio_control_impl(std::move(make_args))
{
    RFNOC_LOG_TRACE("Initializing x400_radio_control");

    UHD_ASSERT_THROW(get_block_id().get_block_count() < 2);
    constexpr char radio_slot_name[2] = {'A', 'B'};
    _radio_slot                       = radio_slot_name[get_block_id().get_block_count()];
    _rpc_prefix = get_block_id().get_block_count() == 1 ? "db_1_" : "db_0_";

    UHD_ASSERT_THROW(get_mb_controller());
    _mb_control = std::dynamic_pointer_cast<mpmd_mb_controller>(get_mb_controller());
    UHD_ASSERT_THROW(_mb_control)

    _x4xx_timekeeper = std::dynamic_pointer_cast<mpmd_mb_controller::mpmd_timekeeper>(
        _mb_control->get_timekeeper(0));
    UHD_ASSERT_THROW(_x4xx_timekeeper);

    _rpcc = _mb_control->dynamic_cast_rpc_as<uhd::usrp::x400_rpc_iface>();
    if (!_rpcc) {
        _rpcc = std::make_shared<uhd::usrp::x400_rpc>(_mb_control->get_rpc_client());
    }

    _db_rpcc = _mb_control->dynamic_cast_rpc_as<uhd::usrp::dboard_base_rpc_iface>();
    if (!_db_rpcc) {
        _db_rpcc = std::make_shared<uhd::usrp::dboard_base_rpc>(
            _mb_control->get_rpc_client(), _rpc_prefix);
    }

    const auto all_dboard_info = _rpcc->get_dboard_info();
    RFNOC_LOG_TRACE("Hardware detected " << all_dboard_info.size() << " daughterboards.");

    // If we have two radio blocks, but there is only one dboard plugged in,
    // we skip initialization. The board needs to be in slot A
    if (all_dboard_info.size() <= get_block_id().get_block_count()) {
        RFNOC_LOG_WARNING("The number of discovered daughterboards did not match the "
                          "number of radio blocks. Skipping front end initialization.");
        _daughterboard = std::make_shared<null_dboard_impl>();
        return;
    }

    const double master_clock_rate = _rpcc->get_master_clock_rate();
    set_tick_rate(master_clock_rate);
    _x4xx_timekeeper->update_tick_rate(master_clock_rate);
    radio_control_impl::set_rate(master_clock_rate);

    for (auto& samp_rate_prop : _samp_rate_in) {
        set_property(samp_rate_prop.get_id(), get_rate(), samp_rate_prop.get_src_info());
    }
    for (auto& samp_rate_prop : _samp_rate_out) {
        set_property(samp_rate_prop.get_id(), get_rate(), samp_rate_prop.get_src_info());
    }

    _validate_master_clock_rate_args();
    _init_mpm();

    RFNOC_LOG_TRACE("Initializing RFDC controls...");
    _rfdcc = std::make_shared<x400::rfdc_control>(
        // clang-format off
        uhd::memmap32_iface_timed{
            [this](const uint32_t addr, const uint32_t data, const uhd::time_spec_t& time_spec) {
                regs().poke32(addr + x400_regs::RFDC_CTRL_BASE, data, time_spec);
            },
            [this](const uint32_t addr) {
                return regs().peek32(addr + x400_regs::RFDC_CTRL_BASE);
            }
        },
        // clang-format on
        get_unique_id() + "::RFDC");

    const auto& dboard = all_dboard_info[get_block_id().get_block_count()];
    const std::string pid(dboard.at("pid").begin(), dboard.at("pid").end());
    RFNOC_LOG_TRACE("Initializing daughterboard driver for PID " << pid);

    // We may have physical daughterboards in the system, but no GPIO interface to the
    // daughterboard in the FPGA. In this case, just instantiate the null daughterboard.
    if (!_rpcc->is_db_gpio_ifc_present(get_block_id().get_block_count())) {
        RFNOC_LOG_WARNING(
            "Skipping daughterboard initialization, no GPIO interface in FPGA");
        _daughterboard = std::make_shared<null_dboard_impl>();
        return;
    }

    if (std::stol(pid) == uhd::usrp::zbx::ZBX_PID) {
        auto zbx_rpc_sptr = _mb_control->dynamic_cast_rpc_as<uhd::usrp::zbx_rpc_iface>();
        if (!zbx_rpc_sptr) {
            zbx_rpc_sptr = std::make_shared<uhd::usrp::zbx_rpc>(
                _mb_control->get_rpc_client(), _rpc_prefix);
        }
        _daughterboard = std::make_shared<uhd::usrp::zbx::zbx_dboard_impl>(
            regs(),
            regmap::PERIPH_BASE,
            [this](const size_t instance) { return get_command_time(instance); },
            get_block_id().get_block_count(),
            _radio_slot,
            _rpc_prefix,
            get_unique_id(),
            _rpcc,
            zbx_rpc_sptr,
            _rfdcc,
            get_tree());
    } else if (std::stol(pid) == uhd::rfnoc::DEBUG_DB_PID) {
        _daughterboard = std::make_shared<debug_dboard_impl>();
    } else if (std::stol(pid) == uhd::rfnoc::IF_TEST_DBOARD_PID) {
        _daughterboard =
            std::make_shared<if_test_dboard_impl>(get_block_id().get_block_count(),
                _rpc_prefix,
                get_unique_id(),
                _mb_control,
                get_tree());
    } else if (std::stol(pid) == uhd::rfnoc::EMPTY_DB_PID) {
        _daughterboard = std::make_shared<empty_slot_dboard_impl>();
        set_num_output_ports(0);
        set_num_input_ports(0);
    } else {
        RFNOC_LOG_WARNING("Skipping Daughterboard initialization for unsupported PID "
                          << "0x" << std::hex << std::stol(pid));
        _daughterboard = std::make_shared<null_dboard_impl>();
        return;
    }

    _init_prop_tree();

    _rx_pwr_mgr = _daughterboard->get_pwr_mgr(uhd::RX_DIRECTION);
    _tx_pwr_mgr = _daughterboard->get_pwr_mgr(uhd::TX_DIRECTION);

    _tx_gain_profile_api = _daughterboard->get_tx_gain_profile_api();
    _rx_gain_profile_api = _daughterboard->get_rx_gain_profile_api();

    if (_daughterboard->is_adc_self_cal_supported()) {
        _adc_self_calibration =
            std::make_shared<uhd::features::adc_self_calibration>(_rpcc,
                _rpc_prefix,
                get_unique_id(),
                get_block_id().get_block_count(),
                _daughterboard);
        register_feature(_adc_self_calibration);
    }

    _fpga_onload = std::make_shared<fpga_onload>(
        get_num_output_ports(), _adc_self_calibration, get_unique_id());
    register_feature(_fpga_onload);
    _mb_control->_fpga_onload->request_cb(_fpga_onload);

    auto mpm_rpc = _mb_control->dynamic_cast_rpc_as<uhd::usrp::mpmd_rpc_iface>();
    if (mpm_rpc->get_gpio_banks().size() > 0) {
        _gpios = std::make_shared<x400::gpio_control>(
            _rpcc, _mb_control, RFNOC_MAKE_WB_IFACE(regmap::PERIPH_BASE + 0xC000, 0));

        auto gpio_port_mapper = std::shared_ptr<uhd::mapper::gpio_port_mapper>(
            new uhd::rfnoc::x400::x400_gpio_port_mapping);

        // Check if SPI is available as GPIO source, otherwise don't register
        // SPI_GETTER_IFace
        auto gpio_srcs = _mb_control->get_gpio_srcs("GPIO0");
        if (std::count(gpio_srcs.begin(), gpio_srcs.end(), "DB0_SPI") > 0) {
            auto spicore = uhd::cores::spi_core_4000::make(
                [this](const uint32_t addr, const uint32_t data) {
                    regs().poke32(addr, data, get_command_time(0));
                },
                [this](const uint32_t addr) {
                    return regs().peek32(addr, get_command_time(0));
                },
                x400_regs::SPI_SLAVE_CFG,
                x400_regs::SPI_TRANSACTION_CFG_REG,
                x400_regs::SPI_TRANSACTION_GO_REG,
                x400_regs::SPI_STATUS_REG,
                x400_regs::SPI_CONTROLLER_INFO_REG,
                gpio_port_mapper);

            _spi_getter_iface = std::make_shared<x400_spi_getter>(spicore);
            register_feature(_spi_getter_iface);
        } else {
            UHD_LOG_INFO("x400_radio_control",
                "SPI functionality not available in this FPGA image. Please update to at "
                "least version 7.7 to use SPI.");
        }
    }
}

void x400_radio_control_impl::_init_prop_tree()
{
    auto subtree = get_tree()->subtree(fs_path("mboard"));

    for (size_t chan_idx = 0; chan_idx < get_num_output_ports(); chan_idx++) {
        const fs_path rx_codec_path =
            fs_path("rx_codec") / get_dboard_fe_from_chan(chan_idx, uhd::RX_DIRECTION);
        const fs_path tx_codec_path =
            fs_path("tx_codec") / get_dboard_fe_from_chan(chan_idx, uhd::TX_DIRECTION);
        RFNOC_LOG_TRACE("Adding non-RFNoC block properties for channel "
                        << chan_idx << " to prop tree paths " << rx_codec_path << " and "
                        << tx_codec_path);

        // ADC calibration state attributes
        subtree->create<bool>(rx_codec_path / "calibration_frozen")
            .add_coerced_subscriber([this, chan_idx](bool state) {
                _rpcc->set_cal_frozen(state, get_block_id().get_block_count(), chan_idx);
            })
            .set_publisher([this, chan_idx]() {
                const auto freeze_states =
                    _rpcc->get_cal_frozen(get_block_id().get_block_count(), chan_idx);
                return freeze_states.at(0) == 1;
            });

        // RFDC NCO
        // RX
        subtree->create<double>(rx_codec_path / "rfdc" / "freq/value")
            .add_desired_subscriber([this, chan_idx](double freq) {
                _rpcc->rfdc_set_nco_freq(_get_trx_string(RX_DIRECTION),
                    get_block_id().get_block_count(),
                    chan_idx,
                    freq);
            })
            .set_publisher([this, chan_idx]() {
                const auto nco_freq =
                    _rpcc->rfdc_get_nco_freq(_get_trx_string(RX_DIRECTION),
                        get_block_id().get_block_count(),
                        chan_idx);
                return nco_freq;
            });

        // TX
        subtree->create<double>(tx_codec_path / "rfdc" / "freq/value")
            .add_desired_subscriber([this, chan_idx](double freq) {
                _rpcc->rfdc_set_nco_freq(_get_trx_string(TX_DIRECTION),
                    get_block_id().get_block_count(),
                    chan_idx,
                    freq);
            })
            .set_publisher([this, chan_idx]() {
                const auto nco_freq =
                    _rpcc->rfdc_get_nco_freq(_get_trx_string(TX_DIRECTION),
                        get_block_id().get_block_count(),
                        chan_idx);
                return nco_freq;
            });
    }
}

void x400_radio_control_impl::_validate_master_clock_rate_args()
{
    auto block_args = get_block_args();

    // Note: MCR gets set during the init() call (prior to this), which takes
    // in arguments from the device args. So if block_args contains a
    // master_clock_rate key, then it should better be whatever the device is
    // configured to do.
    const double master_clock_rate = _rpcc->get_master_clock_rate();
    if (!uhd::math::frequencies_are_equal(get_rate(), master_clock_rate)) {
        throw uhd::runtime_error(
            str(boost::format("Master clock rate mismatch. Device returns %f MHz, "
                              "but should have been %f MHz.")
                % (master_clock_rate / 1e6) % (get_rate() / 1e6)));
    }
    RFNOC_LOG_DEBUG("Master Clock Rate is: " << (master_clock_rate / 1e6) << " MHz.");
}

void x400_radio_control_impl::_init_mpm()
{
    // Init sensors
    for (const auto& dir : std::vector<direction_t>{RX_DIRECTION, TX_DIRECTION}) {
        // TODO: We should pull the number of channels from _daughterboard
        for (size_t chan_idx = 0; chan_idx < uhd::usrp::zbx::ZBX_NUM_CHANS; chan_idx++) {
            _init_mpm_sensors(dir, chan_idx);
        }
    }
}

// @TODO: This should be a method on direction_t
// (or otherwise not duplicated from the implementation in zbx)
std::string x400_radio_control_impl::_get_trx_string(const direction_t dir) const
{
    if (dir == RX_DIRECTION) {
        return "rx";
    } else if (dir == TX_DIRECTION) {
        return "tx";
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}


void x400_radio_control_impl::_init_mpm_sensors(
    const direction_t dir, const size_t chan_idx)
{
    // TODO: We should pull the number of channels from _daughterboard
    UHD_ASSERT_THROW(chan_idx < uhd::usrp::zbx::ZBX_NUM_CHANS);
    const std::string trx = _get_trx_string(dir);
    const fs_path fe_path = fs_path("dboard")
                            / (dir == RX_DIRECTION ? "rx_frontends" : "tx_frontends")
                            / chan_idx;
    auto sensor_list = _db_rpcc->get_sensors(trx);
    RFNOC_LOG_TRACE("Chan " << chan_idx << ": Found " << sensor_list.size() << " " << trx
                            << " sensors.");
    for (const auto& sensor_name : sensor_list) {
        RFNOC_LOG_TRACE("Adding " << trx << " sensor " << sensor_name);
        get_tree()
            ->create<sensor_value_t>(fe_path / "sensors" / sensor_name)
            .add_coerced_subscriber([](const sensor_value_t&) {
                throw uhd::runtime_error("Attempting to write to sensor!");
            })
            .set_publisher([this, trx, sensor_name, chan_idx]() {
                return sensor_value_t(
                    this->_db_rpcc->get_sensor(trx, sensor_name, chan_idx));
            });
    }
}

fs_path x400_radio_control_impl::_get_db_fe_path(
    const size_t chan, const direction_t dir) const
{
    const std::string trx = _get_trx_string(dir);
    return DB_PATH / (trx + "_frontends") / get_dboard_fe_from_chan(chan, dir);
}


double x400_radio_control_impl::set_rate(const double rate)
{
    // X400 does not support runtime rate changes
    if (!uhd::math::frequencies_are_equal(rate, get_rate())) {
        RFNOC_LOG_WARNING("Requesting invalid sampling rate from device: "
                          << (rate / 1e6)
                          << " MHz. Actual rate is: " << (get_rate() / 1e6) << " MHz.");
    }
    return get_rate();
}

std::vector<std::string> x400_radio_control_impl::get_gpio_banks() const
{
    if (!_gpios) {
        return {};
    }
    return {x400::GPIO_BANK_NAME};
}

uint32_t x400_radio_control_impl::get_gpio_attr(
    const std::string& bank, const std::string& attr)
{
    if (!_gpios) {
        throw uhd::runtime_error("X410 does not have sufficient GPIO support!");
    }
    std::lock_guard<std::recursive_mutex> l(_lock);
    if (bank != x400::GPIO_BANK_NAME) {
        throw uhd::key_error("Invalid GPIO bank " + bank);
    }
    if (usrp::gpio_atr::gpio_attr_rev_map.count(attr) == 0) {
        throw uhd::key_error("Invalid GPIO attribute " + attr);
    }
    return _gpios->get_gpio_attr(usrp::gpio_atr::gpio_attr_rev_map.at(attr));
}

void x400_radio_control_impl::set_gpio_attr(
    const std::string& bank, const std::string& attr, const uint32_t value)
{
    if (!_gpios) {
        throw uhd::runtime_error("X410 does not have sufficient GPIO support!");
    }
    std::lock_guard<std::recursive_mutex> l(_lock);
    if (bank != x400::GPIO_BANK_NAME) {
        throw uhd::key_error("Invalid GPIO bank " + bank);
    }
    if (usrp::gpio_atr::gpio_attr_rev_map.count(attr) == 0) {
        throw uhd::key_error("Invalid GPIO attribute " + attr);
    }
    _gpios->set_gpio_attr(usrp::gpio_atr::gpio_attr_rev_map.at(attr), value);
}

eeprom_map_t x400_radio_control_impl::get_db_eeprom()
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_db_eeprom();
}

std::vector<uhd::usrp::pwr_cal_mgr::sptr> x400_radio_control_impl::get_pwr_mgr(
    const uhd::direction_t trx)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_pwr_mgr(trx);
}

std::string x400_radio_control_impl::get_tx_antenna(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_antenna(chan);
}

std::vector<std::string> x400_radio_control_impl::get_tx_antennas(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_antennas(chan);
}

void x400_radio_control_impl::set_tx_antenna(const std::string& ant, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    _daughterboard->set_tx_antenna(ant, chan);
}

std::string x400_radio_control_impl::get_rx_antenna(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_antenna(chan);
}

std::vector<std::string> x400_radio_control_impl::get_rx_antennas(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_antennas(chan);
}

void x400_radio_control_impl::set_rx_antenna(const std::string& ant, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    _daughterboard->set_rx_antenna(ant, chan);
}

double x400_radio_control_impl::get_tx_frequency(const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_frequency(chan);
}

double x400_radio_control_impl::set_tx_frequency(const double freq, size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_tx_frequency(freq, chan);
}

void x400_radio_control_impl::set_tx_tune_args(
    const uhd::device_addr_t& args, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    _daughterboard->set_tx_tune_args(args, chan);
}

uhd::freq_range_t x400_radio_control_impl::get_tx_frequency_range(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_frequency_range(chan);
}

double x400_radio_control_impl::get_rx_frequency(const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_frequency(chan);
}

double x400_radio_control_impl::set_rx_frequency(const double freq, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_rx_frequency(freq, chan);
}

void x400_radio_control_impl::set_rx_tune_args(
    const uhd::device_addr_t& args, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    _daughterboard->set_rx_tune_args(args, chan);
}

uhd::freq_range_t x400_radio_control_impl::get_rx_frequency_range(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_frequency_range(chan);
}

std::vector<std::string> x400_radio_control_impl::get_tx_gain_names(
    const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_gain_names(chan);
}

uhd::gain_range_t x400_radio_control_impl::get_tx_gain_range(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_gain_range(chan);
}

uhd::gain_range_t x400_radio_control_impl::get_tx_gain_range(
    const std::string& name, const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_gain_range(name, chan);
}

double x400_radio_control_impl::get_tx_gain(const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_gain(chan);
}

double x400_radio_control_impl::get_tx_gain(const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_gain(name, chan);
}

double x400_radio_control_impl::set_tx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_tx_gain(gain, chan);
}

double x400_radio_control_impl::set_tx_gain(
    const double gain, const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_tx_gain(gain, name, chan);
}

std::vector<std::string> x400_radio_control_impl::get_rx_gain_names(
    const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_gain_names(chan);
}

uhd::gain_range_t x400_radio_control_impl::get_rx_gain_range(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_gain_range(chan);
}

uhd::gain_range_t x400_radio_control_impl::get_rx_gain_range(
    const std::string& name, const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_gain_range(name, chan);
}

double x400_radio_control_impl::get_rx_gain(const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_gain(chan);
}

double x400_radio_control_impl::get_rx_gain(const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_gain(name, chan);
}

double x400_radio_control_impl::set_rx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_rx_gain(gain, chan);
}

double x400_radio_control_impl::set_rx_gain(
    const double gain, const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_rx_gain(gain, name, chan);
}

void x400_radio_control_impl::set_rx_agc(const bool enable, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    _daughterboard->set_rx_agc(enable, chan);
}

meta_range_t x400_radio_control_impl::get_tx_bandwidth_range(size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_bandwidth_range(chan);
}

double x400_radio_control_impl::get_tx_bandwidth(const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_bandwidth(chan);
}

double x400_radio_control_impl::set_tx_bandwidth(
    const double bandwidth, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_tx_bandwidth(bandwidth, chan);
}

meta_range_t x400_radio_control_impl::get_rx_bandwidth_range(size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_bandwidth_range(chan);
}

double x400_radio_control_impl::get_rx_bandwidth(const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_bandwidth(chan);
}

double x400_radio_control_impl::set_rx_bandwidth(
    const double bandwidth, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_rx_bandwidth(bandwidth, chan);
}

std::vector<std::string> x400_radio_control_impl::get_rx_lo_names(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_lo_names(chan);
}

std::vector<std::string> x400_radio_control_impl::get_rx_lo_sources(
    const std::string& name, const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_lo_sources(name, chan);
}

freq_range_t x400_radio_control_impl::get_rx_lo_freq_range(
    const std::string& name, const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_lo_freq_range(name, chan);
}

void x400_radio_control_impl::set_rx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    _daughterboard->set_rx_lo_source(src, name, chan);
}

const std::string x400_radio_control_impl::get_rx_lo_source(
    const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_lo_source(name, chan);
}

void x400_radio_control_impl::set_rx_lo_export_enabled(
    bool enabled, const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    _daughterboard->set_rx_lo_export_enabled(enabled, name, chan);
}

bool x400_radio_control_impl::get_rx_lo_export_enabled(
    const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_lo_export_enabled(name, chan);
}

double x400_radio_control_impl::set_rx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_rx_lo_freq(freq, name, chan);
}

double x400_radio_control_impl::get_rx_lo_freq(const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_rx_lo_freq(name, chan);
}

std::vector<std::string> x400_radio_control_impl::get_tx_lo_names(const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_lo_names(chan);
}

std::vector<std::string> x400_radio_control_impl::get_tx_lo_sources(
    const std::string& name, const size_t chan) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_lo_sources(name, chan);
}

freq_range_t x400_radio_control_impl::get_tx_lo_freq_range(
    const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_lo_freq_range(name, chan);
}

void x400_radio_control_impl::set_tx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    _daughterboard->set_tx_lo_source(src, name, chan);
}

const std::string x400_radio_control_impl::get_tx_lo_source(
    const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_lo_source(name, chan);
}

void x400_radio_control_impl::set_tx_lo_export_enabled(
    const bool enabled, const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    _daughterboard->set_tx_lo_export_enabled(enabled, name, chan);
}

bool x400_radio_control_impl::get_tx_lo_export_enabled(
    const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_lo_export_enabled(name, chan);
}

double x400_radio_control_impl::set_tx_lo_freq(
    const double freq, const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->set_tx_lo_freq(freq, name, chan);
}

double x400_radio_control_impl::get_tx_lo_freq(const std::string& name, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_tx_lo_freq(name, chan);
}

void x400_radio_control_impl::set_command_time(uhd::time_spec_t time, const size_t chan)
{
    node_t::set_command_time(time, chan);
    _daughterboard->set_command_time(time, chan);
}

/**************************************************************************
 * Sensor API
 *************************************************************************/
std::vector<std::string> x400_radio_control_impl::get_rx_sensor_names(
    const size_t chan) const
{
    const fs_path sensor_path = _get_db_fe_path(chan, RX_DIRECTION) / "sensors";
    if (get_tree()->exists(sensor_path)) {
        return get_tree()->list(sensor_path);
    }
    return {};
}

uhd::sensor_value_t x400_radio_control_impl::get_rx_sensor(
    const std::string& name, const size_t chan)
{
    return get_tree()
        ->access<uhd::sensor_value_t>(
            _get_db_fe_path(chan, RX_DIRECTION) / "sensors" / name)
        .get();
}

std::vector<std::string> x400_radio_control_impl::get_tx_sensor_names(
    const size_t chan) const
{
    const fs_path sensor_path = _get_db_fe_path(chan, TX_DIRECTION) / "sensors";
    if (get_tree()->exists(sensor_path)) {
        return get_tree()->list(sensor_path);
    }
    return {};
}

uhd::sensor_value_t x400_radio_control_impl::get_tx_sensor(
    const std::string& name, const size_t chan)
{
    return get_tree()
        ->access<uhd::sensor_value_t>(
            _get_db_fe_path(chan, TX_DIRECTION) / "sensors" / name)
        .get();
}

/**************************************************************************
 * Radio Identification API Calls
 *************************************************************************/
size_t x400_radio_control_impl::get_chan_from_dboard_fe(
    const std::string& fe, const uhd::direction_t direction) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_chan_from_dboard_fe(fe, direction);
}

std::string x400_radio_control_impl::get_dboard_fe_from_chan(
    const size_t chan, const uhd::direction_t direction) const
{
    std::lock_guard<std::recursive_mutex> l(_lock);
    return _daughterboard->get_dboard_fe_from_chan(chan, direction);
}

UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(
    x400_radio_control, RADIO_BLOCK, X400, "Radio", true, "radio_clk", "ctrl_clk")
}} // namespace uhd::rfnoc
