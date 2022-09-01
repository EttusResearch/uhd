//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/filesystem.hpp>
#include <string>

namespace uhd {

/*! Expand environment variables in paths, like Python's
 *  os.path.expandvars().
 *
 * If expansion fails, will simply return the original path.
 */
std::string path_expandvars(const std::string& path);

//! Compatibility function for deprecated CSV file reader. Remove this when CSV
// format for IQ/DC cal gets removed.
std::string get_appdata_path(void);

//! Return a path to XDG_DATA_HOME
//
// https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
//
// Even on non-Linux systems, this should return the place where app data is
// written to. For UHD, this is data such as calibration data.
boost::filesystem::path get_xdg_data_home();

//! Return a path to XDG_CONFIG_HOME
//
// https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
//
// Even on non-Linux systems, this should return the place where the
// configuration file can be stored.
boost::filesystem::path get_xdg_config_home();

//! Return a path to ~/.uhd
boost::filesystem::path get_legacy_config_home();

} /* namespace uhd */
