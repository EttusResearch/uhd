//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/mb_controller.hpp>
#include <uhdlib/features/discoverable_feature_registry.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::rfnoc;

class mock_timekeeper : public mb_controller::timekeeper
{
public:
    uint64_t get_ticks_now() override
    {
        return _ticks;
    }

    uint64_t get_ticks_last_pps() override
    {
        return _ticks;
    }

    void set_ticks_now(const uint64_t ticks) override
    {
        _ticks = ticks;
    }

    void set_ticks_next_pps(const uint64_t ticks) override
    {
        _ticks = ticks;
    }

    uint64_t _ticks;
    uint64_t _period;

    void update_tick_rate(const double tick_rate)
    {
        set_tick_rate(tick_rate);
    }

private:
    void set_period(const uint64_t period_ns) override
    {
        _period = period_ns;
    }
};

class mock_mb_controller : public mb_controller,
                           public ::uhd::features::discoverable_feature_registry
{
public:
    mock_mb_controller()
    {
        auto tk = std::make_shared<mock_timekeeper>();
        register_timekeeper(0, tk);
    }

    /**************************************************************************
     * Motherboard Control API (see mb_controller.hpp)
     *************************************************************************/
    std::string get_mboard_name() const override
    {
        return "MOCK-MB";
    }

    void set_time_source(const std::string& source) override
    {
        time_source = source;
    }

    std::string get_time_source() const override
    {
        return time_source;
    }

    std::vector<std::string> get_time_sources() const override
    {
        return {"internal", "external"};
    }

    void set_clock_source(const std::string& source) override
    {
        clock_source = source;
    }

    std::string get_clock_source() const override
    {
        return clock_source;
    }

    std::vector<std::string> get_clock_sources() const override
    {
        return {"internal", "external"};
    }

    void set_sync_source(
        const std::string& /*clock_source*/, const std::string& /*time_source*/) override
    {
    }

    void set_sync_source(const device_addr_t& /*sync_source*/) override {}

    device_addr_t get_sync_source() const override
    {
        return {};
    }

    std::vector<device_addr_t> get_sync_sources() override
    {
        return {};
    }

    void set_clock_source_out(const bool enb) override
    {
        clock_source_out = enb;
    }

    void set_time_source_out(const bool enb) override
    {
        time_source_out = enb;
    }

    sensor_value_t get_sensor(const std::string& /*name*/) override
    {
        return sensor_value_t("Ref", false, "locked", "unlocked");
    }

    std::vector<std::string> get_sensor_names() override
    {
        return {"mock_sensor"};
    }

    uhd::usrp::mboard_eeprom_t get_eeprom() override
    {
        return {};
    }

    std::string clock_source = "internal";
    std::string time_source  = "internal";
    bool clock_source_out    = false;
    bool time_source_out     = false;
};

BOOST_AUTO_TEST_CASE(test_mb_controller)
{
    auto mmbc = std::make_shared<mock_mb_controller>();

    BOOST_REQUIRE_EQUAL(mmbc->get_num_timekeepers(), 1);
    auto tk      = mmbc->get_timekeeper(0);
    auto tk_mock = std::dynamic_pointer_cast<mock_timekeeper>(tk);
    BOOST_REQUIRE(tk);

    constexpr double TICK_RATE = 200e6;
    constexpr double PERIOD_NS = 5;
    // This will call set_tick_rate() and thus set_period()
    tk_mock->update_tick_rate(TICK_RATE);
    BOOST_CHECK_EQUAL(tk->get_tick_rate(), TICK_RATE);
    BOOST_CHECK_EQUAL(tk_mock->_period, PERIOD_NS * (uint64_t(1) << 32));

    constexpr double TIME_0 = 1.0;
    tk->set_time_now(uhd::time_spec_t(TIME_0));
    BOOST_CHECK_EQUAL(tk->get_ticks_now(), TICK_RATE * TIME_0);
    constexpr double TIME_1 = 0.5;
    tk->set_time_next_pps(uhd::time_spec_t(TIME_1));
    BOOST_CHECK_EQUAL(tk->get_ticks_last_pps(), TIME_1 * TICK_RATE);
}
