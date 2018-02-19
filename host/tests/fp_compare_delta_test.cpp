//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/math.hpp>
#include <boost/test/unit_test.hpp>

using namespace uhd::math::fp_compare;

BOOST_AUTO_TEST_CASE(fp_compare_delta_constructors) {
    // Test default constructor
    fp_compare_delta<float> alpha = fp_compare_delta<float>(7457392.0);
    fp_compare_delta<float> beta = fp_compare_delta<float>(alpha._value);
    BOOST_CHECK_EQUAL(alpha._value, beta._value);
    BOOST_CHECK_EQUAL(alpha._delta, beta._delta);

    // Test constructor with specified delta
    fp_compare_delta<float> foxtrot = fp_compare_delta<float>(alpha._value,
            uhd::math::SINGLE_PRECISION_DELTA);
    fp_compare_delta<float> gamma = fp_compare_delta<float>(alpha._value,
            2 * uhd::math::SINGLE_PRECISION_DELTA);
    BOOST_CHECK_EQUAL(alpha._delta, foxtrot._delta);
    BOOST_CHECK(not (alpha._delta == gamma._delta));

    // Test copy-constructor
    fp_compare_delta<float> charlie = fp_compare_delta<float>(alpha);
    BOOST_CHECK_EQUAL(alpha._value, charlie._value);
    BOOST_CHECK_EQUAL(alpha._delta, charlie._delta);

    // Test assignment operator
    fp_compare_delta<float> delta = beta;
    BOOST_CHECK_EQUAL(alpha._value, delta._value);
    BOOST_CHECK_EQUAL(alpha._delta, delta._delta);
}

BOOST_AUTO_TEST_CASE(double_compare_constructors) {
    // Test default constructor
    fp_compare_delta<double> alpha = fp_compare_delta<double>(45739210286.0101);
    fp_compare_delta<double> beta = fp_compare_delta<double>(alpha._value);
    BOOST_CHECK_EQUAL(alpha._value, beta._value);
    BOOST_CHECK_EQUAL(alpha._delta, beta._delta);

    // Test constructor with specified delta
    fp_compare_delta<double> foxtrot = fp_compare_delta<double>(alpha._value,
            uhd::math::DOUBLE_PRECISION_DELTA);
    fp_compare_delta<double> gamma = fp_compare_delta<double>(alpha._value, 2.0e-6);
    BOOST_CHECK_EQUAL(alpha._delta, foxtrot._delta);
    BOOST_CHECK(not (alpha._delta == gamma._delta));

    // Test copy-constructor
    fp_compare_delta<double> charlie = fp_compare_delta<double>(alpha);
    BOOST_CHECK_EQUAL(alpha._value, charlie._value);
    BOOST_CHECK_EQUAL(alpha._delta, charlie._delta);

    // Test assignment operator
    fp_compare_delta<double> delta = beta;
    BOOST_CHECK_EQUAL(alpha._value, delta._value);
    BOOST_CHECK_EQUAL(alpha._delta, delta._delta);
}

BOOST_AUTO_TEST_CASE(float_equality_operators) {
    // Test basic equality operator
    fp_compare_delta<float> alpha = fp_compare_delta<float>(1.0);
    fp_compare_delta<float> beta = fp_compare_delta<float>(alpha._value);
    BOOST_CHECK(alpha == beta);
    BOOST_CHECK(alpha == float(alpha._value));

    // Test equality edge case at difference = delta
    fp_compare_delta<float> charlie = fp_compare_delta<float>(alpha._value
            + uhd::math::SINGLE_PRECISION_DELTA);
    BOOST_CHECK(not (alpha == charlie));
    BOOST_CHECK(not (alpha == float(alpha._value + uhd::math::SINGLE_PRECISION_DELTA)));
}

BOOST_AUTO_TEST_CASE(double_equality_operators) {
    // Test basic equality operator
    fp_compare_delta<double> alpha = fp_compare_delta<double>(1.0);
    fp_compare_delta<double> beta = fp_compare_delta<double>(alpha._value);
    BOOST_CHECK(alpha == beta);
    BOOST_CHECK(alpha == double(beta._value));

    // Test equality edge case at delta = delta
    fp_compare_delta<double> charlie = fp_compare_delta<double>(alpha._value
            + uhd::math::DOUBLE_PRECISION_DELTA);
    BOOST_CHECK(not (alpha == charlie));
    BOOST_CHECK(not (alpha == double(alpha._value + uhd::math::DOUBLE_PRECISION_DELTA)));
}

BOOST_AUTO_TEST_CASE(float_inequality_operators) {
    // Test inequality operator, which is based on equality operator
    fp_compare_delta<float> alpha = fp_compare_delta<float>(127.0f);
    fp_compare_delta<float> beta = fp_compare_delta<float>(alpha._value + 1.19e-3f);

    BOOST_CHECK(alpha != beta);
    BOOST_CHECK(alpha != float(alpha._value + 1.19e-3));
}

BOOST_AUTO_TEST_CASE(double_inequality_operators) {
    // Test inequality operator, which is based on equality operator
    fp_compare_delta<double> alpha = fp_compare_delta<double>(1.0);
    fp_compare_delta<double> beta = fp_compare_delta<double>(alpha._value + 1.19e-5);

    BOOST_CHECK(alpha != beta);
    BOOST_CHECK(alpha != double(alpha._value + 1.19e-5));
}

BOOST_AUTO_TEST_CASE(float_lessthan_operators) {
    // Test less-than operator
    fp_compare_delta<float> alpha = fp_compare_delta<float>(274192.7f);
    fp_compare_delta<float> beta = fp_compare_delta<float>(alpha._value - 0.2f);

    BOOST_CHECK(beta < alpha);
    BOOST_CHECK(float(alpha._value - 0.2) < alpha);

    // Confirm false less-than case
    fp_compare_delta<float> charlie = fp_compare_delta<float>(alpha._value - 1.2f);

    BOOST_CHECK(not (alpha < charlie));
    BOOST_CHECK(not (alpha < float(alpha._value - 1.2f)));
}

BOOST_AUTO_TEST_CASE(double_lessthan_operators) {
    // Test less-than operator
    fp_compare_delta<double> alpha = fp_compare_delta<double>(274192856.762312);
    fp_compare_delta<double> beta = fp_compare_delta<double>(alpha._value - 0.0002);

    BOOST_CHECK(beta < alpha);
    BOOST_CHECK(double(alpha._value - 0.0002) < alpha);

    // Confirm false less-than case
    fp_compare_delta<double> charlie = fp_compare_delta<double>(alpha._value - 1.0012);

    BOOST_CHECK(not (alpha < charlie));
    BOOST_CHECK(not (alpha < double(alpha._value - 1.0012)));
}

BOOST_AUTO_TEST_CASE(float_lessthanequals_operators) {
    // Test that <= correctly reports for equal values
    fp_compare_delta<float> alpha = fp_compare_delta<float>(827.3f);
    fp_compare_delta<float> beta = fp_compare_delta<float>(alpha._value);

    BOOST_CHECK(alpha <= beta);
    BOOST_CHECK(alpha <= float(alpha._value));

    // Test that <= correctly reports for less-than values
    fp_compare_delta<float> charlie = fp_compare_delta<float>(alpha._value - 1.2f);

    BOOST_CHECK(charlie <= alpha);
    BOOST_CHECK(float(alpha._value - 1.2) <= alpha);
}

BOOST_AUTO_TEST_CASE(double_lessthanequals_operators) {
    // Test that <= correctly reports for equal values
    fp_compare_delta<double> alpha = fp_compare_delta<double>(837652123.383764);
    fp_compare_delta<double> beta = fp_compare_delta<double>(alpha._value);

    BOOST_CHECK(alpha <= beta);
    BOOST_CHECK(alpha <= double(alpha._value));

    // Test that <= correctly reports for less-than values
    fp_compare_delta<double> charlie = fp_compare_delta<double>(alpha._value - 0.0012);

    BOOST_CHECK(charlie <= alpha);
    BOOST_CHECK(double(alpha._value - 0.0012) <= alpha);
}

BOOST_AUTO_TEST_CASE(float_greaterthan_operators) {
    // Test basic greater-than functionality
    fp_compare_delta<float> alpha = fp_compare_delta<float>(98325.4f);
    fp_compare_delta<float> beta = fp_compare_delta<float>(alpha._value + 0.15f);

    BOOST_CHECK(beta > alpha);
    BOOST_CHECK(float(alpha._value + 0.15) > alpha);

    // Test false greater-than case
    fp_compare_delta<float> charlie = fp_compare_delta<float>(alpha._value + 1.2f);

    BOOST_CHECK(not (alpha > charlie));
    BOOST_CHECK(not (alpha > float(alpha._value + 1.2)));
}

BOOST_AUTO_TEST_CASE(double_greaterthan_operators) {
    // Test basic greater-than functionality
    fp_compare_delta<double> alpha = fp_compare_delta<double>(643907213.428475);
    fp_compare_delta<double> beta = fp_compare_delta<double>(alpha._value + 0.0002);

    BOOST_CHECK(beta > alpha);
    BOOST_CHECK(double(alpha._value + 0.0002) > alpha);

    // Test false greater-than case
    fp_compare_delta<double> charlie = fp_compare_delta<double>(alpha._value + 0.0012);

    BOOST_CHECK(not (alpha > charlie));
    BOOST_CHECK(not (alpha > double(alpha._value + 0.0012)));
}

BOOST_AUTO_TEST_CASE(float_greaterthanequals_operators) {
    // Test that >= correctly reports for equal values
    fp_compare_delta<float> alpha = fp_compare_delta<float>(7834.89f);
    fp_compare_delta<float> beta = fp_compare_delta<float>(alpha._value);

    BOOST_CHECK(alpha >= beta);
    BOOST_CHECK(alpha >= float(alpha._value));

    // Test that >= correctly reports for greater-than values
    fp_compare_delta<float> charlie = fp_compare_delta<float>(alpha._value + 4.8f);

    BOOST_CHECK(charlie >= alpha);
    BOOST_CHECK(float(alpha._value + 4.8) >= alpha);
}

BOOST_AUTO_TEST_CASE(double_greaterthanequals_operators) {
    // Test that >= correctly reports for equal values
    fp_compare_delta<double> alpha = fp_compare_delta<double>(737623834.89843);
    fp_compare_delta<double> beta = fp_compare_delta<double>(alpha._value);

    BOOST_CHECK(alpha >= beta);
    BOOST_CHECK(alpha >= double(alpha._value));

    // Test that >= correctly reports for greater-than values
    fp_compare_delta<double> charlie = fp_compare_delta<double>(alpha._value + 3.0008);

    BOOST_CHECK(charlie >= alpha);
    BOOST_CHECK(double(alpha._value + 3.0008) >= alpha);
}

BOOST_AUTO_TEST_CASE(fp_compare_large_delta) {
    BOOST_CHECK(fp_compare_delta<double>(61440000.047870710492, 0.1) == 61440000.000000000000);
    BOOST_CHECK(fp_compare_delta<double>(61440000.047870710492, 0.1) <= 61440000.000000000000);
    BOOST_CHECK(fp_compare_delta<double>(61440000.047870710492, 0.1) >= 61440000.000000000000);

    BOOST_CHECK(fp_compare_delta<double>(1.0, 10.0) == 2.0);
}

BOOST_AUTO_TEST_CASE(frequency_compare_function) {

    BOOST_CHECK(uhd::math::frequencies_are_equal(6817333232.0, 6817333232.0));
    BOOST_CHECK(!uhd::math::frequencies_are_equal(6817333233.0, 6817333232.0));
    BOOST_CHECK(uhd::math::frequencies_are_equal(6817333232.1, 6817333232.1));
    BOOST_CHECK(!uhd::math::frequencies_are_equal(6817333232.5, 6817333232.6));
    BOOST_CHECK(uhd::math::frequencies_are_equal(16.8173332321e9, 16.8173332321e9));
    BOOST_CHECK(!uhd::math::frequencies_are_equal(16.8173332322e9, 16.8173332321e9));
    BOOST_CHECK(!uhd::math::frequencies_are_equal(5.0, 4.0));
    BOOST_CHECK(uhd::math::frequencies_are_equal(48750000.0, 48749999.9946));
}
