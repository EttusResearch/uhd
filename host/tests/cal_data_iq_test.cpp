//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/iq_cal.hpp>
#include <uhd/exception.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::usrp::cal;
using namespace uhd::math;

BOOST_AUTO_TEST_CASE(test_iq_cal_api)
{
    const std::string name   = "Mock IQ Data";
    const std::string serial = "ABC1234";
    const uint64_t timestamp = 0x12340000;

    auto iq_cal_data = iq_cal::make(name, serial, timestamp);
    BOOST_CHECK_EQUAL(iq_cal_data->get_name(), name);
    BOOST_CHECK_EQUAL(iq_cal_data->get_serial(), serial);
    BOOST_CHECK_EQUAL(iq_cal_data->get_timestamp(), timestamp);
    // Nothing there yet
    BOOST_REQUIRE_THROW(iq_cal_data->get_cal_coeff(0.0), uhd::assertion_error);

    iq_cal_data->set_cal_coeff(1.0, {1.0, 1.0});
    iq_cal_data->set_cal_coeff(2.0, {2.0, 2.0});

    // We're in linear interpolation mode
    auto lo_coeff = iq_cal_data->get_cal_coeff(0.5);
    auto hi_coeff = iq_cal_data->get_cal_coeff(2.5);
    BOOST_CHECK_EQUAL(lo_coeff, std::complex<double>(1.0, 1.0));
    BOOST_CHECK_EQUAL(hi_coeff, std::complex<double>(2.0, 2.0));
    auto mid_coeff = iq_cal_data->get_cal_coeff(1.75);
    BOOST_CHECK_EQUAL(mid_coeff, std::complex<double>(1.75, 1.75));

    iq_cal_data->set_interp_mode(interp_mode::NEAREST_NEIGHBOR);
    lo_coeff = iq_cal_data->get_cal_coeff(0.5);
    hi_coeff = iq_cal_data->get_cal_coeff(2.5);
    BOOST_CHECK_EQUAL(lo_coeff, std::complex<double>(1.0, 1.0));
    BOOST_CHECK_EQUAL(hi_coeff, std::complex<double>(2.0, 2.0));
    mid_coeff = iq_cal_data->get_cal_coeff(1.75);
    BOOST_CHECK_EQUAL(mid_coeff, std::complex<double>(2.0, 2.0));

    iq_cal_data->clear();
    BOOST_REQUIRE_THROW(iq_cal_data->get_cal_coeff(0.0), uhd::assertion_error);
}

BOOST_AUTO_TEST_CASE(test_iq_cal_serdes)
{
    const std::string name   = "Mock IQ Data";
    const std::string serial = "ABC1234";
    const uint64_t timestamp = 0x12340000;

    auto iq_cal_data_blueprint = iq_cal::make(name, serial, timestamp);
    for (double d = 0; d < 5.0; d += 1.0) {
        iq_cal_data_blueprint->set_cal_coeff(d, {d, d}, d * 10, d * 20);
    }

    const auto serialized = iq_cal_data_blueprint->serialize();

    auto iq_cal_data = container::make<iq_cal>(serialized);

    BOOST_CHECK_EQUAL(iq_cal_data->get_name(), name);
    BOOST_CHECK_EQUAL(iq_cal_data->get_serial(), serial);
    BOOST_CHECK_EQUAL(iq_cal_data->get_timestamp(), timestamp);

    iq_cal_data->set_interp_mode(interp_mode::NEAREST_NEIGHBOR);
    for (double d = 0; d < 5.0; d += 1.0) {
        BOOST_CHECK_EQUAL(iq_cal_data->get_cal_coeff(d), std::complex<double>(d, d));
    }
}

BOOST_AUTO_TEST_CASE(test_iq_cal_des_fail)
{
    std::vector<uint8_t> not_actual_data(42, 23);

    // FIXME: Re-enable this test when the verification works
    // BOOST_REQUIRE_THROW(container::make<iq_cal>(not_actual_data), uhd::runtime_error);
}
