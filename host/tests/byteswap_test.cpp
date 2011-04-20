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
#include <uhd/utils/byteswap.hpp>

BOOST_AUTO_TEST_CASE(test_byteswap16){
    boost::uint16_t x = 0x0123;
    boost::uint16_t y = 0x2301;
    BOOST_CHECK_EQUAL(uhd::byteswap(x), y);
}

BOOST_AUTO_TEST_CASE(test_byteswap32){
    boost::uint32_t x = 0x01234567;
    boost::uint32_t y = 0x67452301;
    BOOST_CHECK_EQUAL(uhd::byteswap(x), y);
}

BOOST_AUTO_TEST_CASE(test_byteswap64){
    //split up 64 bit constants to avoid long-long compiler warnings
    boost::uint64_t x = 0x01234567 | (boost::uint64_t(0x89abcdef) << 32);
    boost::uint64_t y = 0xefcdab89 | (boost::uint64_t(0x67452301) << 32);
    BOOST_CHECK_EQUAL(uhd::byteswap(x), y);
}

