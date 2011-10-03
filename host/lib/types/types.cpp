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
