//
// Copyright 2014 Ettus Research LLC
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
#include <boost/cstdint.hpp>
#include <uhd/utils/math.hpp>

// NOTE: This is not the only math test case, see e.g. special tests
// for fp comparison.

BOOST_AUTO_TEST_CASE(test_log2){
    double y = uhd::math::log2(16.0);
    BOOST_CHECK_EQUAL(y, 4.0);
}

