//
// Copyright 2017 Ettus Research (National Instruments)
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

#include "mpmd_impl.hpp"
#include "rpc_block_ctrl.hpp"
#include <../device3/device3_impl.hpp>
#include <uhd/exception.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/component_file.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

using namespace uhd;

namespace {
    /*************************************************************************
     * Local constants
     ************************************************************************/
    const size_t MPMD_CROSSBAR_MAX_LADDR = 255;
    //! How long we wait for discovery responses (in seconds)
    const double MPMD_FIND_TIMEOUT = 0.5;
    //! Most pessimistic time for a CHDR query to go to device and back
    const double MPMD_CHDR_MAX_RTT = 0.02;

    /*************************************************************************
     * Helper functions
     ************************************************************************/
    uhd::usrp::component_files_t _update_component(
        const uhd::usrp::component_files_t& comps,
        mpmd_mboard_impl *mb
    ) {
        // Construct the arguments to update component
        std::vector<std::vector<uint8_t>> all_data;
        std::vector<std::map<std::string, std::string>> all_metadata;
        // Also construct a copy of just the metadata to store in the property tree
        uhd::usrp::component_files_t all_comps_copy;

        for  (const auto& comp : comps) {
            // Make a map for update components args
            std::map<std::string, std::string> metadata;
            // Make a component copy to add to the property tree
            uhd::usrp::component_file_t comp_copy;
            // Copy the metadata
            for (const auto& key : comp.metadata.keys()) {
                metadata[key] = comp.metadata[key];
                comp_copy.metadata[key] = comp.metadata[key];
            }
            // Copy to the update component args
            all_data.push_back(comp.data);
            all_metadata.push_back(metadata);
            // Copy to the property tree
            all_comps_copy.push_back(comp_copy);
        }

        // Now call update component
        mb->rpc->notify_with_token("update_component", all_metadata, all_data);

        return all_comps_copy;
    }


    void init_property_tree(
            uhd::property_tree::sptr tree,
            fs_path mb_path,
            mpmd_mboard_impl *mb
    ) {
        if (not tree->exists(fs_path("/name"))) {
            tree->create<std::string>("/name")
                .set(mb->device_info.get("name", "Unknown MPM device"))
            ;
        }

        /*** Clocking *******************************************************/
        tree->create<std::string>(mb_path / "clock_source/value")
            .add_coerced_subscriber([mb](const std::string &clock_source){
                mb->rpc->notify_with_token("set_clock_source", clock_source);
            })
            .set_publisher([mb](){
                return mb->rpc->request_with_token<std::string>(
                    "get_clock_source"
                );
            })
        ;
        tree->create<std::vector<std::string>>(
                mb_path / "clock_source/options")
            .set_publisher([mb](){
                return mb->rpc->request_with_token<std::vector<std::string>>(
                    "get_clock_sources"
                );
            })
        ;
        tree->create<std::string>(mb_path / "time_source/value")
            .add_coerced_subscriber([mb](const std::string &time_source){
                mb->rpc->notify_with_token("set_time_source", time_source);
            })
            .set_publisher([mb](){
                return mb->rpc->request_with_token<std::string>(
                    "get_time_source"
                );
            })
        ;
        tree->create<std::vector<std::string>>(
                mb_path / "time_source/options")
            .set_publisher([mb](){
                return mb->rpc->request_with_token<std::vector<std::string>>(
                    "get_time_sources"
                );
            })
        ;

        /*** Sensors ********************************************************/
        auto sensor_list =
            mb->rpc->request_with_token<std::vector<std::string>>(
                "get_mb_sensors"
            );
        UHD_LOG_DEBUG("MPMD",
            "Found " << sensor_list.size() << " motherboard sensors."
        );
        for (const auto& sensor_name : sensor_list) {
            UHD_LOG_TRACE("MPMD",
                "Adding motherboard sensor `" << sensor_name << "'"
            );
            tree->create<sensor_value_t>(
                    mb_path / "sensors" / sensor_name)
                .set_publisher([mb, sensor_name](){
                    return sensor_value_t(
                        mb->rpc->request_with_token<sensor_value_t::sensor_map_t>(
                            "get_mb_sensor", sensor_name
                        )
                    );
                })
                .set_coercer([](const sensor_value_t &){
                    throw uhd::runtime_error(
                        "Trying to write read-only sensor value!"
                    );
                    return sensor_value_t("", "", "");
                })
            ;
        }

        /*** EEPROM *********************************************************/
        tree->create<uhd::usrp::mboard_eeprom_t>(mb_path / "eeprom")
            .add_coerced_subscriber([mb](const uhd::usrp::mboard_eeprom_t& mb_eeprom){
                eeprom_map_t eeprom_map;
                for (const auto& key : mb_eeprom.keys()) {
                    eeprom_map[key] = std::vector<uint8_t>(
                            mb_eeprom[key].cbegin(),
                            mb_eeprom[key].cend()
                    );
                }
                mb->rpc->notify_with_token("set_mb_eeprom", eeprom_map);
            })
            .set_publisher([mb](){
                auto mb_eeprom =
                    mb->rpc->request_with_token<std::map<std::string, std::string>>(
                        "get_mb_eeprom"
                    );
                uhd::usrp::mboard_eeprom_t mb_eeprom_dict(
                    mb_eeprom.cbegin(), mb_eeprom.cend()
                );
                return mb_eeprom_dict;
            })
        ;

        /*** Updateable Components ******************************************/
        std::vector<std::string> updateable_components =
                mb->rpc->request<std::vector<std::string>>(
                        "list_updateable_components"
                );
        // TODO: Check the 'id' against the registered property
        UHD_LOG_DEBUG("MPMD",
                    "Found " << updateable_components.size() << " updateable motherboard components."
                );
        for (const auto& comp_name : updateable_components) {
            UHD_LOG_TRACE("MPMD",
                    "Adding motherboard component: " << comp_name);
            tree->create<uhd::usrp::component_files_t>(mb_path / "components" / comp_name)
                        .set_coercer([mb](const uhd::usrp::component_files_t& comp_files) {
                    return _update_component(
                            comp_files,
                            mb
                    );
            })
            ;
        }

    }

    void reset_time_synchronized(uhd::property_tree::sptr tree)
    {
        const size_t n_mboards = tree->list("/mboards").size();
        UHD_LOGGER_DEBUG("MPMD")
            << "Synchronizing " << n_mboards <<" timekeepers...";
        auto get_time_last_pps = [tree](){
            return tree->access<time_spec_t>(
                fs_path("/mboards/0/time/pps")
            ).get();
        };
        auto end_time = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(1100);
        auto time_last_pps = get_time_last_pps();
        UHD_LOG_DEBUG("MPMD", "Waiting for PPS clock edge...");
        while (time_last_pps == get_time_last_pps())
        {
            if (std::chrono::steady_clock::now() > end_time) {
                throw uhd::runtime_error(
                    "Board 0 may not be getting a PPS signal!\n"
                    "No PPS detected within the time interval.\n"
                    "See the application notes for your device.\n"
                );
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        UHD_LOG_DEBUG("MPMD", "Setting all timekeepers to 0...");
        for (size_t mboard_idx = 0; mboard_idx < n_mboards; mboard_idx++) {
            tree->access<time_spec_t>(
                fs_path("/mboards") / mboard_idx / "time" / "pps"
            ).set(time_spec_t(0.0));
        }

        UHD_LOG_DEBUG("MPMD", "Waiting for next PPS edge...");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        UHD_LOG_DEBUG("MPMD", "Verifying all timekeepers are aligned...");
        auto get_time_now = [tree](const size_t mb_index){
            return tree->access<time_spec_t>(
                fs_path("/mboards") / mb_index / "time/now"
            ).get();
        };
        for (size_t m = 1; m < n_mboards; m++){
            time_spec_t time_0 = get_time_now(0);
            time_spec_t time_i = get_time_now(m);
            if (time_i < time_0
                    or (time_i - time_0) > time_spec_t(MPMD_CHDR_MAX_RTT)) {
                UHD_LOGGER_WARNING("MULTI_USRP") << boost::format(
                    "Detected time deviation between board %d and board 0.\n"
                    "Board 0 time is %f seconds.\n"
                    "Board %d time is %f seconds.\n"
                ) % m % time_0.get_real_secs() % m % time_i.get_real_secs();
            }
        }
    }
}

/*****************************************************************************
 * Structors
 ****************************************************************************/
mpmd_impl::mpmd_impl(const device_addr_t& device_args)
    : usrp::device3_impl()
    , _device_args(device_args)
    , _sid_framer(0)
{
    UHD_LOGGER_INFO("MPMD")
        << "Initializing device with args: " << device_args.to_string();

    for (const std::string& key : device_args.keys()) {
        if (key.find("recv") != std::string::npos) {
            recv_args[key] = device_args[key];
        }
        if (key.find("send") != std::string::npos) {
            send_args[key] = device_args[key];
        }
    }

    const device_addrs_t mb_args = separate_device_addr(device_args);
    _mb.reserve(mb_args.size());

    // This can theoretically be parallelized, but then we want to make sure
    // we're distributing crossbar local addresses in some orderly fashion.
    // At the very least, _xbar_local_addr_ctr needs to become atomic.
    for (size_t mb_i = 0; mb_i < mb_args.size(); ++mb_i) {
        _mb.push_back(setup_mb(mb_i, mb_args[mb_i]));
    }

    // Init the prop tree before the blocks get set up -- they might need access
    // to some of the properties. This also means that the prop tree is pristine
    // at this point in time.
    for (size_t mb_i = 0; mb_i < mb_args.size(); ++mb_i) {
        init_property_tree(_tree, fs_path("/mboards") / mb_i, _mb[mb_i].get());
    }

    if (not device_args.has_key("skip_init")) {
        // This might be parallelized. std::tasks would probably be a good way to
        // do that if we want to.
        for (size_t mb_i = 0; mb_i < mb_args.size(); ++mb_i) {
            setup_rfnoc_blocks(mb_i, mb_args[mb_i]);
        }

        // FIXME this section only makes sense for when the time source is external.
        // So, check for that, or something similar.
        // This section of code assumes that the prop tree is set and we have access
        // to the timekeepers. So don't move it anywhere else.
        if (device_args.has_key("sync_time")) {
            reset_time_synchronized(_tree);
        }

        auto filtered_block_args = device_args; // TODO actually filter
        // Blocks will finalize their own setup in this function. They have (and
        // might need) full access to the prop tree, the timekeepers, etc.
        setup_rpc_blocks(filtered_block_args);
    } else {
        UHD_LOG_INFO("MPMD", "Claimed device without full initialization.");
    }
}

mpmd_impl::~mpmd_impl()
{
    /* nop */
}

/*****************************************************************************
 * Private methods
 ****************************************************************************/
mpmd_mboard_impl::uptr mpmd_impl::setup_mb(
    const size_t mb_index,
    const uhd::device_addr_t& device_args
) {
    UHD_LOGGER_DEBUG("MPMD")
        << "Initializing mboard " << mb_index
        << ". Device args: " << device_args.to_string()
    ;

    auto mb = mpmd_mboard_impl::make(
        device_args,
        device_args["addr"]
    );

    if (device_args.has_key("skip_init")) {
        return mb;
    }

    for (size_t xbar_index = 0; xbar_index < mb->num_xbars; xbar_index++) {
        mb->set_xbar_local_addr(xbar_index, allocate_xbar_local_addr());
    }

    const fs_path mb_path = fs_path("/mboards") / mb_index;
    _tree->create<std::string>(mb_path / "name")
        .set(mb->device_info.get("type", "UNKNOWN"));
    _tree->create<std::string>(mb_path / "serial")
        .set(mb->device_info.get("serial", "n/a"));
    _tree->create<std::string>(mb_path / "connection")
        .set(mb->device_info.get("connection", "remote"));

    // Do real MTU discovery (something similar like X300 but with MPM)

    _tree->create<size_t>(mb_path / "mtu/recv").set(1500);
    _tree->create<size_t>(mb_path / "mtu/send").set(1500);
    _tree->create<size_t>(mb_path / "link_max_rate").set(1e9 / 8);

    // query more information about FPGA/MPM


    // Query time/clock sources on mboards/dboards
    // Throw rpc calls with boost bind into the property tree?


    // implicit move
    return mb;
}

void mpmd_impl::setup_rfnoc_blocks(
    const size_t mb_index,
    const uhd::device_addr_t& ctrl_xport_args
) {
    auto &mb = _mb[mb_index];
    mb->num_xbars = mb->rpc->request<size_t>("get_num_xbars");
    UHD_LOG_TRACE("MPM",
        "Mboard " << mb_index << " reports " << mb->num_xbars << " crossbar(s)."
    );

    for (size_t xbar_index = 0; xbar_index < mb->num_xbars; xbar_index++) {
        const size_t num_blocks =
            mb->rpc->request<size_t>("get_num_blocks", xbar_index);
        const size_t base_port =
            mb->rpc->request<size_t>("get_base_port", xbar_index);
        const size_t local_addr = mb->get_xbar_local_addr(xbar_index);
        UHD_LOGGER_TRACE("MPMD")
            << "Enumerating RFNoC blocks for xbar " << xbar_index
            << ". Total blocks: " << num_blocks
            << " Base port: " << base_port
            << " Local address: " << local_addr
        ;
        try {
            enumerate_rfnoc_blocks(
              mb_index,
              num_blocks,
              base_port,
              uhd::sid_t(0, 0, local_addr, 0),
              ctrl_xport_args
            );
        } catch (const std::exception &ex) {
            UHD_LOGGER_ERROR("MPMD")
                << "Failure during block enumeration: "
                << ex.what();
            throw uhd::runtime_error("Failed to run enumerate_rfnoc_blocks()");
        }
    }
}

void mpmd_impl::setup_rpc_blocks(const device_addr_t &block_args)
{
    // This could definitely be parallelized. Blocks may do all sorts of stuff
    // inside set_rpc_client(), and it can take any amount of time (I mean,
    // like, seconds).
    for (const auto &block_ctrl: _rfnoc_block_ctrl) {
        auto rpc_block_id = block_ctrl->get_block_id();
        if (has_block<uhd::rfnoc::rpc_block_ctrl>(block_ctrl->get_block_id())) {
            const size_t mboard_idx = rpc_block_id.get_device_no();
            UHD_LOGGER_DEBUG("MPMD")
                << "Adding RPC access to block: " << rpc_block_id
                << " Block args: " << block_args.to_string()
            ;
            get_block_ctrl<uhd::rfnoc::rpc_block_ctrl>(rpc_block_id)
                ->set_rpc_client(_mb[mboard_idx]->rpc, block_args);
        }
    }
}

size_t mpmd_impl::allocate_xbar_local_addr()
{
    const size_t new_local_addr = _xbar_local_addr_ctr++;
    if (new_local_addr > MPMD_CROSSBAR_MAX_LADDR) {
        throw uhd::runtime_error("Too many crossbars.");
    }

    return new_local_addr;
}


/*****************************************************************************
 * Find, Factory & Registry
 ****************************************************************************/
device_addrs_t mpmd_find_with_addr(const device_addr_t& hint_)
{
    transport::udp_simple::sptr comm = transport::udp_simple::make_broadcast(
        hint_["addr"], std::to_string(MPM_DISCOVERY_PORT));
    comm->send(
        boost::asio::buffer(&MPM_DISCOVERY_CMD, sizeof(MPM_DISCOVERY_CMD)));
    device_addrs_t addrs;
    while (true) {
        char buff[4096] = {};
        const size_t nbytes = comm->recv( // TODO make sure we don't buf overflow
                boost::asio::buffer(buff),
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
        // who else is reposending to our request !?
        if (result[0] != "USRP-MPM") {
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
            }
        }
        if (external_iface) {
            continue;
        }
        device_addr_t new_addr;
        new_addr["addr"] = recv_addr;
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
            new_addr[value[0]] = value[1];
        }
        // filter the discovered device below by matching optional keys
        if (
            (not hint_.has_key("name")    or hint_["name"]    == new_addr["name"])
            and (not hint_.has_key("serial")  or hint_["serial"]  == new_addr["serial"])
            and (not hint_.has_key("type")    or hint_["type"]    == new_addr["type"])
            and (not hint_.has_key("product") or hint_["product"] == new_addr["product"])
        ){
            addrs.push_back(new_addr);
        } else {
            UHD_LOG_DEBUG("MPMD FIND",
                    "Found device, but does not match hint: " << recv_addr
            );
        }
    }
    return addrs;
};

device_addrs_t mpmd_find(const device_addr_t& hint_)
{
    // handle cases:
    //
    //  - empty hint
    //  - multiple addrs
    //  - single addr

    device_addrs_t hints = separate_device_addr(hint_);
    // either hints has:
    // multiple entries
    //   -> search for multiple devices and join them back into one
    //   device_addr_t
    // one entry with addr:
    //   -> search for one device with this addr
    // one
    // multiple addrs
    if (hints.size() > 1) {
        device_addrs_t found_devices;
        found_devices.reserve(hints.size());
        for (const auto& hint : hints) {
            if (not hint.has_key("addr")) { // maybe allow other attributes as well
                return device_addrs_t();
            }
            device_addrs_t reply_addrs = mpmd_find_with_addr(hint);
            if (reply_addrs.size() > 1) {
                throw uhd::value_error(
                    str(boost::format("Could not resolve device hint \"%s\" to "
                                      "a single device.") %
                        hint.to_string()));
            } else if (reply_addrs.empty()) {
                return device_addrs_t();
            }
            found_devices.push_back(reply_addrs[0]);
        }
        return device_addrs_t(1, combine_device_addrs(found_devices));
    }
    hints.resize(1);
    device_addr_t hint = hints[0];
    device_addrs_t addrs;

    if (hint.has_key("addr")) {
        // is this safe?
        return mpmd_find_with_addr(hint);
    }

    for (const transport::if_addrs_t& if_addr : transport::get_if_addrs()) {
        device_addr_t new_hint = hint;
        new_hint["addr"] = if_addr.bcast;

        device_addrs_t reply_addrs = mpmd_find_with_addr(new_hint);
        addrs.insert(addrs.begin(), reply_addrs.begin(), reply_addrs.end());
    }
    return addrs;
}

static device::sptr mpmd_make(const device_addr_t& device_args)
{
    return device::sptr(boost::make_shared<mpmd_impl>(device_args));
}

UHD_STATIC_BLOCK(register_mpmd_device)
{
    device::register_device(&mpmd_find, &mpmd_make, device::USRP);
}
// vim: sw=4 expandtab:
