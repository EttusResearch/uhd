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

#include <uhd/config.hpp>
#include <uhd/transport/convert_types.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/asio.hpp> //endianness conversion
#include <boost/cstdint.hpp>
#include <complex>

using namespace uhd;

/***********************************************************************
 * Constants
 **********************************************************************/
typedef std::complex<float>          fc32_t;

static const float shorts_per_float = float(1 << 15);
static const float floats_per_short = float(1.0/shorts_per_float);

#define unrolled_loop(__inst, __len){ \
    size_t __i = 0; \
    for(; __i < (__len & ~0x3); __i+= 4){ \
        __inst(__i+0); __inst(__i+1); \
        __inst(__i+2); __inst(__i+3); \
    } \
    for(; __i < __len; __i++){ \
        __inst(__i); \
    } \
}

// set a boolean flag that indicates the endianess
#ifdef HAVE_BIG_ENDIAN
static const bool is_big_endian = true;
#else
static const bool is_big_endian = false;
#endif

static UHD_INLINE void host_floats_to_usrp2_items(
    boost::uint32_t *usrp2_items,
    const fc32_t *host_floats,
    size_t num_samps
){
    #define host_floats_to_usrp2_items_i(i){ \
        boost::uint16_t real = boost::int16_t(host_floats[i].real()*shorts_per_float); \
        boost::uint16_t imag = boost::int16_t(host_floats[i].imag()*shorts_per_float); \
        usrp2_items[i] = htonl((real << 16) | (imag << 0)); \
    }
    unrolled_loop(host_floats_to_usrp2_items_i, num_samps);
}

static UHD_INLINE void usrp2_items_to_host_floats(
    fc32_t *host_floats,
    const boost::uint32_t *usrp2_items,
    size_t num_samps
){
    #define usrp2_items_to_host_floats_i(i){ \
        boost::uint32_t item = ntohl(usrp2_items[i]); \
        boost::int16_t real = boost::uint16_t(item >> 16); \
        boost::int16_t imag = boost::uint16_t(item >> 0); \
        host_floats[i] = fc32_t(float(real*floats_per_short), float(imag*floats_per_short)); \
    }
    unrolled_loop(usrp2_items_to_host_floats_i, num_samps);
}

static UHD_INLINE void host_items_to_usrp2_items(
    boost::uint32_t *usrp2_items,
    const boost::uint32_t *host_items,
    size_t num_samps
){
    #define host_items_to_usrp2_items_i(i) usrp2_items[i] = htonl(host_items[i])
    if (is_big_endian){
        std::memcpy(usrp2_items, host_items, num_samps*sizeof(boost::uint32_t));
    }
    else{
        unrolled_loop(host_items_to_usrp2_items_i, num_samps);
    }
}

static UHD_INLINE void usrp2_items_to_host_items(
    boost::uint32_t *host_items,
    const boost::uint32_t *usrp2_items,
    size_t num_samps
){
    #define usrp2_items_to_host_items_i(i) host_items[i] = ntohl(usrp2_items[i])
    if (is_big_endian){
        std::memcpy(host_items, usrp2_items, num_samps*sizeof(boost::uint32_t));
    }
    else{
        unrolled_loop(usrp2_items_to_host_items_i, num_samps);
    }
}

void transport::convert_io_type_to_otw_type(
    const void *io_buff, const io_type_t &io_type,
    void *otw_buff, const otw_type_t &otw_type,
    size_t num_samps
){
    //all we handle for now:
    UHD_ASSERT_THROW(otw_type.width == 16 and otw_type.byteorder == otw_type_t::BO_BIG_ENDIAN);

    switch(io_type.tid){
    case io_type_t::COMPLEX_FLOAT32:
        host_floats_to_usrp2_items((boost::uint32_t *)otw_buff, (const fc32_t*)io_buff, num_samps);
        return;
    case io_type_t::COMPLEX_INT16:
        host_items_to_usrp2_items((boost::uint32_t *)otw_buff, (const boost::uint32_t*)io_buff, num_samps);
        return;
     case io_type_t::CUSTOM_TYPE:
        std::memcpy(otw_buff, io_buff, num_samps*io_type.size);
        return;
    default:
        throw std::runtime_error(str(boost::format("convert_types: cannot handle type \"%c\"") % io_type.tid));
    }
}

void transport::convert_otw_type_to_io_type(
    const void *otw_buff, const otw_type_t &otw_type,
    void *io_buff, const io_type_t &io_type,
    size_t num_samps
){
    //all we handle for now:
    UHD_ASSERT_THROW(otw_type.width == 16 and otw_type.byteorder == otw_type_t::BO_BIG_ENDIAN);

    switch(io_type.tid){
    case io_type_t::COMPLEX_FLOAT32:
        usrp2_items_to_host_floats((fc32_t*)io_buff, (const boost::uint32_t *)otw_buff, num_samps);
        return;
    case io_type_t::COMPLEX_INT16:
        usrp2_items_to_host_items((boost::uint32_t*)io_buff, (const boost::uint32_t *)otw_buff, num_samps);
        return;
    case io_type_t::CUSTOM_TYPE:
        std::memcpy(io_buff, otw_buff, num_samps*io_type.size);
        return;
    default:
        throw std::runtime_error(str(boost::format("convert_types: cannot handle type \"%c\"") % io_type.tid));
    }
}
