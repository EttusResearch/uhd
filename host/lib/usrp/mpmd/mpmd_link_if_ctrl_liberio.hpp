//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_XPORT_CTRL_LIBERIO_HPP
#define INCLUDED_MPMD_XPORT_CTRL_LIBERIO_HPP

#include "mpmd_link_if_ctrl_base.hpp"
#include <uhd/types/device_addr.hpp>

namespace uhd { namespace mpmd { namespace xport {

/*! Liberio transport manager
 */
class mpmd_link_if_ctrl_liberio : public mpmd_link_if_ctrl_base
{
public:
    /* For liberio, get_chdr_link_options returns information about DMA engines.
     * We assume there is only ever one liberio link available
     * first = tx path
     * second = rx path
     */
    using liberio_link_info_t = std::pair<std::string, std::string>;

    mpmd_link_if_ctrl_liberio(const uhd::device_addr_t& mb_args,
        const mpmd_link_if_mgr::xport_info_list_t& xport_info);

    size_t get_num_links() const
    {
        return 1;
    }

    uhd::transport::both_links_t get_link(const size_t link_idx,
        const uhd::transport::link_type_t link_type,
        const uhd::device_addr_t& link_args);

    size_t get_mtu(const uhd::direction_t) const;

    double get_link_rate(const size_t /*link_idx*/) const
    {
        return _link_rate;
    }

    const uhd::rfnoc::chdr::chdr_packet_factory& get_packet_factory() const
    {
        return _pkt_factory;
    }

private:
    const uhd::device_addr_t _mb_args;
    const uhd::dict<std::string, std::string> _recv_args;
    const uhd::dict<std::string, std::string> _send_args;
    //! A list of DMA channels we can use for links
    std::vector<liberio_link_info_t> _dma_channels;
    double _link_rate;

    /*! An index representing the next DMA channel to use, for a simple
     * allocation of channels. For get_link(), increment for each new link, and
     * throw an exception if _next_channel > number of DMA channels.
     */
    size_t _next_channel = 0;
    static const uhd::rfnoc::chdr::chdr_packet_factory _pkt_factory;
};

}}} /* namespace uhd::mpmd::xport */

#endif /* INCLUDED_MPMD_XPORT_CTRL_LIBERIO_HPP */
