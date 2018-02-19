//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/math.hpp>
#include <boost/test/unit_test.hpp>

using namespace uhd::math::fp_compare;

BOOST_AUTO_TEST_CASE(fp_compare_epsilon_constructors) {
    // Test default constructor
    fp_compare_epsilon<float> alpha = fp_compare_epsilon<float>(7457392.0);
    fp_compare_epsilon<float> beta = fp_compare_epsilon<float>(alpha._value);
    BOOST_CHECK_EQUAL(alpha._value, beta._value);
    BOOST_CHECK_EQUAL(alpha._epsilon, beta._epsilon);

    // Test constructor with specified epsilon
    fp_compare_epsilon<float> foxtrot = fp_compare_epsilon<float>(alpha._value,
            uhd::math::SINGLE_PRECISION_EPSILON);
    fp_compare_epsilon<float> gamma = fp_compare_epsilon<float>(alpha._value, 2.0e-1f);
    BOOST_CHECK_EQUAL(alpha._epsilon, foxtrot._epsilon);
    BOOST_CHECK(not (alpha._epsilon == gamma._epsilon));

    // Test copy-constructor
    fp_compare_epsilon<float> charlie = fp_compare_epsilon<float>(alpha);
    BOOST_CHECK_EQUAL(alpha._value, charlie._value);
    BOOST_CHECK_EQUAL(alpha._epsilon, charlie._epsilon);

    // Test assignment operator
    fp_compare_epsilon<float> delta = beta;
    BOOST_CHECK_EQUAL(alpha._value, delta._value);
    BOOST_CHECK_EQUAL(alpha._epsilon, delta._epsilon);
}

BOOST_AUTO_TEST_CASE(double_compare_constructors) {
    // Test default constructor
    fp_compare_epsilon<double> alpha = fp_compare_epsilon<double>(45739210286.0101);
    fp_compare_epsilon<double> beta = fp_compare_epsilon<double>(alpha._value);
    BOOST_CHECK_EQUAL(alpha._value, beta._value);
    BOOST_CHECK_EQUAL(alpha._epsilon, beta._epsilon);

    // Test constructor with specified epsilon
    fp_compare_epsilon<double> foxtrot = fp_compare_epsilon<double>(alpha._value,
            uhd::math::DOUBLE_PRECISION_EPSILON);
    fp_compare_epsilon<double> gamma = fp_compare_epsilon<double>(alpha._value, 2.0e-6);
    BOOST_CHECK_EQUAL(alpha._epsilon, foxtrot._epsilon);
    BOOST_CHECK(not (alpha._epsilon == gamma._epsilon));

    // Test copy-constructor
    fp_compare_epsilon<double> charlie = fp_compare_epsilon<double>(alpha);
    BOOST_CHECK_EQUAL(alpha._value, charlie._value);
    BOOST_CHECK_EQUAL(alpha._epsilon, charlie._epsilon);

    // Test assignment operator
    fp_compare_epsilon<double> delta = beta;
    BOOST_CHECK_EQUAL(alpha._value, delta._value);
    BOOST_CHECK_EQUAL(alpha._epsilon, delta._epsilon);
}

BOOST_AUTO_TEST_CASE(float_equality_operators) {
    // Test basic equality operator
    fp_compare_epsilon<float> alpha = fp_compare_epsilon<float>(1.0);
    fp_compare_epsilon<float> beta = fp_compare_epsilon<float>(alpha._value);
    BOOST_CHECK(alpha == beta);
    BOOST_CHECK(alpha == float(alpha._value));

    // Test equality edge case at delta = epsilon
    fp_compare_epsilon<float> charlie = fp_compare_epsilon<float>(alpha._value
            + uhd::math::SINGLE_PRECISION_EPSILON);
    BOOST_CHECK(not (alpha == charlie));
    BOOST_CHECK(not (alpha == float(alpha._value + uhd::math::SINGLE_PRECISION_EPSILON)));
}

BOOST_AUTO_TEST_CASE(double_equality_operators) {
    // Test basic equality operator
    fp_compare_epsilon<double> alpha = fp_compare_epsilon<double>(1.0);
    fp_compare_epsilon<double> beta = fp_compare_epsilon<double>(alpha._value);
    BOOST_CHECK(alpha == beta);
    BOOST_CHECK(alpha == double(beta._value));

    // Test equality edge case at delta = epsilon
    fp_compare_epsilon<double> charlie = fp_compare_epsilon<double>(alpha._value
            + uhd::math::DOUBLE_PRECISION_EPSILON);
    BOOST_CHECK(not (alpha == charlie));
    BOOST_CHECK(not (alpha == double(alpha._value
                    + uhd::math::DOUBLE_PRECISION_EPSILON)));
}

BOOST_AUTO_TEST_CASE(float_inequality_operators) {
    // Test inequality operator, which is based on equality operator
    fp_compare_epsilon<float> alpha = fp_compare_epsilon<float>(127.0);
    fp_compare_epsilon<float> beta = fp_compare_epsilon<float>(alpha._value + 1.19e-5f);

    BOOST_CHECK(alpha != beta);
    BOOST_CHECK(alpha != float(alpha._value + 1.19e-5f));
}

BOOST_AUTO_TEST_CASE(double_inequality_operators) {
    // Test inequality operator, which is based on equality operator
    fp_compare_epsilon<double> alpha = fp_compare_epsilon<double>(1.0);
    fp_compare_epsilon<double> beta = fp_compare_epsilon<double>(alpha._value + 1.19e-10);

    BOOST_CHECK(alpha != beta);
    BOOST_CHECK(alpha != double(alpha._value + 1.19e-10));
}

BOOST_AUTO_TEST_CASE(float_lessthan_operators) {
    // Test less-than operator
    fp_compare_epsilon<float> alpha = fp_compare_epsilon<float>(274192.7f);
    fp_compare_epsilon<float> beta = fp_compare_epsilon<float>(alpha._value - 0.15f);

    BOOST_CHECK(beta < alpha);
    BOOST_CHECK(float(alpha._value - 0.15) < alpha);

    // Confirm false less-than case
    fp_compare_epsilon<float> charlie = fp_compare_epsilon<float>(alpha._value - 1.2f);

    BOOST_CHECK(not (alpha < charlie));
    BOOST_CHECK(not (alpha < float(alpha._value - 1.2f)));
}

BOOST_AUTO_TEST_CASE(double_lessthan_operators) {
    // Test less-than operator
    fp_compare_epsilon<double> alpha = fp_compare_epsilon<double>(274192856.762312);
    fp_compare_epsilon<double> beta = fp_compare_epsilon<double>(alpha._value - 0.0002);

    BOOST_CHECK(beta < alpha);
    BOOST_CHECK(double(alpha._value - 0.0002) < alpha);

    // Confirm false less-than case
    fp_compare_epsilon<double> charlie = fp_compare_epsilon<double>(alpha._value - 1.0012);

    BOOST_CHECK(not (alpha < charlie));
    BOOST_CHECK(not (alpha < double(alpha._value - 1.0012)));
}

BOOST_AUTO_TEST_CASE(float_lessthanequals_operators) {
    // Test that <= correctly reports for equal values
    fp_compare_epsilon<float> alpha = fp_compare_epsilon<float>(827.3f);
    fp_compare_epsilon<float> beta = fp_compare_epsilon<float>(alpha._value);

    BOOST_CHECK(alpha <= beta);
    BOOST_CHECK(alpha <= float(alpha._value));

    // Test that <= correctly reports for less-than values
    fp_compare_epsilon<float> charlie = fp_compare_epsilon<float>(alpha._value - 1.2f);

    BOOST_CHECK(charlie <= alpha);
    BOOST_CHECK(float(alpha._value - 1.2) <= alpha);
}

BOOST_AUTO_TEST_CASE(double_lessthanequals_operators) {
    // Test that <= correctly reports for equal values
    fp_compare_epsilon<double> alpha = fp_compare_epsilon<double>(837652123.383764);
    fp_compare_epsilon<double> beta = fp_compare_epsilon<double>(alpha._value);

    BOOST_CHECK(alpha <= beta);
    BOOST_CHECK(alpha <= double(alpha._value));

    // Test that <= correctly reports for less-than values
    fp_compare_epsilon<double> charlie = fp_compare_epsilon<double>(alpha._value - 0.0012);

    BOOST_CHECK(charlie <= alpha);
    BOOST_CHECK(double(alpha._value - 0.0012) <= alpha);
}

BOOST_AUTO_TEST_CASE(float_greaterthan_operators) {
    // Test basic greater-than functionality
    fp_compare_epsilon<float> alpha = fp_compare_epsilon<float>(98325.4f);
    fp_compare_epsilon<float> beta = fp_compare_epsilon<float>(alpha._value + 0.15f);

    BOOST_CHECK(beta > alpha);
    BOOST_CHECK(float(alpha._value + 0.15) > alpha);

    // Test false greater-than case
    fp_compare_epsilon<float> charlie = fp_compare_epsilon<float>(alpha._value + 1.2f);

    BOOST_CHECK(not (alpha > charlie));
    BOOST_CHECK(not (alpha > float(alpha._value + 1.2f)));
}

BOOST_AUTO_TEST_CASE(double_greaterthan_operators) {
    // Test basic greater-than functionality
    fp_compare_epsilon<double> alpha = fp_compare_epsilon<double>(643907213.428475);
    fp_compare_epsilon<double> beta = fp_compare_epsilon<double>(alpha._value + 0.0002);

    BOOST_CHECK(beta > alpha);
    BOOST_CHECK(double(alpha._value + 0.0002) > alpha);

    // Test false greater-than case
    fp_compare_epsilon<double> charlie = fp_compare_epsilon<double>(alpha._value + 0.0012);

    BOOST_CHECK(not (alpha > charlie));
    BOOST_CHECK(not (alpha > double(alpha._value + 0.0012)));
}

BOOST_AUTO_TEST_CASE(float_greaterthanequals_operators) {
    // Test that >= correctly reports for equal values
    fp_compare_epsilon<float> alpha = fp_compare_epsilon<float>(7834.89f);
    fp_compare_epsilon<float> beta = fp_compare_epsilon<float>(alpha._value);

    BOOST_CHECK(alpha >= beta);
    BOOST_CHECK(alpha >= float(alpha._value));

    // Test that >= correctly reports for greater-than values
    fp_compare_epsilon<float> charlie = fp_compare_epsilon<float>(alpha._value + 4.8f);

    BOOST_CHECK(charlie >= alpha);
    BOOST_CHECK(float(alpha._value + 4.8f) >= alpha);
}

BOOST_AUTO_TEST_CASE(double_greaterthanequals_operators) {
    // Test that >= correctly reports for equal values
    fp_compare_epsilon<double> alpha = fp_compare_epsilon<double>(737623834.89843);
    fp_compare_epsilon<double> beta = fp_compare_epsilon<double>(alpha._value);

    BOOST_CHECK(alpha >= beta);
    BOOST_CHECK(alpha >= double(alpha._value));

    // Test that >= correctly reports for greater-than values
    fp_compare_epsilon<double> charlie = fp_compare_epsilon<double>(alpha._value + 3.0008);

    BOOST_CHECK(charlie >= alpha);
    BOOST_CHECK(double(alpha._value + 3.0008) >= alpha);
}
