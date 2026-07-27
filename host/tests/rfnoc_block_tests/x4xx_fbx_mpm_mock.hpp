//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/device_addr.hpp>
#include <uhdlib/usrp/dboard/fbx/fbx_constants.hpp>
#include <uhdlib/usrp/dboard/fbx/fbx_dboard.hpp>
#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace uhd::usrp;

namespace uhd { namespace test {

namespace {
constexpr double DEFAULT_MCR = 4e9;
}
/*! \brief Mock MPM server for X440/FBX.
 *
 * This is a mock server that mimicks an X410 with a FBX daughterboard.
 * Uses composition instead of multiple inheritance to avoid duplicate base class issues
 */
class ferrum_mock_rpc_server : public uhd::rpc_client,
                               public std::enable_shared_from_this<ferrum_mock_rpc_server>
{
public:
    ferrum_mock_rpc_server(const uhd::device_addr_t& device_info)
        : _device_info(device_info)
    {
    }

    uhd::rpc_client::sptr get_raw_rpc_client() override
    {
        // Return self as the raw client. In production, this returns the underlying gRPC
        // client, but in tests the mock serves as both the interface and implementation.
        // This is called by x400_dboard_rpc_iface and
        // mpmd_mb_controller::get_rpc_client().
        return shared_from_this();
    }

    // Non-RPC helper methods
    void set_token(const std::string& /*token*/) override {}
    void set_timeout(uint64_t /*timeout_ms*/) override {}
    uint64_t get_client_id() const override
    {
        return 0;
    }
    uhd::rpc_client::timeout_scope_uptr set_scope_timeout(
        uint64_t /*timeout_ms*/) override
    {
        return nullptr;
    }
    uint16_t get_proto_ver() override
    {
        return 0;
    }
    uint32_t get_chdr_width() override
    {
        return 64;
    }

    // Main RPC methods
    std::string ping(std::string /*payload*/) override
    {
        return "";
    }
    std::string claim(std::string /*session_id*/) override
    {
        return "";
    }
    bool reclaim() override
    {
        return true;
    }
    bool unclaim() override
    {
        return true;
    }
    std::vector<uint32_t> get_mpm_compat_number() override
    {
        return {};
    }
    std::vector<std::map<std::string, std::string>> list_methods() override
    {
        throw std::runtime_error("list_methods not implemented in mock");
    }
    std::vector<std::string> list_updateable_components() override
    {
        return {};
    }
    std::map<std::string, std::string> get_component_info(
        std::string /*component_name*/) override
    {
        return {};
    }
    bool init(const std::map<std::string, std::string>& /*args*/) override
    {
        return true;
    }
    void reset_timer_and_mgr() override {}
    bool update_component(
        const std::vector<std::map<std::string, std::string>>& /*metadata*/,
        const std::vector<std::vector<uint8_t>>& /*data*/) override
    {
        return true;
    }
    std::vector<std::map<std::string, std::string>> get_log_buf() override
    {
        return {};
    }
    std::map<std::string, std::string> get_device_info() override
    {
        return {};
    }
    std::vector<std::string> get_init_status() override
    {
        return {};
    }
    uint32_t get_device_id() override
    {
        return 0;
    }
    void set_device_id(uint32_t /*device_id*/) override {}
    std::vector<std::string> get_chdr_link_types() override
    {
        return {};
    }
    std::vector<std::map<std::string, std::string>> get_chdr_link_options(
        std::string /*xport_type*/) override
    {
        return {};
    }
    std::map<std::string, std::string> get_chdr_xport_adapters() override
    {
        return {};
    }
    bool add_remote_chdr_route(std::string /*adapter*/,
        uint32_t /*epid*/,
        const std::map<std::string, std::string>& /*route_args*/) override
    {
        return true;
    }
    std::vector<std::map<std::string, std::string>> get_clocks() override
    {
        return {};
    }
    void set_mb_eeprom(const std::map<std::string, std::string>& /*eeprom_vals*/) override
    {
    }
    void set_db_eeprom(uint32_t /*dboard_idx*/,
        const std::map<std::string, std::vector<uint8_t>>& /*eeprom_data*/) override
    {
    }
    void set_channel_mode(std::string /*channel_mode*/) override {}

    /**************************************************************************
     * RPC Call Mockups
     *
     * The following public methods are replacements of that normally happens in
     * the Python-based MPM. Some notes on writing mocks:
     * - These are mocks, so don't go fancy and only let them do the bare
     *   minimum required for tests
     * - Remember to add them to _init_rpc() further down
     *************************************************************************/
    // TODO: Most probably 2, but not yet clear
    uint32_t get_num_timekeepers() override
    {
        return 1;
    }

    std::vector<std::string> get_mb_sensors() override
    {
        return {"ref_locked"};
    }

    std::vector<std::string> get_gpio_banks() override
    {
        return {"GPIO0", "GPIO1"};
    }

    bool supports_feature(std::string feature) override
    {
        return feature == "ref_clk_calibration";
    }

    std::vector<std::string> list_active_overlays() override
    {
        return {};
    }

    std::vector<std::string> list_available_overlays() override
    {
        return {};
    }

    std::vector<std::string> list_required_dt_overlays(
        const std::map<std::string, std::string>& /*device_info*/) override
    {
        return {};
    }

    bool overlay_apply() override
    {
        return true;
    }

    std::vector<std::map<std::string, std::string>> get_dboard_info() override
    {
        return {{
            // One entry per dboard info
            {"pid", std::to_string(uhd::usrp::fbx::FBX_PID)}
            // End of entries
        }};
    }

    void set_tick_period(uint32_t, uint64_t) override
    {
        // nop
    }

    // get_master_clock_rate, get_sensors, get_sensor moved to dboard_iface
    // set_cal_frozen, get_cal_frozen, set_calibration_mode moved to dboard_iface

    std::map<std::string, std::vector<uint8_t>> get_db_eeprom(uint32_t) override
    {
        return {{
            // One line per entry
            {"pid", s2u8("mock")}, // Used to specify power cal API
            {"serial", s2u8("BADCODE")}
            // End of entries
        }};
    }

    std::vector<std::map<std::string, std::string>> pop_host_tasks(std::string) override
    {
        return {};
    }

    std::vector<std::string> get_gpio_srcs(std::string /*bank*/) override
    {
        return {"DB0_SPI"};
    }

    uint64_t get_timekeeper_time(uint32_t /*timekeeper_idx*/, bool /*last_pps*/) override
    {
        return 0;
    }

    void set_timekeeper_time(
        uint32_t /*timekeeper_idx*/, uint64_t /*ticks*/, bool /*last_pps*/) override
    {
        // nop
    }

    std::string get_time_source() override
    {
        return "";
    }

    std::vector<std::string> get_time_sources() override
    {
        return {};
    }

    std::string get_clock_source() override
    {
        return "";
    }

    std::vector<std::string> get_clock_sources() override
    {
        return {};
    }

    std::map<std::string, std::string> get_sync_source() override
    {
        return {};
    }

    std::vector<std::map<std::string, std::string>> get_sync_sources() override
    {
        return {};
    }

    void set_clock_source_out(bool /*enb*/) override
    {
        // nop
    }

    void set_trigger_io(std::string /*direction*/) override
    {
        // nop
    }

    std::map<std::string, std::string> get_mb_eeprom() override
    {
        return {};
    }

    std::vector<std::string> get_gpio_src(std::string /*bank*/) override
    {
        return {};
    }

    void set_gpio_src(
        std::string /*bank*/, const std::vector<std::string>& /*src*/) override
    {
        // nop
    }

    void set_ref_clk_tuning_word(uint32_t /*tuning_word*/) override
    {
        // nop
    }

    uint32_t get_ref_clk_tuning_word() override
    {
        return 0;
    }

    void store_ref_clk_tuning_word(uint32_t /*tuning_word*/) override
    {
        // nop
    }

    std::map<std::string, std::string> get_mb_sensor(std::string /*sensor*/) override
    {
        return {};
    }

    void set_time_source(std::string /*source*/) override
    {
        // nop
    }

    void set_clock_source(std::string /*source*/) override
    {
        // nop
    }

    void set_sync_source(const std::map<std::string, std::string>& /*source*/) override
    {
        // nop
    }

    void set_dac_mux_data(int32_t /*i*/, int32_t /*q*/) override
    {
        // nop
    }

    void restart_converter(std::string /*trx*/,
        uint32_t /*block_count*/,
        uint32_t /*chan*/,
        uint32_t /*mixer_mode*/)
    {
        // nop
    }

    std::vector<std::string> dio_get_supported_voltage_levels(std::string) override
    {
        return {"OFF", "1V8", "2V5", "3V3"};
    }

    void dio_set_voltage_level(std::string, std::string) override
    {
        // nop
    }

    std::string dio_get_voltage_level(std::string) override
    {
        return "3V3";
    }

    void dio_set_port_mapping(std::string) override
    {
        // nop
    }

    // is_db_gpio_ifc_present moved to dboard_iface

    void dio_set_pin_directions(std::string, uint32_t) override
    {
        // nop
    }

    void dio_set_external_power(std::string, bool) override
    {
        // nop
    }

    std::string dio_get_external_power_state(std::string) override
    {
        return "OFF";
    }

    uint64_t get_fpga_aux_ref_freq() override
    {
        return 0;
    }

    void nsync_change_input_source(std::string) override
    {
        // nop
    }

    void enable_ecpri_clocks(bool, std::string) override
    {
        // nop
    }

    void config_rpll_to_nsync() override
    {
        // nop
    }

    std::string peek_clkaux(uint32_t) override
    {
        return "0x0";
    }

    void poke_clkaux(uint32_t /*addr*/, uint32_t /*val*/) override {}
    std::string peek_ctrlport(uint32_t /*addr*/) override
    {
        return "0x0";
    }
    void poke_ctrlport(uint32_t /*addr*/, uint32_t /*val*/) override {}
    std::string peek_cpld(uint32_t /*addr*/) override
    {
        return "0x0";
    }
    void poke_cpld(uint32_t /*addr*/, uint32_t /*val*/) override {}
    std::string peek_mb(uint32_t /*addr*/) override
    {
        return "0x0";
    }
    void poke_mb(uint32_t /*addr*/, uint32_t /*val*/) override {}
    std::string peek_rfdc(uint32_t /*addr*/) override
    {
        return "0x0";
    }
    void poke_rfdc(uint32_t /*addr*/, uint32_t /*val*/) override {}

    bool clkaux_get_nsync_status0() override
    {
        return false;
    }

    bool clkaux_get_nsync_status1() override
    {
        return false;
    }

    void dio_set_gpio_src(std::string, const std::vector<std::string>&) override
    {
        // nop
    }

    void dio_set_pin_outputs(std::string, uint32_t) override
    {
        // nop
    }

    uint32_t dio_get_pin_inputs(std::string) override
    {
        return 0;
    }

    std::string dio_status() override
    {
        return "";
    }

    std::map<std::string, std::string> synchronize(
        const std::map<std::string, std::string>&, bool) override
    {
        return {};
    }

    std::map<std::string, std::string> aggregate_sync_data(
        const std::vector<std::map<std::string, std::string>>& collated_sync_data)
        override
    {
        return collated_sync_data.empty() ? std::map<std::string, std::string>{}
                                          : collated_sync_data.front();
    }

    // === Stubs for newly added gRPC RPCs ===
    void deinit() override {}
    void tear_down() override {}
    void init_dboards(const std::map<std::string, std::string>& /*args*/) override {}
    std::map<std::string, std::string> get_device_info_dyn() override
    {
        return {};
    }
    std::map<std::string, std::string> get_post_reset_args(
        const std::map<std::string, std::string>& /*original_args*/) override
    {
        return {};
    }
    std::map<std::string, std::string> generate_device_info(
        const std::map<std::string, std::string>& /*eeprom_md*/,
        const std::map<std::string, std::string>& /*mboard_info*/,
        const std::vector<std::map<std::string, std::string>>& /*dboard_infos*/) override
    {
        return {};
    }
    std::map<std::string, std::string> get_ref_lock_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_gps_locked_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_fan_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_temp_sensor(
        std::string /*sensor_name*/) override
    {
        return {};
    }
    std::map<std::string, std::string> get_fpga_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_internal_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_rf_channel_a_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_rf_channel_b_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_main_power_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_gps_time_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_gps_tpv_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_gps_sky_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_gps_gpgga_sensor() override
    {
        return {};
    }
    void enable_fp_gpio(bool /*enable*/) override {}
    void enable_gps(bool /*enable*/) override {}
    void set_fp_gpio_voltage(double /*voltage*/) override {}
    double get_fp_gpio_voltage() override
    {
        return 0.0;
    }
    double get_ref_clock_freq() override
    {
        return 0.0;
    }
    void set_ref_clock_freq(double /*freq*/) override {}
    void reset_clock(bool /*value*/, std::string /*clock_to_reset*/) override {}

    void clkaux_config_dac(uint32_t /*tuning_word*/, uint32_t /*out_select*/) override {}
    void clkaux_config_dpll(std::string /*source*/) override {}
    uint32_t clkaux_read_dac(uint32_t /*out_select*/) override
    {
        return 0;
    }
    void clkaux_store_tuning_word(uint32_t /*tuning_word*/) override {}
    void clkaux_export_clock(bool /*enable*/) override {}
    void clkaux_set_source(
        std::string /*clock_source*/, std::string /*time_source*/) override
    {
    }
    void clkaux_set_trig(bool /*enable*/, std::string /*direction*/) override {}
    std::string clkaux_get_clock_source() override
    {
        return "";
    }
    void clkaux_set_ref_lock_led(uint32_t /*val*/) override {}
    bool clkaux_is_nsync_supported() override
    {
        return false;
    }
    bool clkaux_is_gps_supported() override
    {
        return false;
    }
    bool clkaux_is_gps_enabled() override
    {
        return false;
    }
    bool clkaux_get3v3_pg() override
    {
        return false;
    }
    bool clkaux_get_gps_alarm() override
    {
        return false;
    }
    bool clkaux_get_gps_locked() override
    {
        return false;
    }
    bool clkaux_get_gps_survey() override
    {
        return false;
    }
    bool clkaux_get_gps_warmup() override
    {
        return false;
    }
    bool clkaux_get_gps_phase_lock() override
    {
        return false;
    }
    bool clkaux_get_nsync_chip_id_valid() override
    {
        return false;
    }
    uint32_t clkaux_get_nsync_lmk_eeprom_prog_cycles() override
    {
        return 0;
    }
    std::string clkaux_get_nsync_lmk_status_dpll() override
    {
        return "";
    }
    std::string clkaux_get_nsync_lmk_status_pll_xo() override
    {
        return "";
    }
    void clkaux_set_nsync_lmk_power_en(bool /*enable*/) override {}
    void clkaux_set_nsync_pri_ref_source(std::string /*source*/) override {}
    void clkaux_set_nsync_ref_select(std::string /*source*/) override {}
    bool clkaux_set_nsync_soft_reset(bool /*value*/) override
    {
        return false;
    }
    void clkaux_set_nsync_tcxo_en(bool /*enable*/) override {}
    void clkaux_write_nsync_lmk_cfg_regs_to_eeprom(std::string /*method*/) override {}
    void clkaux_write_nsync_lmk_eeprom_to_cfg_regs() override {}
    uint32_t clkaux_peek8(uint32_t /*addr*/) override
    {
        return 0;
    }
    void clkaux_poke8(
        uint32_t /*addr*/, uint32_t /*val*/, bool /*overwrite_mask*/) override
    {
    }
    uint32_t dio_get_pin_input(std::string /*port*/, uint32_t /*pin*/) override
    {
        return 0;
    }
    void dio_set_pin_direction(
        std::string /*port*/, uint32_t /*pin*/, uint32_t /*value*/) override
    {
    }
    void dio_set_pin_output(
        std::string /*port*/, uint32_t /*pin*/, uint32_t /*value*/) override
    {
    }
    void dio_tear_down() override {}
    std::string dio_debug() override
    {
        return "";
    }
    std::map<std::string, std::string> get_gps_enabled_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_gps_alarm_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_gps_warmup_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_gps_survey_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_gps_phase_lock_sensor() override
    {
        return {};
    }
    std::string get_gps_sensor_status() override
    {
        return "";
    }
    std::map<std::string, std::string> get_main_power_temp_sensor0() override
    {
        return {};
    }
    std::map<std::string, std::string> get_main_power_temp_sensor1() override
    {
        return {};
    }
    std::map<std::string, std::string> get_scu_internal_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_sample_clock_pcb_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_dram_pcb_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_tmp464_internal_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_power_supply_pcb_temp_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_fan0_sensor() override
    {
        return {};
    }
    std::map<std::string, std::string> get_fan1_sensor() override
    {
        return {};
    }
    bool qsfp0_is_available() override
    {
        return false;
    }
    void qsfp0_enable_i2c(bool /*enable*/) override {}
    uint32_t qsfp0_adapter_id() override
    {
        return 0;
    }
    std::string qsfp0_adapter_id_name() override
    {
        return "";
    }
    std::vector<uint32_t> qsfp0_status() override
    {
        return {};
    }
    std::vector<std::string> qsfp0_decoded_status() override
    {
        return {};
    }
    std::string qsfp0_connector_type() override
    {
        return "";
    }
    std::string qsfp0_info() override
    {
        return "";
    }
    std::string qsfp0_vendor_name() override
    {
        return "";
    }
    bool qsfp1_is_available() override
    {
        return false;
    }
    void qsfp1_enable_i2c(bool /*enable*/) override {}
    uint32_t qsfp1_adapter_id() override
    {
        return 0;
    }
    std::string qsfp1_adapter_id_name() override
    {
        return "";
    }
    std::vector<uint32_t> qsfp1_status() override
    {
        return {};
    }
    std::vector<std::string> qsfp1_decoded_status() override
    {
        return {};
    }
    std::string qsfp1_connector_type() override
    {
        return "";
    }
    std::string qsfp1_info() override
    {
        return "";
    }
    std::string qsfp1_vendor_name() override
    {
        return "";
    }
    void init_rpu() override {}
    std::string get_rpu_firmware(uint32_t /*core_number*/) override
    {
        return "";
    }
    std::string get_rpu_state(uint32_t /*core_number*/, bool /*validate*/) override
    {
        return "";
    }
    std::string set_rpu_firmware(
        uint32_t /*core_number*/, std::string /*firmware*/, uint32_t /*start*/) override
    {
        return "";
    }
    std::string set_rpu_state(uint32_t /*core_number*/,
        std::string /*new_state_command*/,
        bool /*validate*/) override
    {
        return "";
    }
    double get_spll_freq() override
    {
        return 0.0;
    }

    void enable_pps_out(bool /*enable*/) override {}
    void enable_ref_clock(bool /*enable*/) override {}
    void enable1g_ref_clock() override {}
    void enable_wr_ref_clock() override {}

    // Daughterboard interface implementation
    // Only implements X400-specific methods; unused AD9361/AD9371 methods are stubbed by
    // base class
    class mock_dboard_iface : public uhd::rpc_client::dboard_iface
    {
    public:
        mock_dboard_iface(ferrum_mock_rpc_server* parent) : _parent(parent) {}

        uint32_t get_revision() override
        {
            return 0;
        }

        std::string get_revision_string() override
        {
            return "A";
        }

        std::string get_serial() override
        {
            return "12345";
        }

        // X400-specific dboard methods (actually used by FBX)
        double rfdc_set_nco_freq(
            std::string trx, uint32_t chan, double freq, uint32_t /*ch_mode*/) override
        {
            BOOST_REQUIRE(trx == "rx" || trx == "tx");
            BOOST_REQUIRE(chan < uhd::usrp::fbx::FBX_MAX_NUM_CHANS);
            _parent->nco_freq[trx][chan] = freq;
            return freq;
        }

        double rfdc_get_nco_freq(
            std::string trx, uint32_t chan, uint32_t /*ch_mode*/) override
        {
            BOOST_REQUIRE(trx == "rx" || trx == "tx");
            BOOST_REQUIRE(chan < uhd::usrp::fbx::FBX_MAX_NUM_CHANS);
            // On construction, the expert will ask for the current nco frequency, and our
            // nco_freq map won't have a value yet.
            if (_parent->nco_freq.find(trx) == _parent->nco_freq.end()
                || _parent->nco_freq.at(trx).find(chan)
                       == _parent->nco_freq.at(trx).end()) {
                return 0;
            }
            return _parent->nco_freq.at(trx).at(chan);
        }

        bool get_threshold_status(
            uint32_t /*channel*/, uint32_t /*mode*/, uint32_t /*threshold_idx*/) override
        {
            return false;
        }

        void setup_threshold(uint32_t /*chan*/,
            uint32_t /*mix_mode*/,
            uint32_t /*threshold_block*/,
            std::string /*mode*/,
            uint32_t /*delay*/,
            uint32_t /*under*/,
            uint32_t /*over*/) override
        {
            // nop
        }

        void set_calibration_mode(
            uint32_t /*chan*/, uint32_t, std::string /*mode*/) override
        {
            // nop
        }

        void set_data_path(uint32_t /*mode*/, std::string /*direction*/) override
        {
            // nop
        }

        std::vector<int32_t> get_cal_frozen(uint32_t /*chan*/, uint32_t) override
        {
            return {};
        }

        void set_cal_frozen(int32_t /*frozen*/, uint32_t, uint32_t /*chan*/) override
        {
            // nop
        }

        // Stub implementations for unused methods (required by base class)
        // These are E3xx/N3xx methods not used by X400
        bool is_lo_dist_present() override
        {
            return false;
        }
        std::vector<std::string> get_sensors(std::string /*trx*/) override
        {
            return {};
        }
        std::map<std::string, std::string> get_sensor(
            std::string /*trx*/, std::string /*sensor*/, uint32_t /*chan*/) override
        {
            return {};
        }
        double get_master_clock_rate() override
        {
            return _parent->_device_info.cast<double>("master_clock_rate", DEFAULT_MCR);
        }
        double get_dboard_sample_rate() override
        {
            const double mcr =
                _parent->_device_info.cast<double>("master_clock_rate", DEFAULT_MCR);
            static const std::map<double, double> spll_map{
                // One line per entry
                {122.88e6, 2.94912e9},
                {122.88e6 * 4, 2.94912e9},
                // TODO: These entries need to be updated for more sample rates.
                {4e9, 4.0e9} // End of entries
            };
            return spll_map.at(mcr);
        }
        double get_dboard_prc_rate() override
        {
            // FBX does not model a PRC path in this mock; return a sentinel value.
            return 0.0;
        }
        double set_bw_filter(std::string /*which*/, double bw) override
        {
            return bw;
        }
        double set_gain(std::string /*which*/, double value) override
        {
            return value;
        }
        void set_agc(std::string /*which*/, bool /*enable*/) override {}
        void set_agc_mode(std::string /*which*/, std::string /*mode*/) override {}
        double set_catalina_clock_rate(double rate) override
        {
            return rate;
        }
        void set_active_chains(
            bool /*tx1*/, bool /*tx2*/, bool /*rx1*/, bool /*rx2*/) override
        {
        }
        double catalina_tune(std::string /*which*/, double value) override
        {
            return value;
        }
        void set_dc_offset_auto(std::string /*which*/, bool /*on*/) override {}
        void set_timing_mode(std::string /*timing_mode*/) override {}
        void set_iq_balance_auto(std::string /*which*/, bool /*on*/) override {}
        double get_freq(std::string /*which*/) override
        {
            return 0.0;
        }
        void data_port_loopback(bool /*on*/) override {}
        std::map<std::string, std::string> get_rssi(std::string /*which*/) override
        {
            return {};
        }
        std::map<std::string, std::string> get_temperature() override
        {
            return {};
        }
        std::vector<std::string> get_filter_names(std::string /*which*/) override
        {
            return {};
        }
        void output_digital_test_tone(bool /*enb*/) override {}
        double set_freq(std::string /*which*/, double freq, bool /*alt_tuning*/) override
        {
            return freq;
        }
        double get_gain(std::string /*which*/) override
        {
            return 0.0;
        }
        double get_bandwidth(std::string /*which*/) override
        {
            return 0.0;
        }
        double set_bandwidth(std::string /*which*/, double bandwidth) override
        {
            return bandwidth;
        }
        std::string set_lo_source(std::string /*which*/, std::string source) override
        {
            return source;
        }
        std::string get_lo_source(std::string /*which*/) override
        {
            return "";
        }
        void set_fir(std::string /*name*/,
            int32_t /*gain*/,
            const std::vector<int32_t>& /*coeffs*/) override
        {
        }
        std::pair<int32_t, std::vector<int32_t>> get_fir(std::string /*name*/) override
        {
            return {0, {}};
        }
        bool get_ad9371_lo_lock(std::string /*which*/) override
        {
            return false;
        }
        bool get_lowband_lo_lock(std::string /*which*/) override
        {
            return false;
        }
        void enable_rx_lowband_lo(bool /*enable*/) override {}
        void enable_tx_lowband_lo(bool /*enable*/) override {}
        double set_master_clock_rate(double rate) override
        {
            return rate;
        }
        void enable_lo_export(std::string /*direction*/, bool /*enable*/) override {}
        void enable_lo_output(
            std::string /*direction*/, uint32_t /*index*/, bool /*enable*/) override
        {
        }
        void set_dac_mux_enable(
            uint32_t /*channel*/, uint32_t /*enable*/, uint32_t /*mode*/) override
        {
        }
        void set_leds(
            uint32_t /*channel*/, bool /*rx*/, bool /*trx_rx*/, bool /*trx_tx*/) override
        {
        }
        bool is_db_gpio_ifc_present() override
        {
            return true;
        }
        void config_tx_path(std::string /*path*/) override {}
        std::string get_tx_path() override
        {
            return "";
        }
        void config_rx_path(std::string /*path*/) override {}
        std::string get_rx_path() override
        {
            return "";
        }

        std::vector<double> get_cal_coefs(
            uint32_t /*channel*/, uint32_t /*cal_block*/, uint32_t /*mode*/) override
        {
            return {0.0};
        }

        void set_cal_coefs(uint32_t /*channel*/,
            uint32_t /*cal_block*/,
            const std::vector<double>& /*coefs*/,
            uint32_t /*mode*/) override
        {
        }

        // === Stubs for newly added gRPC dboard RPCs ===
        bool get_ad9361_lo_lock(std::string /*which*/) override
        {
            return false;
        }
        std::map<std::string, std::string> get_lo_lock_sensor(
            std::string /*which*/) override
        {
            return {};
        }
        std::map<std::string, std::string> get_rx_lo_lock_sensor(
            uint32_t /*chan*/) override
        {
            return {};
        }
        std::map<std::string, std::string> get_tx_lo_lock_sensor(
            uint32_t /*chan*/) override
        {
            return {};
        }
        std::map<std::string, std::string> get_catalina_temp_sensor(
            uint32_t /*chan*/) override
        {
            return {};
        }
        double get_rssi_val(std::string /*which*/) override
        {
            return 0.0;
        }
        std::map<std::string, std::string> get_rssi_sensor(uint32_t /*chan*/) override
        {
            return {};
        }
        std::vector<std::string> get_gain_names(std::string /*which*/) override
        {
            return {};
        }
        std::vector<double> get_rf_freq_range() override
        {
            return {};
        }
        std::vector<double> get_bw_filter_range() override
        {
            return {};
        }
        std::vector<double> get_clock_rate_range() override
        {
            return {};
        }
        double set_clock_rate(double /*rate*/) override
        {
            return 0.0;
        }
        double tune(std::string /*which*/, double /*freq*/) override
        {
            return 0.0;
        }
        void set_dc_offset(std::string /*which*/,
            double /*offset_real*/,
            double /*offset_imag*/) override
        {
        }
        void set_iq_balance(std::string /*which*/,
            double /*correction_real*/,
            double /*correction_imag*/) override
        {
        }
        void set_filter(std::string /*which*/,
            std::string /*filter_name*/,
            std::string /*filter_data*/) override
        {
        }
        std::map<std::string, std::vector<uint8_t>> get_user_eeprom_data() override
        {
            return {};
        }
        void set_user_eeprom_data(
            const std::map<std::string, std::vector<uint8_t>>& /*eeprom_data*/) override
        {
        }
        void update_ref_clock_freq(double /*freq*/) override {}
        void init_rfic(double /*master_clock_rate*/) override {}
        void tear_down_rfic() override {}

        void enable_iq_swap(bool, std::string, uint32_t) override
        {
            // nop
        }

        void poke_db(uint32_t /*addr*/, uint32_t /*val*/) override {}
        std::string peek_db(uint32_t /*addr*/) override
        {
            return "0x0";
        }
        bool has_compat_version(uint32_t /*min_required_version*/) override
        {
            return true;
        }
        uint32_t peek_lo_spi(std::string /*lo_name*/, uint32_t /*addr*/) override
        {
            return 0;
        }
        void poke_lo_spi(
            std::string /*lo_name*/, uint32_t /*addr*/, uint32_t /*val*/) override
        {
        }

        void begin_initialization() override {}
        void start_jesd_rx() override {}
        void start_jesd_tx() override {}
        void start_radio() override {}
        void stop_radio() override {}
        void enable_jesd_loopback(uint32_t /*enable*/) override {}
        uint32_t get_multichip_sync_status() override
        {
            return 0;
        }
        uint32_t get_framer_status() override
        {
            return 0;
        }
        uint32_t get_deframer_status() override
        {
            return 0;
        }
        uint32_t get_ilas_config_match() override
        {
            return 0;
        }
        uint32_t get_product_id() override
        {
            return 0;
        }
        uint32_t get_device_rev() override
        {
            return 0;
        }
        std::string get_api_version() override
        {
            return "";
        }
        std::string get_arm_version() override
        {
            return "";
        }
        std::vector<double> get_clock_rates() override
        {
            return {};
        }
        std::vector<double> get_gain_range(std::string /*which*/) override
        {
            return {};
        }
        void enable_channel(std::string /*which*/, bool /*enable*/) override {}
        bool get_lo_locked(std::string /*which*/) override
        {
            return false;
        }
        void set_clk_safe_state() override {}
        bool get_ref_lock() override
        {
            return false;
        }
        std::map<std::string, std::string> get_lowband_tx_lo_locked_sensor(
            uint32_t /*chan*/) override
        {
            return {};
        }
        std::map<std::string, std::string> get_lowband_rx_lo_locked_sensor(
            uint32_t /*chan*/) override
        {
            return {};
        }
        std::map<std::string, std::string> get_ad9371_tx_lo_locked_sensor(
            uint32_t /*chan*/) override
        {
            return {};
        }
        std::map<std::string, std::string> get_ad9371_rx_lo_locked_sensor(
            uint32_t /*chan*/) override
        {
            return {};
        }
        uint32_t dbcore_peek(uint32_t /*addr*/) override
        {
            return 0;
        }
        void dbcore_poke(uint32_t /*addr*/, uint32_t /*data*/) override {}
        void dump_jesd_core() override {}
        uint32_t peek_db_cpld(uint32_t /*addr*/) override
        {
            return 0;
        }
        uint32_t poke_db_cpld(uint32_t /*addr*/, uint32_t /*data*/) override
        {
            return 0;
        }

    private:
        ferrum_mock_rpc_server* _parent;
    };

    uhd::rpc_client::dboard_iface& get_dboard(size_t /*db_idx*/) override
    {
        if (!_dboard_iface) {
            _dboard_iface = std::make_shared<mock_dboard_iface>(this);
        }
        return *_dboard_iface;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Public attributes for easy inspection
    //
    // Use this in the mock functions to cache values, or expose values that get
    // tested later
    // TODO: We'll need other NCO settings
    std::map<std::string, std::map<size_t, double>> nco_freq;
    //
    ///////////////////////////////////////////////////////////////////////////

private:
    uhd::device_addr_t _device_info;
    std::shared_ptr<mock_dboard_iface> _dboard_iface;

    static std::vector<uint8_t> s2u8(const std::string& s)
    {
        return std::vector<uint8_t>(s.begin(), s.end());
    }
};

}} // namespace uhd::test
