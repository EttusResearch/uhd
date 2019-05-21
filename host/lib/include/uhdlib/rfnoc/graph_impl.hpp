//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_GRAPH_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_GRAPH_IMPL_HPP

#include "async_msg_handler.hpp"
#include <uhd/device3.hpp>
#include <uhd/rfnoc/graph.hpp>

namespace uhd { namespace rfnoc {

class graph_impl : public graph
{
public:
    /************************************************************************
     * Structors
     ***********************************************************************/
    /*!
     * \param name An optional name to describe this graph
     * \param device_ptr Weak pointer to the originating device3
     * \param msg_handler Pointer to the async message handler
     */
    graph_impl(const std::string& name,
        boost::weak_ptr<uhd::device3> device_ptr,
        async_msg_handler::sptr msg_handler);
    virtual ~graph_impl() {}

    /************************************************************************
     * Connection API
     ***********************************************************************/
    void connect(const block_id_t& src_block,
        size_t src_block_port,
        const block_id_t& dst_block,
        size_t dst_block_port,
        const size_t pkt_size = 0);

    void connect(const block_id_t& src_block, const block_id_t& dst_block);

    void connect_src(const block_id_t& src_block,
        const size_t src_block_port,
        const uhd::sid_t dst_sid,
        const size_t buf_size_dst_bytes,
        const size_t pkt_size_);

    void connect_sink(const block_id_t& sink_block,
        const size_t dst_block_port,
        const size_t bytes_per_ack);

    /************************************************************************
     * Utilities
     ***********************************************************************/
    std::string get_name() const
    {
        return _name;
    }


private:
    void handle_overruns(const async_msg_t& async_msg);

    //! Maps 16-bit addresses to block IDs
    std::map<uint32_t, block_id_t> _block_id_map;

    //! For any given block, look up the MIMO group
    std::map<uint32_t, size_t> _mimo_group_map;

    //! For any MIMO group, store the list of blocks in that group
    std::map<size_t, std::set<block_id_t>> _mimo_groups;

    //! Optional: A string to describe this graph
    const std::string _name;

    //! Reference to the generating device object
    const boost::weak_ptr<uhd::device3> _device_ptr;

    //! Reference to the async message handler
    async_msg_handler::sptr _msg_handler;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_GRAPH_IMPL_HPP */
