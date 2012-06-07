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

#ifndef INCLUDED_UHD_UTILS_IMAGES_HPP
#define INCLUDED_UHD_UTILS_IMAGES_HPP

#include <uhd/config.hpp>
#include <string>

namespace uhd{

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
     * Search for the location of the UHD Images Downloader script.
     * \return the full system path to uhd_images_downloader.py
     */

    UHD_API std::string find_images_downloader(void);

    /*!
     * Return the error string for recommending using the UHD Images Downloader.
     * String depends on OS.
     * \return the message suggesting the use of uhd_images_downloader.py
     */

    UHD_API std::string print_images_error(void);

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_IMAGES_HPP */
