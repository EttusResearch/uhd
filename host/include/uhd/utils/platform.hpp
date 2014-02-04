//
// Copyright 2010,2012 Ettus Research LLC
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

#ifndef INCLUDED_UHD_UTILS_PLATFORM_HPP
#define INCLUDED_UHD_UTILS_PLATFORM_HPP

#include <boost/cstdint.hpp>

namespace uhd {

    /* Returns the process ID of the current process */
    boost::int32_t get_process_id();

    /* Returns a unique identifier for the current machine */
    boost::uint32_t get_host_id();

    /* Get a unique identifier for the current machine and process */
    boost::uint32_t get_process_hash();

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_PLATFORM_HPP */
