//
// Copyright 2011,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/device_addr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <regex>
#include <sstream>
#include <stdexcept>

using namespace uhd;

static const char* arg_delim  = ",";
static const char* pair_delim = "=";

static std::string trim(const std::string& in)
{
    return boost::algorithm::trim_copy(in);
}

#define tokenizer(inp, sep) \
    boost::tokenizer<boost::char_separator<char>>(inp, boost::char_separator<char>(sep))

device_addr_t::device_addr_t(const std::string& args)
{
    for (const std::string& pair : tokenizer(args, arg_delim)) {
        if (trim(pair).empty())
            continue;
        std::vector<std::string> toks;
        for (const std::string& tok : tokenizer(pair, pair_delim)) {
            toks.push_back(tok);
        }
        if (toks.size() == 1)
            toks.push_back(""); // pad empty value
        if (toks.size() == 2 and not trim(toks[0]).empty()) { // only valid combination
            this->set(trim(toks[0]), trim(toks[1]));
        } else
            throw uhd::value_error("invalid args string: " + args); // otherwise error
    }
}

device_addr_t::device_addr_t(const char* args) :
    device_addr_t(std::string(args))
{
    // No additional construction is necessary
}

device_addr_t::device_addr_t(const std::map<std::string, std::string>& info)
{
    for (auto& t : info) {
        this->set(t.first, t.second);
    }
}

std::string device_addr_t::to_pp_string(void) const
{
    if (this->size() == 0)
        return "Empty Device Address";

    std::stringstream ss;
    ss << "Device Address:" << std::endl;
    for (std::string key : this->keys()) {
        ss << boost::format("    %s: %s") % key % this->get(key) << std::endl;
    }
    return ss.str();
}

std::string device_addr_t::to_string(void) const
{
    std::string args_str;
    size_t count = 0;
    for (const std::string& key : this->keys()) {
        args_str += ((count++) ? arg_delim : "") + key + pair_delim + this->get(key);
    }
    return args_str;
}

#include <uhd/utils/log.hpp>

device_addrs_t uhd::separate_device_addr(const device_addr_t& dev_addr)
{
    //------------ support old deprecated way and print warning --------
    if (dev_addr.has_key("addr") and not dev_addr["addr"].empty()) {
        std::vector<std::string> addrs;
        boost::split(addrs, dev_addr["addr"], boost::is_any_of(" "));
        if (addrs.size() > 1) {
            device_addr_t fixed_dev_addr = dev_addr;
            fixed_dev_addr.pop("addr");
            for (size_t i = 0; i < addrs.size(); i++) {
                fixed_dev_addr[str(boost::format("addr%d") % i)] = addrs[i];
            }
            UHD_LOGGER_WARNING("UHD")
                << "addr = <space separated list of ip addresses> is deprecated.\n"
                   "To address a multi-device, use multiple <key><index> = <val>.\n"
                   "See the USRP-NXXX application notes. Two device example:\n"
                   "    addr0 = 192.168.10.2\n"
                   "    addr1 = 192.168.10.3\n";
            return separate_device_addr(fixed_dev_addr);
        }
    }
    //------------------------------------------------------------------
    device_addrs_t dev_addrs(1); // must be at least one (obviously)
    std::vector<std::string> global_keys; // keys that apply to all (no numerical suffix)
    for (const std::string& key : dev_addr.keys()) {
        std::cmatch matches;
        // Key must start with a non-digit, and may optionally end with a digit
        // that indicates the mb index. Also allow keys that have integers within
        // the name, such as recv_offload_thread_0_cpu.
        if (not std::regex_match(
                key.c_str(), matches, std::regex("^(\\D+\\d*\\D+)(\\d*)$"))) {
            throw std::runtime_error("unknown key format: " + key);
        }
        std::string key_part(matches[1].first, matches[1].second);
        std::string num_part(matches[2].first, matches[2].second);
        if (num_part.empty()) { // no number? save it for later
            global_keys.push_back(key);
            continue;
        }
        const size_t num = boost::lexical_cast<size_t>(num_part);
        dev_addrs.resize(std::max(num + 1, dev_addrs.size()));
        dev_addrs[num][key_part] = dev_addr[key];
    }

    // copy the global settings across all device addresses
    for (device_addr_t& my_dev_addr : dev_addrs) {
        for (const std::string& global_key : global_keys) {
            my_dev_addr[global_key] = dev_addr[global_key];
        }
    }
    return dev_addrs;
}

device_addr_t uhd::combine_device_addrs(const device_addrs_t& dev_addrs)
{
    device_addr_t dev_addr;
    for (size_t i = 0; i < dev_addrs.size(); i++) {
        for (const std::string& key : dev_addrs[i].keys()) {
            dev_addr[str(boost::format("%s%d") % key % i)] = dev_addrs[i][key];
        }
    }
    return dev_addr;
}
