//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0+
//

#include <config.h>
#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <uhdlib/utils/prefs.hpp>
#include <uhdlib/utils/paths.hpp>
#include <boost/filesystem.hpp>
#include <cstdlib>

using namespace uhd;

namespace {
    constexpr char UHD_CONF_FILE_VAR[] = "UHD_CONFIG_FILE";

    inline void _update_conf_file(
            const std::string& path,
            const std::string& config_type,
            config_parser& conf_file
    ) {
        if (not path.empty()) {
                UHD_LOG_TRACE("PREFS", "Trying to load " << path);
            try {
                conf_file.read_file(path);
                UHD_LOG_DEBUG("PREFS",
                        "Loaded " << config_type << " config file " << path);
            } catch (...) {
                // nop
            }
        }
    }

    void update_from_key(
        const std::string& key, const std::string &value,
        uhd::device_addr_t& user_args
    ) {
        if (value.empty()) {
            return;
        }

        const std::string key_str = key + "=" + value;
        for (const auto& key : uhd::prefs::get_uhd_config().options(key_str)) {
            user_args[key] =
                uhd::prefs::get_uhd_config().get<std::string>(key_str, key);
        }
    }
}

config_parser& uhd::prefs::get_uhd_config()
{
    static config_parser _conf_files{};
    static bool init_done = false;
    if (not init_done) {
        UHD_LOG_TRACE("CONF", "Initializing config file object...");
        const std::string sys_conf_file = path_expandvars(UHD_SYS_CONF_FILE);
        _update_conf_file(sys_conf_file, "system", _conf_files);
        const std::string user_conf_file =
            (boost::filesystem::path(get_app_path())
                / std::string(UHD_USER_CONF_FILE)).string();
        _update_conf_file(user_conf_file, "user", _conf_files);
        std::string env_conf_file;
        try { // getenv into std::string can fail
            if (std::getenv(UHD_CONF_FILE_VAR) != NULL) {
                env_conf_file = std::string(std::getenv(UHD_CONF_FILE_VAR));
            }
            _update_conf_file(env_conf_file, "ENV", _conf_files);
        } catch (const std::exception &) {
            // nop
        }
        UHD_LOG_TRACE("PREFS", "Done initializing.");
    }

    return _conf_files;
}


device_addr_t uhd::prefs::get_usrp_args(
    const uhd::device_addr_t &user_args
) {
    device_addr_t usrp_args;
    const std::vector<std::string> keys_to_update_from = {
        "type",
        "product",
        "serial"
    };

    for (const auto& key : keys_to_update_from) {
        update_from_key(key, user_args.get(key, ""), usrp_args);
    }

    // Finally, copy over the original user args:
    for (const auto &user_key : user_args.keys()) {
        usrp_args[user_key] = user_args[user_key];
    }

    return usrp_args;
}

