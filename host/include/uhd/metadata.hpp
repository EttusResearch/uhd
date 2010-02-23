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

#ifndef INCLUDED_UHD_METADATA_HPP
#define INCLUDED_UHD_METADATA_HPP

#include <uhd/time_spec.hpp>

namespace uhd{

/*!
 * Metadata structure for describing the IF data.
 * Includes stream ID, time specification, and burst flags.
 * The receive routines will convert IF data headers into metadata.
 * The send routines will convert the metadata to IF data headers.
 */
struct metadata_t{
    uint32_t stream_id;
    time_spec_t time_spec;
    bool start_of_burst;
    bool end_of_burst;

    metadata_t(void){
        stream_id = 0;
        time_spec = time_spec_t();
        start_of_burst = false;
        end_of_burst = false;
    }
};

} //namespace uhd

#endif /* INCLUDED_UHD_METADATA_HPP */
