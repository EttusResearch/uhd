//
// Copyright 2010-2011,2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/device.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/utils/prefs.hpp>
#include <boost/functional/hash.hpp>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <tuple>

using namespace uhd;

static std::mutex _device_mutex;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
/*!
 * Make a device hash that maps 1 to 1 with a device address.
 * The hash will be used to identify created devices.
 * \param dev_addr the device address
 * \return the hash number
 */
static size_t hash_device_addr(const device_addr_t& dev_addr)
{
    // combine the hashes of sorted keys/value pairs
    size_t hash = 0;

    // The device addr can contain all sorts of stuff, which sometimes gets in
    // the way of hashing reliably. TODO: Make this a whitelist
    const std::vector<std::string> hash_key_blacklist = {
        "claimed", "skip_dram", "skip_ddc", "skip_duc"};

    if (dev_addr.has_key("resource")) {
        boost::hash_combine(hash, "resource");
        boost::hash_combine(hash, dev_addr["resource"]);
    } else {
        for (const std::string& key : uhd::sorted(dev_addr.keys())) {
            if (std::find(hash_key_blacklist.begin(), hash_key_blacklist.end(), key)
                == hash_key_blacklist.end()) {
                boost::hash_combine(hash, key);
                boost::hash_combine(hash, dev_addr[key]);
            }
        }
    }
    return hash;
}

/***********************************************************************
 * Registration
 **********************************************************************/
typedef std::tuple<device::find_t, device::make_t, device::device_filter_t> dev_fcn_reg_t;

// instantiate the device function registry container
UHD_SINGLETON_FCN(std::vector<dev_fcn_reg_t>, get_dev_fcn_regs)

void device::register_device(
    const find_t& find, const make_t& make, const device_filter_t filter)
{
    // UHD_LOGGER_TRACE("UHD") << "registering device";
    get_dev_fcn_regs().push_back(dev_fcn_reg_t(find, make, filter));
}

device::~device(void)
{
    /* NOP */
}

/***********************************************************************
 * Discover
 **********************************************************************/

// Internal function that discovers devices and returns them with their make functions
static std::vector<std::tuple<device_addr_t, device::make_t>>
discover_devices_with_makers(const device_addr_t& hint, device::device_filter_t filter)
{
    std::vector<std::tuple<device_addr_t, device::make_t>> device_maker_pairs;

    // Launch async find tasks with their corresponding make functions
    std::vector<std::pair<std::future<device_addrs_t>, device::make_t>> tasks;
    for (const auto& fcn : get_dev_fcn_regs()) {
        if (filter == device::ANY or std::get<2>(fcn) == filter) {
            tasks.emplace_back(std::async(std::launch::async,
                                   [fcn, hint]() { return std::get<0>(fcn)(hint); }),
                std::get<1>(fcn));
        }
    }

    // Collect results from async tasks
    for (auto& [task, maker] : tasks) {
        try {
            device_addrs_t discovered_addrs = task.get();
            for (const auto& addr : discovered_addrs) {
                device_maker_pairs.emplace_back(addr, maker);
            }
        } catch (const std::exception& e) {
            UHD_LOGGER_ERROR("UHD") << "Device discovery error: " << e.what();
        }
    }

    return device_maker_pairs;
}

// Remove duplicate addresses from a discovery result.
static device_addrs_t deduplicate_device_addrs(const device_addrs_t& discovered_addrs)
{
    device_addrs_t deduped_addrs;
    std::set<size_t> device_hashes;
    for (const auto& discovered_addr : discovered_addrs) {
        const size_t hash = hash_device_addr(discovered_addr);
        if (device_hashes.insert(hash).second) {
            deduped_addrs.push_back(discovered_addr);
        }
    }

    return deduped_addrs;
}

// Internal function that discovers and de-duplicates device addresses.
static device_addrs_t discover_devices(
    const device_addr_t& hint, device::device_filter_t filter)
{
    auto device_maker_pairs = discover_devices_with_makers(hint, filter);

    device_addrs_t discovered_addrs;
    discovered_addrs.reserve(device_maker_pairs.size());
    for (const auto& pair : device_maker_pairs) {
        discovered_addrs.push_back(std::get<0>(pair));
    }

    return deduplicate_device_addrs(discovered_addrs);
}

// Internal function that discovers and de-duplicates device-maker pairs.
static std::vector<std::tuple<device_addr_t, device::make_t>>
discover_unique_devices_with_makers(
    const device_addr_t& hint, device::device_filter_t filter)
{
    auto discovered_pairs = discover_devices_with_makers(hint, filter);

    // find might return duplicate entries if a device received a broadcast
    // multiple times. These entries need to be removed from the result.
    std::set<size_t> device_hashes;
    discovered_pairs.erase(
        std::remove_if(discovered_pairs.begin(),
            discovered_pairs.end(),
            [&device_hashes](const std::tuple<device_addr_t, device::make_t>& pair) {
                const size_t hash = hash_device_addr(std::get<0>(pair));
                const bool seen   = device_hashes.count(hash);
                device_hashes.insert(hash);
                return seen;
            }),
        discovered_pairs.end());

    return discovered_pairs;
}

device_addrs_t device::find(const device_addr_t& hint, device_filter_t filter)
{
    std::lock_guard<std::mutex> lock(_device_mutex);

    return discover_devices(hint, filter);
}

/***********************************************************************
 * Make
 **********************************************************************/
device::sptr device::make(const device_addr_t& hint, device_filter_t filter, size_t which)
{
    std::lock_guard<std::mutex> lock(_device_mutex);

    // Use the internal function that returns unique device-maker pairs.
    auto discovered_pairs = discover_unique_devices_with_makers(hint, filter);

    typedef std::tuple<device_addr_t, make_t> dev_addr_make_t;
    std::vector<dev_addr_make_t> dev_addr_makers;

    // Convert to the expected format (no additional filtering needed)
    for (const auto& pair : discovered_pairs) {
        dev_addr_makers.push_back(dev_addr_make_t(std::get<0>(pair), std::get<1>(pair)));
    }

    // check that we found any devices
    if (dev_addr_makers.empty()) {
        throw uhd::key_error(
            std::string("No devices found for ----->\n") + hint.to_pp_string());
    }

    // check that the which index is valid
    if (dev_addr_makers.size() <= which) {
        throw uhd::index_error("No device at index " + std::to_string(which)
                               + " for ----->\n" + hint.to_pp_string());
    }

    // create a unique hash for the device address
    device_addr_t dev_addr;
    make_t maker;
    std::tie(dev_addr, maker) = dev_addr_makers.at(which);
    size_t dev_hash           = hash_device_addr(dev_addr);
    UHD_LOGGER_TRACE("UHD") << "Device hash: " << dev_hash;

    // copy keys that were in hint but not in dev_addr
    // this way, we can pass additional transport arguments
    for (const std::string& key : hint.keys()) {
        if (not dev_addr.has_key(key))
            dev_addr[key] = hint[key];
    }

    // map device address hash to created devices
    static uhd::dict<size_t, std::weak_ptr<device>> hash_to_device;

    // try to find an existing device
    if (hash_to_device.has_key(dev_hash)) {
        if (device::sptr p = hash_to_device[dev_hash].lock()) {
            return p;
        }
    }

    // Add keys from the config files (note: the user-defined keys will
    // always be applied, see also get_usrp_args()
    // Then, create and register a new device.
    device::sptr dev         = maker(prefs::get_usrp_args(dev_addr));
    hash_to_device[dev_hash] = dev;
    return dev;
}

uhd::property_tree::sptr device::get_tree(void) const
{
    return _tree;
}

device::device_filter_t device::get_device_type() const
{
    return _type;
}
