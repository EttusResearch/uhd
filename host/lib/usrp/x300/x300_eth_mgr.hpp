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
#include <uhd/property_tree.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_constants.hpp>
#include <uhd/transport/udp_simple.hpp> //mtu
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <functional>
#include <map>
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
    uhd::wb_iface::sptr get_ctrl_iface() override;

    void init_link(
        const mboard_eeprom_t& mb_eeprom, const std::string& loaded_fpga_image);

    size_t get_mtu(uhd::direction_t dir) override;

    /*! Safely release a ZPU control object
     *
     * This embeds the release call (provided by \p release_fn) within a safe
     * context to avoid multiple accesses to the eth bus.
     */
    void release_ctrl_iface(std::function<void(void)>&& release_fn);

    /*! Return the list of local device IDs associated with this link
     *
     * Note: this will only be valid after init_link() is called.
     */
    std::vector<uhd::rfnoc::device_id_t> get_local_device_ids() override
    {
        return _local_device_ids;
    }

    uhd::transport::both_links_t get_links(uhd::transport::link_type_t link_type,
        const uhd::rfnoc::device_id_t local_device_id,
        const uhd::rfnoc::sep_id_t& local_epid,
        const uhd::rfnoc::sep_id_t& remote_epid,
        const uhd::device_addr_t& link_args) override;

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
        return eth_conns.at(_local_device_ids.at(0));
    }

    //! Create a factory function for UDP traffic
    //
    // \note This is static rather than local to x300_eth_mgr.cpp to get access
    //       to udp_simple_factory_t
    // \param use_dpdk If true, use a DPDK transport instead of a regular UDP
    //                 transport
    static udp_simple_factory_t x300_get_udp_factory(const bool use_dpdk);

    /*!
     * Automatically determine the maximum frame size available by sending a UDP packet
     * to the device and see which packet sizes actually work. This way, we can take
     * switches etc. into account which might live between the device and the host.
     */
    frame_size_t determine_max_frame_size(
        const std::string& addr, const frame_size_t& user_mtu);

    //! Discover the ethernet connections per motherboard
    //
    // - Gets called during init_link()
    // - Populates eth_conn
    // - Populates _local_device_ids
    //
    // \throws uhd::runtime_error if no Ethernet connections can be found
    void discover_eth(
        const uhd::usrp::mboard_eeprom_t mb_eeprom, const std::string& loaded_fpga_image);

    /**************************************************************************
     * Attributes
     *************************************************************************/
    // Cache the initial device args that brought up this motherboard
    const x300_device_args_t _args;

    udp_simple_factory_t _x300_make_udp_connected;

    std::map<uhd::rfnoc::device_id_t, x300_eth_conn_t> eth_conns;

    frame_size_t _max_frame_sizes;

    uhd::device_addr_t recv_args;
    uhd::device_addr_t send_args;

    std::vector<uhd::rfnoc::device_id_t> _local_device_ids;
};

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_X300_ETH_MGR_HPP */
