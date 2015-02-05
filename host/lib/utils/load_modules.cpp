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

#include <uhd/utils/paths.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace fs = boost::filesystem;

/***********************************************************************
 * Module Load Function
 **********************************************************************/
#ifdef HAVE_DLOPEN
#include <dlfcn.h>
static void load_module(const std::string &file_name){
    if (dlopen(file_name.c_str(), RTLD_LAZY) == NULL){
        throw uhd::os_error(str(
            boost::format("dlopen failed to load \"%s\"") % file_name
        ));
    }
}
#endif /* HAVE_DLOPEN */


#ifdef HAVE_LOAD_LIBRARY
#include <windows.h>
static void load_module(const std::string &file_name){
    if (LoadLibrary(file_name.c_str()) == NULL){
        throw uhd::os_error(str(
            boost::format("LoadLibrary failed to load \"%s\"") % file_name
        ));
    }
}
#endif /* HAVE_LOAD_LIBRARY */


#ifdef HAVE_LOAD_MODULES_DUMMY
static void load_module(const std::string &file_name){
    throw uhd::not_implemented_error(str(
        boost::format("Module loading not supported: Cannot load \"%s\"") % file_name
    ));
}
#endif /* HAVE_LOAD_MODULES_DUMMY */

/***********************************************************************
 * Load Modules
 **********************************************************************/
/*!
 * Load all modules in a given path.
 * This will recurse into sub-directories.
 * Does not throw, prints to std error.
 * \param path the filesystem path
 */
static void load_module_path(const fs::path &path){
    if (not fs::exists(path)){
        //std::cerr << boost::format("Module path \"%s\" not found.") % path.string() << std::endl;
        return;
    }

    //try to load the files in this path
    if (fs::is_directory(path)){
        for(
            fs::directory_iterator dir_itr(path);
            dir_itr != fs::directory_iterator();
            ++dir_itr
        ){
            load_module_path(dir_itr->path());
        }
        return;
    }

    //its not a directory, try to load it
    try{
        load_module(path.string());
    }
    catch(const std::exception &err){
        std::cerr << boost::format("Error: %s") % err.what() << std::endl;
    }
}

/*!
 * Load all the modules given in the module paths.
 */
UHD_STATIC_BLOCK(load_modules){
    BOOST_FOREACH(const fs::path &path, uhd::get_module_paths()){
        load_module_path(path);
    }
}
