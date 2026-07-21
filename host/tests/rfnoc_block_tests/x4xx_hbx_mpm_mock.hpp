//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/device_addr.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_constants.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_dboard.hpp>
#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace uhd::usrp;

namespace uhd { namespace test {

namespace {
constexpr double DEFAULT_MCR = 1250e6;
}

/*! \brief Mock MPM server for X4xx/HBX.
 *
 * This is a mock server that mimics an X4xx with an HBX daughterboard.
 * Uses composition with get_dboard() to match the gRPC rpc_client structure.
 */
class x4xx_hbx_mock_rpc_server
    : public uhd::rpc_client,
      public std::enable_shared_from_this<x4xx_hbx_mock_rpc_server>
{
public:
    x4xx_hbx_mock_rpc_server(const uhd::device_addr_t& device_info)
        : _device_info(device_info)
    {
    }

    uhd::rpc_client::sptr get_raw_rpc_client() override
    {
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

    // Core session methods
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
    std::map<std::string, std::string> get_component_info(std::string) override
    {
        return {};
    }
    bool init(const std::map<std::string, std::string>& /*args*/) override
    {
        return true;
    }
    void reset_timer_and_mgr() override {}
    bool update_component(const std::vector<std::map<std::string, std::string>>&,
        const std::vector<std::vector<uint8_t>>&) override
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
    void set_device_id(uint32_t) override {}
    std::vector<std::string> get_chdr_link_types() override
    {
        return {};
    }
    std::vector<std::map<std::string, std::string>> get_chdr_link_options(
        std::string) override
    {
        return {};
    }
    std::map<std::string, std::string> get_chdr_xport_adapters() override
    {
        return {};
    }
    bool add_remote_chdr_route(
        std::string, uint32_t, const std::map<std::string, std::string>&) override
    {
        return true;
    }
    std::vector<std::map<std::string, std::string>> get_clocks() override
    {
        return {};
    }
    void set_mb_eeprom(const std::map<std::string, std::string>&) override {}
    void set_db_eeprom(
        uint32_t, const std::map<std::string, std::vector<uint8_t>>&) override
    {
    }
    void set_channel_mode(std::string) override {}
    void deinit() override {}
    void tear_down() override {}
    void init_dboards(const std::map<std::string, std::string>&) override {}
    std::map<std::string, std::string> get_device_info_dyn() override
    {
        return {};
    }
    std::map<std::string, std::string> get_post_reset_args(
        const std::map<std::string, std::string>&) override
    {
        return {};
    }
    std::map<std::string, std::string> generate_device_info(
        const std::map<std::string, std::string>&,
        const std::map<std::string, std::string>&,
        const std::vector<std::map<std::string, std::string>>&) override
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
    std::map<std::string, std::string> get_temp_sensor(std::string) override
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
    void enable_fp_gpio(bool) override {}
    void enable_gps(bool) override {}
    void set_fp_gpio_voltage(double) override {}
    double get_fp_gpio_voltage() override
    {
        return 0.0;
    }
    double get_ref_clock_freq() override
    {
        return 0.0;
    }
    void set_ref_clock_freq(double) override {}
    void reset_clock(bool, std::string) override {}

    /**************************************************************************
     * RPC Call Mockups
     *************************************************************************/
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
        const std::map<std::string, std::string>&) override
    {
        return {};
    }
    bool overlay_apply() override
    {
        return true;
    }

    std::vector<std::map<std::string, std::string>> get_dboard_info() override
    {
        return {{{"pid", std::to_string(uhd::usrp::hbx::HBX_PID)}}};
    }

    void set_tick_period(uint32_t, uint64_t) override {}

    std::map<std::string, std::vector<uint8_t>> get_db_eeprom(uint32_t) override
    {
        return {{{"pid", s2u8("mock")}, {"serial", s2u8("HBX_MOCK_NO_CAL")}}};
    }

    std::vector<std::map<std::string, std::string>> pop_host_tasks(std::string) override
    {
        return {};
    }

    std::vector<std::string> get_gpio_srcs(std::string /*bank*/) override
    {
        return {};
    }

    uint64_t get_timekeeper_time(uint32_t, bool) override
    {
        return 0;
    }
    void set_timekeeper_time(uint32_t, uint64_t, bool) override {}
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
    void set_clock_source_out(bool) override {}
    void set_trigger_io(std::string) override {}
    std::map<std::string, std::string> get_mb_eeprom() override
    {
        return {};
    }
    std::vector<std::string> get_gpio_src(std::string) override
    {
        return {};
    }
    void set_gpio_src(std::string, const std::vector<std::string>&) override {}
    void set_ref_clk_tuning_word(uint32_t) override {}
    uint32_t get_ref_clk_tuning_word() override
    {
        return 0;
    }
    void store_ref_clk_tuning_word(uint32_t) override {}
    std::map<std::string, std::string> get_mb_sensor(std::string) override
    {
        return {};
    }
    void set_time_source(std::string) override {}
    void set_clock_source(std::string) override {}
    void set_sync_source(const std::map<std::string, std::string>&) override {}
    void set_dac_mux_data(int32_t, int32_t) override {}

    std::vector<std::string> dio_get_supported_voltage_levels(std::string) override
    {
        return {"OFF", "1V8", "2V5", "3V3"};
    }
    void dio_set_voltage_level(std::string, std::string) override {}
    std::string dio_get_voltage_level(std::string) override
    {
        return "3V3";
    }
    void dio_set_port_mapping(std::string) override {}
    void dio_set_pin_directions(std::string, uint32_t) override {}
    void dio_set_external_power(std::string, bool) override {}
    std::string dio_get_external_power_state(std::string) override
    {
        return "OFF";
    }

    uint64_t get_fpga_aux_ref_freq() override
    {
        return 0;
    }
    void nsync_change_input_source(std::string) override {}
    void enable_ecpri_clocks(bool, std::string) override {}
    void config_rpll_to_nsync() override {}
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
    void dio_set_gpio_src(std::string, const std::vector<std::string>&) override {}
    void dio_set_pin_outputs(std::string, uint32_t) override {}
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

    // Daughterboard interface — wraps HBX-specific dboard methods
    class mock_dboard_iface : public uhd::rpc_client::dboard_iface
    {
    public:
        mock_dboard_iface(x4xx_hbx_mock_rpc_server* parent) : _parent(parent) {}

        // HBX-specific dboard methods (tested)
        double rfdc_set_nco_freq(
            std::string trx, uint32_t chan, double freq, uint32_t /*ch_mode*/) override
        {
            BOOST_REQUIRE(trx == "rx" || trx == "tx");
            BOOST_REQUIRE(chan < uhd::usrp::hbx::HBX_MAX_NUM_CHANS);
            _parent->nco_freq[trx][chan] = freq;
            return freq;
        }

        double rfdc_get_nco_freq(
            std::string trx, uint32_t chan, uint32_t /*ch_mode*/) override
        {
            BOOST_REQUIRE(trx == "rx" || trx == "tx");
            BOOST_REQUIRE(chan < uhd::usrp::hbx::HBX_MAX_NUM_CHANS);
            if (_parent->nco_freq.find(trx) == _parent->nco_freq.end()
                || _parent->nco_freq.at(trx).find(chan)
                       == _parent->nco_freq.at(trx).end()) {
                return 0;
            }
            return _parent->nco_freq.at(trx).at(chan);
        }

        void set_data_path(uint32_t mode, std::string direction) override
        {
            BOOST_REQUIRE(direction == "rx" || direction == "tx");
            _parent->data_path[direction] = mode;
        }

        bool is_db_gpio_ifc_present() override
        {
            return true;
        }

        void set_leds(
            uint32_t /*channel*/, bool /*rx*/, bool /*trx_rx*/, bool /*trx_tx*/) override
        {
        }

        // Stub implementations for unused methods (required by base class)
        bool get_threshold_status(uint32_t, uint32_t, uint32_t) override
        {
            return false;
        }
        void setup_threshold(
            uint32_t, uint32_t, uint32_t, std::string, uint32_t, uint32_t, uint32_t)
            override
        {
        }
        void set_calibration_mode(uint32_t, uint32_t, std::string) override {}
        std::vector<int32_t> get_cal_frozen(uint32_t, uint32_t) override
        {
            return {};
        }
        void set_cal_frozen(int32_t, uint32_t, uint32_t) override {}
        std::vector<std::string> get_sensors(std::string) override
        {
            return {};
        }
        std::map<std::string, std::string> get_sensor(
            std::string, std::string, uint32_t) override
        {
            return {};
        }
        double get_master_clock_rate() override
        {
            return _parent->_device_info.cast<double>("master_clock_rate", DEFAULT_MCR);
        }
        double get_dboard_prc_rate() override
        {
            const double mcr =
                _parent->_device_info.cast<double>("master_clock_rate", DEFAULT_MCR);
            static const std::map<double, double> prc_rate_map{
                {1250e6, 62.5e6},
                {491.52e6, 61.44e6},
                {245.76e6, 61.44e6},
                {250e6, 62.5e6},
            };
            const auto it = prc_rate_map.find(mcr);
            if (it != prc_rate_map.end()) {
                return it->second;
            }
            return 62.5e6;
        }
        double get_dboard_sample_rate() override
        {
            const double mcr =
                _parent->_device_info.cast<double>("master_clock_rate", DEFAULT_MCR);
            static const std::map<double, double> spll_map{
                {1250e6, 2.5e9},
                {491.52e6, 3932160000.0},
                {245.76e6, 1966080000.0},
                {250e6, 2e9},
            };
            return spll_map.at(mcr);
        }
        double set_bw_filter(std::string, double bw) override
        {
            return bw;
        }
        double set_gain(std::string, double value) override
        {
            return value;
        }
        void set_agc(std::string, bool) override {}
        void set_agc_mode(std::string, std::string) override {}
        double set_catalina_clock_rate(double rate) override
        {
            return rate;
        }
        void set_active_chains(bool, bool, bool, bool) override {}
        double catalina_tune(std::string, double value) override
        {
            return value;
        }
        void set_dc_offset_auto(std::string, bool) override {}
        void set_timing_mode(std::string) override {}
        void set_iq_balance_auto(std::string, bool) override {}
        double get_freq(std::string) override
        {
            return 0.0;
        }
        void data_port_loopback(bool) override {}
        std::map<std::string, std::string> get_rssi(std::string) override
        {
            return {};
        }
        std::map<std::string, std::string> get_temperature() override
        {
            return {};
        }
        std::vector<std::string> get_filter_names(std::string) override
        {
            return {};
        }
        void output_digital_test_tone(bool) override {}
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
            return "HBX_MOCK";
        }
        bool get_ad9361_lo_lock(std::string) override
        {
            return false;
        }
        std::map<std::string, std::string> get_lo_lock_sensor(std::string) override
        {
            return {};
        }
        std::map<std::string, std::string> get_rx_lo_lock_sensor(uint32_t) override
        {
            return {};
        }
        std::map<std::string, std::string> get_tx_lo_lock_sensor(uint32_t) override
        {
            return {};
        }
        std::map<std::string, std::string> get_catalina_temp_sensor(uint32_t) override
        {
            return {};
        }
        double get_rssi_val(std::string) override
        {
            return 0.0;
        }
        std::map<std::string, std::string> get_rssi_sensor(uint32_t) override
        {
            return {};
        }
        std::vector<std::string> get_gain_names(std::string) override
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
        double set_clock_rate(double) override
        {
            return 0.0;
        }
        double tune(std::string, double) override
        {
            return 0.0;
        }
        void set_dc_offset(std::string, double, double) override {}
        void set_iq_balance(std::string, double, double) override {}
        void set_filter(std::string, std::string, std::string) override {}
        std::map<std::string, std::vector<uint8_t>> get_user_eeprom_data() override
        {
            return {};
        }
        void set_user_eeprom_data(
            const std::map<std::string, std::vector<uint8_t>>&) override
        {
        }
        void update_ref_clock_freq(double) override {}
        void init_rfic(double) override {}
        void tear_down_rfic() override {}
        double set_freq(std::string, double freq, bool) override
        {
            return freq;
        }
        double get_gain(std::string) override
        {
            return 0.0;
        }
        double get_bandwidth(std::string) override
        {
            return 0.0;
        }
        double set_bandwidth(std::string, double bw) override
        {
            return bw;
        }
        std::string set_lo_source(std::string, std::string source) override
        {
            return source;
        }
        std::string get_lo_source(std::string) override
        {
            return "";
        }
        void set_fir(std::string, int32_t, const std::vector<int32_t>&) override {}
        std::pair<int32_t, std::vector<int32_t>> get_fir(std::string) override
        {
            return {0, {}};
        }
        bool get_ad9371_lo_lock(std::string) override
        {
            return false;
        }
        bool get_lowband_lo_lock(std::string) override
        {
            return false;
        }
        void enable_rx_lowband_lo(bool) override {}
        void enable_tx_lowband_lo(bool) override {}
        double set_master_clock_rate(double rate) override
        {
            return rate;
        }
        bool is_lo_dist_present() override
        {
            return false;
        }
        void enable_lo_export(std::string, bool) override {}
        void enable_lo_output(std::string, uint32_t, bool) override {}
        void set_dac_mux_enable(
            uint32_t /*channel*/, uint32_t /*enable*/, uint32_t /*mode*/) override
        {
        }
        void config_tx_path(std::string) override {}
        std::string get_tx_path() override
        {
            return "";
        }
        void config_rx_path(std::string) override {}
        std::string get_rx_path() override
        {
            return "";
        }

        std::vector<double> get_cal_coefs(uint32_t, uint32_t, uint32_t) override
        {
            return {0.0};
        }

        void set_cal_coefs(uint32_t /*channel*/,
            uint32_t /*cal_block*/,
            const std::vector<double>& /*coefs*/,
            uint32_t /*mode*/) override
        {
        }

        void enable_iq_swap(
            bool is_band_inverted, std::string trx, uint32_t chan) override
        {
            BOOST_REQUIRE(trx == "rx" || trx == "tx");
            BOOST_REQUIRE(chan < uhd::usrp::hbx::HBX_MAX_NUM_CHANS);
            _parent->iq_swap[trx][chan] = is_band_inverted;
        }

        void poke_db(uint32_t /*addr*/, uint32_t /*val*/) override {}
        std::string peek_db(uint32_t /*addr*/) override
        {
            return "0x0";
        }

    private:
        x4xx_hbx_mock_rpc_server* _parent;
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
    std::map<std::string, std::map<size_t, double>> nco_freq;
    std::map<std::string, std::map<size_t, bool>> iq_swap;
    std::map<std::string, size_t> data_path;
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
