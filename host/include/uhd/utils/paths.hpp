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

    //! Get a string representing the system's pkg directory
    UHD_API std::string get_pkg_path(void);

    /*!
     * Search for an image in the system image paths:
     * Search compiled-in paths and environment variable paths
     * for a specific image file with the provided file name.
     * \param image_name the name of the file
     * \return the full system path to the file
     * \throw exception if the image was not found
     */
    UHD_API std::string find_image_path(const std::string &image_name);

    /*!
     * Search for the location of a particular UHD utility.
     * The utility must be installed in the `uhd/utils` directory.
     * \param the name of the utility to search for
     * \return the full system path to @param
     */
    UHD_API std::string find_utility(std::string name);

    /*!
     * Return an error string recommending the user run the utility.
     * The error string will include the full path to the utility to run.
     * \return the message suggesting the use of the named utility.
     */
    UHD_API std::string print_utility_error(std::string name);

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_PATHS_HPP */
