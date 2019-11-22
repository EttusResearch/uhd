//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_PCI_MGR_HPP
#define INCLUDED_X300_PCI_MGR_HPP

#include "../device3/device3_impl.hpp"
#include "x300_conn_mgr.hpp"
#include "x300_device_args.hpp"
#include "x300_mboard_type.hpp"
#include <uhd/transport/muxed_zero_copy_if.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhdlib/rfnoc/xports.hpp>

namespace uhd { namespace usrp { namespace x300 {

/*! Helper class to manage the PCIe connections
 */
class pcie_manager : public conn_manager
{
public:
    pcie_manager(const x300_device_args_t& args,
        uhd::property_tree::sptr tree,
        const uhd::fs_path& root_path);

    //! Return the motherboard type using PCIe
    static x300_mboard_t get_mb_type_from_pcie(
        const std::string& resource, const std::string& rpc_port);

    static uhd::device_addrs_t find(const device_addr_t& hint, bool explicit_query);

    /*! Return a reference to a ZPU ctrl interface object
     */
    uhd::wb_iface::sptr get_ctrl_iface();

    void init_link();

    size_t get_mtu(uhd::direction_t dir);

    /*! Safely release a ZPU control object
     *
     * This embeds the release call (provided by \p release_fn) within a safe
     * context to avoid multiple accesses to the PCIe bus.
     */
    void release_ctrl_iface(std::function<void(void)>&& release_fn);

    both_xports_t make_transport(both_xports_t xports,
        const uhd::usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& args,
        const size_t send_mtu,
        const size_t recv_mtu);

private:
    /*! Allocate or return a previously allocated PCIe channel pair
     *
     * Note the SID is always the transmit SID (i.e. from host to device).
     */
    uint32_t allocate_pcie_dma_chan(
        const uhd::sid_t& tx_sid, const uhd::usrp::device3_impl::xport_type_t xport_type);

    uhd::transport::muxed_zero_copy_if::sptr make_muxed_pcie_msg_xport(
        uint32_t dma_channel_num, size_t max_muxed_ports, const double recv_timeout_s);

    const x300_device_args_t _args;
    const std::string _resource;

    uhd::niusrprio::niusrprio_session::sptr _rio_fpga_interface;

    //! Maps SID -> DMA channel
    std::map<uint32_t, uint32_t> _dma_chan_pool;

    //! Control transport for one PCIe connection
    uhd::transport::muxed_zero_copy_if::sptr _ctrl_dma_xport;
    //! Async message transport
    uhd::transport::muxed_zero_copy_if::sptr _async_msg_dma_xport;
};

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_X300_PCI_MGR_HPP */
