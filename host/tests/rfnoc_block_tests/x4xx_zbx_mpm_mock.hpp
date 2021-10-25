//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/device_addr.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_constants.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_dboard.hpp>
#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace uhd::usrp;

namespace uhd { namespace test {

namespace {
constexpr double DEFAULT_MCR = 122.88e6;
}
//! Mock MPM server for X410/ZBX
//
// This is a mock server that mimicks an X410 with a ZBX daughterboard.
class x4xx_mock_rpc_server : public x400_rpc_iface, public mpmd_rpc_iface, public dboard_base_rpc_iface, public zbx_rpc_iface, public dio_rpc_iface
{
public:
    x4xx_mock_rpc_server(const uhd::device_addr_t& device_info)
        : _device_info(device_info)
    {}

    uhd::rpc_client::sptr get_raw_rpc_client() override
    {
        // This function is unimplemented! Perhaps you need to:
        // - Add it to the appropriate RPC interface,
        // - Retrofit all calls to your desired function to directly use the RPC interface, and
        // - Add a mock implementation here.
        UHD_THROW_INVALID_CODE_PATH();
    }

    /**************************************************************************
     * RPC Call Mockups
     *
     * The following public methods are replacements of that normally happens in
     * the Python-based MPM. Some notes on writing mocks:
     * - These are mocks, so don't go fancy and only let them do the bare
     *   minimum required for tests
     * - Remember to add them to _init_rpc() further down
     *************************************************************************/
    size_t get_num_timekeepers() override
    {
        return 1;
    }

    std::vector<std::string> get_mb_sensors() override
    {
        return {"ref_locked"};
    }

    std::vector<std::string> get_gpio_banks() override
    {
        return {};
    }

    bool supports_feature(const std::string& feature) override
    {
        return feature == "ref_clk_calibration";
    }

    std::vector<std::map<std::string, std::string>> get_dboard_info() override
    {
        return {{
            // One entry per dboard info
            {"pid", std::to_string(uhd::usrp::zbx::ZBX_PID)}
            // End of entries
        }};
    }

    bool is_db_gpio_ifc_present(const size_t) override
    {
        return true;
    }

    void set_tick_period(const size_t, const uint64_t) override
    {
        // nop
    }

    double get_master_clock_rate() override
    {
        return _device_info.cast<double>("master_clock_rate", DEFAULT_MCR);
    }

    std::vector<std::string> get_sensors(const std::string&) override
    {
        return {};
    }

    std::map<std::string, std::string> get_sensor(const std::string&, const std::string&, size_t) override
    {
        return {};
    }

    void set_cal_frozen(bool, size_t, size_t) override
    {
        // nop
    }

    std::vector<int> get_cal_frozen(size_t, size_t) override
    {
        return {};
    }

    std::map<std::string, std::vector<uint8_t>> get_db_eeprom(const size_t) override
    {
        return {{
            // One line per entry
            {"pid", s2u8("mock")}, // Used to specify power cal API
            {"serial", s2u8("BADCODE")}
            // End of entries
        }};
    }

    double get_dboard_prc_rate() override
    {
        const double mcr = _device_info.cast<double>("master_clock_rate", DEFAULT_MCR);
        static const std::map<double, double> prc_map{
            {122.88e6, 61.44e6}, {125e6, 62.5e6}};
        return prc_map.at(mcr);
    }

    double rfdc_set_nco_freq(const std::string& trx,
        const size_t /*db_id*/,
        const size_t chan,
        const double freq) override
    {
        BOOST_REQUIRE(trx == "rx" || trx == "tx");
        BOOST_REQUIRE(chan < uhd::usrp::zbx::ZBX_NUM_CHANS);
        nco_freq[trx][chan] = freq;
        return freq;
    }

    double rfdc_get_nco_freq(
        const std::string& trx, const size_t /*db_id*/, const size_t chan) override
    {
        BOOST_REQUIRE(trx == "rx" || trx == "tx");
        BOOST_REQUIRE(chan < uhd::usrp::zbx::ZBX_NUM_CHANS);
        // On construction, the expert will ask for the current nco frequency, and our
        // nco_freq map won't have a value yet.
        if (nco_freq.find(trx) == nco_freq.end()
            || nco_freq.at(trx).find(chan) == nco_freq.at(trx).end()) {
            return 0;
        }
        return nco_freq.at(trx).at(chan);
    }

    double get_dboard_sample_rate() override
    {
        const double mcr = _device_info.cast<double>("master_clock_rate", DEFAULT_MCR);
        static const std::map<double, double> spll_map{
            // One line per entry
            {122.88e6, 2.94912e9},
            {122.88e6 * 4, 2.94912e9}
            // End of entries
        };
        return spll_map.at(mcr);
    }

    void enable_iq_swap(const bool,
        const std::string&,
        const size_t) override
    {
        // nop
    }

    std::vector<std::string> get_gpio_srcs(const std::string& /*bank*/) override
    {
        return {};
    }

    uint64_t get_timekeeper_time(size_t /*timekeeper_idx*/, bool /*last_pps*/) override
    {
        return 0;
    }

    void set_timekeeper_time(size_t /*timekeeper_idx*/, uint64_t /*ticks*/, bool /*last_pps*/) override
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

    void set_trigger_io(const std::string& /*direction*/) override
    {
        // nop
    }

    std::map<std::string, std::string> get_mb_eeprom() override
    {
        return {};
    }

    std::vector<std::string> get_gpio_src(const std::string& /*bank*/) override
    {
        return {};
    }

    void set_gpio_src(const std::string& /*bank*/, const std::vector<std::string>& /*src*/) override
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

    sensor_value_t::sensor_map_t get_mb_sensor(const std::string& /*sensor*/) override
    {
        return {};
    }

    void set_time_source(const std::string& /*source*/) override
    {
        // nop
    }

    void set_clock_source(const std::string& /*source*/) override
    {
        // nop
    }

    void set_sync_source(const std::map<std::string, std::string>& /*source*/) override
    {
        // nop
    }

    bool get_threshold_status(size_t /*db_number*/, size_t /*chan*/, size_t /*threshold_block*/) override
    {
        return false;
    }

    void set_dac_mux_enable(size_t /*motherboard_channel_number*/, int /*enable*/) override
    {
        // nop
    }

    void set_dac_mux_data(size_t /*i*/, size_t /*q*/) override
    {
        // nop
    }

    double get_spll_freq() override
    {
        return 0.0;
    }

    void setup_threshold(
        size_t /*db_number*/,
        size_t /*chan*/,
        size_t /*threshold_block*/,
        const std::string& /*mode*/,
        size_t /*delay*/,
        size_t /*under*/,
        size_t /*over*/) override
    {
        // nop
    }

    std::vector<std::string> dio_get_supported_voltage_levels(const std::string&) override
    {
        return {"OFF", "1V8", "2V5", "3V3"};
    }

    void dio_set_voltage_level(const std::string&, const std::string&) override
    {
        // nop
    }

    std::string dio_get_voltage_level(const std::string&) override
    {
        return "3V3";
    }

    void dio_set_port_mapping(const std::string&) override
    {
        // nop
    }

    void dio_set_pin_directions(const std::string&, uint32_t) override
    {
        // nop
    }

    void dio_set_external_power(const std::string&, bool) override
    {
        // nop
    }

    std::string dio_get_external_power_state(const std::string&) override
    {
        return "OFF";
    }

    ///////////////////////////////////////////////////////////////////////////
    // Public attributes for easy inspection
    //
    // Use this in the mock functions to cache values, or expose values that get
    // tested later
    std::map<std::string, std::map<size_t, double>> if2_freq;
    std::map<std::string, std::map<size_t, double>> nco_freq;
    //
    ///////////////////////////////////////////////////////////////////////////

private:
    uhd::device_addr_t _device_info;

    static std::vector<uint8_t> s2u8(const std::string& s)
    {
        return std::vector<uint8_t>(s.begin(), s.end());
    }
};

}} // namespace uhd::test
