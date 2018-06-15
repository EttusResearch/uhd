//
// Copyright 2013-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_impl.hpp"
#include "x300_lvbitx.hpp"
#include "x310_lvbitx.hpp"
#include "x300_mb_eeprom_iface.hpp"
#include <uhdlib/usrp/common/apply_corrections.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/transport/udp_constants.hpp>
#include <uhd/transport/zero_copy_recv_offload.hpp>
#include <uhd/transport/nirio_zero_copy.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/utils/platform.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/utils/math.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/functional/hash.hpp>
#include <boost/assign/list_of.hpp>
#include <fstream>
#include <chrono>
#include <thread>

#define NIUSRPRIO_DEFAULT_RPC_PORT "5444"

#define X300_REV(x) ((x) - "A" + 1)

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::transport;
using namespace uhd::niusrprio;
using namespace uhd::usrp::gpio_atr;
using namespace uhd::usrp::x300;
namespace asio = boost::asio;

static std::string get_fpga_option(wb_iface::sptr zpu_ctrl) {
    //Possible options:
    //1G  = {0:1G, 1:1G} w/ DRAM, HG  = {0:1G, 1:10G} w/ DRAM, XG  = {0:10G, 1:10G} w/ DRAM
    //HA  = {0:1G, 1:Aurora} w/ DRAM, XA  = {0:10G, 1:Aurora} w/ DRAM

    std::string option;
    uint32_t sfp0_type = zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_SFP0_TYPE));
    uint32_t sfp1_type = zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_SFP1_TYPE));

    if (sfp0_type == RB_SFP_1G_ETH  and sfp1_type == RB_SFP_1G_ETH) {
        option = "1G";
    } else if (sfp0_type == RB_SFP_1G_ETH  and sfp1_type == RB_SFP_10G_ETH) {
        option = "HG";
    } else if (sfp0_type == RB_SFP_10G_ETH  and sfp1_type == RB_SFP_10G_ETH) {
        option = "XG";
    } else if (sfp0_type == RB_SFP_1G_ETH  and sfp1_type == RB_SFP_AURORA) {
        option = "HA";
    } else if (sfp0_type == RB_SFP_10G_ETH  and sfp1_type == RB_SFP_AURORA) {
        option = "XA";
    } else {
        option = "HG";  //Default
    }
    return option;
}

/***********************************************************************
 * Discovery over the udp and pcie transport
 **********************************************************************/

//@TODO: Refactor the find functions to collapse common code for ethernet and PCIe
static device_addrs_t x300_find_with_addr(const device_addr_t &hint)
{
    udp_simple::sptr comm = udp_simple::make_broadcast(
        hint["addr"], BOOST_STRINGIZE(X300_FW_COMMS_UDP_PORT));

    //load request struct
    x300_fw_comms_t request = x300_fw_comms_t();
    request.flags = uhd::htonx<uint32_t>(X300_FW_COMMS_FLAGS_ACK);
    request.sequence = uhd::htonx<uint32_t>(std::rand());

    //send request
    comm->send(asio::buffer(&request, sizeof(request)));

    //loop for replies until timeout
    device_addrs_t addrs;
    while (true)
    {
        char buff[X300_FW_COMMS_MTU] = {};
        const size_t nbytes = comm->recv(asio::buffer(buff), 0.050);
        if (nbytes == 0) break;
        const x300_fw_comms_t *reply = (const x300_fw_comms_t *)buff;
        if (request.flags != reply->flags) continue;
        if (request.sequence != reply->sequence) continue;
        device_addr_t new_addr;
        new_addr["type"] = "x300";
        new_addr["addr"] = comm->get_recv_addr();

        //Attempt to read the name from the EEPROM and perform filtering.
        //This operation can throw due to compatibility mismatch.
        try
        {
            wb_iface::sptr zpu_ctrl = x300_make_ctrl_iface_enet(
                udp_simple::make_connected(new_addr["addr"],
                    BOOST_STRINGIZE(X300_FW_COMMS_UDP_PORT)),
                false /* Suppress timeout errors */
            );

            new_addr["fpga"] = get_fpga_option(zpu_ctrl);

            i2c_core_100_wb32::sptr zpu_i2c = i2c_core_100_wb32::make(zpu_ctrl, I2C1_BASE);
            x300_mb_eeprom_iface::sptr eeprom_iface = x300_mb_eeprom_iface::make(zpu_ctrl, zpu_i2c);
            const mboard_eeprom_t mb_eeprom =
                x300_impl::get_mb_eeprom(eeprom_iface);
            if (mb_eeprom.size() == 0 or x300_impl::claim_status(zpu_ctrl) == x300_impl::CLAIMED_BY_OTHER)
            {
                // Skip device claimed by another process
                continue;
            }
            new_addr["name"] = mb_eeprom["name"];
            new_addr["serial"] = mb_eeprom["serial"];
            switch (x300_impl::get_mb_type_from_eeprom(mb_eeprom)) {
                case x300_impl::USRP_X300_MB:
                    new_addr["product"] = "X300";
                    break;
                case x300_impl::USRP_X310_MB:
                    new_addr["product"] = "X310";
                    break;
                default:
                    break;
            }
        }
        catch(const std::exception &)
        {
            //set these values as empty string so the device may still be found
            //and the filter's below can still operate on the discovered device
            new_addr["name"] = "";
            new_addr["serial"] = "";
        }
        //filter the discovered device below by matching optional keys
        if (
            (not hint.has_key("name")    or hint["name"]    == new_addr["name"]) and
            (not hint.has_key("serial")  or hint["serial"]  == new_addr["serial"]) and
            (not hint.has_key("product") or hint["product"] == new_addr["product"])
        ){
            addrs.push_back(new_addr);
        }
    }

    return addrs;
}

//We need a zpu xport registry to ensure synchronization between the static finder method
//and the instances of the x300_impl class.
typedef uhd::dict< std::string, boost::weak_ptr<wb_iface> > pcie_zpu_iface_registry_t;
UHD_SINGLETON_FCN(pcie_zpu_iface_registry_t, get_pcie_zpu_iface_registry)
static boost::mutex pcie_zpu_iface_registry_mutex;

static device_addrs_t x300_find_pcie(const device_addr_t &hint, bool explicit_query)
{
    std::string rpc_port_name(NIUSRPRIO_DEFAULT_RPC_PORT);
    if (hint.has_key("niusrpriorpc_port")) {
        rpc_port_name = hint["niusrpriorpc_port"];
    }

    device_addrs_t addrs;
    niusrprio_session::device_info_vtr dev_info_vtr;
    nirio_status status = niusrprio_session::enumerate(rpc_port_name, dev_info_vtr);
    if (explicit_query) nirio_status_to_exception(status, "x300_find_pcie: Error enumerating NI-RIO devices.");

    for(niusrprio_session::device_info &dev_info:  dev_info_vtr)
    {
        device_addr_t new_addr;
        new_addr["type"] = "x300";
        new_addr["resource"] = dev_info.resource_name;
        std::string resource_d(dev_info.resource_name);
        boost::to_upper(resource_d);

        switch (x300_impl::get_mb_type_from_pcie(resource_d, rpc_port_name)) {
            case x300_impl::USRP_X300_MB:
                new_addr["product"] = "X300";
                break;
            case x300_impl::USRP_X310_MB:
                new_addr["product"] = "X310";
                break;
            default:
                continue;
        }

        niriok_proxy::sptr kernel_proxy = niriok_proxy::make_and_open(dev_info.interface_path);

        //Attempt to read the name from the EEPROM and perform filtering.
        //This operation can throw due to compatibility mismatch.
        try
        {
            //This block could throw an exception if the user is switching to using UHD
            //after LabVIEW FPGA. In that case, skip reading the name and serial and pick
            //a default FPGA flavor. During make, a new image will be loaded and everything
            //will be OK

            wb_iface::sptr zpu_ctrl;

            //Hold on to the registry mutex as long as zpu_ctrl is alive
            //to prevent any use by different threads while enumerating
            boost::mutex::scoped_lock(pcie_zpu_iface_registry_mutex);

            if (get_pcie_zpu_iface_registry().has_key(resource_d)) {
                zpu_ctrl = get_pcie_zpu_iface_registry()[resource_d].lock();
                if (!zpu_ctrl)
                {
                    get_pcie_zpu_iface_registry().pop(resource_d);
                }
            }

            // if the registry didn't have a key OR that key was an orphaned weak_ptr
            if (!zpu_ctrl)
            {
                zpu_ctrl = x300_make_ctrl_iface_pcie(kernel_proxy, false /* suppress timeout errors */);
                //We don't put this zpu_ctrl in the registry because we need
                //a persistent niriok_proxy associated with the object
            }

            //Attempt to autodetect the FPGA type
            if (not hint.has_key("fpga")) {
                new_addr["fpga"] = get_fpga_option(zpu_ctrl);
            }

            i2c_core_100_wb32::sptr zpu_i2c = i2c_core_100_wb32::make(zpu_ctrl, I2C1_BASE);
            x300_mb_eeprom_iface::sptr eeprom_iface = x300_mb_eeprom_iface::make(zpu_ctrl, zpu_i2c);
            const mboard_eeprom_t mb_eeprom =
                x300_impl::get_mb_eeprom(eeprom_iface);
            if (mb_eeprom.size() == 0 or x300_impl::claim_status(zpu_ctrl) == x300_impl::CLAIMED_BY_OTHER)
            {
                // Skip device claimed by another process
                continue;
            }
            new_addr["name"] = mb_eeprom["name"];
            new_addr["serial"] = mb_eeprom["serial"];
        }
        catch(const std::exception &)
        {
            //set these values as empty string so the device may still be found
            //and the filter's below can still operate on the discovered device
            if (not hint.has_key("fpga")) {
                new_addr["fpga"] = "HG";
            }
            new_addr["name"] = "";
            new_addr["serial"] = "";
        }

        //filter the discovered device below by matching optional keys
        std::string resource_i = hint.has_key("resource") ? hint["resource"] : "";
        boost::to_upper(resource_i);

        if (
            (not hint.has_key("resource") or resource_i     == resource_d) and
            (not hint.has_key("name")     or hint["name"]   == new_addr["name"]) and
            (not hint.has_key("serial")   or hint["serial"] == new_addr["serial"]) and
            (not hint.has_key("product") or hint["product"] == new_addr["product"])
        ){
            addrs.push_back(new_addr);
        }
    }
    return addrs;
}

device_addrs_t x300_find(const device_addr_t &hint_)
{
    //handle the multi-device discovery
    device_addrs_t hints = separate_device_addr(hint_);
    if (hints.size() > 1)
    {
        device_addrs_t found_devices;
        std::string error_msg;
        for(const device_addr_t &hint_i:  hints)
        {
            device_addrs_t found_devices_i = x300_find(hint_i);
            if (found_devices_i.size() != 1) error_msg += str(boost::format(
                "Could not resolve device hint \"%s\" to a single device."
            ) % hint_i.to_string());
            else found_devices.push_back(found_devices_i[0]);
        }
        if (found_devices.empty()) return device_addrs_t();
        if (not error_msg.empty()) throw uhd::value_error(error_msg);

        return device_addrs_t(1, combine_device_addrs(found_devices));
    }

    //initialize the hint for a single device case
    UHD_ASSERT_THROW(hints.size() <= 1);
    hints.resize(1); //in case it was empty
    device_addr_t hint = hints[0];
    device_addrs_t addrs;
    if (hint.has_key("type") and hint["type"] != "x300") return addrs;


    //use the address given
    if (hint.has_key("addr"))
    {
        device_addrs_t reply_addrs;
        try
        {
            reply_addrs = x300_find_with_addr(hint);
        }
        catch(const std::exception &ex)
        {
            UHD_LOGGER_ERROR("X300") << "X300 Network discovery error " << ex.what() ;
        }
        catch(...)
        {
            UHD_LOGGER_ERROR("X300") << "X300 Network discovery unknown error " ;
        }
        return reply_addrs;
    }

    if (!hint.has_key("resource"))
    {
        //otherwise, no address was specified, send a broadcast on each interface
        for(const if_addrs_t &if_addrs:  get_if_addrs())
        {
            //avoid the loopback device
            if (if_addrs.inet == asio::ip::address_v4::loopback().to_string()) continue;

            //create a new hint with this broadcast address
            device_addr_t new_hint = hint;
            new_hint["addr"] = if_addrs.bcast;

            //call discover with the new hint and append results
            device_addrs_t new_addrs = x300_find(new_hint);
            //if we are looking for a serial, only add the one device with a matching serial
            if (hint.has_key("serial")) {
                bool found_serial = false; //signal to break out of the interface loop
                for (device_addrs_t::iterator new_addr_it=new_addrs.begin(); new_addr_it != new_addrs.end(); new_addr_it++) {
                    if ((*new_addr_it)["serial"] == hint["serial"]) {
                        addrs.insert(addrs.begin(), *new_addr_it);
                        found_serial = true;
                        break;
                    }
                }
                if (found_serial) break;
            } else {
                // Otherwise, add all devices we find
                addrs.insert(addrs.begin(), new_addrs.begin(), new_addrs.end());
            }
        }
    }

    device_addrs_t pcie_addrs = x300_find_pcie(hint, hint.has_key("resource"));
    if (not pcie_addrs.empty()) addrs.insert(addrs.end(), pcie_addrs.begin(), pcie_addrs.end());

    return addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr x300_make(const device_addr_t &device_addr)
{
    return device::sptr(new x300_impl(device_addr));
}

UHD_STATIC_BLOCK(register_x300_device)
{
    device::register_device(&x300_find, &x300_make, device::USRP);
}

static void x300_load_fw(wb_iface::sptr fw_reg_ctrl, const std::string &file_name)
{
    UHD_LOGGER_INFO("X300") << "Loading firmware " << file_name;

    //load file into memory
    std::ifstream fw_file(file_name.c_str());
    uint32_t fw_file_buff[X300_FW_NUM_BYTES/sizeof(uint32_t)];
    fw_file.read((char *)fw_file_buff, sizeof(fw_file_buff));
    fw_file.close();

    //Poke the fw words into the WB boot loader
    fw_reg_ctrl->poke32(SR_ADDR(BOOT_LDR_BASE, BL_ADDRESS), 0);
    for (size_t i = 0; i < X300_FW_NUM_BYTES; i+=sizeof(uint32_t))
    {
        //@TODO: FIXME: Since x300_ctrl_iface acks each write and traps exceptions, the first try for the last word
        //              written will print an error because it triggers a FW reload and fails to reply.
        fw_reg_ctrl->poke32(SR_ADDR(BOOT_LDR_BASE, BL_DATA), uhd::byteswap(fw_file_buff[i/sizeof(uint32_t)]));
    }

    //Wait for fimrware to reboot. 3s is an upper bound
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    UHD_LOGGER_INFO("X300") << "Firmware loaded!" ;
}

x300_impl::x300_impl(const uhd::device_addr_t &dev_addr)
    : device3_impl()
    , _sid_framer(0)
{
    UHD_LOGGER_INFO("X300") << "X300 initialization sequence...";
    _ignore_cal_file = dev_addr.has_key("ignore-cal-file");
    _tree->create<std::string>("/name").set("X-Series Device");

    const device_addrs_t device_args = separate_device_addr(dev_addr);
    _mb.resize(device_args.size());

    // Serialize the initialization process
    if (dev_addr.has_key("serialize_init") or device_args.size() == 1) {
        for (size_t i = 0; i < device_args.size(); i++)
        {
            this->setup_mb(i, device_args[i]);
        }
        return;
    }


    // Initialize groups of USRPs in parallel
    size_t total_usrps = device_args.size();
    size_t num_usrps   = 0;
    while (num_usrps < total_usrps)
    {
        size_t init_usrps = std::min(total_usrps - num_usrps, X300_MAX_INIT_THREADS);
        boost::thread_group setup_threads;
        for (size_t i = 0; i < init_usrps; i++)
        {
            size_t index = num_usrps + i;
            setup_threads.create_thread(
                boost::bind(&x300_impl::setup_mb, this, index, device_args[index])
            );
        }
        setup_threads.join_all();
        num_usrps += init_usrps;
    }

}

void x300_impl::mboard_members_t::discover_eth(
        const mboard_eeprom_t mb_eeprom,
        const std::vector<std::string> &ip_addrs)
{
    // Clear any previous addresses added
    eth_conns.clear();

    // Index the MB EEPROM addresses
    std::vector<std::string> mb_eeprom_addrs;
    const size_t num_mb_eeprom_addrs = 4;
    for (size_t i = 0; i < num_mb_eeprom_addrs; i++) {
        const std::string key = "ip-addr" + boost::to_string(i);

        // Show a warning if there exists duplicate addresses in the mboard eeprom
        if (std::find(mb_eeprom_addrs.begin(), mb_eeprom_addrs.end(), mb_eeprom[key]) != mb_eeprom_addrs.end()) {
            UHD_LOGGER_WARNING("X300") << str(boost::format(
                "Duplicate IP address %s found in mboard EEPROM. "
                "Device may not function properly. View and reprogram the values "
                "using the usrp_burn_mb_eeprom utility.") % mb_eeprom[key]);
        }
        mb_eeprom_addrs.push_back(mb_eeprom[key]);
    }

    for(const std::string& addr:  ip_addrs) {
        x300_eth_conn_t conn_iface;
        conn_iface.addr = addr;
        conn_iface.type = X300_IFACE_NONE;

        // Decide from the mboard eeprom what IP corresponds
        // to an interface
        for (size_t i = 0; i < mb_eeprom_addrs.size(); i++) {
            if (addr == mb_eeprom_addrs[i]) {
                // Choose the interface based on the index parity
                if (i % 2 == 0) {
                    conn_iface.type = X300_IFACE_ETH0;
                } else {
                    conn_iface.type = X300_IFACE_ETH1;
                }
                break;
            }
        }

        // Check default IP addresses if we couldn't
        // determine the IP from the mboard eeprom
        if (conn_iface.type == X300_IFACE_NONE) {
            UHD_LOGGER_WARNING("X300") << str(boost::format(
                "Address %s not found in mboard EEPROM. Address may be wrong or "
                "the EEPROM may be corrupt. Attempting to continue with default "
                "IP addresses.") % conn_iface.addr
            );

            if (addr == boost::asio::ip::address_v4(
                uint32_t(X300_DEFAULT_IP_ETH0_1G)).to_string()) {
                conn_iface.type = X300_IFACE_ETH0;
            } else if (addr == boost::asio::ip::address_v4(
                uint32_t(X300_DEFAULT_IP_ETH1_1G)).to_string()) {
                conn_iface.type = X300_IFACE_ETH1;
            } else if (addr == boost::asio::ip::address_v4(
                uint32_t(X300_DEFAULT_IP_ETH0_10G)).to_string()) {
                conn_iface.type = X300_IFACE_ETH0;
            } else if (addr == boost::asio::ip::address_v4(
                uint32_t(X300_DEFAULT_IP_ETH1_10G)).to_string()) {
                conn_iface.type = X300_IFACE_ETH1;
            } else {
                throw uhd::assertion_error(str(boost::format(
                    "X300 Initialization Error: Failed to match address %s with "
                    "any addresses for the device. Please check the address.")
                    % conn_iface.addr
                ));
            }
        }

        // Save to a vector of connections
        if (conn_iface.type != X300_IFACE_NONE) {
            // Check the address before we add it
            try
            {
                wb_iface::sptr zpu_ctrl = x300_make_ctrl_iface_enet(
                    udp_simple::make_connected(conn_iface.addr,
                        BOOST_STRINGIZE(X300_FW_COMMS_UDP_PORT)),
                    false /* Suppress timeout errors */
                );

                // Peek the ZPU ctrl to make sure this connection works
                zpu_ctrl->peek32(0);
            }

            // If the address does not work, throw an error
            catch(std::exception &)
            {
                throw uhd::io_error(str(boost::format(
                    "X300 Initialization Error: Invalid address %s")
                    % conn_iface.addr));
            }
            eth_conns.push_back(conn_iface);
        }
    }

    if (eth_conns.size() == 0)
        throw uhd::assertion_error("X300 Initialization Error: No ethernet interfaces specified.");
}

void x300_impl::setup_mb(const size_t mb_i, const uhd::device_addr_t &dev_addr)
{
    const fs_path mb_path = fs_path("/mboards") / mb_i;
    mboard_members_t &mb = _mb[mb_i];
    mb.initialization_done = false;

    const std::string thread_id(
        boost::lexical_cast<std::string>(boost::this_thread::get_id())
    );
    const std::string thread_msg(
        "Thread ID " + thread_id + " for motherboard "
        + std::to_string(mb_i)
    );

    std::vector<std::string> eth_addrs;
    // Not choosing eth0 based on resource might cause user issues
    std::string eth0_addr = dev_addr.has_key("resource") ? dev_addr["resource"] : dev_addr["addr"];
    eth_addrs.push_back(eth0_addr);

    mb.next_src_addr = 0;   //Host source address for blocks
    mb.next_tx_src_addr = 0;
    mb.next_rx_src_addr = 0;
    if (dev_addr.has_key("second_addr")) {
        std::string eth1_addr = dev_addr["second_addr"];

        // Ensure we do not have duplicate addresses
        if (eth1_addr != eth0_addr)
            eth_addrs.push_back(eth1_addr);
    }

    // Initially store the first address provided to setup communication
    // Once we read the eeprom, we use it to map IP to its interface
    x300_eth_conn_t init;
    init.addr = eth_addrs[0];
    mb.eth_conns.push_back(init);

    mb.xport_path = dev_addr.has_key("resource") ? "nirio" : "eth";
    mb.if_pkt_is_big_endian = mb.xport_path != "nirio";

    if (mb.xport_path == "nirio")
    {
        nirio_status status = 0;

        std::string rpc_port_name(NIUSRPRIO_DEFAULT_RPC_PORT);
        if (dev_addr.has_key("niusrpriorpc_port")) {
            rpc_port_name = dev_addr["niusrpriorpc_port"];
        }
        UHD_LOGGER_INFO("X300") << boost::format("Connecting to niusrpriorpc at localhost:%s...") % rpc_port_name;

        //Instantiate the correct lvbitx object
        nifpga_lvbitx::sptr lvbitx;
        switch (get_mb_type_from_pcie(dev_addr["resource"], rpc_port_name)) {
            case USRP_X300_MB:
                lvbitx.reset(new x300_lvbitx(dev_addr["fpga"]));
                break;
            case USRP_X310_MB:
                lvbitx.reset(new x310_lvbitx(dev_addr["fpga"]));
                break;
            default:
                nirio_status_to_exception(status, "Motherboard detection error. Please ensure that you \
                    have a valid USRP X3x0, NI USRP-294xR or NI USRP-295xR device and that all the device \
                    drivers have loaded successfully.");
        }
        //Load the lvbitx onto the device
        UHD_LOGGER_INFO("X300") << boost::format("Using LVBITX bitfile %s...") % lvbitx->get_bitfile_path();
        mb.rio_fpga_interface.reset(new niusrprio_session(dev_addr["resource"], rpc_port_name));
        nirio_status_chain(mb.rio_fpga_interface->open(lvbitx, dev_addr.has_key("download-fpga")), status);
        nirio_status_to_exception(status, "x300_impl: Could not initialize RIO session.");

        //Tell the quirks object which FIFOs carry TX stream data
        const uint32_t tx_data_fifos[2] = {X300_RADIO_DEST_PREFIX_TX, X300_RADIO_DEST_PREFIX_TX + 3};
        mb.rio_fpga_interface->get_kernel_proxy()->get_rio_quirks().register_tx_streams(tx_data_fifos, 2);

        _tree->create<size_t>(mb_path / "mtu/recv").set(X300_PCIE_RX_DATA_FRAME_SIZE);
        _tree->create<size_t>(mb_path / "mtu/send").set(X300_PCIE_TX_DATA_FRAME_SIZE);
        _tree->create<double>(mb_path / "link_max_rate").set(X300_MAX_RATE_PCIE);
    }

    for(const std::string &key:  dev_addr.keys())
    {
        if (key.find("recv") != std::string::npos) mb.recv_args[key] = dev_addr[key];
        if (key.find("send") != std::string::npos) mb.send_args[key] = dev_addr[key];
    }

    if (mb.xport_path == "eth" ) {
        /* This is an ETH connection. Figure out what the maximum supported frame
         * size is for the transport in the up and down directions. The frame size
         * depends on the host PIC's NIC's MTU settings. To determine the frame size,
         * we test for support up to an expected "ceiling". If the user
         * specified a frame size, we use that frame size as the ceiling. If no
         * frame size was specified, we use the maximum UHD frame size.
         *
         * To optimize performance, the frame size should be greater than or equal
         * to the frame size that UHD uses so that frames don't get split across
         * multiple transmission units - this is why the limits passed into the
         * 'determine_max_frame_size' function are actually frame sizes. */
        frame_size_t req_max_frame_size;
        req_max_frame_size.recv_frame_size = (mb.recv_args.has_key("recv_frame_size")) \
            ? boost::lexical_cast<size_t>(mb.recv_args["recv_frame_size"]) \
            : X300_10GE_DATA_FRAME_MAX_SIZE;
        req_max_frame_size.send_frame_size = (mb.send_args.has_key("send_frame_size")) \
            ? boost::lexical_cast<size_t>(mb.send_args["send_frame_size"]) \
            : X300_10GE_DATA_FRAME_MAX_SIZE;

        #if defined UHD_PLATFORM_LINUX
            const std::string mtu_tool("ip link");
        #elif defined UHD_PLATFORM_WIN32
            const std::string mtu_tool("netsh");
        #else
            const std::string mtu_tool("ifconfig");
        #endif

        // Detect the frame size on the path to the USRP
        try {
            frame_size_t pri_frame_sizes = determine_max_frame_size(
                eth_addrs.at(0), req_max_frame_size
            );

            _max_frame_sizes = pri_frame_sizes;
            if (eth_addrs.size() > 1) {
                frame_size_t sec_frame_sizes = determine_max_frame_size(
                    eth_addrs.at(1), req_max_frame_size
                );

                // Choose the minimum of the max frame sizes
                // to ensure we don't exceed any one of the links' MTU
                _max_frame_sizes.recv_frame_size = std::min(
                    pri_frame_sizes.recv_frame_size,
                    sec_frame_sizes.recv_frame_size
                );

                _max_frame_sizes.send_frame_size = std::min(
                    pri_frame_sizes.send_frame_size,
                    sec_frame_sizes.send_frame_size
                );
            }
        } catch(std::exception &e) {
            UHD_LOGGER_ERROR("X300") << e.what() ;
        }

        if ((mb.recv_args.has_key("recv_frame_size"))
                && (req_max_frame_size.recv_frame_size > _max_frame_sizes.recv_frame_size)) {
            UHD_LOGGER_WARNING("X300")
                << boost::format("You requested a receive frame size of (%lu) but your NIC's max frame size is (%lu).")
                % req_max_frame_size.recv_frame_size
                % _max_frame_sizes.recv_frame_size
                << boost::format("Please verify your NIC's MTU setting using '%s' or set the recv_frame_size argument appropriately.")
                % mtu_tool
                << "UHD will use the auto-detected max frame size for this connection."
            ;
        }

        if ((mb.recv_args.has_key("send_frame_size"))
                && (req_max_frame_size.send_frame_size > _max_frame_sizes.send_frame_size)) {
            UHD_LOGGER_WARNING("X300")
                << boost::format("You requested a send frame size of (%lu) but your NIC's max frame size is (%lu).")
                % req_max_frame_size.send_frame_size
                % _max_frame_sizes.send_frame_size
                << boost::format("Please verify your NIC's MTU setting using '%s' or set the send_frame_size argument appropriately.")
                % mtu_tool
                << "UHD will use the auto-detected max frame size for this connection."
            ;
        }

        _tree->create<size_t>(mb_path / "mtu/recv").set(_max_frame_sizes.recv_frame_size);
        _tree->create<size_t>(mb_path / "mtu/send").set(std::min(_max_frame_sizes.send_frame_size, X300_ETH_DATA_FRAME_MAX_TX_SIZE));
        _tree->create<double>(mb_path / "link_max_rate").set(X300_MAX_RATE_10GIGE);
    }

    //create basic communication
    UHD_LOGGER_DEBUG("X300") << "Setting up basic communication...";
    if (mb.xport_path == "nirio") {
        boost::mutex::scoped_lock(pcie_zpu_iface_registry_mutex);
        if (get_pcie_zpu_iface_registry().has_key(mb.get_pri_eth().addr)) {
            throw uhd::assertion_error("Someone else has a ZPU transport to the device open. Internal error!");
        } else {
            mb.zpu_ctrl = x300_make_ctrl_iface_pcie(mb.rio_fpga_interface->get_kernel_proxy());
            get_pcie_zpu_iface_registry()[mb.get_pri_eth().addr] = boost::weak_ptr<wb_iface>(mb.zpu_ctrl);
        }
    } else {
        mb.zpu_ctrl = x300_make_ctrl_iface_enet(udp_simple::make_connected(
                    mb.get_pri_eth().addr, BOOST_STRINGIZE(X300_FW_COMMS_UDP_PORT)));
    }

    // Claim device
    if (not try_to_claim(mb.zpu_ctrl)) {
        throw uhd::runtime_error("Failed to claim device");
    }
    mb.claimer_task = uhd::task::make(boost::bind(&x300_impl::claimer_loop, this, mb.zpu_ctrl), "x300_claimer");

    //extract the FW path for the X300
    //and live load fw over ethernet link
    if (dev_addr.has_key("fw"))
    {
        const std::string x300_fw_image = find_image_path(
            dev_addr.has_key("fw")? dev_addr["fw"] : X300_FW_FILE_NAME
        );
        x300_load_fw(mb.zpu_ctrl, x300_fw_image);
    }

    //check compat numbers
    //check fpga compat before fw compat because the fw is a subset of the fpga image
    this->check_fpga_compat(mb_path, mb);
    this->check_fw_compat(mb_path, mb.zpu_ctrl);

    mb.fw_regmap = boost::make_shared<fw_regmap_t>();
    mb.fw_regmap->initialize(*mb.zpu_ctrl.get(), true);

    //store which FPGA image is loaded
    mb.loaded_fpga_image = get_fpga_option(mb.zpu_ctrl);

    //low speed perif access
    mb.zpu_spi = spi_core_3000::make(mb.zpu_ctrl, SR_ADDR(SET0_BASE, ZPU_SR_SPI),
            SR_ADDR(SET0_BASE, ZPU_RB_SPI));
    mb.zpu_i2c = i2c_core_100_wb32::make(mb.zpu_ctrl, I2C1_BASE);
    mb.zpu_i2c->set_clock_rate(X300_BUS_CLOCK_RATE/2);

    ////////////////////////////////////////////////////////////////////
    // print network routes mapping
    ////////////////////////////////////////////////////////////////////
    /*
    const uint32_t routes_addr = mb.zpu_ctrl->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_ROUTE_MAP_ADDR));
    const uint32_t routes_len = mb.zpu_ctrl->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_ROUTE_MAP_LEN));
    UHD_VAR(routes_len);
    for (size_t i = 0; i < routes_len; i+=1)
    {
        const uint32_t node_addr = mb.zpu_ctrl->peek32(SR_ADDR(routes_addr, i*2+0));
        const uint32_t nbor_addr = mb.zpu_ctrl->peek32(SR_ADDR(routes_addr, i*2+1));
        if (node_addr != 0 and nbor_addr != 0)
        {
            UHD_LOGGER_INFO("X300") << boost::format("%u: %s -> %s")
                % i
                % asio::ip::address_v4(node_addr).to_string()
                % asio::ip::address_v4(nbor_addr).to_string();
        }
    }
    */

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    UHD_LOGGER_DEBUG("X300") << "Loading values from EEPROM...";
    x300_mb_eeprom_iface::sptr eeprom16 = x300_mb_eeprom_iface::make(mb.zpu_ctrl, mb.zpu_i2c);
    if (dev_addr.has_key("blank_eeprom")) {
        UHD_LOGGER_WARNING("X300") << "Obliterating the motherboard EEPROM...";
        eeprom16->write_eeprom(0x50, 0, byte_vector_t(256, 0xff));
    }

    const mboard_eeprom_t mb_eeprom = get_mb_eeprom(eeprom16);
    _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
        // Initialize the property with a current copy of the EEPROM contents
        .set(mb_eeprom)
        // Whenever this property is written, update the chip
        .add_coerced_subscriber(
            [this, eeprom16](const mboard_eeprom_t &mb_eeprom){
                this->set_mb_eeprom(eeprom16, mb_eeprom);
            }
        )
    ;

    bool recover_mb_eeprom = dev_addr.has_key("recover_mb_eeprom");
    if (recover_mb_eeprom) {
        UHD_LOGGER_WARNING("X300") << "UHD is operating in EEPROM Recovery Mode which disables hardware version "
                            "checks.\nOperating in this mode may cause hardware damage and unstable "
                            "radio performance!";
    }

    ////////////////////////////////////////////////////////////////////
    // parse the product number
    ////////////////////////////////////////////////////////////////////
    std::string product_name = "X300?";
    switch (get_mb_type_from_eeprom(mb_eeprom)) {
        case USRP_X300_MB:
            product_name = "X300";
            break;
        case USRP_X310_MB:
            product_name = "X310";
            break;
        default:
            if (not recover_mb_eeprom)
                throw uhd::runtime_error("Unrecognized product type.\n"
                                         "Either the software does not support this device in which case please update your driver software to the latest version and retry OR\n"
                                         "The product code in the EEPROM is corrupt and may require reprogramming.");
    }
    _tree->create<std::string>(mb_path / "name").set(product_name);
    _tree->create<std::string>(mb_path / "codename").set("Yetti");

    ////////////////////////////////////////////////////////////////////
    // determine routing based on address match
    ////////////////////////////////////////////////////////////////////
    if (mb.xport_path != "nirio") {
        // Discover ethernet interfaces
        mb.discover_eth(mb_eeprom, eth_addrs);
    }

    ////////////////////////////////////////////////////////////////////
    // read hardware revision and compatibility number
    ////////////////////////////////////////////////////////////////////
    mb.hw_rev = 0;
    if(mb_eeprom.has_key("revision") and not mb_eeprom["revision"].empty()) {
        try {
            mb.hw_rev = boost::lexical_cast<size_t>(mb_eeprom["revision"]);
        } catch(...) {
            if (not recover_mb_eeprom)
                throw uhd::runtime_error("Revision in EEPROM is invalid! Please reprogram your EEPROM.");
        }
    } else {
        if (not recover_mb_eeprom)
            throw uhd::runtime_error("No revision detected. MB EEPROM must be reprogrammed!");
    }

    size_t hw_rev_compat = 0;
    if (mb.hw_rev >= 7) { //Revision compat was added with revision 7
        if (mb_eeprom.has_key("revision_compat") and not mb_eeprom["revision_compat"].empty()) {
            try {
                hw_rev_compat = boost::lexical_cast<size_t>(mb_eeprom["revision_compat"]);
            } catch(...) {
                if (not recover_mb_eeprom)
                    throw uhd::runtime_error("Revision compat in EEPROM is invalid! Please reprogram your EEPROM.");
            }
        } else {
            if (not recover_mb_eeprom)
                throw uhd::runtime_error("No revision compat detected. MB EEPROM must be reprogrammed!");
        }
    } else {
        //For older HW just assume that revision_compat = revision
        hw_rev_compat = mb.hw_rev;
    }

    if (hw_rev_compat > X300_REVISION_COMPAT) {
        if (not recover_mb_eeprom)
            throw uhd::runtime_error(str(boost::format(
                "Hardware is too new for this software. Please upgrade to a driver that supports hardware revision %d.")
                % mb.hw_rev));
    } else if (mb.hw_rev < X300_REVISION_MIN) { //Compare min against the revision (and not compat) to give us more leeway for partial support for a compat
        if (not recover_mb_eeprom)
            throw uhd::runtime_error(str(boost::format(
                "Software is too new for this hardware. Please downgrade to a driver that supports hardware revision %d.")
                % mb.hw_rev));
    }

    ////////////////////////////////////////////////////////////////////
    // create clock control objects
    ////////////////////////////////////////////////////////////////////
    UHD_LOGGER_DEBUG("X300") << "Setting up RF frontend clocking...";

    //Initialize clock control registers. NOTE: This does not configure the LMK yet.
    mb.clock = x300_clock_ctrl::make(mb.zpu_spi,
        1 /*slaveno*/,
        mb.hw_rev,
        dev_addr.cast<double>("master_clock_rate", X300_DEFAULT_TICK_RATE),
        dev_addr.cast<double>("dboard_clock_rate", X300_DEFAULT_DBOARD_CLK_RATE),
        dev_addr.cast<double>("system_ref_rate", X300_DEFAULT_SYSREF_RATE));

    //Initialize clock source to use internal reference and generate
    //a valid radio clock. This may change after configuration is done.
    //This will configure the LMK and wait for lock
    update_clock_source(mb, X300_DEFAULT_CLOCK_SOURCE);

    ////////////////////////////////////////////////////////////////////
    // create clock properties
    ////////////////////////////////////////////////////////////////////
    _tree->create<double>(mb_path / "master_clock_rate")
        .set_publisher(boost::bind(&x300_clock_ctrl::get_master_clock_rate, mb.clock))
    ;

    UHD_LOGGER_INFO("X300")
        << "Radio 1x clock: " << (mb.clock->get_master_clock_rate()/1e6)
        << " MHz";

    ////////////////////////////////////////////////////////////////////
    // Create the GPSDO control
    ////////////////////////////////////////////////////////////////////
    static const uint32_t dont_look_for_gpsdo = 0x1234abcdul;

    //otherwise if not disabled, look for the internal GPSDO
    if (mb.zpu_ctrl->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_GPSDO_STATUS)) != dont_look_for_gpsdo)
    {
        UHD_LOG_DEBUG("X300", "Detecting internal GPSDO....");
        try {
            // gps_ctrl will print its own log statements if a GPSDO was found
            mb.gps = gps_ctrl::make(x300_make_uart_iface(mb.zpu_ctrl));
        }
        catch(std::exception &e) {
            UHD_LOGGER_ERROR("X300")
                << "An error occurred making GPSDO control: " << e.what();
        }
        if (mb.gps and mb.gps->gps_detected()) {
            for(const std::string& name : mb.gps->get_sensors()) {
                _tree->create<sensor_value_t>(mb_path / "sensors" / name)
                    .set_publisher(boost::bind(&gps_ctrl::get_sensor, mb.gps, name));
            }
        }
        else {
            mb.zpu_ctrl->poke32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_GPSDO_STATUS), dont_look_for_gpsdo);
        }
    }

    ////////////////////////////////////////////////////////////////////
    // setup time sources and properties
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::string>(mb_path / "time_source" / "value")
        .set("internal")
        .add_coerced_subscriber([this, &mb](const std::string& time_source){
            this->update_time_source(mb, time_source);
        })
    ;
    static const std::vector<std::string> time_sources =
        {"internal", "external", "gpsdo"};
    _tree->create<std::vector<std::string>>(mb_path / "time_source" / "options")
        .set(time_sources);

    //setup the time output, default to ON
    _tree->create<bool>(mb_path / "time_source" / "output")
        .add_coerced_subscriber([this, &mb](const bool time_output){
            this->set_time_source_out(mb, time_output);
        })
        .set(true)
    ;

    ////////////////////////////////////////////////////////////////////
    // setup clock sources and properties
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::string>(mb_path / "clock_source" / "value")
        .set(X300_DEFAULT_CLOCK_SOURCE)
        .add_coerced_subscriber([this, &mb](const std::string& clock_source){
            this->update_clock_source(mb, clock_source);
        })
    ;
    static const std::vector<std::string> clock_source_options =
        {"internal", "external", "gpsdo"};
    _tree->create<std::vector<std::string>>(mb_path / "clock_source" / "options")
        .set(clock_source_options);

    //setup external reference options. default to 10 MHz input reference
    _tree->create<std::string>(mb_path / "clock_source" / "external");
    static const std::vector<double> external_freq_options = {10e6, 30.72e6, 200e6};
    _tree->create<std::vector<double>>(mb_path / "clock_source" / "external" / "freq" / "options")
        .set(external_freq_options);
    _tree->create<double>(mb_path / "clock_source" / "external" / "value")
        .set(mb.clock->get_sysref_clock_rate());
    // FIXME the external clock source settings need to be more robust

    //setup the clock output, default to ON
    _tree->create<bool>(mb_path / "clock_source" / "output")
        .add_coerced_subscriber([&mb](const bool clock_output){
            mb.clock->set_ref_out(clock_output);
        })
    ;

    // Initialize tick rate (must be done before setting time)
    // Note: The master tick rate can't be changed at runtime!
    const double master_clock_rate = mb.clock->get_master_clock_rate();
    _tree->create<double>(mb_path / "tick_rate")
        .set_coercer([master_clock_rate](const double rate){
            // The contract of multi_usrp::set_master_clock_rate() is to coerce
            // and not throw, so we'll follow that behaviour here.
            if (!uhd::math::frequencies_are_equal(rate, master_clock_rate)) {
                UHD_LOGGER_WARNING("X300") <<
                    "Cannot update master clock rate! X300 Series does not "
                    "allow changing the clock rate during runtime."
                ;
            }
            return master_clock_rate;
        })
        .add_coerced_subscriber([this](const double rate){
            this->update_tx_streamers(rate);
        })
        .add_coerced_subscriber([this](const double rate){
            this->update_rx_streamers(rate);
        })
        .set(master_clock_rate)
    ;

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    _tree->create<sensor_value_t>(mb_path / "sensors" / "ref_locked")
        .set_publisher(boost::bind(&x300_impl::get_ref_locked, this, mb));

    //////////////// RFNOC /////////////////
    const size_t n_rfnoc_blocks = mb.zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_NUM_CE));
    enumerate_rfnoc_blocks(
        mb_i,
        n_rfnoc_blocks,
        X300_XB_DST_PCI + 1, /* base port */
        uhd::sid_t(X300_SRC_ADDR0, 0, X300_DST_ADDR + mb_i, 0),
        dev_addr
    );
    //////////////// RFNOC /////////////////

    // If we have a radio, we must configure its codec control:
    const std::string radio_blockid_hint = str(boost::format("%d/Radio") % mb_i);
    std::vector<rfnoc::block_id_t> radio_ids =
                find_blocks<rfnoc::x300_radio_ctrl_impl>(radio_blockid_hint);
    if (not radio_ids.empty()) {
        if (radio_ids.size() > 2) {
            UHD_LOGGER_WARNING("X300")
                << "Too many Radio Blocks found. Using only the first two.";
            radio_ids.resize(2);
        }

        for(const rfnoc::block_id_t &id:  radio_ids) {
            rfnoc::x300_radio_ctrl_impl::sptr radio(get_block_ctrl<rfnoc::x300_radio_ctrl_impl>(id));
            mb.radios.push_back(radio);
            radio->setup_radio(
                    mb.zpu_i2c,
                    mb.clock,
                    dev_addr.has_key("ignore-cal-file"),
                    dev_addr.has_key("self_cal_adc_delay")
            );
        }

        ////////////////////////////////////////////////////////////////////
        // ADC test and cal
        ////////////////////////////////////////////////////////////////////
        if (dev_addr.has_key("self_cal_adc_delay")) {
            rfnoc::x300_radio_ctrl_impl::self_cal_adc_xfer_delay(
                mb.radios, mb.clock,
                boost::bind(&x300_impl::wait_for_clk_locked, this, mb, fw_regmap_t::clk_status_reg_t::LMK_LOCK, _1),
                true /* Apply ADC delay */);
        }
        if (dev_addr.has_key("ext_adc_self_test")) {
            rfnoc::x300_radio_ctrl_impl::extended_adc_test(
                mb.radios,
                dev_addr.cast<double>("ext_adc_self_test", 30));
        } else if (not dev_addr.has_key("recover_mb_eeprom")){
            for (size_t i = 0; i < mb.radios.size(); i++) {
                mb.radios.at(i)->self_test_adc();
            }
        }

        ////////////////////////////////////////////////////////////////////
        // Synchronize times (dboard initialization can desynchronize them)
        ////////////////////////////////////////////////////////////////////
        if (radio_ids.size() == 2) {
            this->sync_times(mb, mb.radios[0]->get_time_now());
        }

    } else {
        UHD_LOGGER_INFO("X300")
            << "No Radio Block found. Assuming radio-less operation.";
    } /* end of radio block(s) initialization */

    mb.initialization_done = true;
}

x300_impl::~x300_impl(void)
{
    try
    {
        for(mboard_members_t &mb:  _mb)
        {
            //kill the claimer task and unclaim the device
            mb.claimer_task.reset();
            {   //Critical section
                boost::mutex::scoped_lock(pcie_zpu_iface_registry_mutex);
                release(mb.zpu_ctrl);
                //If the process is killed, the entire registry will disappear so we
                //don't need to worry about unclean shutdowns here.
                if (get_pcie_zpu_iface_registry().has_key(mb.get_pri_eth().addr)) {
                    get_pcie_zpu_iface_registry().pop(mb.get_pri_eth().addr);
                }
            }
        }
    }
    catch(...)
    {
        UHD_SAFE_CALL(throw;)
    }
}

uint32_t x300_impl::mboard_members_t::allocate_pcie_dma_chan(const uhd::sid_t &tx_sid, const xport_type_t xport_type)
{
    static const uint32_t CTRL_CHANNEL       = 0;
    static const uint32_t ASYNC_MSG_CHANNEL  = 1;
    static const uint32_t FIRST_DATA_CHANNEL = 2;
    if (xport_type == CTRL) {
        return CTRL_CHANNEL;
    } else if (xport_type == ASYNC_MSG) {
        return ASYNC_MSG_CHANNEL;
    } else {
        // sid_t has no comparison defined, so we need to convert it uint32_t
        uint32_t raw_sid = tx_sid.get();

        if (_dma_chan_pool.count(raw_sid) == 0) {
            size_t channel = _dma_chan_pool.size() + FIRST_DATA_CHANNEL;
            if (channel > X300_PCIE_MAX_CHANNELS) {
                throw uhd::runtime_error("Trying to allocate more DMA channels than are available");
            }
            _dma_chan_pool[raw_sid] = channel;
            UHD_LOGGER_DEBUG("X300")
                << "Assigning PCIe DMA channel " << _dma_chan_pool[raw_sid]
                << " to SID " << tx_sid.to_pp_string_hex();
        }

        return _dma_chan_pool[raw_sid];
    }
}

static uint32_t extract_sid_from_pkt(void* pkt, size_t) {
    return uhd::sid_t(uhd::wtohx(static_cast<const uint32_t*>(pkt)[1])).get_dst();
}

static uhd::transport::muxed_zero_copy_if::sptr make_muxed_pcie_msg_xport
(
    uhd::niusrprio::niusrprio_session::sptr rio_fpga_interface,
    uint32_t dma_channel_num,
    size_t max_muxed_ports
) {
    zero_copy_xport_params buff_args;
    buff_args.send_frame_size = X300_PCIE_MSG_FRAME_SIZE;
    buff_args.recv_frame_size = X300_PCIE_MSG_FRAME_SIZE;
    buff_args.num_send_frames = X300_PCIE_MSG_NUM_FRAMES * max_muxed_ports;
    buff_args.num_recv_frames = X300_PCIE_MSG_NUM_FRAMES * max_muxed_ports;

    zero_copy_if::sptr base_xport = nirio_zero_copy::make(
        rio_fpga_interface, dma_channel_num,
        buff_args, uhd::device_addr_t());
    return muxed_zero_copy_if::make(base_xport, extract_sid_from_pkt, max_muxed_ports);
}

uhd::both_xports_t x300_impl::make_transport(
    const uhd::sid_t &address,
    const xport_type_t xport_type,
    const uhd::device_addr_t& args
) {
    const size_t mb_index = address.get_dst_addr() - X300_DST_ADDR;
    mboard_members_t &mb = _mb[mb_index];
    const uhd::device_addr_t& xport_args = (xport_type == CTRL) ? uhd::device_addr_t() : args;
    zero_copy_xport_params default_buff_args;

    both_xports_t xports;
    xports.endianness = mb.if_pkt_is_big_endian ? ENDIANNESS_BIG : ENDIANNESS_LITTLE;
    if (mb.xport_path == "nirio") {
        xports.send_sid = this->allocate_sid(mb, address, X300_SRC_ADDR0, X300_XB_DST_PCI);
        xports.recv_sid = xports.send_sid.reversed();

        uint32_t dma_channel_num = mb.allocate_pcie_dma_chan(xports.send_sid, xport_type);
        if (xport_type == CTRL) {
            //Transport for control stream
            if (not mb.ctrl_dma_xport) {
                //One underlying DMA channel will handle
                //all control traffic
                mb.ctrl_dma_xport = make_muxed_pcie_msg_xport(
                    mb.rio_fpga_interface,
                    dma_channel_num,
                    X300_PCIE_MAX_MUXED_CTRL_XPORTS);
            }
            //Create a virtual control transport
            xports.recv = mb.ctrl_dma_xport->make_stream(xports.recv_sid.get_dst());
        } else if (xport_type == ASYNC_MSG) {
            //Transport for async message stream
            if (not mb.async_msg_dma_xport) {
                //One underlying DMA channel will handle
                //all async message traffic
                mb.async_msg_dma_xport = make_muxed_pcie_msg_xport(
                    mb.rio_fpga_interface,
                    dma_channel_num,
                    X300_PCIE_MAX_MUXED_ASYNC_XPORTS);
            }
            //Create a virtual async message transport
            xports.recv = mb.async_msg_dma_xport->make_stream(xports.recv_sid.get_dst());
        } else {
            //Transport for data stream
            default_buff_args.send_frame_size =
                (xport_type == TX_DATA)
                ? X300_PCIE_TX_DATA_FRAME_SIZE
                : X300_PCIE_MSG_FRAME_SIZE;

            default_buff_args.recv_frame_size =
                (xport_type == RX_DATA)
                ? X300_PCIE_RX_DATA_FRAME_SIZE
                : X300_PCIE_MSG_FRAME_SIZE;

			default_buff_args.num_send_frames =
				(xport_type == TX_DATA)
                ? X300_PCIE_TX_DATA_NUM_FRAMES
                : X300_PCIE_MSG_NUM_FRAMES;

            default_buff_args.num_recv_frames =
                (xport_type == RX_DATA)
                ? X300_PCIE_RX_DATA_NUM_FRAMES
                : X300_PCIE_MSG_NUM_FRAMES;

            xports.recv = nirio_zero_copy::make(
                mb.rio_fpga_interface, dma_channel_num,
                default_buff_args, xport_args);
        }

        xports.send = xports.recv;

        // Router config word is:
        // - Upper 16 bits: Destination address (e.g. 0.0)
        // - Lower 16 bits: DMA channel
        uint32_t router_config_word = (xports.recv_sid.get_dst() << 16) | dma_channel_num;
        mb.rio_fpga_interface->get_kernel_proxy()->poke(PCIE_ROUTER_REG(0), router_config_word);

        //For the nirio transport, buffer size is depends on the frame size and num frames
        xports.recv_buff_size = xports.recv->get_num_recv_frames() * xports.recv->get_recv_frame_size();
        xports.send_buff_size = xports.send->get_num_send_frames() * xports.send->get_send_frame_size();

    } else if (mb.xport_path == "eth") {
        // Decide on the IP/Interface pair based on the endpoint index
        size_t &next_src_addr =
            xport_type == TX_DATA ? mb.next_tx_src_addr :
            xport_type == RX_DATA ? mb.next_rx_src_addr :
            mb.next_src_addr;
        std::string interface_addr = mb.eth_conns[next_src_addr].addr;
        const uint32_t xbar_src_addr =
            next_src_addr==0 ? X300_SRC_ADDR0 : X300_SRC_ADDR1;
        const uint32_t xbar_src_dst =
            mb.eth_conns[next_src_addr].type==X300_IFACE_ETH0 ? X300_XB_DST_E0 : X300_XB_DST_E1;
        next_src_addr = (next_src_addr + 1) % mb.eth_conns.size();

        xports.send_sid = this->allocate_sid(mb, address, xbar_src_addr, xbar_src_dst);
        xports.recv_sid = xports.send_sid.reversed();

        /* Determine what the recommended frame size is for this
         * connection type.*/
        size_t eth_data_rec_frame_size = 0;

        fs_path mboard_path = fs_path("/mboards") / mb_index / "link_max_rate";

        if (mb.loaded_fpga_image == "HG") {
            size_t max_link_rate = 0;
            if (xbar_src_dst == X300_XB_DST_E0) {
                eth_data_rec_frame_size = X300_1GE_DATA_FRAME_MAX_SIZE;
                max_link_rate += X300_MAX_RATE_1GIGE;
            } else if (xbar_src_dst == X300_XB_DST_E1) {
                eth_data_rec_frame_size = X300_10GE_DATA_FRAME_MAX_SIZE;
                max_link_rate += X300_MAX_RATE_10GIGE;
            }
            _tree->access<double>(mboard_path).set(max_link_rate);
        } else if (mb.loaded_fpga_image == "XG" or mb.loaded_fpga_image == "XA") {
            eth_data_rec_frame_size = X300_10GE_DATA_FRAME_MAX_SIZE;
            size_t max_link_rate = X300_MAX_RATE_10GIGE;
            max_link_rate *= mb.eth_conns.size();
            _tree->access<double>(mboard_path).set(max_link_rate);
        } else if (mb.loaded_fpga_image == "HA") {
            eth_data_rec_frame_size = X300_1GE_DATA_FRAME_MAX_SIZE;
            size_t max_link_rate = X300_MAX_RATE_1GIGE;
            max_link_rate *= mb.eth_conns.size();
            _tree->access<double>(mboard_path).set(max_link_rate);
        }

        if (eth_data_rec_frame_size == 0) {
            throw uhd::runtime_error("Unable to determine ETH link type.");
        }

        /* Print a warning if the system's max available frame size is less than the most optimal
         * frame size for this type of connection. */
        if (_max_frame_sizes.send_frame_size < eth_data_rec_frame_size) {
            UHD_LOGGER_WARNING("X300")
                << boost::format("For this connection, UHD recommends a send frame size of at least %lu for best\nperformance, but your system's MTU will only allow %lu.")
                % eth_data_rec_frame_size
                % _max_frame_sizes.send_frame_size
                << "This will negatively impact your maximum achievable sample rate."
            ;
        }

        if (_max_frame_sizes.recv_frame_size < eth_data_rec_frame_size) {
            UHD_LOGGER_WARNING("X300")
                << boost::format("For this connection, UHD recommends a receive frame size of at least %lu for best\nperformance, but your system's MTU will only allow %lu.")
                % eth_data_rec_frame_size
                % _max_frame_sizes.recv_frame_size
                << "This will negatively impact your maximum achievable sample rate."
            ;
        }

        size_t system_max_send_frame_size = (size_t) _max_frame_sizes.send_frame_size;
        size_t system_max_recv_frame_size = (size_t) _max_frame_sizes.recv_frame_size;

        // Make sure frame sizes do not exceed the max available value supported by UHD
        default_buff_args.send_frame_size =
            (xport_type == TX_DATA)
            ? std::min(system_max_send_frame_size, X300_10GE_DATA_FRAME_MAX_SIZE)
            : std::min(system_max_send_frame_size, X300_ETH_MSG_FRAME_SIZE);

        default_buff_args.recv_frame_size =
            (xport_type == RX_DATA)
            ? std::min(system_max_recv_frame_size, X300_10GE_DATA_FRAME_MAX_SIZE)
            : std::min(system_max_recv_frame_size, X300_ETH_MSG_FRAME_SIZE);

        default_buff_args.num_send_frames =
            (xport_type == TX_DATA)
            ? X300_ETH_DATA_NUM_FRAMES
            : X300_ETH_MSG_NUM_FRAMES;

        default_buff_args.num_recv_frames =
            (xport_type == RX_DATA)
            ? X300_ETH_DATA_NUM_FRAMES
            : X300_ETH_MSG_NUM_FRAMES;

        //make a new transport - fpga has no idea how to talk to us on this yet
        udp_zero_copy::buff_params buff_params;

        xports.recv = udp_zero_copy::make(
                interface_addr,
                BOOST_STRINGIZE(X300_VITA_UDP_PORT),
                default_buff_args,
                buff_params,
                xport_args);

        // Create a threaded transport for the receive chain only
        // Note that this shouldn't affect PCIe
        if (xport_type == RX_DATA) {
            xports.recv = zero_copy_recv_offload::make(
                    xports.recv,
                    X300_THREAD_BUFFER_TIMEOUT
            );
        }
        xports.send = xports.recv;

        //For the UDP transport the buffer size if the size of the socket buffer
        //in the kernel
        xports.recv_buff_size = buff_params.recv_buff_size;
        xports.send_buff_size = buff_params.send_buff_size;

        //clear the ethernet dispatcher's udp port
        //NOT clearing this, the dispatcher is now intelligent
        //_zpu_ctrl->poke32(SR_ADDR(SET0_BASE, (ZPU_SR_ETHINT0+8+3)), 0);

        //send a mini packet with SID into the ZPU
        //ZPU will reprogram the ethernet framer
        UHD_LOGGER_TRACE("X300")
            << "programming packet for new xport on "
            << interface_addr <<  " sid " << xports.send_sid;
        //YES, get a __send__ buffer from the __recv__ socket
        //-- this is the only way to program the framer for recv:
        managed_send_buffer::sptr buff = xports.recv->get_send_buff();
        buff->cast<uint32_t *>()[0] = 0; //eth dispatch looks for != 0
        buff->cast<uint32_t *>()[1] = uhd::htonx(xports.send_sid.get());
        buff->commit(8);
        buff.reset();

        //reprogram the ethernet dispatcher's udp port (should be safe to always set)
        UHD_LOGGER_TRACE("X300")
            << "reprogram the ethernet dispatcher's udp port" ;
        mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, (ZPU_SR_ETHINT0+8+3)), X300_VITA_UDP_PORT);
        mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, (ZPU_SR_ETHINT1+8+3)), X300_VITA_UDP_PORT);

        //Do a peek to an arbitrary address to guarantee that the
        //ethernet framer has been programmed before we return.
        mb.zpu_ctrl->peek32(0);
    }
    return xports;
}


uhd::sid_t x300_impl::allocate_sid(
        mboard_members_t &mb,
        const uhd::sid_t &address,
        const uint32_t src_addr,
        const uint32_t src_dst
) {
    uhd::sid_t sid = address;
    sid.set_src_addr(src_addr);
    sid.set_src_endpoint(_sid_framer++);    //increment for next setup

    // TODO Move all of this setup_mb()
    // Program the X300 to recognise it's own local address.
    mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_XB_LOCAL), address.get_dst_addr());
    // Program CAM entry for outgoing packets matching a X300 resource (for example a Radio)
    // This type of packet matches the XB_LOCAL address and is looked up in the upper half of the CAM
    mb.zpu_ctrl->poke32(SR_ADDR(SETXB_BASE, 256 + address.get_dst_endpoint()), address.get_dst_xbarport());
    // Program CAM entry for returning packets to us (for example GR host via Eth0)
    // This type of packet does not match the XB_LOCAL address and is looked up in the lower half of the CAM
    mb.zpu_ctrl->poke32(SR_ADDR(SETXB_BASE, 0 + src_addr), src_dst);

    UHD_LOGGER_TRACE("X300") << "done router config for sid " << sid ;

    return sid;
}

/***********************************************************************
 * clock and time control logic
 **********************************************************************/
void x300_impl::set_time_source_out(mboard_members_t &mb, const bool enb)
{
    mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_OUT_EN, enb?1:0);
}

void x300_impl::update_clock_source(mboard_members_t &mb, const std::string &source)
{
    //Optimize for the case when the current source is internal and we are trying
    //to set it to internal. This is the only case where we are guaranteed that
    //the clock has not gone away so we can skip setting the MUX and reseting the LMK.
    const bool reconfigure_clks = (mb.current_refclk_src != "internal") or (source != "internal");
    if (reconfigure_clks) {
        //Update the clock MUX on the motherboard to select the requested source
        if (source == "internal") {
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::CLK_SOURCE, fw_regmap_t::clk_ctrl_reg_t::SRC_INTERNAL);
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::TCXO_EN, 1);
        } else if (source == "external") {
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::CLK_SOURCE, fw_regmap_t::clk_ctrl_reg_t::SRC_EXTERNAL);
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::TCXO_EN, 0);
        } else if (source == "gpsdo") {
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::CLK_SOURCE, fw_regmap_t::clk_ctrl_reg_t::SRC_GPSDO);
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::TCXO_EN, 0);
        } else {
            throw uhd::key_error("update_clock_source: unknown source: " + source);
        }
        mb.fw_regmap->clock_ctrl_reg.flush();

        //Reset the LMK to make sure it re-locks to the new reference
        mb.clock->reset_clocks();
    }

    //Wait for the LMK to lock (always, as a sanity check that the clock is useable)
    //* Currently the LMK can take as long as 30 seconds to lock to a reference but we don't
    //* want to wait that long during initialization.
    //TODO: Need to verify timeout and settings to make sure lock can be achieved in < 1.0 seconds
    double timeout = mb.initialization_done ? 30.0 : 1.0;

    //The programming code in x300_clock_ctrl is not compatible with revs <= 4 and may
    //lead to locking issues. So, disable the ref-locked check for older (unsupported) boards.
    if (mb.hw_rev > 4) {
        if (not wait_for_clk_locked(mb, fw_regmap_t::clk_status_reg_t::LMK_LOCK, timeout)) {
            //failed to lock on reference
            if (mb.initialization_done) {
                throw uhd::runtime_error((boost::format("Reference Clock PLL failed to lock to %s source.") % source).str());
            } else {
                //TODO: Re-enable this warning when we figure out a reliable lock time
                //UHD_LOGGER_WARNING("X300") << "Reference clock failed to lock to " + source + " during device initialization.  " <<
                //    "Check for the lock before operation or ignore this warning if using another clock source." ;
            }
        }
    }

    if (reconfigure_clks) {
        //Reset the radio clock PLL in the FPGA
        mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), ZPU_SR_SW_RST_RADIO_CLK_PLL);
        mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), 0);

        //Wait for radio clock PLL to lock
        if (not wait_for_clk_locked(mb, fw_regmap_t::clk_status_reg_t::RADIO_CLK_LOCK, 0.01)) {
            throw uhd::runtime_error((boost::format("Reference Clock PLL in FPGA failed to lock to %s source.") % source).str());
        }

        //Reset the IDELAYCTRL used to calibrate the data interface delays
        mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), ZPU_SR_SW_RST_ADC_IDELAYCTRL);
        mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), 0);

        //Wait for the ADC IDELAYCTRL to be ready
        if (not wait_for_clk_locked(mb, fw_regmap_t::clk_status_reg_t::IDELAYCTRL_LOCK, 0.01)) {
            throw uhd::runtime_error((boost::format("ADC Calibration Clock in FPGA failed to lock to %s source.") % source).str());
        }

        // Reset ADCs and DACs
        for(rfnoc::x300_radio_ctrl_impl::sptr r:  mb.radios) {
            r->reset_codec();
        }
    }

    //Update cache value
    mb.current_refclk_src = source;
}

void x300_impl::update_time_source(mboard_members_t &mb, const std::string &source)
{
    if (source == "internal") {
        mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_SELECT, fw_regmap_t::clk_ctrl_reg_t::SRC_INTERNAL);
    } else if (source == "external") {
        mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_SELECT, fw_regmap_t::clk_ctrl_reg_t::SRC_EXTERNAL);
    } else if (source == "gpsdo") {
        mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_SELECT, fw_regmap_t::clk_ctrl_reg_t::SRC_GPSDO);
    } else {
        throw uhd::key_error("update_time_source: unknown source: " + source);
    }

    /* TODO - Implement intelligent PPS detection
    //check for valid pps
    if (!is_pps_present(mb)) {
        throw uhd::runtime_error((boost::format("The %d PPS was not detected.  Please check the PPS source and try again.") % source).str());
    }
    */
}

void x300_impl::sync_times(mboard_members_t &mb, const uhd::time_spec_t& t)
{
    std::vector<rfnoc::block_id_t> radio_ids = find_blocks<rfnoc::x300_radio_ctrl_impl>("Radio");
    for(const rfnoc::block_id_t &id:  radio_ids) {
        get_block_ctrl<rfnoc::x300_radio_ctrl_impl>(id)->set_time_sync(t);
    }

    mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::TIME_SYNC, 0);
    mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::TIME_SYNC, 1);
    mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::TIME_SYNC, 0);
}

bool x300_impl::wait_for_clk_locked(mboard_members_t& mb, uint32_t which, double timeout)
{
    const auto timeout_time =
        std::chrono::steady_clock::now()
        + std::chrono::milliseconds(int64_t(timeout * 1000));
    do {
        if (mb.fw_regmap->clock_status_reg.read(which) == 1) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < timeout_time);

    //Check one last time
    return (mb.fw_regmap->clock_status_reg.read(which)==1);
}

sensor_value_t x300_impl::get_ref_locked(mboard_members_t& mb)
{
    mb.fw_regmap->clock_status_reg.refresh();
    const bool lock = (mb.fw_regmap->clock_status_reg.get(fw_regmap_t::clk_status_reg_t::LMK_LOCK)==1) &&
                      (mb.fw_regmap->clock_status_reg.get(fw_regmap_t::clk_status_reg_t::RADIO_CLK_LOCK)==1) &&
                      (mb.fw_regmap->clock_status_reg.get(fw_regmap_t::clk_status_reg_t::IDELAYCTRL_LOCK)==1);
    return sensor_value_t("Ref", lock, "locked", "unlocked");
}

bool x300_impl::is_pps_present(mboard_members_t& mb)
{
    // The ZPU_RB_CLK_STATUS_PPS_DETECT bit toggles with each rising edge of the PPS.
    // We monitor it for up to 1.5 seconds looking for it to toggle.
    uint32_t pps_detect = mb.fw_regmap->clock_status_reg.read(fw_regmap_t::clk_status_reg_t::PPS_DETECT);
    for (int i = 0; i < 15; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (pps_detect != mb.fw_regmap->clock_status_reg.read(fw_regmap_t::clk_status_reg_t::PPS_DETECT))
            return true;
    }
    return false;
}

/***********************************************************************
 * claimer logic
 **********************************************************************/

void x300_impl::claimer_loop(wb_iface::sptr iface)
{
    claim(iface);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

x300_impl::claim_status_t x300_impl::claim_status(wb_iface::sptr iface)
{
    claim_status_t claim_status = CLAIMED_BY_OTHER; // Default to most restrictive
    boost::system_time timeout_time = boost::get_system_time() + boost::posix_time::seconds(1);
    while (boost::get_system_time() < timeout_time)
    {
        //If timed out, then device is definitely unclaimed
        if (iface->peek32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_STATUS)) == 0)
        {
            claim_status = UNCLAIMED;
            break;
        }

        //otherwise check claim src to determine if another thread with the same src has claimed the device
        uint32_t hash = iface->peek32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_SRC));
        if (hash == 0)
        {
            // A non-zero claim status and an empty hash means the claim might
            // be in the process of being released.  This is possible because
            // older firmware takes a long time to update the status.  Wait and
            // check status again.
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        claim_status = (hash == get_process_hash() ? CLAIMED_BY_US : CLAIMED_BY_OTHER);
        break;
    }
    return claim_status;
}

void x300_impl::claim(wb_iface::sptr iface)
{
    iface->poke32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_TIME), uint32_t(time(NULL)));
    iface->poke32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_SRC), get_process_hash());
}

bool x300_impl::try_to_claim(wb_iface::sptr iface, long timeout)
{
    boost::system_time start_time = boost::get_system_time();
    while (1)
    {
        claim_status_t status = claim_status(iface);
        if (status == UNCLAIMED)
        {
            claim(iface);
            // It takes the claimer 10ms to update status, so wait 20ms before verifying claim
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }
        if (status == CLAIMED_BY_US)
        {
            break;
        }
        if (boost::get_system_time() - start_time > boost::posix_time::milliseconds(timeout))
        {
            // Another process owns the device - give up
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

void x300_impl::release(wb_iface::sptr iface)
{
    iface->poke32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_TIME), 0);
    iface->poke32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_SRC), 0);
}

/***********************************************************************
 * Frame size detection
 **********************************************************************/
x300_impl::frame_size_t x300_impl::determine_max_frame_size(const std::string &addr,
        const frame_size_t &user_frame_size)
{
    udp_simple::sptr udp = udp_simple::make_connected(addr,
            BOOST_STRINGIZE(X300_MTU_DETECT_UDP_PORT));

    std::vector<uint8_t> buffer(std::max(user_frame_size.recv_frame_size, user_frame_size.send_frame_size));
    x300_mtu_t *request = reinterpret_cast<x300_mtu_t *>(&buffer.front());
    static const double echo_timeout = 0.020; //20 ms

    //test holler - check if its supported in this fw version
    request->flags = uhd::htonx<uint32_t>(X300_MTU_DETECT_ECHO_REQUEST);
    request->size = uhd::htonx<uint32_t>(sizeof(x300_mtu_t));
    udp->send(boost::asio::buffer(buffer, sizeof(x300_mtu_t)));
    udp->recv(boost::asio::buffer(buffer), echo_timeout);
    if (!(uhd::ntohx<uint32_t>(request->flags) & X300_MTU_DETECT_ECHO_REPLY))
        throw uhd::not_implemented_error("Holler protocol not implemented");

    //Reducing range of (min,max) by setting max value to 10gig max_frame_size as larger sizes are not supported
    size_t min_recv_frame_size = sizeof(x300_mtu_t);
    size_t max_recv_frame_size = std::min(user_frame_size.recv_frame_size, X300_10GE_DATA_FRAME_MAX_SIZE) & size_t(~3);
    size_t min_send_frame_size = sizeof(x300_mtu_t);
    size_t max_send_frame_size = std::min(user_frame_size.send_frame_size, X300_10GE_DATA_FRAME_MAX_SIZE) & size_t(~3);

    UHD_LOGGER_DEBUG("X300") << "Determining maximum frame size... ";
    while (min_recv_frame_size < max_recv_frame_size)
    {
       size_t test_frame_size = (max_recv_frame_size/2 + min_recv_frame_size/2 + 3) & ~3;

       request->flags = uhd::htonx<uint32_t>(X300_MTU_DETECT_ECHO_REQUEST);
       request->size = uhd::htonx<uint32_t>(test_frame_size);
       udp->send(boost::asio::buffer(buffer, sizeof(x300_mtu_t)));

       size_t len = udp->recv(boost::asio::buffer(buffer), echo_timeout);

       if (len >= test_frame_size)
           min_recv_frame_size = test_frame_size;
       else
           max_recv_frame_size = test_frame_size - 4;
    }

    if(min_recv_frame_size < IP_PROTOCOL_MIN_MTU_SIZE-IP_PROTOCOL_UDP_PLUS_IP_HEADER) {
        throw uhd::runtime_error("System receive MTU size is less than the minimum required by the IP protocol.");
    }

    while (min_send_frame_size < max_send_frame_size)
    {
        size_t test_frame_size = (max_send_frame_size/2 + min_send_frame_size/2 + 3) & ~3;

        request->flags = uhd::htonx<uint32_t>(X300_MTU_DETECT_ECHO_REQUEST);
        request->size = uhd::htonx<uint32_t>(sizeof(x300_mtu_t));
        udp->send(boost::asio::buffer(buffer, test_frame_size));

        size_t len = udp->recv(boost::asio::buffer(buffer), echo_timeout);
        if (len >= sizeof(x300_mtu_t))
            len = uhd::ntohx<uint32_t>(request->size);

        if (len >= test_frame_size)
            min_send_frame_size = test_frame_size;
        else
            max_send_frame_size = test_frame_size - 4;
    }

    if(min_send_frame_size < IP_PROTOCOL_MIN_MTU_SIZE-IP_PROTOCOL_UDP_PLUS_IP_HEADER) {
        throw uhd::runtime_error("System send MTU size is less than the minimum required by the IP protocol.");
    }

    frame_size_t frame_size;
    // There are cases when NICs accept oversized packets, in which case we'd falsely
    // detect a larger-than-possible frame size. A safe and sensible value is the minimum
    // of the recv and send frame sizes.
    frame_size.recv_frame_size = std::min(min_recv_frame_size, min_send_frame_size);
    frame_size.send_frame_size = std::min(min_recv_frame_size, min_send_frame_size);
    UHD_LOGGER_INFO("X300")
        << "Maximum frame size: " << frame_size.send_frame_size << " bytes.";
    return frame_size;
}

/***********************************************************************
 * compat checks
 **********************************************************************/

void x300_impl::check_fw_compat(const fs_path &mb_path, wb_iface::sptr iface)
{
    uint32_t compat_num = iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_COMPAT_NUM));
    uint32_t compat_major = (compat_num >> 16);
    uint32_t compat_minor = (compat_num & 0xffff);

    if (compat_major != X300_FW_COMPAT_MAJOR)
    {
        throw uhd::runtime_error(str(boost::format(
            "Expected firmware compatibility number %d.%d, but got %d.%d:\n"
            "The firmware build is not compatible with the host code build.\n"
            "%s"
        )   % int(X300_FW_COMPAT_MAJOR) % int(X300_FW_COMPAT_MINOR)
            % compat_major % compat_minor % print_utility_error("uhd_images_downloader.py")));
    }
    _tree->create<std::string>(mb_path / "fw_version").set(str(boost::format("%u.%u")
                % compat_major % compat_minor));
}

void x300_impl::check_fpga_compat(const fs_path &mb_path, const mboard_members_t &members)
{
    uint32_t compat_num = members.zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_COMPAT_NUM));
    uint32_t compat_major = (compat_num >> 16);
    uint32_t compat_minor = (compat_num & 0xffff);

    if (compat_major != X300_FPGA_COMPAT_MAJOR)
    {
        std::string image_loader_path = (fs::path(uhd::get_pkg_path()) / "bin" / "uhd_image_loader").string();
        std::string image_loader_cmd = str(boost::format("\"%s\" --args=\"type=x300,%s=%s\"")
                                              % image_loader_path
                                              % (members.xport_path == "eth" ? "addr"
                                                                             : "resource")
                                              % members.get_pri_eth().addr);

        std::cout << "=========================================================" << std::endl;
        std::cout << "Warning:" << std::endl;
        //throw uhd::runtime_error(str(boost::format(
        std::cout << (str(boost::format(
            "Expected FPGA compatibility number %d, but got %d:\n"
            "The FPGA image on your device is not compatible with this host code build.\n"
            "Download the appropriate FPGA images for this version of UHD.\n"
            "%s\n\n"
            "Then burn a new image to the on-board flash storage of your\n"
            "USRP X3xx device using the image loader utility. Use this command:\n\n%s\n\n"
            "For more information, refer to the UHD manual:\n\n"
            " http://files.ettus.com/manual/page_usrp_x3x0.html#x3x0_flash"
        )   % int(X300_FPGA_COMPAT_MAJOR) % compat_major
            % print_utility_error("uhd_images_downloader.py")
            % image_loader_cmd));
        std::cout << "=========================================================" << std::endl;
    }
    _tree->create<std::string>(mb_path / "fpga_version").set(str(boost::format("%u.%u")
                % compat_major % compat_minor));

    const uint32_t git_hash = members.zpu_ctrl->peek32(SR_ADDR(SET0_BASE,
                                                              ZPU_RB_GIT_HASH));
    _tree->create<std::string>(mb_path / "fpga_version_hash").set(
        str(boost::format("%07x%s")
        % (git_hash & 0x0FFFFFFF)
        % ((git_hash & 0xF0000000) ? "-dirty" : "")));
}

x300_impl::x300_mboard_t x300_impl::get_mb_type_from_pcie(const std::string& resource, const std::string& rpc_port)
{
    x300_mboard_t mb_type = UNKNOWN;

    //Detect the PCIe product ID to distinguish between X300 and X310
    nirio_status status = NiRio_Status_Success;
    uint32_t pid;
    niriok_proxy::sptr discovery_proxy =
        niusrprio_session::create_kernel_proxy(resource, rpc_port);
    if (discovery_proxy) {
        nirio_status_chain(discovery_proxy->get_attribute(RIO_PRODUCT_NUMBER, pid), status);
        discovery_proxy->close();
        if (nirio_status_not_fatal(status)) {
            //The PCIe ID -> MB mapping may be different from the EEPROM -> MB mapping
            switch (pid) {
                case X300_USRP_PCIE_SSID_ADC_33:
                case X300_USRP_PCIE_SSID_ADC_18:
                    mb_type = USRP_X300_MB; break;
                case X310_USRP_PCIE_SSID_ADC_33:
                case X310_2940R_40MHz_PCIE_SSID_ADC_33:
                case X310_2940R_120MHz_PCIE_SSID_ADC_33:
                case X310_2942R_40MHz_PCIE_SSID_ADC_33:
                case X310_2942R_120MHz_PCIE_SSID_ADC_33:
                case X310_2943R_40MHz_PCIE_SSID_ADC_33:
                case X310_2943R_120MHz_PCIE_SSID_ADC_33:
                case X310_2944R_40MHz_PCIE_SSID_ADC_33:
                case X310_2950R_40MHz_PCIE_SSID_ADC_33:
                case X310_2950R_120MHz_PCIE_SSID_ADC_33:
                case X310_2952R_40MHz_PCIE_SSID_ADC_33:
                case X310_2952R_120MHz_PCIE_SSID_ADC_33:
                case X310_2953R_40MHz_PCIE_SSID_ADC_33:
                case X310_2953R_120MHz_PCIE_SSID_ADC_33:
                case X310_2954R_40MHz_PCIE_SSID_ADC_33:
                case X310_USRP_PCIE_SSID_ADC_18:
                case X310_2940R_40MHz_PCIE_SSID_ADC_18:
                case X310_2940R_120MHz_PCIE_SSID_ADC_18:
                case X310_2942R_40MHz_PCIE_SSID_ADC_18:
                case X310_2942R_120MHz_PCIE_SSID_ADC_18:
                case X310_2943R_40MHz_PCIE_SSID_ADC_18:
                case X310_2943R_120MHz_PCIE_SSID_ADC_18:
                case X310_2944R_40MHz_PCIE_SSID_ADC_18:
                case X310_2945R_PCIE_SSID_ADC_18:
                case X310_2950R_40MHz_PCIE_SSID_ADC_18:
                case X310_2950R_120MHz_PCIE_SSID_ADC_18:
                case X310_2952R_40MHz_PCIE_SSID_ADC_18:
                case X310_2952R_120MHz_PCIE_SSID_ADC_18:
                case X310_2953R_40MHz_PCIE_SSID_ADC_18:
                case X310_2953R_120MHz_PCIE_SSID_ADC_18:
                case X310_2954R_40MHz_PCIE_SSID_ADC_18:
                case X310_2955R_PCIE_SSID_ADC_18:
                    mb_type = USRP_X310_MB; break;
                default:
                    mb_type = UNKNOWN;      break;
            }
        }
    }

    return mb_type;
}

x300_impl::x300_mboard_t x300_impl::get_mb_type_from_eeprom(const uhd::usrp::mboard_eeprom_t& mb_eeprom)
{
    x300_mboard_t mb_type = UNKNOWN;
    if (not mb_eeprom["product"].empty())
    {
        uint16_t product_num = 0;
        try {
            product_num = boost::lexical_cast<uint16_t>(mb_eeprom["product"]);
        } catch (const boost::bad_lexical_cast &) {
            product_num = 0;
        }

        switch (product_num) {
            //The PCIe ID -> MB mapping may be different from the EEPROM -> MB mapping
            case X300_USRP_PCIE_SSID_ADC_33:
            case X300_USRP_PCIE_SSID_ADC_18:
                mb_type = USRP_X300_MB; break;
            case X310_USRP_PCIE_SSID_ADC_33:
            case X310_2940R_40MHz_PCIE_SSID_ADC_33:
            case X310_2940R_120MHz_PCIE_SSID_ADC_33:
            case X310_2942R_40MHz_PCIE_SSID_ADC_33:
            case X310_2942R_120MHz_PCIE_SSID_ADC_33:
            case X310_2943R_40MHz_PCIE_SSID_ADC_33:
            case X310_2943R_120MHz_PCIE_SSID_ADC_33:
            case X310_2944R_40MHz_PCIE_SSID_ADC_33:
            case X310_2950R_40MHz_PCIE_SSID_ADC_33:
            case X310_2950R_120MHz_PCIE_SSID_ADC_33:
            case X310_2952R_40MHz_PCIE_SSID_ADC_33:
            case X310_2952R_120MHz_PCIE_SSID_ADC_33:
            case X310_2953R_40MHz_PCIE_SSID_ADC_33:
            case X310_2953R_120MHz_PCIE_SSID_ADC_33:
            case X310_2954R_40MHz_PCIE_SSID_ADC_33:
            case X310_USRP_PCIE_SSID_ADC_18:
            case X310_2940R_40MHz_PCIE_SSID_ADC_18:
            case X310_2940R_120MHz_PCIE_SSID_ADC_18:
            case X310_2942R_40MHz_PCIE_SSID_ADC_18:
            case X310_2942R_120MHz_PCIE_SSID_ADC_18:
            case X310_2943R_40MHz_PCIE_SSID_ADC_18:
            case X310_2943R_120MHz_PCIE_SSID_ADC_18:
            case X310_2944R_40MHz_PCIE_SSID_ADC_18:
            case X310_2945R_PCIE_SSID_ADC_18:
            case X310_2950R_40MHz_PCIE_SSID_ADC_18:
            case X310_2950R_120MHz_PCIE_SSID_ADC_18:
            case X310_2952R_40MHz_PCIE_SSID_ADC_18:
            case X310_2952R_120MHz_PCIE_SSID_ADC_18:
            case X310_2953R_40MHz_PCIE_SSID_ADC_18:
            case X310_2953R_120MHz_PCIE_SSID_ADC_18:
            case X310_2954R_40MHz_PCIE_SSID_ADC_18:
            case X310_2955R_PCIE_SSID_ADC_18:
                mb_type = USRP_X310_MB; break;
            default:
                UHD_LOGGER_WARNING("X300") << "X300 unknown product code in EEPROM: " << product_num ;
                mb_type = UNKNOWN;      break;
        }
    }
    return mb_type;
}
