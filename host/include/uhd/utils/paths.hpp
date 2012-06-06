//
// Copyright 2011-2012 Ettus Research LLC
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

#ifndef INCLUDED_UHD_UTILS_PATHS_HPP
#define INCLUDED_UHD_UTILS_PATHS_HPP

#include <uhd/config.hpp>
#include <string>

namespace uhd{

    //! Get a string representing the system's temporary directory
    UHD_API std::string get_tmp_path(void);

    //! Get a string representing the system's appdata directory
    UHD_API std::string get_app_path(void);

    //! Get a string representing the system's pkg data directory
    UHD_API std::string get_pkg_data_path(void);

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_PATHS_HPP */
