//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/graph_edge.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/stream.hpp>
#include <uhd/transport/adapter_id.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <boost/units/detail/utility.hpp> // for demangle
#include <memory>
#include <vector>

namespace uhd { namespace rfnoc {

class mb_controller;

/*! The core class for a UHD session with (an) RFNoC device(s)
 *
 * This class is a superset of uhd::device. It does not only hold a device
 * session, but also manages the RFNoC blocks on those devices. Only devices
 * compatible with a modern version of RFNoC can be addressed by this class.
 */
class UHD_API rfnoc_graph : public uhd::noncopyable, public std::enable_shared_from_this<rfnoc_graph>
{
public:
    /*! A shared pointer to allow easy access to this class and for
     *  automatic memory management.
     */
    using sptr = std::shared_ptr<rfnoc_graph>;

    virtual ~rfnoc_graph() {}

    /******************************************
     * Factory
     ******************************************/
    /*! Make a new USRP graph from the specified device address(es).
     *
     * \param dev_addr the device address
     * \return A new rfnoc_graph object
     *
     * \throws uhd::key_error no device found
     * \throws uhd::index_error fewer devices found than expected
     */
    static sptr make(const device_addr_t& dev_addr);

    /******************************************
     * Block Discovery/Retrieval
     ******************************************/
    /*! Returns the block ids of all blocks that match the specified hint
     *
     * Uses block_id_t::match() internally.
     * If no matching block is found, it returns an empty vector.
     *
     * The returned vector is sorted lexicographically.
     *
     * To access specialized block controller classes (i.e. derived from
     * uhd::rfnoc::noc_block_base), use the templated version of this function:
     * \code{.cpp}
     * // Assume DEV is an rfnoc_graph::sptr
     * auto null_blocks = DEV->find_blocks<null_block_control>("NullSrcSink");
     * if (null_blocks.empty()) { cout << "No null blocks found!" << endl; }
     * \endcode
     *
     * \note this access is not thread safe if performed during block enumeration
     *
     * \returns A sorted list of block IDs that match the hint
     */
    virtual std::vector<block_id_t> find_blocks(
        const std::string& block_id_hint) const = 0;

    /*! Type-cast version of find_blocks().
     */
    template <typename T>
    std::vector<block_id_t> find_blocks(const std::string& block_id_hint) const
    {
        std::vector<block_id_t> all_block_ids = find_blocks(block_id_hint);
        std::vector<block_id_t> filt_block_ids;
        for (size_t i = 0; i < all_block_ids.size(); i++) {
            if (has_block<T>(all_block_ids[i])) {
                filt_block_ids.push_back(all_block_ids[i]);
            }
        }
        return filt_block_ids;
    }

    /*! \brief Checks if a specific NoC block exists on the device.
     *
     * \param block_id Canonical block name (e.g. "0/FFT#1").
     * \return true if a block with the specified id exists
     * \note this access is not thread safe if performed during block enumeration
     */
    virtual bool has_block(const block_id_t& block_id) const = 0;

    /*! Same as has_block(), but with a type check.
     *
     * \return true if a block of type T with the specified id exists
     * \note this access is not thread safe if performed during block enumeration
     */
    template <typename T>
    bool has_block(const block_id_t& block_id) const
    {
        return has_block(block_id)
               && bool(std::dynamic_pointer_cast<T>(get_block(block_id)));
    }

    /*! \brief Returns a block controller class for an NoC block.
     *
     * If the given block ID is not valid (i.e. such a block does not exist
     * on this device), it will throw a uhd::lookup_error.
     *
     * \param block_id Canonical block name (e.g. "0/FFT#1").
     * \note this access is not thread safe if peformed during block enumeration
     */
    virtual noc_block_base::sptr get_block(const block_id_t& block_id) const = 0;

    /*! Same as get_block(), but with a type cast.
     *
     * If you have a block controller class that is derived from noc_block_base,
     * use this function to access its specific methods.
     * If the given block ID is not valid (i.e. such a block does not exist
     * on this device) or if the type does not match, it will throw a uhd::lookup_error.
     *
     * \code{.cpp}
     * // Assume DEV is an rfnoc_graph::sptr
     * auto block_controller = DEV->get_block<my_noc_block>("0/MyBlock#0");
     * block_controller->my_own_block_method();
     * \endcode
     * \note this access is not thread safe if performed during block enumeration
     */
    template <typename T>
    std::shared_ptr<T> get_block(const block_id_t& block_id) const
    {
        std::shared_ptr<T> blk = std::dynamic_pointer_cast<T>(get_block(block_id));
        if (blk) {
            return blk;
        } else {
            throw uhd::lookup_error(
                std::string("This device does not have a block of type ")
                + boost::units::detail::demangle(typeid(T).name())
                + " with ID: " + block_id.to_string());
        }
    }

    /**************************************************************************
     * Connection APIs
     *************************************************************************/
    /*! Verify if two blocks/ports are connectable.
     *
     * If this call returns true, then connect() can be called with the same
     * arguments. It does not, however, check if the block was already
     * connnected.
     *
     * \returns true if the two blocks are connectable
     */
    virtual bool is_connectable(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port) = 0;

    /*! Connect a RFNoC block with block ID \p src_block to another with block ID \p
     * dst_block.
     *
     * Note you need to also call this on statically connected blocks if you
     * desire to use them.
     *
     * Connections that are made using this API call will be listed when calling
     * enumerate_active_connections().
     *
     * \param src_blk The block ID of the source block to connect.
     * \param src_port The port of the source block to connect.
     * \param dst_blk The block ID of the destination block to connect to.
     * \param dst_port The port of the destination block to connect to.
     * \param is_back_edge Flag this edge as a back-edge.
     *        See also \ref props_graph_resolution_back_edges.
     *
     * \throws uhd::routing_error if the source or destination ports are
     *                            statically connected to a *different* block
     */
    virtual void connect(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port,
        bool is_back_edge = false) = 0;

    /*! Connect TX streamer to an input of an NoC block
     *
     * \param streamer The streamer to connect.
     * \param strm_port The port of the streamer to connect.
     * \param dst_blk The block ID of the destination block to connect to.
     * \param dst_port The port of the destination block to connect to.
     * \param adapter_id The local device ID (transport) to use for this connection.
     *
     * \throws connect_disallowed_on_dst
     *     if the destination port is statically connected to a *different* block
     */
    virtual void connect(uhd::tx_streamer::sptr streamer,
        size_t strm_port,
        const block_id_t& dst_blk,
        size_t dst_port,
        uhd::transport::adapter_id_t adapter_id = uhd::transport::NULL_ADAPTER_ID) = 0;

    /*! Connect RX streamer to an output of an NoC block
     *
     * \param src_blk The block ID of the source block to connect.
     * \param src_port The port of the source block to connect.
     * \param streamer The streamer to connect.
     * \param strm_port The port of the streamer to connect.
     * \param adapter_id The local device ID (transport) to use for this connection.
     *
     * \throws connect_disallowed_on_src
     *     if the source port is statically connected to a *different* block
     */
    virtual void connect(const block_id_t& src_blk,
        size_t src_port,
        uhd::rx_streamer::sptr streamer,
        size_t strm_port,
        uhd::transport::adapter_id_t adapter_id = uhd::transport::NULL_ADAPTER_ID) = 0;

    /*! Disconnect a RFNoC block with block ID \p src_blk from another with block ID
     * \p dst_blk.  This will logically disconnect the blocks, but the physical
     * connection will not be changed until a new connection is made on the source
     * port.
     *
     * Disconnected edges will no longer be listed in
     * enumerate_active_connections().
     *
     * \param src_blk The block ID of the source block to disconnect.
     * \param src_port The port of the source block to disconnect.
     * \param dst_blk The block ID of the destination block to disconnect from.
     * \param dst_port The port of the destination block to disconnect from.
     */
    virtual void disconnect(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port) = 0;

    /*! Disconnect a streamer with ID \p streamer_id. This will logically
     * disconnect the streamer, but physical connection will not be changed.
     * For RX streamers, the physical connection will be changed when a new
     * connection is made to the upstream source port(s).  For TX streamers,
     * the physical connection will be changed when the TX streamer is
     * destroyed or the port(s) are connected to another block.
     *
     * \param streamer_id The ID of the streamer to disconnect.
     */
    virtual void disconnect(const std::string& streamer_id) = 0;

    /*! Disconnect port \p port of a streamer with ID \p streamer_id. This
     * will logically disconnect the streamer, but physical connection will
     * not be changed.  For RX streamers, the physical connection will be
     * changed when a new connection is made to the upstreame source port.
     * For TX streamers, the physical connection will be changed when the TX
     * streamer is destroyed or the port is connected to another block.
     *
     * \param streamer_id The ID of the streamer.
     * \param port The port to disconnect.
     */
    virtual void disconnect(const std::string& streamer_id, size_t port) = 0;

    /*! Enumerate all the possible host transport adapters that can be used to
     * receive from the specified block
     *
     * If addr and second_addr were specified in device_args, the adapter_id_t
     * associated with addr will come first in the vector, then second_addr.
     *
     * \param src_blk The block ID of the source block to connect to.
     * \param src_port The port of the source block to connect to.
     */
    virtual std::vector<uhd::transport::adapter_id_t> enumerate_adapters_from_src(
        const block_id_t& src_blk, size_t src_port) = 0;

    /*! Enumerate all the possible host transport adapters that can be used to
     * send to the specified block
     *
     * If addr and second_addr were specified in device_args, the adapter_id_t
     * associated with addr will come first in the vector, then second_addr.
     *
     * \param dst_blk The block ID of the destination block to connect to.
     * \param dst_port The port of the destination block to connect to.
     */
    virtual std::vector<uhd::transport::adapter_id_t> enumerate_adapters_to_dst(
        const block_id_t& dst_blk, size_t dst_port) = 0;

    /*! Enumerate all the possible static connections in the graph
     *
     * A static connection is a connection that is statically connected in
     * hardware. Note that static connections also need to be declared using
     * connect() if an application wants to use them, or they won't be available
     * for property propagation or any other graph mechanism.
     *
     * \return A vector containing all the static edges in the graph.
     */
    virtual std::vector<graph_edge_t> enumerate_static_connections() const = 0;

    /*! Enumerate all the active connections in the graph
     *
     * An active connection is a connection which was previously created by
     * calling connect().
     *
     * \return A vector containing all the active edges in the graph.
     */
    virtual std::vector<graph_edge_t> enumerate_active_connections() = 0;

    /*! Commit graph and run initial checks
     *
     * This method needs to be called when the graph is ready for action.
     * It will run checks on the graph and run a property propagation.
     *
     * \throws uhd::resolve_error if the properties fail to resolve.
     */
    virtual void commit() = 0;

    /*! Release graph: Opposite of commit()
     *
     * Calling this will disable property propagation until commit() has been
     * called an equal number of times.
     */
    virtual void release() = 0;

    /******************************************
     * Streaming
     ******************************************/

    /*! Create a new receive streamer from the streamer arguments
     *  The created streamer is still not connected to anything yet.
     *  The graph::connect call has to be made on this streamer to
     *  start using it. If a different streamer is already connected
     *  to the intended source then that call may fail.
     *
     * \param num_ports Number of ports that will be connected to the streamer
     * \param args Arguments to aid the construction of the streamer
     * \return a shared pointer to a new streamer
     */
    virtual rx_streamer::sptr create_rx_streamer(
        const size_t num_ports, const stream_args_t& args) = 0;

    /*! Create a new transmit streamer from the streamer arguments
     *  The created streamer is still not connected to anything yet.
     *  The graph::connect call has to be made on this streamer to
     *  start using it. If a different streamer is already connected
     *  to the intended sink then that call may fail.
     *
     * \param num_ports Number of ports that will be connected to the streamer
     * \param args Arguments to aid the construction of the streamer
     * \return a shared pointer to a new streamer
     */
    virtual tx_streamer::sptr create_tx_streamer(
        const size_t num_ports, const stream_args_t& args) = 0;

    /**************************************************************************
     * Hardware Control
     *************************************************************************/
    virtual size_t get_num_mboards() const = 0;
    //! Return a reference to a motherboard controller
    //
    // See also uhd::rfnoc::mb_controller
    virtual std::shared_ptr<mb_controller> get_mb_controller(
        const size_t mb_index = 0) = 0;

    /*! Run any routines necessary to synchronize devices
     *
     * The specific implementation of this call are device-specific. In all
     * cases, it will set the time to a common value.
     *
     * Any application that requires any kind of phase or time alignment (if
     * supported by the hardware) must call this before operation.
     *
     * \param time_spec The timestamp to be used to sync the devices. It will be
     *                  an input to set_time_next_pps() on the motherboard
     *                  controllers.
     * \param quiet If true, there will be no errors or warnings printed if the
     *              synchronization happens. This call will always be called
     *              during initialization, but preconditions might not yet be
     *              met (e.g., the time and reference sources might still be
     *              internal), and will fail quietly in that case.
     *
     * \returns the success status of this call (true means devices are now
     *          synchronized)
     */
    virtual bool synchronize_devices(
        const uhd::time_spec_t& time_spec, const bool quiet) = 0;

    //! Return a reference to the property tree
    virtual uhd::property_tree::sptr get_tree(void) const = 0;
}; // class rfnoc_graph

}}; // namespace uhd::rfnoc
