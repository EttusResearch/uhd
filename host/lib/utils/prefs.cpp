//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <uhdlib/utils/paths.hpp>
#include <uhdlib/utils/prefs.hpp>
#include <config.h>
#include <boost/filesystem.hpp>
#include <cstdlib>

using namespace uhd;

namespace {
constexpr char UHD_CONF_FILE_VAR[] = "UHD_CONFIG_FILE";

inline bool _update_conf_file(
    const std::string& path, const std::string& config_type, config_parser& conf_file)
{
    if (not path.empty()) {
        UHD_LOG_TRACE("PREFS", "Trying to load " << path);
        if (boost::filesystem::exists(path)) {
            try {
                conf_file.read_file(path);
                UHD_LOG_DEBUG(
                    "PREFS", "Loaded " << config_type << " config file " << path);
                return true;
            } catch (...) {
                UHD_LOG_DEBUG(
                    "PREFS", "Failed to load " << config_type << " config file " << path);
                return false;
            }
        } else {
            UHD_LOG_TRACE(
                "PREFS", "No " << config_type << " config file found at " << path);
            return false;
        }
    }
    return false;
}

void update_from_key(
    const std::string& key, const std::string& value, uhd::device_addr_t& user_args)
{
    if (value.empty()) {
        return;
    }

    const std::string key_str = key + "=" + value;
    for (const auto& key : uhd::prefs::get_uhd_config().options(key_str)) {
        user_args[key] = uhd::prefs::get_uhd_config().get<std::string>(key_str, key);
    }
}

device_addr_t get_args(const uhd::device_addr_t& user_args,
    const std::vector<std::string>& keys_to_update_from)
{
    device_addr_t args;
    for (const auto& key : keys_to_update_from) {
        update_from_key(key, user_args.get(key, ""), args);
    }

    // Finally, copy over the original user args:
    for (const auto& user_key : user_args.keys()) {
        args[user_key] = user_args[user_key];
    }

    return args;
}
} // namespace

config_parser& uhd::prefs::get_uhd_config()
{
    static config_parser _conf_files{};
    static bool init_done = false;
    if (not init_done) {
        UHD_LOG_TRACE("CONF", "Initializing config file object...");
        const std::string sys_conf_file = path_expandvars(UHD_SYS_CONF_FILE);
        _update_conf_file(sys_conf_file, "system", _conf_files);
        // prefer .config/uhd.conf
        // otherwise ~/.uhd/uhd.conf
        const std::string user_conf_file =
            (get_xdg_config_home() / std::string(UHD_USER_CONF_FILE)).string();
        const bool user_conf_loaded =
            _update_conf_file(user_conf_file, "user", _conf_files);
        // Config files can be in ~/.config/ or in ~/.uhd. The latter is
        // considered deprecated. We load from there (if we have not already
        // loaded from ~/.config), but we show a warning.
        if (!user_conf_loaded
            && _update_conf_file(
                   (get_legacy_config_home() / std::string(UHD_USER_CONF_FILE)).string(),
                   "user",
                   _conf_files)) {
            UHD_LOG_WARNING("PREFS",
                "Loaded config from " << get_legacy_config_home().string()
                                      << ". This location is considered deprecated, "
                                         "consider moving your config file to "
                                      << get_xdg_config_home().string() << " instead.");
        }
        std::string env_conf_file;
        try { // getenv into std::string can fail
            if (std::getenv(UHD_CONF_FILE_VAR) != NULL) {
                env_conf_file = std::string(std::getenv(UHD_CONF_FILE_VAR));
            }
            _update_conf_file(env_conf_file, "ENV", _conf_files);
        } catch (const std::exception&) {
            // nop
        }
        init_done = true;
        UHD_LOG_TRACE("PREFS", "Done initializing.");
    }

    return _conf_files;
}


device_addr_t uhd::prefs::get_usrp_args(const uhd::device_addr_t& user_args)
{
    const std::vector<std::string> keys_to_update_from = {"type", "product", "serial"};
    return get_args(user_args, keys_to_update_from);
}

device_addr_t uhd::prefs::get_dpdk_args(const uhd::device_addr_t& user_args)
{
    const std::vector<std::string> keys_to_update_from = {"use_dpdk"};
    return get_args(user_args, keys_to_update_from);
}

device_addr_t uhd::prefs::get_dpdk_nic_args(const uhd::device_addr_t& user_args)
{
    const std::vector<std::string> keys_to_update_from = {"dpdk_mac"};
    return get_args(user_args, keys_to_update_from);
}
