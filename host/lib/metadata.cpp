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

#include <uhd/metadata.hpp>

using namespace uhd;

rx_metadata_t::rx_metadata_t(void){
    stream_id = 0;
    has_stream_id = false;
    time_spec = time_spec_t();
    has_time_spec = false;
    is_fragment = false;
}

tx_metadata_t::tx_metadata_t(void){
    stream_id = 0;
    has_stream_id = false;
    time_spec = time_spec_t();
    has_time_spec = false;
    start_of_burst = false;
    end_of_burst = false;
}
