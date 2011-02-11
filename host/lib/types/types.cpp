//
// Copyright 2011 Ettus Research LLC
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

#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/io_type.hpp>
#include <boost/cstdint.hpp>
#include <stdexcept>
#include <complex>
#include <vector>

using namespace uhd;

/***********************************************************************
 * stream command
 **********************************************************************/
stream_cmd_t::stream_cmd_t(const stream_mode_t &stream_mode):
    stream_mode(stream_mode),
    num_samps(0),
    stream_now(true)
{
    /* NOP */
}

/***********************************************************************
 * metadata
 **********************************************************************/
tx_metadata_t::tx_metadata_t(void):
    has_time_spec(false),
    time_spec(time_spec_t()),
    start_of_burst(false),
    end_of_burst(false)
{
    /* NOP */
}

/***********************************************************************
 * otw type
 **********************************************************************/
size_t otw_type_t::get_sample_size(void) const{
    return (this->width * 2) / 8;
}

otw_type_t::otw_type_t(void):
    width(0),
    shift(0),
    byteorder(BO_NATIVE)
{
    /* NOP */
}

/***********************************************************************
 * io type
 **********************************************************************/
static std::vector<size_t> get_tid_size_table(void){
    std::vector<size_t> table(128, 0);
    table[size_t(io_type_t::COMPLEX_FLOAT64)] = sizeof(std::complex<double>);
    table[size_t(io_type_t::COMPLEX_FLOAT32)] = sizeof(std::complex<float>);
    table[size_t(io_type_t::COMPLEX_INT16)]   = sizeof(std::complex<boost::int16_t>);
    table[size_t(io_type_t::COMPLEX_INT8)]    = sizeof(std::complex<boost::int8_t>);
    return table;
}

static const std::vector<size_t> tid_size_table(get_tid_size_table());

io_type_t::io_type_t(tid_t tid):
    size(tid_size_table[size_t(tid) & 0x7f]), tid(tid)
{
    /* NOP */
}

io_type_t::io_type_t(size_t size):
    size(size), tid(CUSTOM_TYPE)
{
    /* NOP */
}
