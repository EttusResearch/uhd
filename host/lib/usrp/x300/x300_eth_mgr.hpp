//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_ETH_MGR_HPP
#define INCLUDED_X300_ETH_MGR_HPP

#include "x300_conn_mgr.hpp"
#include "x300_device_args.hpp"
#include "x300_mboard_type.hpp"
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_constants.hpp>
#include <uhd/transport/udp_simple.hpp> //mtu
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/rfnoc/xports.hpp>
#include <functional>
#include <vector>

namespace uhd { namespace usrp { namespace x300 {

/*! Helper class to manage the eth connections
 */
class eth_manager : public conn_manager
{
public:
    eth_manager(const x300_device_args_t& args,
        uhd::property_tree::sptr tree,
        const uhd::fs_path& root_path);

    //! Return the motherboard type using eth
    static x300_mboard_t get_mb_type_from_eth(
        const std::string& resource, const std::string& rpc_port);

    static uhd::device_addrs_t find(const uhd::device_addr_t& hint);

    /*! Return a reference to a ZPU ctrl interface object
     */
    uhd::wb_iface::sptr get_ctrl_iface();

    void init_link(
        const mboard_eeprom_t& mb_eeprom, const std::string& loaded_fpga_image);

    size_t get_mtu(uhd::direction_t dir);

    /*! Safely release a ZPU control object
     *
     * This embeds the release call (provided by \p release_fn) within a safe
     * context to avoid multiple accesses to the eth bus.
     */
    void release_ctrl_iface(std::function<void(void)>&& release_fn);

    both_xports_t make_transport(both_xports_t xports,
        const uhd::usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& args,
        const size_t send_mtu,
        const size_t recv_mtu,
        std::function<uhd::sid_t(uint32_t, uint32_t)>&& allocate_sid);

private:
    //! Function to create a udp_simple::sptr (kernel-based or DPDK-based)
    using udp_simple_factory_t = std::function<uhd::transport::udp_simple::sptr(
        const std::string&, const std::string&)>;

    // Ethernet ports
    enum x300_eth_iface_t {
        X300_IFACE_NONE = 0,
        X300_IFACE_ETH0 = 1,
        X300_IFACE_ETH1 = 2,
    };

    struct x300_eth_conn_t
    {
        std::string addr;
        x300_eth_iface_t type;
        size_t link_rate;
    };

    struct frame_size_t
    {
        size_t recv_frame_size;
        size_t send_frame_size;
    };

    // Get the primary ethernet connection
    inline const x300_eth_conn_t& get_pri_eth() const
    {
        return eth_conns[0];
    }

    static udp_simple_factory_t x300_get_udp_factory(const device_addr_t& args);

    /*!
     * Automatically determine the maximum frame size available by sending a UDP packet
     * to the device and see which packet sizes actually work. This way, we can take
     * switches etc. into account which might live between the device and the host.
     */
    frame_size_t determine_max_frame_size(
        const std::string& addr, const frame_size_t& user_mtu);

    // Discover the ethernet connections per motherboard
    void discover_eth(
        const uhd::usrp::mboard_eeprom_t mb_eeprom, const std::string& loaded_fpga_image);


    const x300_device_args_t _args;

    uhd::property_tree::sptr _tree;

    udp_simple_factory_t _x300_make_udp_connected;

    std::vector<x300_eth_conn_t> eth_conns;
    size_t _next_src_addr    = 0;
    size_t _next_tx_src_addr = 0;
    size_t _next_rx_src_addr = 0;

    frame_size_t _max_frame_sizes;

    uhd::device_addr_t recv_args;
    uhd::device_addr_t send_args;
};

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_X300_ETH_MGR_HPP */
