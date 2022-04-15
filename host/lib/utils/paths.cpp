//
// Copyright 2010-2012,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/build_info.hpp>
#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <uhdlib/utils/paths.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/version.hpp>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <streambuf>
#include <string>
#include <vector>

#ifdef BOOST_MSVC
#    define USE_GET_TEMP_PATH
#    include <windows.h> //GetTempPath
#endif

namespace fs = boost::filesystem;

static constexpr char UHD_CAL_DATA_PATH_VAR[] = "UHD_CAL_DATA_PATH";

/*! Get the value of an environment variable.
 *
 * The returned std::string is the full environment variable string, and thus
 * may actually contain multiple fields in the string with delimiters.
 *
 * \param var_name The name of the variable to search for.
 * \param default_val A default string value to use if the path isn't found.
 * \returns The string value of the environment variable.
 */
static std::string get_env_var(
    const std::string& var_name, const std::string& default_val = "")
{
    std::string env_result = default_val;
    char* env_var_str      = NULL;

    /* Some versions of MinGW don't expose `_dupenv_s` */
#if defined(UHD_PLATFORM_WIN32) && !defined(__MINGW32__)
    size_t len;
    errno_t err = _dupenv_s(&env_var_str, &len, var_name.c_str());
    if ((not err) and (env_var_str != NULL))
        env_result = std::string(env_var_str);
    free(env_var_str);
#else
    env_var_str = std::getenv(var_name.c_str());
    if (env_var_str != NULL)
        env_result = std::string(env_var_str);
#endif

    return env_result;
}

/*! Get a vector of paths from an environment variable.
 *
 * Reads an environment variable, which should contain a list of paths, and
 * returns a vector of those paths in the form of strings.
 *
 * \param var_name The environment variable name to read.
 * \returns The vector of paths from the environment variable.
 */
static std::vector<std::string> get_env_paths(const std::string& var_name)
{
#ifdef UHD_PLATFORM_WIN32
    static const std::string env_path_sep = ";";
#else
    static const std::string env_path_sep = ":";
#endif /*UHD_PLATFORM_WIN32*/

#define path_tokenizer(inp)                        \
    boost::tokenizer<boost::char_separator<char>>( \
        inp, boost::char_separator<char>(env_path_sep.c_str()))

    std::string var_value = get_env_var(var_name);

    std::vector<std::string> paths;

    // convert to full filesystem path, filter blank paths
    if (var_value.empty())
        return paths;
    for (const std::string& path_string : path_tokenizer(var_value)) {
        if (path_string.empty())
            continue;
        paths.push_back(fs::system_complete(path_string).string());
    }

    return paths;
}

#ifndef UHD_PLATFORM_WIN32
// NOTE: This could be replaced by path_expandvars()
/*! Expand a tilde character to the $HOME path.
 *
 * The path passed to this function must start with the tilde character in order
 * for this function to work properly. If it does not, it will simply return the
 * original path. The $HOME environment variable must exist.
 *
 * \param path The path starting with the tilde character
 * \returns The same path with the tilde expanded to contents of $HOME.
 */
static std::string expand_home_directory(std::string path)
{
    boost::trim(path);

    if (path.empty() || (path[0] != '~')) {
        return path;
    }

    std::string user_home_path = get_env_var("HOME");
    path.replace(0, 1, user_home_path);

    return path;
}
#endif

fs::path uhd::get_xdg_data_home()
{
    std::string xdg_data_home_str = get_env_var("XDG_DATA_HOME", "");
    fs::path xdg_data_home(xdg_data_home_str);
    if (!xdg_data_home_str.empty()) {
        return fs::path(xdg_data_home_str);
    }
#ifdef UHD_PLATFORM_WIN32
    const std::string localappdata = get_env_var("LOCALAPPDATA", "");
    if (!localappdata.empty()) {
        return fs::path(localappdata);
    }
    const std::string appdata = get_env_var("APPDATA", "");
    if (!appdata.empty()) {
        return fs::path(appdata);
    }
#endif
    const std::string home = get_env_var("HOME", "");
    if (home.empty()) {
#ifdef UHD_PLATFORM_WIN32
        const std::string err_msg =
            "get_xdg_data_home(): Unable to find \%HOME\%, \%XDG_DATA_HOME\%, "
            "\%LOCALAPPDATA\% or \%APPDATA\%.";
#else
        const std::string err_msg =
            "get_xdg_data_home(): Unable to find $HOME or $XDG_DATA_HOME.";
#endif
        throw uhd::runtime_error(err_msg);
    }
    return fs::path(home) / ".local" / "share";
}

fs::path uhd::get_xdg_config_home()
{
    std::string xdg_config_home_str = get_env_var("XDG_CONFIG_HOME", "");
    fs::path xdg_config_home(xdg_config_home_str);
    if (!xdg_config_home_str.empty()) {
        return fs::path(xdg_config_home_str);
    }
#ifdef UHD_PLATFORM_WIN32
    const std::string localappdata = get_env_var("LOCALAPPDATA", "");
    if (!localappdata.empty()) {
        return fs::path(localappdata);
    }
    const std::string appdata = get_env_var("APPDATA", "");
    if (!appdata.empty()) {
        return fs::path(appdata);
    }
#endif
    const std::string home = get_env_var("HOME", "");
    if (home.empty()) {
#ifdef UHD_PLATFORM_WIN32
        const std::string err_msg =
            "get_xdg_config_home(): Unable to find \%HOME\%, \%XDG_CONFIG_HOME\%, "
            "\%LOCALAPPDATA\% or \%APPDATA\%.";
#else
        const std::string err_msg =
            "get_xdg_config_home(): Unable to find $HOME or $XDG_CONFIG_HOME.";
#endif
        throw uhd::runtime_error(err_msg);
    }
    return fs::path(home) / ".config";
}

fs::path uhd::get_legacy_config_home()
{
#ifdef UHD_PLATFORM_WIN32
    const std::string localappdata = get_env_var("LOCALAPPDATA", "");
    if (!localappdata.empty()) {
        return fs::path(localappdata) / ".uhd";
    }
    const std::string appdata = get_env_var("APPDATA", "");
    if (!appdata.empty()) {
        return fs::path(appdata) / ".uhd";
    }
#endif
    const std::string home = get_env_var("HOME", "");
    if (home.empty()) {
        throw uhd::runtime_error("Unable to find $HOME.");
    }
    return fs::path(home) / ".uhd";
}

/***********************************************************************
 * Implement the functions in paths.hpp
 **********************************************************************/


std::string uhd::get_tmp_path(void)
{
    const char* tmp_path = NULL;

    // try the official uhd temp path environment variable
    tmp_path = std::getenv("UHD_TEMP_PATH");
    if (tmp_path != NULL)
        return tmp_path;

// try the windows function if available
#ifdef USE_GET_TEMP_PATH
    char lpBuffer[2048];
    if (GetTempPath(sizeof(lpBuffer), lpBuffer))
        return lpBuffer;
#endif

    // try windows environment variables
    tmp_path = std::getenv("TMP");
    if (tmp_path != NULL)
        return tmp_path;

    tmp_path = std::getenv("TEMP");
    if (tmp_path != NULL)
        return tmp_path;

// try the stdio define if available
#ifdef P_tmpdir
    return P_tmpdir;
#else

    // try unix environment variables
    tmp_path = std::getenv("TMPDIR");
    if (tmp_path != NULL)
        return tmp_path;

    // give up and use the unix default
    return "/tmp";
#endif
}

// Only used for deprecated CSV file loader. Delete this once CSV support is
// removed.
std::string uhd::get_appdata_path(void)
{
    const std::string uhdcalib_path = get_env_var("UHD_CONFIG_DIR");
    if (not uhdcalib_path.empty()) {
        UHD_LOG_WARNING("UHD",
            "The environment variable UHD_CONFIG_DIR is deprecated. Refer to "
            "https://files.ettus.com/manual/page_calibration.html for how to store "
            "calibration data.");
        return uhdcalib_path;
    }

    const std::string appdata_path = get_env_var("APPDATA");
    if (not appdata_path.empty())
        return appdata_path;

    const std::string home_path = get_env_var("HOME");
    if (not home_path.empty())
        return home_path;

    return uhd::get_tmp_path();
}


std::string uhd::get_pkg_path(void)
{
    fs::path pkg_path = fs::path(uhd::get_lib_path()).parent_path().lexically_normal();
    return get_env_var("UHD_PKG_PATH", pkg_path.string());
}

std::string uhd::get_lib_path(void)
{
    fs::path runtime_libfile_path = boost::dll::this_line_location();
    // Normalize before decomposing path so result is reliable
    fs::path lib_path = runtime_libfile_path.lexically_normal().parent_path();
    return lib_path.string();
}

std::string uhd::get_cal_data_path(void)
{
    // The easy case: User has set the environment variable
    const std::string uhdcalib_path = get_env_var(UHD_CAL_DATA_PATH_VAR);
    if (not uhdcalib_path.empty()) {
        return uhdcalib_path;
    }

    // If not, we use the default location
    const fs::path cal_data_path = get_xdg_data_home() / "uhd" / "cal";
    return cal_data_path.string();
}

std::vector<fs::path> uhd::get_module_paths(void)
{
    std::vector<fs::path> paths;

    std::vector<std::string> env_paths = get_env_paths("UHD_MODULE_PATH");
    for (std::string& str_path : env_paths) {
        paths.push_back(str_path);
    }

    paths.push_back(fs::path(uhd::get_lib_path()) / "uhd" / "modules");
    paths.push_back(fs::path(uhd::get_pkg_path()) / "share" / "uhd" / "modules");

    return paths;
}

#ifdef UHD_PLATFORM_WIN32
#    include <windows.h>
/*!
 * On Windows, query the system registry for the UHD images install path.
 * If the key isn't found in the registry, an empty string is returned.
 * \param registry_key_path The registry key to look for.
 * \return The images path, formatted for windows.
 */
std::string _get_images_path_from_registry(const std::string& registry_key_path)
{
    std::smatch reg_key_match;
    // If a substring in the search path is enclosed in [] (square brackets) then it is
    // interpreted as a registry path
    if (not std::regex_search(registry_key_path,
            reg_key_match,
            std::regex("\\[(.+)\\](.*)", std::regex::icase)))
        return std::string();
    std::string reg_key_path =
        std::string(reg_key_match[1].first, reg_key_match[1].second);
    std::string path_suffix =
        std::string(reg_key_match[2].first, reg_key_match[2].second);

    // Split the registry path into parent, key-path and value.
    std::smatch reg_parent_match;
    if (not std::regex_search(reg_key_path,
            reg_parent_match,
            std::regex("^(.+?)\\\\(.+)\\\\(.+)$", std::regex::icase)))
        return std::string();
    std::string reg_parent =
        std::string(reg_parent_match[1].first, reg_parent_match[1].second);
    std::string reg_path =
        std::string(reg_parent_match[2].first, reg_parent_match[2].second);
    std::string reg_val_name =
        std::string(reg_parent_match[3].first, reg_parent_match[3].second);

    HKEY hkey_parent = HKEY_LOCAL_MACHINE;
    if (reg_parent == "HKEY_LOCAL_MACHINE")
        hkey_parent = HKEY_LOCAL_MACHINE;
    else if (reg_parent == "HKEY_CURRENT_USER")
        hkey_parent = HKEY_CURRENT_USER;
    else if (reg_parent == "HKEY_CLASSES_ROOT")
        hkey_parent = HKEY_CLASSES_ROOT;
    else if (reg_parent == "HKEY_CURRENT_CONFIG")
        hkey_parent = HKEY_CURRENT_CONFIG;
    else if (reg_parent == "HKEY_USERS")
        hkey_parent = HKEY_CURRENT_USER;

    TCHAR value_buff[1024];
    DWORD value_buff_size = 1024 * sizeof(TCHAR);

    // Get a handle to the key location
    HKEY hkey_location;
    if (RegOpenKeyExA(hkey_parent, reg_path.c_str(), 0, KEY_QUERY_VALUE, &hkey_location)
        != ERROR_SUCCESS)
        return std::string();

    // Query key value
    DWORD dw_type = REG_SZ;
    if (RegQueryValueExA(hkey_location,
            reg_val_name.c_str(),
            NULL,
            &dw_type,
            (LPBYTE)value_buff,
            &value_buff_size)
        == ERROR_SUCCESS) {
        RegCloseKey(hkey_location);
        if (value_buff_size >= 1024 * sizeof(TCHAR)) {
            return std::string();
        } else {
            std::string return_value(value_buff,
                value_buff_size - 1); // value_buff_size includes the null terminator
            return_value += path_suffix;
            return return_value;
        }
    } else {
        return std::string();
    }
}
#endif /*UHD_PLATFORM_WIN32*/

std::string uhd::get_images_dir(const std::string& search_paths)
{
    /*   This function will check for the existence of directories in this
     *   order:
     *
     *   1) `UHD_IMAGES_DIR` environment variable
     *   2) Any paths passed to this function via `search_paths' (may contain
     *      Windows registry keys)
     *   3) `UHD package path` / share / uhd / images
     */

    std::string possible_dir;

    /* We will start by looking for a path indicated by the `UHD_IMAGES_DIR`
     * environment variable. */
    std::vector<std::string> env_paths = get_env_paths("UHD_IMAGES_DIR");
    for (auto possible_dir : env_paths) {
        if (fs::is_directory(fs::path(possible_dir))) {
            return possible_dir;
        }
    }

    /* On Windows systems, we may need to modify the `search_paths` parameter
     * (see below). Making a local copy for const correctness. */
    std::string _search_paths = search_paths;

#ifdef UHD_IMAGES_DIR_WINREG_KEY
    _search_paths = std::string(STR(UHD_IMAGES_DIR_WINREG_KEY)) + "," + search_paths;
#endif

    /* Now we will parse and attempt to qualify the paths in the `search_paths`
     * parameter. If this is Windows, we will check the system registry for
     * these strings. */
    if (!_search_paths.empty()) {
        std::vector<std::string> search_paths_vector;

        boost::split(search_paths_vector, _search_paths, boost::is_any_of(",;"));
        for (std::string& search_path : search_paths_vector) {
            boost::algorithm::trim(search_path);
            if (search_path.empty())
                continue;

#ifdef UHD_PLATFORM_WIN32
            possible_dir = _get_images_path_from_registry(search_path);
            if (possible_dir.empty()) {
                // Could not read from the registry due to missing key, invalid
                // values, etc Just use the search path. The is_directory check
                // will fail if this is a registry path and we will move on to
                // the next item in the list.
                possible_dir = search_path;
            }
#else
            possible_dir = expand_home_directory(search_path);
#endif

            if (fs::is_directory(fs::path(possible_dir))) {
                return possible_dir;
            }
        }
    }

    /* Finally, check for the default UHD images installation paths */
    for (auto& prefix : {uhd::get_pkg_path(), uhd::build_info::install_prefix()}) {
        fs::path default_images_path = fs::path(prefix) / "share" / "uhd" / "images";
        if (fs::is_directory(default_images_path)) {
            return default_images_path.string();
        }
    }

    /* No luck. Return an empty string. */
    return std::string("");
}

std::string uhd::find_image_path(
    const std::string& image_name, const std::string& search_paths)
{
    /* If a path was provided on the command-line or as a hint from the caller,
     * we default to that. */
    if (fs::exists(image_name)) {
        return fs::system_complete(image_name).string();
    }

    /* Otherwise, look for the image in the images directory. */
    std::string images_dir = get_images_dir(search_paths);
    if (!images_dir.empty()) {
        fs::path image_path = fs::path(images_dir) / image_name;
        if (fs::exists(image_path)) {
            return image_path.string();
        } else {
            throw uhd::io_error("Could not find the image '" + image_name
                                + "' in the image directory " + images_dir
                                + "\nFor more information regarding image paths, please "
                                  "refer to the UHD manual.");
        }
    }

    /* If we made it this far, then we didn't find anything. */
    images_dir = "<no images directory located>";
    throw uhd::io_error("Could not find path for image: " + image_name + "\n\n"
                        + "Using images directory: " + images_dir + "\n\n"
                        + "Set the environment variable 'UHD_IMAGES_DIR' appropriately or"
                        + " follow the below instructions to download the images package."
                        + "\n\n" + uhd::print_utility_error("uhd_images_downloader.py"));
}

std::string uhd::find_utility(const std::string& name)
{
    return fs::path(fs::path(uhd::get_lib_path()) / "uhd" / "utils" / name).string();
}

std::string uhd::print_utility_error(const std::string& name, const std::string& args)
{
#ifdef UHD_PLATFORM_WIN32
    return "As an Administrator, please run:\n\n\"" + find_utility(name) + args + "\"";
#else
    return "Please run:\n\n \"" + find_utility(name) + (args.empty() ? "" : (" " + args))
           + "\"";
#endif
}
