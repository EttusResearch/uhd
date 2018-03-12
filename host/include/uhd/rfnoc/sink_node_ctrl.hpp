//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_SINK_NODE_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_SINK_NODE_CTRL_BASE_HPP

#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhd/rfnoc/sink_node_ctrl.hpp>
#include <boost/thread.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Abstract class for sink nodes.
 *
 * Sink nodes can have upstream blocks.
 */
class UHD_RFNOC_API sink_node_ctrl;
class sink_node_ctrl : virtual public node_ctrl_base
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<sink_node_ctrl> sptr;
    typedef std::map< size_t, boost::weak_ptr<sink_node_ctrl> > node_map_t;
    typedef std::pair< size_t, boost::weak_ptr<sink_node_ctrl> > node_map_pair_t;

    /***********************************************************************
     * Sink block controls
     **********************************************************************/
    /*! Connect another node upstream of this node.
     *
     * *Note:* If additional settings are required to make this connection work,
     * e.g. configure flow control, these need to be done separately.
     *
     * If the requested connection is not possible, this function will throw.
     *
     * \p upstream_node Pointer to the node class to connect
     * \p port Suggested port number on this block to connect the upstream
     *         block to.
     * \p args Any arguments that can be useful for determining the port number.
     *
     * \returns The actual port number used.
     */
    size_t connect_upstream(
            node_ctrl_base::sptr upstream_node,
            size_t port=ANY_PORT,
            const uhd::device_addr_t &args=uhd::device_addr_t()
    );

    /*! Call this function to notify a node about its streamer activity.
     *
     * When \p active is set to true, this means this block is now part of
     * an active tx streamer chain. Conversely, when set to false, this means
     * the node has been removed from an tx streamer chain.
     */
    virtual void set_tx_streamer(bool active, const size_t port);


protected:

    /*! Ask for a port number to connect an upstream block to.
     *
     * Typically, this will be overridden for custom behaviour.
     * The default is to return the suggested port, disregarding
     * \p args, unless \p port == ANY_PORT, in which case the first
     * unused input port is returned.
     *
     * When deriving this function for custom behaviour, consider:
     * - The result is used to call register_upstream_node(), which
     *   has its own checks in place.
     * - This function may throw if the arguments can't be resolved.
     *   The exception will propagate to the user space.
     * - Alternatively, the function may return ANY_PORT to signify
     *   failure.
     * - \p args and \p suggested_port should be treated as strong
     *   suggestions, but there's no reason to just return any valid
     *   port.
     *
     * *Note:* For reasons of thread safety, it is recommended to
     * never, ever call this function directly. It will be used by
     * connect_upstream() which will handle the connection process
     * in a thread-safe manner.
     *
     * \param suggested_port Try and connect here.
     * \param args When deciding on a port number, these arguments may be used.
     *
     * \returns A valid input port, or ANY_PORT on failure.
     */
    virtual size_t _request_input_port(
            const size_t suggested_port,
            const uhd::device_addr_t &args
    ) const;

private:
    /*! Makes connecting something to the input thread-safe.
     */
    boost::mutex _input_mutex;

    /*! Register a node upstream of this one (i.e., a node that can send data to this node).
     *
     * By definition, the upstream node must of type source_node_ctrl.
     *
     * This saves a *weak pointer* to the upstream node and checks the port is
     * available. Will throw otherwise.
     *
     * \param upstream_node A pointer to the node instantiation
     * \param port Port number the upstream node is connected to
     */
    void _register_upstream_node(
            node_ctrl_base::sptr upstream_node,
            size_t port
    );

}; /* class sink_node_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_SINK_NODE_CTRL_BASE_HPP */
// vim: sw=4 et:
