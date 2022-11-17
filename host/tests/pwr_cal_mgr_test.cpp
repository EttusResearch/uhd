//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/database.hpp>
#include <uhd/cal/pwr_cal.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/utils/gain_group.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/pwr_cal_mgr.hpp>
#include <boost/test/unit_test.hpp>

using namespace uhd::usrp;

BOOST_AUTO_TEST_CASE(test_pwr_cal_mgr_static)
{
    BOOST_CHECK_EQUAL(pwr_cal_mgr::sanitize_antenna_name("TX/RX"), "tx+rx");
    BOOST_CHECK_EQUAL(pwr_cal_mgr::sanitize_antenna_name("RX2"), "rx2");
    BOOST_CHECK(pwr_cal_mgr::is_valid_antenna("RX2"));
    BOOST_CHECK(!pwr_cal_mgr::is_valid_antenna("CAL"));
}

BOOST_AUTO_TEST_CASE(test_pwr_cal_mgr_empty)
{
    const std::string MOCK_SERIAL = "MOCK_SERIAL";
    const std::string MOCK_KEY    = "MOCK_KEY";
    const std::string LOG_ID      = "TEST_MGR";
    auto gg                       = uhd::gain_group::make();
    gg->register_fcns("MAIN",
        {[&]() { return uhd::meta_range_t(0.0, 10.0); },
            [&]() { return 5.0; },
            [&](const double&) { ; }});

    auto mgr_dut = pwr_cal_mgr::make(
        MOCK_SERIAL,
        LOG_ID,
        [&]() { return 0.0; },
        [&]() -> std::string { return "MOCK_KEY"; },
        gg);

    BOOST_CHECK(!mgr_dut->has_power_data());
    auto tree = uhd::property_tree::make();
    mgr_dut->populate_subtree(tree);
    BOOST_CHECK(tree->exists("ref_power/key"));
    BOOST_CHECK(tree->exists("ref_power/serial"));
    BOOST_CHECK(!tree->exists("ref_power/value"));
    BOOST_CHECK(!tree->exists("ref_power/range"));
    BOOST_CHECK_EQUAL(tree->access<std::string>("ref_power/key").get(), MOCK_KEY);
    BOOST_CHECK_EQUAL(tree->access<std::string>("ref_power/serial").get(), MOCK_SERIAL);
    BOOST_CHECK_EQUAL(mgr_dut->get_serial(), MOCK_SERIAL);
    BOOST_CHECK_EQUAL(mgr_dut->get_key(), MOCK_KEY);

    auto gg2 = uhd::gain_group::make();
    gg2->register_fcns("MAIN",
        {[&]() { return uhd::meta_range_t(0.0, 10.0); },
            [&]() { return 3.0; },
            [&](const double&) { ; }});
    mgr_dut->set_gain_group(gg2);
    BOOST_REQUIRE_THROW(mgr_dut->set_power(0), uhd::runtime_error);
    BOOST_REQUIRE_THROW(mgr_dut->get_power(), uhd::runtime_error);
    BOOST_REQUIRE_THROW(mgr_dut->get_power_range(), uhd::runtime_error);
    // Should do nothing
    mgr_dut->update_power();
    mgr_dut->set_tracking_mode(pwr_cal_mgr::tracking_mode::TRACK_POWER);
    BOOST_REQUIRE_THROW(mgr_dut->update_power(), uhd::runtime_error);
    const std::string MOCK_SERIAL2 = "MOCK_SERIAL2";
    mgr_dut->set_serial(MOCK_SERIAL2);
    BOOST_CHECK_EQUAL(tree->access<std::string>("ref_power/serial").get(), MOCK_SERIAL2);
    BOOST_CHECK_EQUAL(mgr_dut->get_serial(), MOCK_SERIAL2);
}


BOOST_AUTO_TEST_CASE(test_pwr_cal_mgr_populated)
{
    const std::string MOCK_SERIAL = "MOCK_SERIAL";
    const std::string MOCK_KEY    = "MOCK_KEY";
    const std::string LOG_ID      = "TEST_MGR";
    constexpr double min_gain     = 0.0;
    constexpr double max_gain     = 10.0;
    double gain                   = 5.0; // Starting  value
    double freq                   = 1e9;
    auto gg                       = uhd::gain_group::make();
    gg->register_fcns("MAIN",
        {[&]() { return uhd::meta_range_t(min_gain, max_gain, 1.0); },
            [&]() { return gain; },
            [&](const double new_gain) { gain = new_gain; }});
    // Now we craft some mock cal data
    auto cal_data       = cal::pwr_cal::make("mock_cal_data", MOCK_SERIAL, 0xBAD71113);
    constexpr int TEMP1 = 20, TEMP2 = 30; // Some temperatures
    cal_data->add_power_table({{0.0, -30.0}, {10.0, -20.0}}, -40.0, -10.0, 1e9, TEMP1);
    cal_data->add_power_table({{0.0, -40.0}, {10.0, -30.0}}, -40.0, -10.0, 2e9, TEMP1);
    // Let's add another temperature TEMP2 with a weird power behaviour
    cal_data->add_power_table({{0.0, -40.0}, {10.0, -40.0}}, -40.0, -10.0, 1e9, TEMP2);

    // Insert the cal data into the database
    cal::database::register_lookup(
        [&](const std::string& key, const std::string& serial) {
            return key == MOCK_KEY && serial == MOCK_SERIAL;
        },
        [&](const std::string& key, const std::string& serial) {
            if (key == MOCK_KEY && serial == MOCK_SERIAL) {
                return cal_data->serialize();
            }
            throw uhd::assertion_error("");
        });
    // Now create the DUT
    auto mgr_dut = pwr_cal_mgr::make(
        MOCK_SERIAL,
        LOG_ID,
        [&]() { return freq; },
        [&]() -> std::string { return MOCK_KEY; },
        gg);
    mgr_dut->set_temperature(TEMP1);

    // OK let's go test some basics
    BOOST_CHECK(mgr_dut->has_power_data());
    auto tree = uhd::property_tree::make();
    mgr_dut->populate_subtree(tree);
    BOOST_CHECK(tree->exists("ref_power/key"));
    BOOST_CHECK(tree->exists("ref_power/serial"));
    BOOST_CHECK(tree->exists("ref_power/value"));
    BOOST_CHECK(tree->exists("ref_power/range"));
    BOOST_CHECK_EQUAL(tree->access<std::string>("ref_power/key").get(), MOCK_KEY);
    BOOST_CHECK_EQUAL(tree->access<std::string>("ref_power/serial").get(), MOCK_SERIAL);
    BOOST_CHECK_EQUAL(mgr_dut->get_serial(), MOCK_SERIAL);
    BOOST_CHECK_EQUAL(mgr_dut->get_key(), MOCK_KEY);

    // Now fiddle with the gains/power and see if they behave properly
    BOOST_CHECK_EQUAL(gain, 5.0);
    mgr_dut->set_power(-30.0); // -30 is the lower limit for this
    BOOST_CHECK_EQUAL(gain, 0.0);
    BOOST_CHECK_EQUAL(mgr_dut->get_power(), -30.0);
    gain = 10.0; // mgr_dut will read from this variable
    mgr_dut->update_power();
    BOOST_CHECK_EQUAL(gain, 0.0);
    auto range = mgr_dut->get_power_range();
    BOOST_CHECK_EQUAL(range.start(), -40.0);
    BOOST_CHECK_EQUAL(range.stop(), -10.0);
    // Fake-tune:
    UHD_LOG_INFO("TEST", "Doing mock tune...");
    freq = 2e9; // mgr_dut will read from this variable
    mgr_dut->update_power();
    BOOST_CHECK_EQUAL(mgr_dut->get_power(), -30.0);
    mgr_dut->set_temperature(TEMP2);
    mgr_dut->update_power();
    BOOST_CHECK_EQUAL(mgr_dut->get_power(), -40.0);
}
