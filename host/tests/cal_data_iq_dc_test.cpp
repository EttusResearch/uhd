//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/iq_dc_cal.hpp>
#include <uhd/exception.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::usrp::cal;
using namespace uhd::math;

BOOST_AUTO_TEST_CASE(test_iq_dc_cal_api)
{
    const std::string name   = "Mock IQ DC Data";
    const std::string serial = "ABC1234";
    const uint64_t timestamp = 0x12340000;

    auto cal_data = iq_dc_cal::make(name, serial, timestamp);
    BOOST_CHECK_EQUAL(cal_data->get_name(), name);
    BOOST_CHECK_EQUAL(cal_data->get_serial(), serial);
    BOOST_CHECK_EQUAL(cal_data->get_timestamp(), timestamp);

    // Nothing there yet
    BOOST_REQUIRE_THROW(cal_data->get_cal_coeff(0.0), uhd::assertion_error);

    // Set some coefficients: freq, scaling_factor, icross, qinline, delay,
    // dc_offset_real, dc_offset_imag
    cal_data->set_cal_coeff(1e9, 1.0, {0.1, 0.2}, {0.3, 0.4}, 5.0, 0.01, 0.02);
    cal_data->set_cal_coeff(2e9, 2.0, {0.5, 0.6}, {0.7, 0.8}, 10.0, 0.03, 0.04);

    // Linear interpolation mode (default): test below range
    auto lo_coeff = cal_data->get_cal_coeff(0.5e9);
    BOOST_CHECK_EQUAL(lo_coeff.scaling_coeff, 1.0);
    BOOST_CHECK_EQUAL(lo_coeff.coeffs.size(), 2u);
    BOOST_CHECK_EQUAL(lo_coeff.coeffs[0], std::complex<double>(0.1, 0.3));
    BOOST_CHECK_EQUAL(lo_coeff.coeffs[1], std::complex<double>(0.2, 0.4));
    BOOST_CHECK_EQUAL(lo_coeff.group_delay, 5.0);
    BOOST_CHECK_EQUAL(lo_coeff.dc_offset, std::complex<double>(0.01, 0.02));

    // Test above range
    auto hi_coeff = cal_data->get_cal_coeff(2.5e9);
    BOOST_CHECK_EQUAL(hi_coeff.scaling_coeff, 2.0);
    BOOST_CHECK_EQUAL(hi_coeff.coeffs.size(), 2u);
    BOOST_CHECK_EQUAL(hi_coeff.coeffs[0], std::complex<double>(0.5, 0.7));
    BOOST_CHECK_EQUAL(hi_coeff.coeffs[1], std::complex<double>(0.6, 0.8));
    BOOST_CHECK_EQUAL(hi_coeff.group_delay, 10.0);
    BOOST_CHECK_EQUAL(hi_coeff.dc_offset, std::complex<double>(0.03, 0.04));

    // Test midpoint interpolation (linear)
    auto mid_coeff = cal_data->get_cal_coeff(1.5e9);
    BOOST_CHECK_CLOSE(mid_coeff.scaling_coeff, 1.5, 1e-6);
    BOOST_CHECK_EQUAL(mid_coeff.coeffs.size(), 2u);
    BOOST_CHECK_CLOSE(mid_coeff.coeffs[0].real(), 0.3, 1e-6);
    BOOST_CHECK_CLOSE(mid_coeff.coeffs[0].imag(), 0.5, 1e-6);
    BOOST_CHECK_CLOSE(mid_coeff.coeffs[1].real(), 0.4, 1e-6);
    BOOST_CHECK_CLOSE(mid_coeff.coeffs[1].imag(), 0.6, 1e-6);
    BOOST_CHECK_CLOSE(mid_coeff.group_delay, 7.5, 1e-6);
    BOOST_CHECK_CLOSE(mid_coeff.dc_offset.real(), 0.02, 1e-6);
    BOOST_CHECK_CLOSE(mid_coeff.dc_offset.imag(), 0.03, 1e-6);

    // Test get_group_delay
    BOOST_CHECK_CLOSE(cal_data->get_group_delay(1.5e9), 7.5, 1e-6);
    BOOST_CHECK_EQUAL(cal_data->get_group_delay(0.5e9), 5.0);
    BOOST_CHECK_EQUAL(cal_data->get_group_delay(2.5e9), 10.0);

    // Switch to nearest-neighbor
    cal_data->set_interp_mode(interp_mode::NEAREST_NEIGHBOR);

    // Below midpoint -> nearest to 1e9
    auto nn_lo = cal_data->get_cal_coeff(1.25e9);
    BOOST_CHECK_EQUAL(nn_lo.scaling_coeff, 1.0);
    BOOST_CHECK_EQUAL(nn_lo.coeffs[0], std::complex<double>(0.1, 0.3));
    BOOST_CHECK_EQUAL(nn_lo.coeffs[1], std::complex<double>(0.2, 0.4));
    BOOST_CHECK_EQUAL(nn_lo.group_delay, 5.0);
    // DC offset is always interpolated
    BOOST_CHECK_CLOSE(nn_lo.dc_offset.real(), 0.015, 1e-6);
    BOOST_CHECK_CLOSE(nn_lo.dc_offset.imag(), 0.025, 1e-6);

    // Above midpoint -> nearest to 2e9
    auto nn_hi = cal_data->get_cal_coeff(1.75e9);
    BOOST_CHECK_EQUAL(nn_hi.scaling_coeff, 2.0);
    BOOST_CHECK_EQUAL(nn_hi.coeffs[0], std::complex<double>(0.5, 0.7));
    BOOST_CHECK_EQUAL(nn_hi.coeffs[1], std::complex<double>(0.6, 0.8));
    BOOST_CHECK_EQUAL(nn_hi.group_delay, 10.0);
    // DC offset is always interpolated
    BOOST_CHECK_CLOSE(nn_hi.dc_offset.real(), 0.025, 1e-6);
    BOOST_CHECK_CLOSE(nn_hi.dc_offset.imag(), 0.035, 1e-6);

    // Test clear
    cal_data->clear();
    BOOST_REQUIRE_THROW(cal_data->get_cal_coeff(0.0), uhd::assertion_error);
}

BOOST_AUTO_TEST_CASE(test_iq_dc_cal_serdes)
{
    const std::string name   = "Mock IQ DC Data";
    const std::string serial = "ABC1234";
    const uint64_t timestamp = 0x12340000;

    auto cal_data_blueprint = iq_dc_cal::make(name, serial, timestamp);
    for (double d = 0; d < 5.0; d += 1.0) {
        cal_data_blueprint->set_cal_coeff(d * 1e9, // freq
            d + 1.0, // scaling_factor
            {d, d + 0.1}, // icross
            {d + 0.2, d + 0.3}, // qinline
            d * 2.0, // delay
            d * 0.01, // dc_offset_real
            d * 0.02 // dc_offset_imag
        );
    }

    const auto serialized = cal_data_blueprint->serialize();

    auto cal_data = container::make<iq_dc_cal>(serialized);

    BOOST_CHECK_EQUAL(cal_data->get_name(), name);
    BOOST_CHECK_EQUAL(cal_data->get_serial(), serial);
    BOOST_CHECK_EQUAL(cal_data->get_timestamp(), timestamp);

    cal_data->set_interp_mode(interp_mode::NEAREST_NEIGHBOR);
    for (double d = 0; d < 5.0; d += 1.0) {
        auto coeff = cal_data->get_cal_coeff(d * 1e9);
        BOOST_CHECK_CLOSE(coeff.scaling_coeff, d + 1.0, 1e-6);
        BOOST_CHECK_EQUAL(coeff.coeffs.size(), 2u);
        BOOST_CHECK_CLOSE(coeff.coeffs[0].real(), d, 1e-6);
        BOOST_CHECK_CLOSE(coeff.coeffs[0].imag(), d + 0.2, 1e-6);
        BOOST_CHECK_CLOSE(coeff.coeffs[1].real(), d + 0.1, 1e-6);
        BOOST_CHECK_CLOSE(coeff.coeffs[1].imag(), d + 0.3, 1e-6);
        BOOST_CHECK_CLOSE(coeff.group_delay, d * 2.0, 1e-6);
        // For d == 0, dc_offset is (0,0) -- use CHECK_SMALL instead
        if (d > 0) {
            BOOST_CHECK_CLOSE(coeff.dc_offset.real(), d * 0.01, 1e-6);
            BOOST_CHECK_CLOSE(coeff.dc_offset.imag(), d * 0.02, 1e-6);
        } else {
            BOOST_CHECK_SMALL(coeff.dc_offset.real(), 1e-12);
            BOOST_CHECK_SMALL(coeff.dc_offset.imag(), 1e-12);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_iq_dc_cal_des_fail)
{
    std::vector<uint8_t> not_actual_data(42, 23);

    BOOST_REQUIRE_THROW(container::make<iq_dc_cal>(not_actual_data), uhd::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_iq_dc_cal_default_factory)
{
    auto cal_data = iq_dc_cal::make();
    // Default factory should create a valid empty object
    BOOST_REQUIRE_THROW(cal_data->get_cal_coeff(0.0), uhd::assertion_error);

    // Can add data and retrieve it
    cal_data->set_cal_coeff(1e9, 1.5, {0.1}, {0.2}, 3.0, 0.01, 0.02);
    auto coeff = cal_data->get_cal_coeff(1e9);
    BOOST_CHECK_CLOSE(coeff.scaling_coeff, 1.5, 1e-6);
    BOOST_CHECK_EQUAL(coeff.coeffs.size(), 1u);
    BOOST_CHECK_EQUAL(coeff.coeffs[0], std::complex<double>(0.1, 0.2));
    BOOST_CHECK_CLOSE(coeff.group_delay, 3.0, 1e-6);
    BOOST_CHECK_CLOSE(coeff.dc_offset.real(), 0.01, 1e-6);
    BOOST_CHECK_CLOSE(coeff.dc_offset.imag(), 0.02, 1e-6);
}
