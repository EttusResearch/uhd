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

#include <uhd/utils/images.hpp>
#include <uhd/exception.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <vector>

namespace fs = boost::filesystem;

std::vector<fs::path> get_image_paths(void); //defined in paths.cpp

/***********************************************************************
 * Find a image in the image paths
 **********************************************************************/
std::string uhd::find_image_path(const std::string &image_name){
    if (fs::exists(image_name)){
        return fs::system_complete(image_name).string();
    }
    BOOST_FOREACH(const fs::path &path, get_image_paths()){
        fs::path image_path = path / image_name;
        if (fs::exists(image_path)) return image_path.string();
    }
    throw uhd::io_error("Could not find path for image: " + image_name);
}
