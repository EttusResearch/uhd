//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/register_iface_holder.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_endpoint.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace detail {

/*!
 * Class that uses a register_iface to read important configuration information from the
 * RFNoC backend registers
 */
class client_zero : public uhd::rfnoc::register_iface_holder
{
public:
    using sptr = std::shared_ptr<client_zero>;

    static sptr make(chdr_ctrl_endpoint& chdr_ctrl_ep, sep_id_t dst_epid);

    client_zero(register_iface::sptr reg);

    //! Definition of an edge in the static router
    struct edge_def_t
    {
        uint16_t src_blk_index;
        uint8_t src_blk_port;
        uint16_t dst_blk_index;
        uint8_t dst_blk_port;
    };

    //! Contents of the backend status block configuration register
    struct block_config_info
    {
        uint8_t protover;
        uint8_t num_inputs;
        uint8_t num_outputs;
        uint8_t ctrl_fifo_size;
        uint8_t ctrl_max_async_msgs;
        uint8_t data_mtu;
        uint8_t ctrl_clk_idx;
        uint8_t tb_clk_idx;
    };

    //! Return the RFNoC protocol version for this motherboard
    uint16_t get_proto_ver()
    {
        return _proto_ver;
    };

    //! Return the device type
    uint16_t get_device_type()
    {
        return _device_type;
    };

    //! Return the number of blocks in our RFNoC graph
    size_t get_num_blocks()
    {
        return _num_blocks;
    };

    //! Return the number of stream endpoints in our RFNoC graph
    size_t get_num_stream_endpoints()
    {
        return _num_stream_endpoints;
    };

    //! Return the number of stream endpoints connected to the control crossbar
    size_t get_num_ctrl_endpoints() const
    {
        return _num_ctrl_endpoints;
    };

    //! Return the number of transports available
    size_t get_num_transports()
    {
        return _num_transports;
    };

    //! Return the control crossbar port of the block \p block_idx
    size_t get_ctrl_xbar_port(const size_t block_idx) const
    {
        return 1 + _num_ctrl_endpoints + block_idx;
    }

    //! Return whether or not the device includes a CHDR crossbar
    bool has_chdr_crossbar()
    {
        return _has_chdr_crossbar;
    };

    //! Return the number of edges in our graph (the number of static connections)
    size_t get_num_edges()
    {
        return _num_edges;
    };

    //! Return a vector containing the edge definitions
    std::vector<edge_def_t>& get_adjacency_list()
    {
        return _adjacency_list;
    };

    /*! Return the NOC ID of the block located at `portno`
     *
     * \throws uhd::index_error if no NOC block is connected to the port
     */
    uint32_t get_noc_id(uint16_t portno);

    /*! Return whether the port is actively flushing
     *
     * \throws uhd::index_error if no NOC block is connected to the port
     * \return boolean status
     */
    bool get_flush_active(uint16_t portno);

    /*! Return whether the port is done flushing
     *
     * \throws uhd::index_error if no NOC block is connected to the port
     * \return boolean status
     */
    bool get_flush_done(uint16_t portno);

    /*! Returns once the port is done flushing
     *
     * Note: this function queries the port once every millisecond
     *
     * \param portno Port number
     * \param timeout time, in milliseconds, to poll before quitting
     * \throws uhd::index_error if no NOC block is connected to the port
     * \return boolean whether or not the flush had completed in the timeout period
     */
    bool poll_flush_done(uint16_t portno, std::chrono::milliseconds timeout);

    /*! Set the port's hardware flush timeout
     *
     * \param timeout number of cycles the device waits for the flushing to complete
     * \param portno Port number
     * \throws uhd::index_error if no NOC block is connected to the port
     */
    void set_flush_timeout(uint32_t timeout, uint16_t portno);

    /*! Send a request to flush a port
     *
     * \param portno Port number
     * \throws uhd::index_error if no NOC block is connected to the port
     */
    void set_flush(uint16_t portno);

    /*! Go through the entire flush process for a port
     *
     * \param portno Port number
     * \throws uhd::index_error if no NOC block is connected to the port
     * \return whether or not the flush succeeded
     */
    bool complete_flush(uint16_t portno);

    /*! Go through the entire flush process for all ports
     * \return whether or not the flush succeeded
     */
    bool complete_flush_all_blocks();

    /*! Reset a port's control logic
     *
     * It is recommended to flush a port calling this.
     *
     * \param portno Port number
     * \throws uhd::index_error if no NOC block is connected to the port
     */
    void reset_ctrl(uint16_t portno);

    /*! Reset a port's CHDR logic
     *
     * It is recommended to flush a port calling this.
     *
     * \param portno Port number
     * \throws uhd::index_error if no NOC block is connected to the port
     */
    void reset_chdr(uint16_t portno);

    /*! Get the port's configuration information
     *
     * \return Struct containing configuration information
     */
    block_config_info get_block_info(uint16_t portno);

    // TODO: handle callbacks?

private:
    uint16_t _proto_ver;
    uint16_t _device_type;
    uint16_t _num_blocks;
    uint16_t _num_stream_endpoints;
    uint16_t _num_ctrl_endpoints;
    uint16_t _num_transports;
    bool _has_chdr_crossbar;
    uint16_t _num_edges;
    std::vector<edge_def_t> _adjacency_list;

    std::vector<client_zero::edge_def_t> _get_adjacency_list();

    /* Helper function to determine if the given port number has a block connected
     *
     * \throws uhd::index_error if no NOC block is connected to the port
     */
    void _check_port_number(uint16_t portno);
    //! Translate port number to base address for the register
    uint32_t _get_port_base_addr(uint16_t portno);
    //! Helper function to get the backend control flush status flags
    uint32_t _get_flush_status_flags(uint16_t portno);
};

}}} /* namespace uhd::rfnoc::detail */
