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
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/types/sensors.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <memory>
#include <mutex>
#include <random>
#include <string>

using namespace uhd;

namespace {
    /*************************************************************************
     * Local constants
     ************************************************************************/
    const size_t MPMD_CROSSBAR_MAX_LADDR = 255;
    //! How long we wait for discovery responses (in seconds)
    const double MPMD_FIND_TIMEOUT = 0.5;

    /*************************************************************************
     * Helper functions
     ************************************************************************/
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
        tree->create<sensor_value_t>(
                mb_path / "sensors/ref_locked")
            .set_publisher([](){
                return sensor_value_t (
                    "Ref", true, "locked", "unlocked" // FIXME: Remove hardcoded "true"
                );
            })
        ;
        tree->create<int>(
                mb_path / "rx_codecs" / "A" / "gains")
            .set_publisher([](){
                return 1                              // FIXME: Remove hardcoding
                ;
            })
        ;
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

    //! This might be parallelized. std::tasks would probably be a good way to
    // do that if we want to.
    for (size_t mb_i = 0; mb_i < mb_args.size(); ++mb_i) {
        setup_rfnoc_blocks(mb_i, mb_args[mb_i]);
    }

    for (size_t mb_i = 0; mb_i < mb_args.size(); ++mb_i) {
        init_property_tree(_tree, fs_path("/mboards") / mb_i, _mb[mb_i].get());
    }

    auto filtered_block_args = device_args; // TODO actually filter
    setup_rpc_blocks(filtered_block_args);
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

    // Call init on periph_manager, this will init the dboards/mboard, maybe
    // even selfcal and everything

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

size_t mpmd_impl::identify_mboard_by_sid(const size_t remote_addr)
{
    for (size_t mb_index = 0; mb_index < _mb.size(); mb_index++) {
        for (size_t xbar_index = 0;
                xbar_index < _mb[mb_index]->num_xbars;
                xbar_index++) {
            if (_mb[mb_index]->get_xbar_local_addr(xbar_index) == remote_addr) {
                return mb_index;
            }
        }
    }
    throw uhd::lookup_error(str(
        boost::format("Cannot identify mboard for remote address %d")
        % remote_addr
    ));
}


/*****************************************************************************
 * API
 ****************************************************************************/
// TODO this does not consider the liberio use case!
uhd::device_addr_t mpmd_impl::get_rx_hints(size_t /* mb_index */)
{
    //device_addr_t rx_hints = _mb[mb_index].recv_args;
    device_addr_t rx_hints; // TODO don't ignore what the user tells us
    // (default to a large recv buff)
    if (not rx_hints.has_key("recv_buff_size"))
    {
        //For the ethernet transport, the buffer has to be set before creating
        //the transport because it is independent of the frame size and # frames
        //For nirio, the buffer size is not configurable by the user
        #if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
            //limit buffer resize on macos or it will error
            rx_hints["recv_buff_size"] = boost::lexical_cast<std::string>(MPMD_RX_SW_BUFF_SIZE_ETH_MACOS);
        #elif defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
            //set to half-a-second of buffering at max rate
            rx_hints["recv_buff_size"] = boost::lexical_cast<std::string>(MPMD_RX_SW_BUFF_SIZE_ETH);
        #endif
    }
    return rx_hints;
}


// frame_size_t determine_max_frame_size(const std::string &addr,
//                                       const frame_size_t &user_frame_size){
//     transport::udp_simple::sptr udp =
//     transport::udp_simple::make_connected(addr,
//                                                                             std::to_string(MPM_DISCOVERY_PORT));
//     std::vector<uint8_t> buffer(std::max(user_frame_size.rec))
// }
// Everything fake below here

both_xports_t mpmd_impl::make_transport(
        const sid_t& address,
        usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& args
) {
    const size_t mb_index = identify_mboard_by_sid(address.get_dst_addr());

    UHD_LOGGER_TRACE("MPMD")
        << "Creating new transport of type: "
        << (xport_type == CTRL ? "CTRL" : (xport_type == RX_DATA ? "RX" : "TX"))
        << " To mboard: " << mb_index
        << " Destination address: " << address.to_pp_string_hex().substr(6)
        << " User-defined xport args: " << args.to_string()
    ;

    both_xports_t xports;
    const uhd::device_addr_t& xport_args = (xport_type == CTRL) ? uhd::device_addr_t() : args;
    transport::zero_copy_xport_params default_buff_args;

    std::string interface_addr = _mb[mb_index]->mb_args.get("addr");
    UHD_ASSERT_THROW(not interface_addr.empty());
    const uint32_t xbar_src_addr = address.get_src_addr();
    const uint32_t xbar_src_dst = 0;

    default_buff_args.send_frame_size = 8000;
    default_buff_args.recv_frame_size = 8000;
    default_buff_args.num_recv_frames = 32;
    default_buff_args.num_send_frames = 32;
    // hardcode frame size for now

    transport::udp_zero_copy::buff_params buff_params;
    auto recv = transport::udp_zero_copy::make(
        interface_addr,
        BOOST_STRINGIZE(49153),
        default_buff_args,
        buff_params,
        xport_args);
    uint16_t port  = recv->get_local_port();

    xports.endianness = uhd::ENDIANNESS_BIG;
    xports.send_sid = _mb[mb_index]->allocate_sid(port,
            address, xbar_src_addr, xbar_src_dst, _sid_framer++
        );
    xports.recv_sid = xports.send_sid.reversed();
    xports.recv_buff_size = buff_params.recv_buff_size;
    xports.send_buff_size = buff_params.send_buff_size;
    xports.recv = recv; // Note: This is a type cast!
    xports.send = xports.recv;
    UHD_LOGGER_TRACE("MPMD")
        << "xport info: send_sid==" << xports.send_sid.to_pp_string_hex()
        << " recv_sid==" << xports.recv_sid.to_pp_string_hex()
        << " endianness=="
            << (xports.endianness == uhd::ENDIANNESS_BIG ? "BE" : "LE")
        << " recv_buff_size==" << xports.recv_buff_size
        << " send_buff_size==" << xports.send_buff_size
    ;

    return xports;
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
        addrs.push_back(new_addr);
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
