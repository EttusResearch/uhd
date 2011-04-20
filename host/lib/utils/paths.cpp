//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/config.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <cstdlib>
#include <string>
#include <vector>

namespace fs = boost::filesystem;

/***********************************************************************
 * Determine the paths separator
 **********************************************************************/
#ifdef UHD_PLATFORM_WIN32
    static const std::string env_path_sep = ";";
#else
    static const std::string env_path_sep = ":";
#endif /*UHD_PLATFORM_WIN32*/

#define path_tokenizer(inp) \
    boost::tokenizer<boost::char_separator<char> > \
    (inp, boost::char_separator<char>(env_path_sep.c_str()))

/***********************************************************************
 * Get a list of paths for an environment variable
 **********************************************************************/
static std::string get_env_var(const std::string &var_name, const std::string &def_val = ""){
    const char *var_value_ptr = std::getenv(var_name.c_str());
    return (var_value_ptr == NULL)? def_val : var_value_ptr;
}

static std::vector<fs::path> get_env_paths(const std::string &var_name){

    std::string var_value = get_env_var(var_name);

    //convert to filesystem path, filter blank paths
    std::vector<fs::path> paths;
    if (var_value.empty()) return paths; //FIXME boost tokenizer throws w/ blank strings on some platforms
    BOOST_FOREACH(const std::string &path_string, path_tokenizer(var_value)){
        if (path_string.empty()) continue;
        paths.push_back(fs::system_complete(path_string));
    }
    return paths;
}

/***********************************************************************
 * Get a list of special purpose paths
 **********************************************************************/
static fs::path get_uhd_pkg_data_path(void){
    return fs::path(get_env_var("UHD_PKG_DATA_PATH", UHD_PKG_DATA_PATH));
}

std::vector<fs::path> get_image_paths(void){
    std::vector<fs::path> paths = get_env_paths("UHD_IMAGE_PATH");
    paths.push_back(get_uhd_pkg_data_path() / "images");
    return paths;
}

std::vector<fs::path> get_module_paths(void){
    std::vector<fs::path> paths = get_env_paths("UHD_MODULE_PATH");
    paths.push_back(get_uhd_pkg_data_path() / "modules");
    return paths;
}
