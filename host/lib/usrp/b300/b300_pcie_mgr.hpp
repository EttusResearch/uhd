//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/property_tree.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/transport/links.hpp>
#include <uhdlib/usrp/b300/b300_pcie_session.hpp>
#include <mutex>

namespace uhd { namespace usrp { namespace b300 {

/*! Helper class to manage the PCIe connections
 */
class b300_pcie_manager : public uhd::wb_iface
{
public:
    using sptr = std::shared_ptr<b300_pcie_manager>;
    b300_pcie_manager(const std::string& resource,
        uhd::property_tree::sptr tree,
        const uhd::fs_path& root_path);

    size_t get_mtu(uhd::direction_t dir);

    /*! Return list of local device IDs associated with this link
     */
    std::vector<uhd::rfnoc::device_id_t> get_local_device_ids()
    {
        return {_local_device_id};
    }

    uhd::transport::both_links_t get_links(uhd::transport::link_type_t link_type,
        const uhd::rfnoc::device_id_t local_device_id,
        const uhd::rfnoc::sep_id_t& local_epid,
        const uhd::rfnoc::sep_id_t& remote_epid,
        const uhd::device_addr_t& link_args,
        uhd::rfnoc::chdr_w_t chdr_w);

    void poke32(const uint32_t addr, const uint32_t value)
    {
        _session->poke32(addr, value);
    }

    uint32_t peek32(const uint32_t addr)
    {
        return _session->peek32(addr);
    }

    void poke64(const uint32_t addr, const uint64_t value)
    {
        _session->poke64(addr, value);
    }

    uint64_t peek64(const uint32_t addr)
    {
        return _session->peek64(addr);
    }

    uint32_t get_session_count()
    {
        return _session->get_session_count();
    }

private:
    /*! Allocate or return a previously allocated PCIe channel pair
     *
     * Note the SID is always the transmit SID (i.e. from host to device).
     */
    uint32_t allocate_pcie_dma_chan(const uhd::rfnoc::sep_id_t& remote_epid,
        const uhd::transport::link_type_t link_type);

    void test_pcie_registers();

    /*** Attributes **********************************************************/
    const std::string _resource;
    uhd::rfnoc::device_id_t _local_device_id;

    //! PCIe session
    std::shared_ptr<b300_pcie_session> _session;

    //! Property tree and path
    uhd::property_tree::sptr _tree;
    uhd::fs_path _root_path;

    //! Maps Remote DMA channel -> EPID
    std::unordered_map<uint32_t, uhd::rfnoc::sep_id_t> _dma_chan_pool;

    //! Locks access to the map
    std::mutex _dma_chan_mutex;
};

}}} // namespace uhd::usrp::b300
