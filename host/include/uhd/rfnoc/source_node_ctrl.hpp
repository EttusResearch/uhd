//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_SOURCE_NODE_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_SOURCE_NODE_CTRL_BASE_HPP

#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <boost/thread.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Abstract class for source nodes.
 *
 * Source nodes can have downstream blocks.
 */
class UHD_RFNOC_API source_node_ctrl;
class source_node_ctrl : virtual public node_ctrl_base
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<source_node_ctrl> sptr;
    typedef std::map< size_t, boost::weak_ptr<source_node_ctrl> > node_map_t;
    typedef std::pair< size_t, boost::weak_ptr<source_node_ctrl> > node_map_pair_t;

    /***********************************************************************
     * Source block controls
     **********************************************************************/
    /*! Issue a stream command for this block.
     * \param stream_cmd The stream command.
     * \param chan Channel Index
     */
    virtual void issue_stream_cmd(
        const uhd::stream_cmd_t &stream_cmd,
        const size_t chan=0
    ) = 0;

    /*! Connect another node downstream of this node.
     *
     * *Note:* If additional settings are required to make this connection work,
     * e.g. configure flow control, these need to be done separately.
     *
     * If the requested connection is not possible, this function will throw.
     *
     * \p downstream_node Pointer to the node class to connect
     * \p port Suggested port number on this block to connect the downstream
     *         block to.
     * \p args Any arguments that can be useful for determining the port number.
     *
     * \returns The actual port number used.
     */
    size_t connect_downstream(
            node_ctrl_base::sptr downstream_node,
            size_t port=ANY_PORT,
            const uhd::device_addr_t &args=uhd::device_addr_t()
    );

    /*! Call this function to notify a node about its streamer activity.
     *
     * When \p active is set to true, this means this block is now part of
     * an active rx streamer chain. Conversely, when set to false, this means
     * the node has been removed from an rx streamer chain.
     */
    virtual void set_rx_streamer(bool active, const size_t port);

protected:

    /*! Ask for a port number to connect a downstream block to.
     *
     * See sink_node_ctrl::_request_input_port(). This is the same
     * for output.
     *
     * \param suggested_port Try and connect here.
     * \param args When deciding on a port number, these arguments may be used.
     *
     * \returns A valid input port, or ANY_PORT on failure.
     */
    virtual size_t _request_output_port(
            const size_t suggested_port,
            const uhd::device_addr_t &args
    ) const;


private:
    /*! Makes connecting something to the output thread-safe.
     */
    boost::mutex _output_mutex;

    /*! Register a node downstream of this one (i.e., a node that receives data from this node).
     *
     * By definition, the upstream node must of type sink_node_ctrl.
     *
     * This saves a *weak pointer* to the downstream node and checks
     * the port is available. Will throw otherwise.
     *
     * \param downstream_node A pointer to the node instantiation
     * \param port Port number the downstream node is connected to
     */
    void _register_downstream_node(
            node_ctrl_base::sptr downstream_node,
            size_t port
    );

}; /* class source_node_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_SOURCE_NODE_CTRL_BASE_HPP */
// vim: sw=4 et:
