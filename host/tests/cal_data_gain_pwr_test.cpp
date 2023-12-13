//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/pwr_cal.hpp>
#include <uhd/exception.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::usrp::cal;

BOOST_AUTO_TEST_CASE(test_pwr_cal_api)
{
    const std::string name   = "Mock Gain/Power Data";
    const std::string serial = "ABC1234";
    const uint64_t timestamp = 0x12340000;

    auto gain_power_data = pwr_cal::make(name, serial, timestamp);
    BOOST_CHECK_EQUAL(gain_power_data->get_name(), name);
    BOOST_CHECK_EQUAL(gain_power_data->get_serial(), serial);
    BOOST_CHECK_EQUAL(gain_power_data->get_timestamp(), timestamp);

    constexpr int NEW_TEMP = 25;
    gain_power_data->set_temperature(NEW_TEMP);
    BOOST_CHECK_EQUAL(gain_power_data->get_temperature(), NEW_TEMP);

    // Write a table...
    gain_power_data->add_power_table({{0.0, -30.0}, {10.0, -20.0}}, -40.0, -10.0, 1e9);
    // ...read it back...
    BOOST_CHECK_EQUAL(gain_power_data->get_power(0.0, 1e9), -30.0);
    BOOST_CHECK_EQUAL(gain_power_data->get_power(10.0, 1e9), -20.0);
    // ...and make sure any other index will do the same
    BOOST_CHECK_EQUAL(gain_power_data->get_power(47.2, 23e6), -20.0);

    // Let's say power goes down 10 dB per octave
    gain_power_data->add_power_table({{0.0, -40.0}, {10.0, -30.0}}, -40.0, -10.0, 2e9);
    // Do uninterpolated readbacks again
    BOOST_CHECK_EQUAL(gain_power_data->get_power(10.0, 1e9), -20.0);
    BOOST_CHECK_EQUAL(gain_power_data->get_power(47.2, 23e6), -20.0);
    // Now interpolate
    BOOST_CHECK_CLOSE(gain_power_data->get_power(5.0, 1.5e9), -30.0, 1e-6);

    // Some ref gain checks
    BOOST_CHECK_EQUAL(gain_power_data->get_ref_gain(), 0.0);
    gain_power_data->set_ref_gain(5.0);
    BOOST_CHECK_EQUAL(gain_power_data->get_ref_gain(), 5.0);

    // Clear, make sure the container doesn't work anymore, and add the value
    // back for future tests
    gain_power_data->clear();
    BOOST_REQUIRE_THROW(gain_power_data->get_power(47.2, 23e6), uhd::exception);
    gain_power_data->add_power_table({{10.0, -20.0}}, -20.0, -20.0, 1e9);

    constexpr int ROOM_TEMP = 20;
    gain_power_data->set_temperature(ROOM_TEMP);
    BOOST_CHECK_EQUAL(gain_power_data->get_temperature(), ROOM_TEMP);
    // Shouldn't change anything yet
    BOOST_CHECK_EQUAL(gain_power_data->get_power(10.0, 1e9), -20.0);
    // Power shall go down 5 dB at this new temperature (whoa)
    gain_power_data->add_power_table({{10.0, -25.0}}, -20.0, -20.0, 1e9);
    BOOST_CHECK_EQUAL(gain_power_data->get_power(10.0, 1e9), -25.0);
    // And if we have to interpolate, temp compensation uses NN:
    BOOST_CHECK_EQUAL(gain_power_data->get_power(10.0, 1e9, 21), -25.0);

    // Now clear, then add a more useful data set. We can stay at ROOM_TEMP.
    gain_power_data->clear();
    constexpr double power_offset = -20.0; // 0 dB shall map to -20 dBm
    constexpr double lin_error    = 0.1; // The linearization error shall increase by 0.1
                                      // dB per dB gain
    constexpr double ref_freq = 1e9;
    std::map<double, double> test_gain_power;
    constexpr double max_gain = 10.0;
    for (double gain = 0.0; gain <= max_gain; gain += 1.0) {
        test_gain_power[gain] = gain + power_offset + lin_error * gain;
    }
    const double min_power = test_gain_power.cbegin()->second;
    const double max_power = test_gain_power.crbegin()->second;
    gain_power_data->add_power_table(test_gain_power, min_power, max_power, ref_freq);
    // Quick check
    BOOST_CHECK_EQUAL(gain_power_data->get_power(0.0, ref_freq), power_offset);
    const std::pair<double, double> expected_limits{min_power, max_power};
    const auto limits = gain_power_data->get_power_limits(ref_freq);
    BOOST_CHECK_EQUAL(limits.start(), min_power);
    BOOST_CHECK_EQUAL(limits.stop(), max_power);

    BOOST_CHECK_CLOSE(gain_power_data->get_gain(-20.0, ref_freq), 0.0, 1e-6);
    BOOST_CHECK_CLOSE(
        gain_power_data->get_gain(test_gain_power.crbegin()->second, ref_freq),
        test_gain_power.crbegin()->first,
        1e-6);
    BOOST_CHECK_CLOSE(gain_power_data->get_gain(-19.0, ref_freq), 1 / 1.1, 1e-6);
}

BOOST_AUTO_TEST_CASE(test_pwr_cal_api_irreg)
{
    const std::string name   = "Mock Gain/Power Data";
    const std::string serial = "ABC1234";
    const uint64_t timestamp = 0x12340000;

    auto gain_power_data    = pwr_cal::make(name, serial, timestamp);
    constexpr int ROOM_TEMP = 20;
    gain_power_data->set_temperature(ROOM_TEMP);

    constexpr double MIN_POWER = -40;
    constexpr double MAX_POWER = -10;
    // Write a table...
    gain_power_data->add_power_table(
        {{0.0, -30.0}, {10.0, -20.0}}, MIN_POWER, MAX_POWER, 1e9);
    // Let's say power goes down 10 dB per octave
    gain_power_data->add_power_table(
        {{0.0, -40.0}, {10.0, -30.0}}, MIN_POWER, MAX_POWER, 2e9);
    // Interpolated readback:
    BOOST_CHECK_CLOSE(gain_power_data->get_power(5.0, 1.5e9), -30.0, 1e-6);
    BOOST_CHECK_CLOSE(gain_power_data->get_gain(-30.0, 1.5e9), 5.0, 0.1);
}

BOOST_AUTO_TEST_CASE(test_pwr_cal_serdes)
{
    const std::string name         = "Mock Gain/Power Data";
    const std::string serial       = "ABC1234";
    const uint64_t timestamp       = 0x12340000;
    auto gain_power_data_blueprint = pwr_cal::make(name, serial, timestamp);

    constexpr double power_offset = -20.0;
    constexpr double lin_error    = 0.1;
    constexpr double ref_freq     = 1e9;
    std::map<double, double> test_gain_power;
    for (double gain = 0.0; gain < 10.0; gain += 1.0) {
        test_gain_power[gain] = gain + power_offset + lin_error * gain;
    }
    const double min_power = test_gain_power[0.0];
    const double max_power = test_gain_power[9.0];
    gain_power_data_blueprint->add_power_table(
        test_gain_power, min_power, max_power, ref_freq);

    const auto serialized = gain_power_data_blueprint->serialize();
    auto pwr_cal_data     = container::make<pwr_cal>(serialized);

    BOOST_CHECK_EQUAL(pwr_cal_data->get_name(), name);
    BOOST_CHECK_EQUAL(pwr_cal_data->get_serial(), serial);
    BOOST_CHECK_EQUAL(pwr_cal_data->get_timestamp(), timestamp);

    for (auto& gp : test_gain_power) {
        BOOST_CHECK_EQUAL(pwr_cal_data->get_power(gp.first, ref_freq), gp.second);
    }
}

BOOST_AUTO_TEST_CASE(test_pwr_cal_des_fail)
{
    std::vector<uint8_t> not_actual_data(42, 23);

    BOOST_REQUIRE_THROW(container::make<pwr_cal>(not_actual_data), uhd::runtime_error);
}
