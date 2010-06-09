//
// Copyright 2010 Ettus Research LLC
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

#include <uhd/transport/convert_types.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/cstdint.hpp>
#include <complex>

using namespace uhd;

template <typename host_type, typename dev_type, size_t nsamps>
void loopback(
    const io_type_t &io_type,
    const otw_type_t &otw_type,
    const host_type *input,
    host_type *output
){
    dev_type dev[nsamps];

    //convert to dev type
    transport::convert_io_type_to_otw_type(
        input, io_type,
        dev, otw_type,
        nsamps
    );

    //convert back to host type
    transport::convert_otw_type_to_io_type(
        dev, otw_type,
        output, io_type,
        nsamps
    );
}

typedef std::complex<boost::uint16_t> sc16_t;

BOOST_AUTO_TEST_CASE(test_convert_types_be_sc16){
    sc16_t in_sc16[] = {
        sc16_t(0, -1234), sc16_t(4321, 1234),
        sc16_t(9876, -4567), sc16_t(8912, 0)
    }, out_sc16[4];

    io_type_t io_type(io_type_t::COMPLEX_INT16);
    otw_type_t otw_type;
    otw_type.byteorder = otw_type_t::BO_BIG_ENDIAN;
    otw_type.width = 16;

    loopback<sc16_t, boost::uint32_t, 4>(io_type, otw_type, in_sc16, out_sc16);
    BOOST_CHECK_EQUAL_COLLECTIONS(in_sc16, in_sc16+4, out_sc16, out_sc16+4);
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_sc16){
    sc16_t in_sc16[] = {
        sc16_t(0, -1234), sc16_t(4321, 1234),
        sc16_t(9876, -4567), sc16_t(8912, 0)
    }, out_sc16[4];

    io_type_t io_type(io_type_t::COMPLEX_INT16);
    otw_type_t otw_type;
    otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;
    otw_type.width = 16;

    loopback<sc16_t, boost::uint32_t, 4>(io_type, otw_type, in_sc16, out_sc16);
    BOOST_CHECK_EQUAL_COLLECTIONS(in_sc16, in_sc16+4, out_sc16, out_sc16+4);
}

typedef std::complex<float> fc32_t;

#define BOOST_CHECK_CLOSE_COMPLEX(a1, a2, p) \
    BOOST_CHECK_CLOSE(a1.real(), a2.real(), p); \
    BOOST_CHECK_CLOSE(a1.imag(), a2.imag(), p);

static const float tolerance = float(0.1);

BOOST_AUTO_TEST_CASE(test_convert_types_be_fc32){
    fc32_t in_fc32[] = {
        fc32_t(float(0), float(-0.2)), fc32_t(float(0.03), float(-0.16)),
        fc32_t(float(1.0), float(.45)), fc32_t(float(0.09), float(0))
    }, out_fc32[4];

    io_type_t io_type(io_type_t::COMPLEX_FLOAT32);
    otw_type_t otw_type;
    otw_type.byteorder = otw_type_t::BO_BIG_ENDIAN;
    otw_type.width = 16;

    loopback<fc32_t, boost::uint32_t, 4>(io_type, otw_type, in_fc32, out_fc32);

    BOOST_CHECK_CLOSE_COMPLEX(in_fc32[0], out_fc32[0], tolerance);
    BOOST_CHECK_CLOSE_COMPLEX(in_fc32[1], out_fc32[1], tolerance);
    BOOST_CHECK_CLOSE_COMPLEX(in_fc32[2], out_fc32[2], tolerance);
    BOOST_CHECK_CLOSE_COMPLEX(in_fc32[3], out_fc32[3], tolerance);
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_fc32){
    fc32_t in_fc32[] = {
        fc32_t(float(0), float(-0.2)), fc32_t(float(0.03), float(-0.16)),
        fc32_t(float(1.0), float(.45)), fc32_t(float(0.09), float(0))
    }, out_fc32[4];

    io_type_t io_type(io_type_t::COMPLEX_FLOAT32);
    otw_type_t otw_type;
    otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;
    otw_type.width = 16;

    loopback<fc32_t, boost::uint32_t, 4>(io_type, otw_type, in_fc32, out_fc32);

    BOOST_CHECK_CLOSE_COMPLEX(in_fc32[0], out_fc32[0], tolerance);
    BOOST_CHECK_CLOSE_COMPLEX(in_fc32[1], out_fc32[1], tolerance);
    BOOST_CHECK_CLOSE_COMPLEX(in_fc32[2], out_fc32[2], tolerance);
    BOOST_CHECK_CLOSE_COMPLEX(in_fc32[3], out_fc32[3], tolerance);
}
