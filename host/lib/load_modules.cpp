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

#include <uhd/utils/static.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

namespace fs = boost::filesystem;

/***********************************************************************
 * Module Load Function
 **********************************************************************/
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>

static void load_module(const std::string &file_name){
    if (dlopen(file_name.c_str(), RTLD_LAZY) == NULL){
        throw std::runtime_error(str(
            boost::format("dlopen failed to load \"%s\"") % file_name
        ));
    }
}

#elif HAVE_WINDOWS_H
#include <windows.h>

static void load_module(const std::string &file_name){
    if (LoadLibrary(file_name.c_str()) == NULL){
        throw std::runtime_error(str(
            boost::format("LoadLibrary failed to load \"%s\"") % file_name
        ));
    }
}

#else

static void load_module(const std::string &file_name){
    throw std::runtime_error(str(
        boost::format("Module loading not supported: Cannot load \"%s\"") % file_name
    ));
}

#endif

/***********************************************************************
 * Load Modules
 **********************************************************************/
/*!
 * Load all modules in a given path.
 * This will recurse into sub-directories.
 * Does not throw, prints to std error.
 * \param path the filesystem path
 */
static void load_path(const fs::path &path){
    if (not fs::exists(path)){
        std::cerr << boost::format("Module path \"%s\" not found.") % path.file_string() << std::endl;
        return;
    }

    //try to load the files in this path
    if (fs::is_directory(path)){
        for(
            fs::directory_iterator dir_itr(path);
            dir_itr != fs::directory_iterator();
            ++dir_itr
        ){
            load_path(dir_itr->path());
        }
        return;
    }

    //its not a directory, try to load it
    try{
        load_module(path.file_string());
    }
    catch(const std::exception &err){
        std::cerr << boost::format("Error: %s") % err.what() << std::endl;
    }
}

/*!
 * Load all the modules given by the module path enviroment variable.
 * The path variable may be several paths split by path separators.
 */
UHD_STATIC_BLOCK(load_modules){
    //get the environment variable module path
    char *env_module_path = std::getenv("UHD_MODULE_PATH");
    if (env_module_path == NULL) return;

    //split the path at the path separators
    std::vector<std::string> module_paths;
    boost::split(module_paths, env_module_path, boost::is_any_of(":;"));

    //load modules in each path
    BOOST_FOREACH(const std::string &module_path, module_paths){
        load_path(fs::system_complete(fs::path(module_path)));
    }
}
