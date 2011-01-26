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

#include "constants.hpp"
#include <uhd/config.hpp>
#include <boost/tokenizer.hpp>
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

#define path_tokenizer(inp) \
    boost::tokenizer<boost::char_separator<char> > \
    (inp, boost::char_separator<char>(env_path_sep.c_str()))

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
std::vector<fs::path> get_image_paths(void){
    std::vector<fs::path> paths = get_env_paths("UHD_IMAGE_PATH");
    paths.push_back(fs::path(LOCAL_PKG_DATA_DIR) / "images");
    if (not std::string(INSTALLER_PKG_DATA_DIR).empty())
        paths.push_back(fs::path(INSTALLER_PKG_DATA_DIR) / "images");
    return paths;
}

std::vector<fs::path> get_module_paths(void){
    std::vector<fs::path> paths = get_env_paths("UHD_MODULE_PATH");
    paths.push_back(fs::path(LOCAL_PKG_DATA_DIR) / "modules");
    if (not std::string(INSTALLER_PKG_DATA_DIR).empty())
        paths.push_back(fs::path(INSTALLER_PKG_DATA_DIR) / "modules");
    return paths;
}
