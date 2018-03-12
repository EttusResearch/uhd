//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/config_parser.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>

using namespace uhd;

config_parser::config_parser(const std::string &path)
{
    if (not path.empty() and boost::filesystem::exists(path)) {
        try {
            boost::property_tree::ini_parser::read_ini(path, _pt);
        } catch (const boost::property_tree::ini_parser_error &) {
            throw uhd::runtime_error(str(
                boost::format("Unable to parse file %s")
                % path
            ));
        }
    }
}

void config_parser::read_file(const std::string &path)
{
    config_parser new_config(path);
    for (const auto& section : new_config.sections()) {
        for (const auto& key : new_config.options(section)) {
            set<std::string>(
                    section,
                    key,
                    new_config.get<std::string>(section, key)
            );
        }
    }
}

std::vector<std::string> config_parser::sections()
{
    try {
        return _options(_pt);
    } catch (const boost::property_tree::ptree_bad_path &) {
        return std::vector<std::string>{};
    }
}

std::vector<std::string> config_parser::options(const std::string &section)
{
    try {
        return _options(_pt.get_child(section));
    } catch (const boost::property_tree::ptree_bad_path &) {
        return std::vector<std::string>{};
    }
}

