//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// find-related code for MPM devices

#include "mpmd_impl.hpp"
#include "mpmd_devices.hpp"
#include <uhd/types/device_addr.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

using namespace uhd;
using namespace uhd::mpmd;

namespace {
    //! How long we wait for discovery responses (in seconds)
    constexpr double MPMD_FIND_TIMEOUT = 0.5;
    constexpr char MPMD_CHDR_REACHABILITY_KEY[] = "reachable";
    constexpr char MPMD_CHDR_REACHABILITY_NEGATIVE[] = "No";
    //! The preamble for any response on the discovery port. Can be used to
    //  verify that the response is actually an MPM device.
    constexpr char MPM_DISC_RESPONSE_PREAMBLE[] = "USRP-MPM";

    device_addr_t flag_dev_as_unreachable(const device_addr_t& device_args)
    {
        device_addr_t flagged_device_args(device_args);
        flagged_device_args[MPMD_CHDR_REACHABILITY_KEY] =
            MPMD_CHDR_REACHABILITY_NEGATIVE;
        return flagged_device_args;
    }
}

device_addrs_t mpmd_find_with_addr(
        const std::string& mgmt_addr,
        const device_addr_t& hint_
) {
    UHD_ASSERT_THROW(not mgmt_addr.empty());
    const std::string mpm_discovery_port = hint_.get(
        mpmd_impl::MPM_DISCOVERY_PORT_KEY,
        std::to_string(mpmd_impl::MPM_DISCOVERY_PORT)
    );
    UHD_LOG_DEBUG("MPMD",
        "Discovering MPM devices on port " << mpm_discovery_port);

    device_addrs_t addrs;
    transport::udp_simple::sptr comm = transport::udp_simple::make_broadcast(
        mgmt_addr, mpm_discovery_port);
    comm->send(
        boost::asio::buffer(
            mpmd_impl::MPM_DISCOVERY_CMD.c_str(),
            mpmd_impl::MPM_DISCOVERY_CMD.size()
        )
    );
    while (true) {
        const size_t MAX_MTU = 8000;
        char buff[MAX_MTU] = {};
        const size_t nbytes = comm->recv(
                boost::asio::buffer(buff, MAX_MTU),
                MPMD_FIND_TIMEOUT
        );
        if (nbytes == 0) {
            break;
        }
        const char* reply = (const char*)buff;
        std::string reply_string = std::string(reply);
        std::vector<std::string> result;
        boost::algorithm::split(result, reply_string,
                                [](const char& in) { return in == ';'; },
                                boost::token_compress_on);
        if (result.empty()) {
            continue;
        }
        // Verify we didn't receive something other than an MPM discovery
        // response
        if (result[0] != MPM_DISC_RESPONSE_PREAMBLE) {
            continue;
        }
        const std::string recv_addr = comm->get_recv_addr();

        // remove external iface addrs if executed directly on device
        bool external_iface = false;
        for (const auto& addr : transport::get_if_addrs()) {
            if ((addr.inet == comm->get_recv_addr()) &&
                recv_addr !=
                    boost::asio::ip::address_v4::loopback().to_string()) {
                external_iface = true;
                break;
            }
        }
        if (external_iface) {
            continue;
        }

        // Create result to return
        device_addr_t new_addr;
        new_addr[xport::MGMT_ADDR_KEY] = recv_addr;
        new_addr["type"] = "mpmd"; // hwd will overwrite this
        // remove ident string and put other informations into device_args dict
        result.erase(result.begin());
        // parse key-value pairs in the discovery string and add them to the
        // device_args
        for (const auto& el : result) {
            std::vector<std::string> value;
            boost::algorithm::split(value, el,
                                    [](const char& in) { return in == '='; },
                                    boost::token_compress_on);
            if (value[0] != xport::MGMT_ADDR_KEY) {
                new_addr[value[0]] = value[1];
            }
        }
        // filter the discovered device below by matching optional keys
        if (
            (not hint_.has_key("name")        or hint_["name"]    == new_addr["name"])
            and (not hint_.has_key("serial")  or hint_["serial"]  == new_addr["serial"])
            and (not hint_.has_key("type")    or hint_["type"]    == new_addr["type"] or hint_["type"] == MPM_CATCHALL_DEVICE_TYPE)
            and (not hint_.has_key("product") or hint_["product"] == new_addr["product"])
        ){
            UHD_LOG_TRACE("MPMD FIND",
                "Found device that matches hints: " << new_addr.to_string());
            addrs.push_back(new_addr);
        } else {
            UHD_LOG_DEBUG("MPMD FIND",
                    "Found device, but does not match hint: " << recv_addr
            );
        }
    }
    return addrs;
};



// Implements scenario 1) (see below)
device_addrs_t mpmd_find_with_addrs(const device_addrs_t& hints)
{
    UHD_LOG_TRACE("MPMD FIND", "Finding with addrs.");
    device_addrs_t found_devices;
    found_devices.reserve(hints.size());
    for (const auto& hint : hints) {
        if (not (hint.has_key(xport::FIRST_ADDR_KEY) or
                 hint.has_key(xport::MGMT_ADDR_KEY))) {
            UHD_LOG_DEBUG("MPMD FIND",
                "No address given in hint " << hint.to_string());
            continue;
        }
        const std::string mgmt_addr =
            hint.get(xport::MGMT_ADDR_KEY, hint.get(xport::FIRST_ADDR_KEY, ""));
        device_addrs_t reply_addrs = mpmd_find_with_addr(mgmt_addr, hint);
        if (reply_addrs.size() > 1) {
            UHD_LOG_ERROR("MPMD",
                "Could not resolve device hint \"" << hint.to_string()
                << "\" to a unique device.");
            continue;
        } else if (reply_addrs.empty()) {
            continue;
        }
        UHD_LOG_TRACE("MPMD FIND",
            "Device responded: " << reply_addrs[0].to_string());
        found_devices.push_back(reply_addrs[0]);
    }
    if (found_devices.size() == 0) {
        return device_addrs_t();
    } else if (found_devices.size() == 1) {
        return found_devices;
    } else {
        return device_addrs_t(1, combine_device_addrs(found_devices));
    }
}

device_addrs_t mpmd_find_with_bcast(const device_addr_t& hint)
{
    device_addrs_t addrs;
    UHD_LOG_TRACE("MPMD FIND",
            "Broadcasting on all available interfaces to find MPM devices.");
    for (const transport::if_addrs_t& if_addr : transport::get_if_addrs()) {
        device_addrs_t reply_addrs = mpmd_find_with_addr(if_addr.bcast, hint);
        addrs.insert(addrs.begin(), reply_addrs.begin(), reply_addrs.end());
    }
    return addrs;
}

/*! Find MPM devices based on a hint
 *
 * There are two scenarios:
 *
 * 1) an addr or mgmt_addr was defined
 *
 * In this case, we will go through all the addrs. If they point to a device,
 * we will then compare the other attributes (serial, etc.). If they match,
 * the device goes into a list.
 *
 * 2) No addrs were defined
 *
 * In this case, we do a broadcast ping to see if any devices respond. After
 * that, we do the same matching.
 *
 */
device_addrs_t mpmd_find(const device_addr_t& hint_)
{
    device_addrs_t hints = separate_device_addr(hint_);
    if (hint_.has_key("type")) {
        if (std::find(MPM_DEVICE_TYPES.cbegin(),
                      MPM_DEVICE_TYPES.cend(),
                      hint_["type"]) == MPM_DEVICE_TYPES.cend()) {
            UHD_LOG_TRACE("MPMD FIND",
                "Returning early, type does not match an MPM device.");
            return {};
        }
    }
    UHD_LOG_TRACE("MPMD FIND",
        "Finding with " << hints.size() << " different hint(s).");

    // Scenario 1): User gave us at least one address
    if (not hints.empty() and
            (hints[0].has_key(xport::FIRST_ADDR_KEY) or
             hints[0].has_key(xport::MGMT_ADDR_KEY))) {
        // Note: We don't try and connect to the devices in this mode, because
        // we only get here if the user specified addresses, and we assume she
        // knows what she's doing.
        return mpmd_find_with_addrs(hints);
    }

    // Scenario 2): User gave us no address, and we need to broadcast
    if (hints.empty()) {
        hints.resize(1);
    }
    const auto bcast_mpm_devs = mpmd_find_with_bcast(hints[0]);
    UHD_LOG_TRACE("MPMD FIND",
        "Found " << bcast_mpm_devs.size() << " device via broadcast.");
    const bool find_all = hint_.has_key(mpmd_impl::MPM_FINDALL_KEY);
    if (find_all) {
        UHD_LOG_TRACE("MPMD FIND",
            "User provided " << mpmd_impl::MPM_FINDALL_KEY << ", will not "
            "check devices for CHDR accessibility.");
    }
    // Filter found devices for those that we can actually talk to via CHDR
    device_addrs_t filtered_mpm_devs;
    for (const auto &mpm_dev : bcast_mpm_devs) {
        const auto reachable_device_addr =
            mpmd_mboard_impl::is_device_reachable(mpm_dev);
        if (bool(reachable_device_addr)) {
            filtered_mpm_devs.push_back(reachable_device_addr.get());
        } else if (find_all) {
            filtered_mpm_devs.emplace_back(
                flag_dev_as_unreachable(mpm_dev)
            );
        }
    }

    if (filtered_mpm_devs.empty() and not bcast_mpm_devs.empty()) {
        UHD_LOG_INFO("MPMD FIND",
            "Found MPM devices, but none are reachable for a UHD session. "
            "Specify " << mpmd_impl::MPM_FINDALL_KEY << " to find all devices."
        );
    }

    return filtered_mpm_devs;
}
