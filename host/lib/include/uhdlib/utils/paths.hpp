//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0+
//

#ifndef INCLUDED_UHDLIB_UTILS_PATHS_HPP
#define INCLUDED_UHDLIB_UTILS_PATHS_HPP

#include <string>

namespace uhd {

    /*! Expand environment variables in paths, like Python's
     *  os.path.expandvars().
     *
     * If expansion fails, will simply return the original path.
     */
    std::string path_expandvars(const std::string& path);

} /* namespace uhd */

#endif /* INCLUDED_UHDLIB_UTILS_PATHS_HPP */

