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

#include <uhd/config.hpp>
#include <uhd/utils/paths.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstdlib> //getenv
#include <cstdio>  //P_tmpdir
#ifdef BOOST_MSVC
#define USE_GET_TEMP_PATH
#include <windows.h> //GetTempPath
#endif

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
std::string uhd::get_pkg_data_path(void)
{
    return get_env_var("UHD_PKG_DATA_PATH", UHD_PKG_DATA_PATH);
}

std::vector<fs::path> get_image_paths(void){
    std::vector<fs::path> paths = get_env_paths("UHD_IMAGE_PATH");
    paths.push_back(fs::path(uhd::get_pkg_data_path()) / "images");
    return paths;
}

std::vector<fs::path> get_module_paths(void){
    std::vector<fs::path> paths = get_env_paths("UHD_MODULE_PATH");
    paths.push_back(fs::path(uhd::get_pkg_data_path()) / "modules");
    return paths;
}

/***********************************************************************
 * Implement the functions in paths.hpp
 **********************************************************************/
std::string uhd::get_tmp_path(void){
    const char *tmp_path = NULL;

    //try the official uhd temp path environment variable
    tmp_path = std::getenv("UHD_TEMP_PATH");
    if (tmp_path != NULL) return tmp_path;

    //try the windows function if available
    #ifdef USE_GET_TEMP_PATH
    char lpBuffer[2048];
    if (GetTempPath(sizeof(lpBuffer), lpBuffer)) return lpBuffer;
    #endif

    //try windows environment variables
    tmp_path = std::getenv("TMP");
    if (tmp_path != NULL) return tmp_path;

    tmp_path = std::getenv("TEMP");
    if (tmp_path != NULL) return tmp_path;

    //try the stdio define if available
    #ifdef P_tmpdir
    if (P_tmpdir != NULL) return P_tmpdir;
    #endif

    //try unix environment variables
    tmp_path = std::getenv("TMPDIR");
    if (tmp_path != NULL) return tmp_path;

    //give up and use the unix default
    return "/tmp";
}

std::string uhd::get_app_path(void){
    const char *appdata_path = std::getenv("APPDATA");
    if (appdata_path != NULL) return appdata_path;

    const char *home_path = std::getenv("HOME");
    if (home_path != NULL) return home_path;

    return uhd::get_tmp_path();
}
