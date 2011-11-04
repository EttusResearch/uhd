//
// Copyright 2010-2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <boost/test/unit_test.hpp>
#include <uhd/types/ranges.hpp>
#include <iostream>

using namespace uhd;

static const double tolerance = 0.001;

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
