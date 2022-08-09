//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_PCI_MGR_HPP
#define INCLUDED_X300_PCI_MGR_HPP

#include "x300_conn_mgr.hpp"
#include "x300_device_args.hpp"
#include "x300_mboard_type.hpp"
#include <uhd/property_tree.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/types/direction.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/transport/links.hpp>
#include <mutex>

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
    uhd::wb_iface::sptr get_ctrl_iface() override;

    void init_link();

    size_t get_mtu(uhd::direction_t dir) override;

    /*! Safely release a ZPU control object
     *
     * This embeds the release call (provided by \p release_fn) within a safe
     * context to avoid multiple accesses to the PCIe bus.
     */
    void release_ctrl_iface(std::function<void(void)>&& release_fn);

    /*! Return list of local device IDs associated with this link
     */
    std::vector<uhd::rfnoc::device_id_t> get_local_device_ids() override
    {
        return {_local_device_id};
    }

    uhd::transport::both_links_t get_links(uhd::transport::link_type_t link_type,
        const uhd::rfnoc::device_id_t local_device_id,
        const uhd::rfnoc::sep_id_t& local_epid,
        const uhd::rfnoc::sep_id_t& remote_epid,
        const uhd::device_addr_t& link_args) override;

private:
    /*! Allocate or return a previously allocated PCIe channel pair
     *
     * Note the SID is always the transmit SID (i.e. from host to device).
     */
    uint32_t allocate_pcie_dma_chan(const uhd::rfnoc::sep_id_t& remote_epid,
        const uhd::transport::link_type_t link_type);

    /*** Attributes **********************************************************/
    const x300_device_args_t _args;
    const std::string _resource;
    uhd::niusrprio::niusrprio_session::sptr _rio_fpga_interface;
    uhd::rfnoc::device_id_t _local_device_id;

    //! Maps Remote DMA channel -> EPID
    std::unordered_map<uint32_t, uhd::rfnoc::sep_id_t> _dma_chan_pool;

    //! Locks access to the map
    std::mutex _dma_chan_mutex;
};

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_X300_PCI_MGR_HPP */
