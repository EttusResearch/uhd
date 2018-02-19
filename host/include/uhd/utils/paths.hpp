//
// Copyright 2011-2012,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_PATHS_HPP
#define INCLUDED_UHD_UTILS_PATHS_HPP

#include <uhd/config.hpp>

#include <boost/filesystem.hpp>

#include <string>
#include <vector>

namespace fs = boost::filesystem;

namespace uhd {

    //! Get a string representing the system's temporary directory
    UHD_API std::string get_tmp_path(void);

    //! Get a string representing the system's appdata directory
    UHD_API std::string get_app_path(void);

    //! Get a string representing the system's pkg directory
    UHD_API std::string get_pkg_path(void);

    //! Get UHD library paths
    UHD_API std::vector<fs::path> get_module_paths(void);

    /*! Return the UHD images directory path.
    *
    * This function returns the UHD images installation path on this system. The
    * returned directory path is guaranteed to exist (assuming a valid path is
    * found). This function will look for a directory that exists using this
    * order of precedence:
    *
    *   1) `UHD_IMAGES_DIR` environment variable
    *   2) Any paths passed to this function via `search_paths'
    *   3) UHD package path / share / uhd / images
    *
    * The `search_paths` parameter may contain Windows registry keys.  If no
    * directory is found, an empty string is returned.
    *
    * \param search_paths A comma-separated list of hints for paths to include.
    * \returns A path string if one is found, or an empty string on failure.
    */
    UHD_API std::string get_images_dir(const std::string &search_paths);

    /*! Return the full path to particular UHD binary image.
     *
     * This function searches for the passed image name, and returns an absolute
     * path to it. The returned path is guaranteed to exist. The caller can also
     * provide a full path to the image in the argument, and this function will
     * validate it and convert it to an absolute system path.
     *
     * \param image_name The name of the file to search for, or the full path.
     * \param search_paths Hints / paths to use when calling `get_images_dir`
     * \return the full system path to the file
     * \throw exception uhd::io_error if the file was not found.
     */
    UHD_API std::string find_image_path(const std::string &image_name, const std::string &search_paths = "");

    /*!
     * Search for the location of a particular UHD utility.
     * The utility must be installed in the `uhd/utils` directory.
     * \param name the name of the utility to search for
     * \return the full system path to the given utility
     */
    UHD_API std::string find_utility(const std::string &name);

    /*!
     * Return an error string recommending the user run the utility.
     * The error string will include the full path to the utility to run.
     * \return the message suggesting the use of the named utility.
     */
    UHD_API std::string print_utility_error(const std::string &name, const std::string &args = "");
} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_PATHS_HPP */
