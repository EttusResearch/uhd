//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/types/ranges.hpp>
#include <iostream>

using namespace uhd;

static const double tolerance = 0.001; // %


BOOST_AUTO_TEST_CASE(test_ranges_bounds){
    meta_range_t mr;
    mr.push_back(range_t(-1.0, +1.0, 0.1));
    BOOST_CHECK_CLOSE(mr.start(), -1.0, tolerance);
    BOOST_CHECK_CLOSE(mr.stop(), +1.0, tolerance);
    BOOST_CHECK_CLOSE(mr.step(), 0.1, tolerance);

    mr.push_back(range_t(40.0, 60.0, 1.0));
    BOOST_CHECK_CLOSE(mr.start(), -1.0, tolerance);
    BOOST_CHECK_CLOSE(mr.stop(), 60.0, tolerance);
    BOOST_CHECK_CLOSE(mr.step(), 0.1, tolerance);

    BOOST_CHECK_EQUAL(mr.size(), unsigned(2));

    BOOST_CHECK_CLOSE(mr[0].start(), -1.0, tolerance);
    BOOST_CHECK_CLOSE(mr[0].stop(), +1.0, tolerance);
    BOOST_CHECK_CLOSE(mr[0].step(), 0.1, tolerance);
}

BOOST_AUTO_TEST_CASE(test_ranges_clip){
    meta_range_t mr;
    mr.push_back(range_t(-1.0, +1.0, 0.1));
    mr.push_back(range_t(40.0, 60.0, 1.0));

    BOOST_CHECK_CLOSE(mr.clip(-30.0), -1.0, tolerance);
    BOOST_CHECK_CLOSE(mr.clip(70.0), 60.0, tolerance);
    BOOST_CHECK_CLOSE(mr.clip(20.0), 1.0, tolerance);
    BOOST_CHECK_CLOSE(mr.clip(50.0), 50.0, tolerance);

    BOOST_CHECK_CLOSE(mr.clip(50.9, false), 50.9, tolerance);
    BOOST_CHECK_CLOSE(mr.clip(50.9, true), 51.0, tolerance);
}

BOOST_AUTO_TEST_CASE(test_meta_range_t_ctor){
    meta_range_t mr1(0.0, 10.0, 1.0);
    BOOST_CHECK_CLOSE(mr1.clip(5.0), 5.0, tolerance);
    BOOST_CHECK_CLOSE(mr1.clip(11.0), 10.0, tolerance);
    BOOST_CHECK_CLOSE(mr1.clip(5.1, true), 5.0, tolerance);

    meta_range_t mr2(0.0, 10.0);
    BOOST_CHECK_CLOSE(mr2.clip(5.0), 5.0, tolerance);
    BOOST_CHECK_CLOSE(mr2.clip(11.0), 10.0, tolerance);
    BOOST_CHECK_CLOSE(mr2.clip(5.1, true), 5.1, tolerance);

    meta_range_t mr3(mr2.begin(), mr2.end());
    BOOST_CHECK_CLOSE(mr3.clip(5.0), 5.0, tolerance);
    BOOST_CHECK_CLOSE(mr3.clip(11.0), 10.0, tolerance);
    BOOST_CHECK_CLOSE(mr3.clip(5.1, true), 5.1, tolerance);
}

BOOST_AUTO_TEST_CASE(test_ranges_clip2){
    meta_range_t mr;
    mr.push_back(range_t(1.));
    mr.push_back(range_t(2.));
    mr.push_back(range_t(3.));

    BOOST_CHECK_CLOSE(mr.clip(2., true), 2., tolerance);
    BOOST_CHECK_CLOSE(mr.clip(0., true), 1., tolerance);
    BOOST_CHECK_CLOSE(mr.clip(1.2, true), 1., tolerance);
    BOOST_CHECK_CLOSE(mr.clip(3.1, true), 3., tolerance);
    BOOST_CHECK_CLOSE(mr.clip(4., true), 3., tolerance);
}

BOOST_AUTO_TEST_CASE(test_ranges_compare){
    range_t range(1);
    range_t n_range(1);
    range_t d_range(2);

    BOOST_CHECK(range == n_range);
    BOOST_CHECK(range != d_range);
}
