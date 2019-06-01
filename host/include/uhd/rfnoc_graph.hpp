//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOCDEV_GRAPH_HPP
#define INCLUDED_LIBUHD_RFNOCDEV_GRAPH_HPP

#include <uhd/config.hpp>
#include <uhd/stream.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <uhd/rfnoc/graph_edge.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/device_addr.hpp>
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
class UHD_API rfnoc_graph : public uhd::noncopyable
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
     * Uses block_id_t::match() internally.
     * If no matching block is found, it returns an empty vector.
     *
     * To access specialized block controller classes (i.e. derived from noc_block_base),
     * use the templated version of this function, e.g.
     * \code{.cpp}
     * // Assume DEV is an rfnoc_graph::sptr
     * auto null_blocks = DEV->find_blocks<null_noc_block>("NullSrcSink");
     * if (null_blocks.empty()) { cout << "No null blocks found!" << endl; }
     * \endcode
     * \note this access is not thread safe if performed during block enumeration
     */
    virtual std::vector<block_id_t> find_blocks(const std::string& block_id_hint) const = 0;

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
    noc_block_base::sptr get_block(const block_id_t& block_id) const;

    /*! Same as get_block(), but with a type cast.
     *
     * If you have a block controller class that is derived from noc_block_base,
     * use this function to access its specific methods.
     * If the given block ID is not valid (i.e. such a block does not exist
     * on this device) or if the type does not match, it will throw a uhd::lookup_error.
     *
     * \code{.cpp}
     * // Assume DEV is a device3::sptr
     * auto block_controller = get_block<my_noc_block>("0/MyBlock#0");
     * block_controller->my_own_block_method();
     * \endcode
     * \note this access is not thread safe if performed during block enumeration
     */
    template <typename T>
    std::shared_ptr<T> get_block(const block_id_t& block_id) const
    {
        std::shared_ptr<T> blk =
            std::dynamic_pointer_cast<T>(get_block(block_id));
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
    /*! Connect a RFNOC block with block ID \p src_block to another with block ID \p
     * dst_block.
     *
     * \param src_blk The block ID of the source block to connect.
     * \param src_port The port of the source block to connect.
     * \param dst_blk The block ID of the destination block to connect to.
     * \param dst_port The port of the destination block to connect to.
     * \param skip_property_propagation Skip property propagation for this edge
     *
     * \throws connect_disallowed_on_src
     *     if the source port is statically connected to a *different* block
     * \throws connect_disallowed_on_dst
     *     if the destination port is statically connected to a *different* block
     */
    void connect(const block_id_t& src_blk,
        size_t src_port,
        const block_id_t& dst_blk,
        size_t dst_port,
        bool skip_property_propagation = false);

    /*! Connect TX streamer to an input of an NoC block
     *
     * \param streamer The streamer to connect.
     * \param strm_port The port of the streamer to connect.
     * \param dst_blk The block ID of the destination block to connect to.
     * \param dst_port The port of the destination block to connect to.
     *
     * \throws connect_disallowed_on_dst
     *     if the destination port is statically connected to a *different* block
     */
    void connect(uhd::tx_streamer& streamer,
        size_t strm_port,
        const block_id_t& dst_blk,
        size_t dst_port);

    /*! Connect RX streamer to an output of an NoC block
     *
     * \param src_blk The block ID of the destination block to connect to.
     * \param src_port The port of the destination block to connect to.
     * \param streamer The streamer to connect.
     * \param strm_port The port of the streamer to connect.
     *
     * \throws connect_disallowed_on_src
     *     if the source port is statically connected to a *different* block
     */
    void connect(const block_id_t& src_blk,
        size_t src_port,
        uhd::rx_streamer& streamer,
        size_t strm_port);

    /*! Enumerate all the connections in the graph
     *
     * \return A vector containing all the edges in the graph.
     */
    std::vector<graph_edge_t> enumerate_connections();

    /******************************************
     * Streaming
     ******************************************/

    /*! Create a new receive streamer from the streamer arguments
     *  The created streamer is still not connected to anything yet.
     *  The graph::connect call has to be made on this streamer to
     *  start using it. If a different streamer is already connected
     *  to the intended source then that call may fail.
     *
     * \param args Arguments to aid the construction of the streamer
     * \return a shared pointer to a new streamer
     */
    //virtual rx_streamer::sptr create_rx_streamer(const stream_args_t& args) = 0;

    /*! Create a new transmit streamer from the streamer arguments
     *  The created streamer is still not connected to anything yet.
     *  The graph::connect call has to be made on this streamer to
     *  start using it. If a different streamer is already connected
     *  to the intended sink then that call may fail.
     *
     * \param args Arguments to aid the construction of the streamer
     * \return a shared pointer to a new streamer
     */
    //virtual tx_streamer::sptr create_tx_streamer(const stream_args_t& args) = 0;

    /**************************************************************************
     * Hardware Control
     *************************************************************************/
    //! Return a reference to a motherboard controller
    //
    // See also uhd::rfnoc::mb_controller
    virtual std::shared_ptr<mb_controller> get_mb_controller(
        const size_t mb_index = 0) = 0;
}; // class rfnoc_graph

}}; // namespace uhd::rfnoc

#endif /* INCLUDED_LIBUHD_RFNOCDEV_GRAPH_HPP */
