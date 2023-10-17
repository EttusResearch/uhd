//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/dsa_cal.hpp>
#include <uhd/exception.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <iostream>

using namespace uhd::usrp::cal;

BOOST_AUTO_TEST_CASE(test_pwr_cal_api)
{
    const std::string name   = "Mock Gain/Power Data";
    const std::string serial = "ABC1234";
    const uint64_t timestamp = 0x12340000;

    auto dsa_data = zbx_tx_dsa_cal::make(name, serial, timestamp);
    BOOST_CHECK_EQUAL(dsa_data->get_name(), name);
    BOOST_CHECK_EQUAL(dsa_data->get_serial(), serial);
    BOOST_CHECK_EQUAL(dsa_data->get_timestamp(), timestamp);

    BOOST_REQUIRE_THROW(dsa_data->get_dsa_setting(0, 0), uhd::runtime_error);

    std::array<std::array<uint32_t, 3>, 61> gains1{{{1, 2}, {3, 4}, {5, 6}}};
    std::array<std::array<uint32_t, 3>, 61> gains2{{{7, 8}, {9, 0}, {1, 2}}};

    dsa_data->add_frequency_band(1E9, "low", gains1);
    dsa_data->add_frequency_band(4E9, "high", gains2);

    auto expected   = gains1[0];
    auto calculated = dsa_data->get_dsa_setting(1E9, 0);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        calculated.begin(), calculated.end(), expected.begin(), expected.end());

    calculated = dsa_data->get_dsa_setting(1E8, 0);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        calculated.begin(), calculated.end(), expected.begin(), expected.end());

    expected   = gains1[1];
    calculated = dsa_data->get_dsa_setting(1E9, 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        calculated.begin(), calculated.end(), expected.begin(), expected.end());

    calculated = dsa_data->get_dsa_setting(1E8, 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        calculated.begin(), calculated.end(), expected.begin(), expected.end());

    expected   = gains2[1];
    calculated = dsa_data->get_dsa_setting(3E9, 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        calculated.begin(), calculated.end(), expected.begin(), expected.end());

    calculated = dsa_data->get_dsa_setting(4E9, 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        calculated.begin(), calculated.end(), expected.begin(), expected.end());

    BOOST_REQUIRE_THROW(dsa_data->get_dsa_setting(5E9, 1), uhd::value_error);
}

BOOST_AUTO_TEST_CASE(test_pwr_cal_serdes)
{
    const std::string name   = "Mock Gain/DSA Data";
    const std::string serial = "FOOBAR";
    const uint64_t timestamp = 0xCAFEBABE;
    auto cal_data            = zbx_tx_dsa_cal::make(name, serial, timestamp);

    std::vector<double> freqs{2e9, 6e9};

    int i = 0;
    for (auto freq : freqs) {
        std::array<std::array<uint32_t, 3>, 61> gains;
        for (auto& gain : gains) {
            for (auto& step : gain) {
                step = i++;
            }
        }
        cal_data->add_frequency_band(freq, std::to_string(freq), gains);
    }

    const auto serialized = cal_data->serialize();
    BOOST_REQUIRE_THROW(container::make<zbx_rx_dsa_cal>(serialized), uhd::runtime_error);
    auto des_cal_data = container::make<zbx_tx_dsa_cal>(serialized);
    BOOST_CHECK_EQUAL(des_cal_data->get_name(), cal_data->get_name());
    BOOST_CHECK_EQUAL(des_cal_data->get_serial(), cal_data->get_serial());
    BOOST_CHECK_EQUAL(des_cal_data->get_timestamp(), cal_data->get_timestamp());
}

BOOST_AUTO_TEST_CASE(test_pwr_cal_des_fail)
{
    std::vector<uint8_t> not_actual_data(42, 23);

    BOOST_REQUIRE_THROW(
        container::make<zbx_tx_dsa_cal>(not_actual_data), uhd::runtime_error);
}
