//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/node.hpp>
#include <uhdlib/rfnoc/chdr_tx_data_xport.hpp>
#include <uhdlib/rfnoc/tx_async_msg_queue.hpp>
#include <uhdlib/transport/tx_streamer_impl.hpp>
#include <string>

namespace uhd { namespace rfnoc {

/*!
 *  Extends the streamer_impl to be an rfnoc node so it can connect to the
 *  graph. Configures the streamer conversion and rate parameters with values
 *  received through property propagation.
 */
class rfnoc_tx_streamer : public node_t,
                          public transport::tx_streamer_impl<chdr_tx_data_xport>
{
    using disconnect_fn_t = std::function<void(const std::string&)>;

public:
    /*! Constructor
     *
     * \param num_ports The number of ports
     * \param stream_args Arguments to aid the construction of the streamer
     * \param disconnect_cb Callback function to disconnect the streamer when
     *                      the object is destroyed
     */
    rfnoc_tx_streamer(const size_t num_ports,
        const uhd::stream_args_t stream_args,
        disconnect_fn_t disconnect_cb);

    /*! Destructor
     */
    ~rfnoc_tx_streamer() override;

    /*! Returns a unique identifier string for this node. In every RFNoC graph,
     * no two nodes cannot have the same ID. Returns a string in the form of
     * "TxStreamer#0".
     *
     * \returns The unique ID as a string
     */
    std::string get_unique_id() const override;

    /*! Returns the number of input ports for this block.
     *
     * Always returns 0 for this block.
     *
     * \return noc_id The number of ports
     */
    size_t get_num_input_ports() const override;

    /*! Returns the number of output ports for this block.
     *
     * \return noc_id The number of ports
     */
    size_t get_num_output_ports() const override;

    /*! Returns stream args provided at creation
     *
     * \return stream args provided when streamer is created
     */
    const uhd::stream_args_t& get_stream_args() const;

    /*! Check that all streamer ports are connected to blocks
     *
     * Overrides node_t to ensure there are no unconnected ports.
     *
     * \param connected_inputs A list of input ports that are connected
     * \param connected_outputs A list of output ports that are connected
     * \returns true if the block can deal with this configuration
     */
    bool check_topology(const std::vector<size_t>& connected_inputs,
        const std::vector<size_t>& connected_outputs) override;

    /*! Connects a channel to the streamer port
     *
     * Overrides method in tx_streamer_impl.
     *
     * \param channel The streamer channel to which to connect
     * \param xport The transport for the specified channel
     */
    void connect_channel(const size_t channel, chdr_tx_data_xport::uptr xport) override;

    /*! Receive an asynchronous message from this tx stream
     *
     *  Implementation of tx_streamer API method.
     *
     * \param async_metadata the metadata to be filled in
     * \param timeout the timeout in seconds to wait for a message
     * \return true when the async_metadata is valid, false for timeout
     */
    bool recv_async_msg(uhd::async_metadata_t& async_metadata, double timeout) override;

private:
    void _register_props(const size_t chan, const std::string& otw_format);

    void _handle_tx_event_action(
        const res_source_info& src, tx_event_action_info::sptr tx_event_action);

    // Queue for async messages
    tx_async_msg_queue::sptr _async_msg_queue;

    // Properties
    std::vector<property_t<double>> _scaling_out;
    std::vector<property_t<double>> _samp_rate_out;
    std::vector<property_t<double>> _tick_rate_out;
    std::vector<property_t<std::string>> _type_out;
    std::vector<property_t<size_t>> _mtu_out;
    std::vector<property_t<size_t>> _atomic_item_size_out;

    // Streamer unique ID
    const std::string _unique_id;

    // Stream args provided at construction
    const uhd::stream_args_t _stream_args;

    // Callback function to disconnect
    const disconnect_fn_t _disconnect_cb;
};

}} // namespace uhd::rfnoc
