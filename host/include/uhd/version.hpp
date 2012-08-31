//
// Copyright 2010-2012 Ettus Research LLC
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

#ifndef INCLUDED_UHD_VERSION_HPP
#define INCLUDED_UHD_VERSION_HPP

#include <uhd/config.hpp>
#include <string>

/*!
 * The ABI version string that the client application builds against.
 * Call get_abi_string() to check this against the library build.
 * The format is oldest API compatible release - ABI compat number.
 * The compatibility number allows pre-release ABI to be versioned.
 */
#define UHD_VERSION_ABI_STRING "3.4.0-3"

namespace uhd{

    //! Get the version string (dotted version number + build info)
    UHD_API std::string get_version_string(void);

    //! Get the ABI compatibility string for this build of the library
    UHD_API std::string get_abi_string(void);

} //namespace uhd

#endif /* INCLUDED_UHD_VERSION_HPP */
