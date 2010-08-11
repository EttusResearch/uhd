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

#include "constants.hpp"
#include <uhd/config.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

/***********************************************************************
 * Determine the paths separator
 **********************************************************************/
#ifdef UHD_PLATFORM_WIN32
    static const std::string env_path_sep = ";";
#else
    static const std::string env_path_sep = ":";
#endif /*UHD_PLATFORM_WIN32*/

/***********************************************************************
 * Get a list of paths for an environment variable
 **********************************************************************/
static std::string name_mapper(const std::string &key, const std::string &var_name){
    return (var_name == key)? var_name : "";
}

static std::vector<fs::path> get_env_paths(const std::string &var_name){
    //register the options
    std::string var_value;
    po::options_description desc;
    desc.add_options()
        (var_name.c_str(), po::value<std::string>(&var_value)->default_value(""))
    ;

    //parse environment variables
    po::variables_map vm;
    po::store(po::parse_environment(desc, boost::bind(&name_mapper, var_name, _1)), vm);
    po::notify(vm);

    //split the path at the path separators
    std::vector<std::string> path_strings;
    boost::split(path_strings, var_value, boost::is_any_of(env_path_sep));

    //convert to filesystem path, filter blank paths
    std::vector<fs::path> paths;
    BOOST_FOREACH(std::string &path_string, path_strings){
        if (path_string.size() == 0) continue;
        paths.push_back(fs::system_complete(path_string));
    }
    return paths;
}

/***********************************************************************
 * Get a list of special purpose paths
 **********************************************************************/
static const fs::path pkg_data_path = fs::path(UHD_INSTALL_PREFIX) / UHD_PKG_DATA_DIR;

std::vector<fs::path> get_image_paths(void){
    std::vector<fs::path> paths = get_env_paths("UHD_IMAGE_PATH");
    paths.push_back(pkg_data_path / "images");
    return paths;
}

std::vector<fs::path> get_module_paths(void){
    std::vector<fs::path> paths = get_env_paths("UHD_MODULE_PATH");
    paths.push_back(pkg_data_path / "modules");
    return paths;
}

/***********************************************************************
 * Find a image in the image paths
 **********************************************************************/
std::string find_image_path(const std::string &image_name){
    if (fs::exists(image_name)){
        return fs::system_complete(image_name).file_string();
    }
    BOOST_FOREACH(const fs::path &path, get_image_paths()){
        fs::path image_path = path / image_name;
        if (fs::exists(image_path)) return image_path.file_string();
    }
    throw std::runtime_error("Could not find path for image: " + image_name);
}
