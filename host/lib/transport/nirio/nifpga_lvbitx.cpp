//
// Copyright 2013-2014 Ettus Research LLC
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

#include <uhd/transport/nirio/nifpga_lvbitx.h>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace uhd { namespace niusrprio {

std::string nifpga_lvbitx::_get_bitstream_checksum(const std::string& file_path)
{
    std::string checksum;
    std::ifstream lvbitx_stream(file_path.c_str());
    if (lvbitx_stream.is_open()) {
        std::string lvbitx_contents;
        lvbitx_stream.seekg(0, std::ios::end);
        lvbitx_contents.reserve(static_cast<size_t>(lvbitx_stream.tellg()));
        lvbitx_stream.seekg(0, std::ios::beg);
        lvbitx_contents.assign((std::istreambuf_iterator<char>(lvbitx_stream)), std::istreambuf_iterator<char>());
        try {
            boost::smatch md5_match;
            if (boost::regex_search(lvbitx_contents, md5_match, boost::regex("<BitstreamMD5>([a-zA-Z0-9]{32})<\\/BitstreamMD5>", boost::regex::icase))) {
                checksum = std::string(md5_match[1].first, md5_match[1].second);
            }
        } catch (boost::exception&) {
            checksum = "";
        }
    }
    boost::to_upper(checksum);
    return checksum;
}

#ifdef UHD_PLATFORM_WIN32
#include <windows.h>

std::string _get_path_from_registry(const std::string& registry_key_path)
{
    boost::smatch reg_key_match;
    //If a substring in the search path is enclosed in [] (square brackets) then it is interpreted as a registry path
    if (not boost::regex_search(registry_key_path, reg_key_match, boost::regex("\\[(.+)\\](.*)", boost::regex::icase)))
        return std::string();
    std::string reg_key_path = std::string(reg_key_match[1].first, reg_key_match[1].second);
    std::string path_suffix = std::string(reg_key_match[2].first, reg_key_match[2].second);

    //Split the registry path into parent, key-path and value.
    boost::smatch reg_parent_match;
    if (not boost::regex_search(reg_key_path, reg_parent_match, boost::regex("^(.+?)\\\\(.+)\\\\(.+)$", boost::regex::icase)))
        return std::string();
    std::string reg_parent = std::string(reg_parent_match[1].first, reg_parent_match[1].second);
    std::string reg_path = std::string(reg_parent_match[2].first, reg_parent_match[2].second);
    std::string reg_val_name = std::string(reg_parent_match[3].first, reg_parent_match[3].second);

    HKEY hkey_parent = HKEY_LOCAL_MACHINE;
    if      (reg_parent == "HKEY_LOCAL_MACHINE")    hkey_parent = HKEY_LOCAL_MACHINE;
    else if (reg_parent == "HKEY_CURRENT_USER")     hkey_parent = HKEY_CURRENT_USER;
    else if (reg_parent == "HKEY_CLASSES_ROOT")     hkey_parent = HKEY_CLASSES_ROOT;
    else if (reg_parent == "HKEY_CURRENT_CONFIG")   hkey_parent = HKEY_CURRENT_CONFIG;
    else if (reg_parent == "HKEY_USERS")            hkey_parent = HKEY_CURRENT_USER;

    TCHAR value_buff[1024];
    DWORD value_buff_size = 1024*sizeof(TCHAR);

    //Get a handle to the key location
    HKEY hkey_location;
    if (RegOpenKeyExA(hkey_parent, reg_path.c_str(), NULL, KEY_QUERY_VALUE, &hkey_location) != ERROR_SUCCESS)
        return std::string();

    //Query key value
    DWORD dw_type = REG_SZ;
    if(RegQueryValueExA(hkey_location, reg_val_name.c_str(), NULL, &dw_type, (LPBYTE)value_buff, &value_buff_size) == ERROR_SUCCESS) {
        RegCloseKey(hkey_location);
        if (value_buff_size >= 1024*sizeof(TCHAR)) {
            return std::string();
        } else {
            std::string return_value(value_buff, value_buff_size-1); //value_buff_size includes the null terminator
            return_value += path_suffix;
            return return_value;
        }
    } else {
        return std::string();
    }
}

#endif  /*UHD_PLATFORM_WIN32*/

std::string nifpga_lvbitx::_get_fpga_images_dir(const std::string search_paths)
{
    std::vector<std::string> search_path_vtr;
    boost::split(search_path_vtr, search_paths, boost::is_any_of(","));

    //
    // Add the value of the UHD_IMAGES_DIR environment variable to the list of
    // directories searched for a LVBITX image.
    //
    char* uhd_images_dir;
#ifdef UHD_PLATFORM_WIN32
    size_t len;
    errno_t err = _dupenv_s(&uhd_images_dir, &len, "UHD_IMAGES_DIR");
    if(not err and uhd_images_dir != NULL) search_path_vtr.push_back(std::string(uhd_images_dir));
    free(uhd_images_dir);
#else
    uhd_images_dir = getenv("UHD_IMAGES_DIR");
    if(uhd_images_dir != NULL) search_path_vtr.push_back(std::string(uhd_images_dir));
#endif

    std::string lvbitx_dir;
    //Traverse through the list of search paths. Priority: lexical
    BOOST_FOREACH(std::string& search_path, search_path_vtr) {
        boost::algorithm::trim(search_path);
        if (search_path.empty()) continue;

#ifdef UHD_PLATFORM_WIN32
        lvbitx_dir = _get_path_from_registry(search_path);
        if (lvbitx_dir.empty()) {
            //Could not read from the registry due to missing key, invalid values, etc
            //Just use the search path. The is_directory check will fail if this is a
            //registry path and we will move on to the next item in the list.
            lvbitx_dir = search_path;
        }
#else
        lvbitx_dir = search_path;
#endif

        //If the current directory exists then stop traversing the search path list.
        if (boost::filesystem::is_directory(lvbitx_dir)) break;
    }

    return lvbitx_dir;
}


}}
