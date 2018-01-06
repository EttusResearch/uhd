//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
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
#include <future>
#include <thread>

using namespace uhd;
using namespace uhd::mpmd;

namespace {
    /*************************************************************************
     * Local constants
     ************************************************************************/
    const size_t MPMD_CROSSBAR_MAX_LADDR = 255;
    //! How long we wait for discovery responses (in seconds)
    const double MPMD_FIND_TIMEOUT = 0.5;
    //! Most pessimistic time for a CHDR query to go to device and back
    const double MPMD_CHDR_MAX_RTT = 0.02;
    //! MPM Compatibility number
    const std::vector<size_t> MPM_COMPAT_NUM = {1, 0};
    //! Timeout value for the update_component RPC call (ms)
    const size_t MPMD_UPDATE_COMPONENT_TIMEOUT     = 10000;

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
        mb->rpc->set_timeout(MPMD_UPDATE_COMPONENT_TIMEOUT);
        mb->rpc->notify_with_token("update_component", all_metadata, all_data);
        mb->set_timeout_default();

        return all_comps_copy;
    }


    /*! Initialize property tree for a single device.
     *
     * \param tree Property tree reference (to the whole tree)
     * \param mb_path Subtree path for this device
     * \param mb Reference to the actual device
     */
    void init_property_tree(
            uhd::property_tree::sptr tree,
            fs_path mb_path,
            mpmd_mboard_impl *mb
    ) {
        /*** Device info ****************************************************/
        if (not tree->exists("/name")) {
            tree->create<std::string>("/name")
                .set(mb->device_info.get("name", "Unknown MPM device"))
            ;
        }
        tree->create<std::string>(mb_path / "name")
            .set(mb->device_info.get("type", "UNKNOWN"));
        tree->create<std::string>(mb_path / "serial")
            .set(mb->device_info.get("serial", "n/a"));
        tree->create<std::string>(mb_path / "connection")
            .set(mb->device_info.get("connection", "UNKNOWN"));
        tree->create<size_t>(mb_path / "link_max_rate").set(1e9 / 8);

        /*** Clocking *******************************************************/
        tree->create<std::string>(mb_path / "clock_source/value")
            .add_coerced_subscriber([mb](const std::string &clock_source){
                // FIXME: Undo these changes
                //mb->rpc->notify_with_token("set_clock_source", clock_source);
                auto current_src = mb->rpc->request_with_token<std::string>(
                    "get_clock_source"
                );
                if (current_src != clock_source) {
                    UHD_LOG_WARNING("MPMD",
                        "Setting clock source at runtime is currently not "
                        "supported. Use clock_source=XXX as a device arg to "
                        "select a clock source. The current source is: "
                        << current_src);
                }
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
                //mb->rpc->notify_with_token("set_time_source", time_source);
                // FIXME: Undo these changes
                auto current_src = mb->rpc->request_with_token<std::string>(
                    "get_time_source"
                );
                if (current_src != time_source) {
                    UHD_LOG_WARNING("MPMD",
                        "Setting time source at runtime is currently not "
                        "supported. Use time_source=XXX as a device arg to "
                        "select a time source. The current source is: "
                        << current_src);
                }
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

        /*** MTUs ***********************************************************/
        tree->create<size_t>(mb_path / "mtu/recv")
            .add_coerced_subscriber([](const size_t){
                throw uhd::runtime_error(
                    "Attempting to write read-only value (MTU)!");
            })
            .set_publisher([mb](){
                return mb->get_mtu(uhd::RX_DIRECTION);
            })
        ;
        tree->create<size_t>(mb_path / "mtu/send")
            .add_coerced_subscriber([](const size_t){
                throw uhd::runtime_error(
                    "Attempting to write read-only value (MTU)!");
            })
            .set_publisher([mb](){
                return mb->get_mtu(uhd::TX_DIRECTION);
            })
        ;
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
 * Static class attributes
 ****************************************************************************/
const size_t mpmd_impl::MPM_DISCOVERY_PORT = 49600;
const std::string mpmd_impl::MPM_DISCOVERY_PORT_KEY = "discovery_port";
const size_t mpmd_impl::MPM_RPC_PORT = 49601;
const std::string mpmd_impl::MPM_RPC_PORT_KEY = "rpc_port";
const std::string mpmd_impl::MPM_RPC_GET_LAST_ERROR_CMD = "get_last_error";
const std::string mpmd_impl::MPM_DISCOVERY_CMD = "MPM-DISC";
const std::string mpmd_impl::MPM_ECHO_CMD = "MPM-ECHO";

/*****************************************************************************
 * Structors
 ****************************************************************************/
mpmd_impl::mpmd_impl(const device_addr_t& device_args)
    : usrp::device3_impl()
    , _device_args(device_args)
{
    const device_addrs_t mb_args = separate_device_addr(device_args);
    const size_t num_mboards = mb_args.size();
    _mb.reserve(num_mboards);
    const bool serialize_init = device_args.has_key("serialize_init");
    const bool skip_init = device_args.has_key("skip_init");
    UHD_LOGGER_INFO("MPMD")
        << "Initializing " << num_mboards << " device(s) "
        << (serialize_init ? "serially " : "in parallel ")
        << "with args: " << device_args.to_string();

    // First, claim all the devices (so we own them and no one else can claim
    // them).
    // This can be parallelized as long as uptrs are stored in the right spot;
    // they need to be correctly indexed.
    for (size_t mb_i = 0; mb_i < num_mboards; ++mb_i) {
        UHD_LOG_DEBUG("MPMD", "Claiming mboard " << mb_i);
        _mb.push_back(claim_and_make(mb_args[mb_i]));
    }

    // Next figure out the number of base xport addresses. This way, we
    // can run _mb[*]->init() in parallel on all the _mb.
    // This can *not* be parallelized.
    std::vector<size_t> base_xport_addr(num_mboards, 2); // Starts at 2 [sic]
    for (size_t mb_i = 0; mb_i < num_mboards-1; ++mb_i) {
        base_xport_addr[mb_i+1] = base_xport_addr[mb_i] + _mb[mb_i]->num_xbars;
    }

    if (not skip_init) {
        // Run the actual device initialization. This can run in parallel.
        for (size_t mb_i = 0; mb_i < num_mboards; ++mb_i) {
            // Note: This is the only place we do compat number checks. They're
            // effectively disabled for skip_init=1
            setup_mb(_mb[mb_i].get(), mb_i, base_xport_addr[mb_i]);
        }
    } else {
        UHD_LOG_DEBUG("MPMD", "Claimed device, but skipped init.");
    }

    // Init the prop tree before the blocks get set up -- they might need access
    // to some of the properties. This also means that the prop tree is pristine
    // at this point in time.
    // This might be parallelized, need to verify the prop tree can handle the
    // concurrent accesses. Would shave of milliseconds per device -- probably
    // not worth it.
    for (size_t mb_i = 0; mb_i < mb_args.size(); ++mb_i) {
        init_property_tree(_tree, fs_path("/mboards") / mb_i, _mb[mb_i].get());
    }

    if (not skip_init) {
        // This can be parallelized, because the blocks of individual mboards
        // live on different subtrees.
        for (size_t mb_i = 0; mb_i < mb_args.size(); ++mb_i) {
            setup_rfnoc_blocks(_mb[mb_i].get(), mb_i, mb_args[mb_i]);
        }

        // FIXME this section only makes sense for when the time source is external.
        // So, check for that, or something similar.
        // This section of code assumes that the prop tree is set and we have access
        // to the timekeepers. So don't move it anywhere else.
        if (device_args.has_key("sync_time")) {
            reset_time_synchronized(_tree);
        }

        auto filtered_block_args = device_args; // TODO actually filter
        // Blocks will finalize their own setup in this function. They have
        // (and might need) full access to the prop tree, the timekeepers, etc.
        // This is already internally parallelized.
        setup_rpc_blocks(filtered_block_args, serialize_init);
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
mpmd_mboard_impl::uptr mpmd_impl::claim_and_make(
    const uhd::device_addr_t& device_args
) {
    const std::string rpc_addr = device_args.get(xport::MGMT_ADDR_KEY);
    UHD_LOGGER_DEBUG("MPMD")
        << "Device args: `" << device_args.to_string()
        << "'. RPC address: " << rpc_addr
    ;

    if (rpc_addr.empty()) {
        UHD_LOG_ERROR("MPMD",
            "Could not determine RPC address from device args: "
            << device_args.to_string());
        throw uhd::runtime_error("Could not determine device RPC address.");
    }
    return mpmd_mboard_impl::make(device_args, rpc_addr);
}

void mpmd_impl::setup_mb(
    mpmd_mboard_impl *mb,
    const size_t mb_index,
    const size_t base_xport_addr
) {

    // Check the compatibility number
    UHD_LOGGER_TRACE("MPMD") << boost::format(
            "Checking MPM compat number against ours: %i.%i")
            % MPM_COMPAT_NUM[0] % MPM_COMPAT_NUM[1];
    const auto compat_num =
        mb->rpc->request<std::vector<size_t>>("get_mpm_compat_num");
    UHD_LOGGER_TRACE("MPMD") << boost::format(
            "Compat number received: %d.%d")
            % compat_num[0] % compat_num[1];

    const size_t c_major = compat_num[0], c_minor = compat_num[1];
    if (c_major != MPM_COMPAT_NUM[0]) {
        UHD_LOGGER_ERROR("MPMD") << boost::format(
                                        "MPM major compat number mismatch."
                                        "Expected %i.%i Actual %i.%i")
                                        % MPM_COMPAT_NUM[0] % MPM_COMPAT_NUM[1] % c_major % c_minor;
        throw uhd::runtime_error("MPM compatibility number mismatch.");
    }
    if (c_minor < MPM_COMPAT_NUM[1]) {
        UHD_LOGGER_ERROR("MPMD") << boost::format(
                                        "MPM minor compat number mismatch."
                                        "Expected %d.%d Actual %d.%d")
                                        % MPM_COMPAT_NUM[0] % MPM_COMPAT_NUM[1] % c_major % c_minor;
        throw uhd::runtime_error("MPM compatibility number mismatch.");
    }

    UHD_LOG_DEBUG("MPMD", "Initializing mboard " << mb_index);
    mb->init();
    for (size_t xbar_index = 0; xbar_index < mb->num_xbars; xbar_index++) {
        mb->set_xbar_local_addr(xbar_index, base_xport_addr + xbar_index);
    }
}

void mpmd_impl::setup_rfnoc_blocks(
    mpmd_mboard_impl* mb,
    const size_t mb_index,
    const uhd::device_addr_t& ctrl_xport_args
) {
    UHD_LOG_TRACE("MPMD",
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

void mpmd_impl::setup_rpc_blocks(
    const device_addr_t &block_args,
    const bool serialize_init
) {
    std::vector<std::future<void>> task_list;
    // If we don't force async, most compilers, at least now, will default to
    // deferred.
    const auto launch_policy = serialize_init ?
        std::launch::deferred :
        std::launch::async;

    // Preload all the tasks (they might start running on emplace_back)
    for (const auto &block_ctrl: _rfnoc_block_ctrl) {
        auto rpc_block_id = block_ctrl->get_block_id();
        if (has_block<uhd::rfnoc::rpc_block_ctrl>(rpc_block_id)) {
            const size_t mboard_idx = rpc_block_id.get_device_no();
            auto rpc_block_ctrl =
                get_block_ctrl<uhd::rfnoc::rpc_block_ctrl>(rpc_block_id);
            auto rpc_sptr = _mb[mboard_idx]->rpc;
            task_list.emplace_back(std::async(launch_policy,
                [rpc_block_id, rpc_block_ctrl, &block_args, rpc_sptr](){
                    UHD_LOGGER_DEBUG("MPMD")
                        << "Adding RPC access to block: " << rpc_block_id
                        << " Block args: " << block_args.to_string()
                    ;
                    rpc_block_ctrl->set_rpc_client(rpc_sptr, block_args);
                }
            ));
        }
    }

    // Execute all the calls to set_rpc_client(), either concurrently, or
    // serially
    for (auto &task : task_list) {
        task.get();
    }
}

/*****************************************************************************
 * Find, Factory & Registry
 ****************************************************************************/
device_addrs_t mpmd_find_with_addr(const std::string& mgmt_addr, const device_addr_t& hint_)
{
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
            new_addr[value[0]] = value[1];
        }
        // filter the discovered device below by matching optional keys
        if (
            (not hint_.has_key("name")        or hint_["name"]    == new_addr["name"])
            and (not hint_.has_key("serial")  or hint_["serial"]  == new_addr["serial"])
            and (not hint_.has_key("type")    or hint_["type"]    == new_addr["type"])
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
    UHD_LOG_TRACE("MPMD FIND",
        "Finding with " << hints.size() << " different hint(s).");

    // Scenario 1): User gave us at least one address
    if (not hints.empty() and
            (hints[0].has_key(xport::FIRST_ADDR_KEY) or
             hints[0].has_key(xport::MGMT_ADDR_KEY))) {
        return mpmd_find_with_addrs(hints);
    }

    // Scenario 2): User gave us no address, and we need to broadcast
    if (hints.empty()) {
        hints.resize(1);
    }
    return mpmd_find_with_bcast(hints[0]);
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
