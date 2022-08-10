//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/node.hpp>
#include <uhd/rfnoc/res_source_info.hpp>
#include <functional>

namespace uhd { namespace rfnoc {

//! Special class which may access nodes
//
// For the sake of property resolution, we require access to certain private
// members of nodes. Instead of giving the entire graph
// access to everything, we create this accessor class which is not available
// in the public API.
class node_accessor_t
{
public:
    using prop_ptrs_t = node_t::prop_ptrs_t;

    /*! Initializes the properties of a node. See node_t::init_props() for
     * details.
     */
    void init_props(node_t* node)
    {
        node->init_props();
    }

    /*! Does a local resolution of properties on \p node.
     *
     * See node_t::resolve_props for details.
     */
    void resolve_props(node_t* node)
    {
        node->resolve_props();
    }

    /*! Returns a filtered list of properties.
     *
     * The return list contains all properties that match a given predicate.
     */
    template <typename PredicateType>
    node_t::prop_ptrs_t filter_props(node_t* node, PredicateType&& predicate)
    {
        return node->filter_props(std::forward<PredicateType>(predicate));
    }

    /*! Mark all properties on this node as clean
     *
     * See node_t::clean_props() for details.
     */
    void clean_props(node_t* node)
    {
        node->clean_props();
    }

    /*! Set a resolver callback for the node
     *
     * See node_t::set_resolve_all_callback() for details.
     */
    void set_resolve_all_callback(node_t* node, node_t::resolve_callback_t&& resolver)
    {
        node->set_resolve_all_callback(std::move(resolver));
    }

    /* Restore the resolver callback to its default implementation
     *
     * See node_t::clear_resolve_all_callback() for details.
     */
    void clear_resolve_all_callback(node_t* node)
    {
        node->clear_resolve_all_callback();
    }

    /*! Sets a callback that the node can call to the mutex to the graph it
     * is connected to. Used for setting properties and propagation.
     *
     * See node_t::set_graph_mutex_callback() for details.
     */
    void set_graph_mutex_callback(node_t* node, node_t::graph_mutex_callback_t&& mutex)
    {
        node->set_graph_mutex_callback(std::move(mutex));
    }

    /*! Clears the graph mutex callback. Called when node is disconnected
     * from the graph.
     *
     * See node_t::clear_graph_mutex_callback() for details.
     */
    void clear_graph_mutex_callback(node_t* node)
    {
        node->clear_graph_mutex_callback();
    }

    /*! Forward an edge property to \p dst_node
     *
     * See node_t::forward_edge_property() for details.
     */
    void forward_edge_property(
        node_t* dst_node, const size_t dst_port, property_base_t* incoming_prop)
    {
        dst_node->forward_edge_property(incoming_prop, dst_port);
    }

    /*! Set post action callback for the node
     *
     * See node_t::set_post_action_callback() for details.
     */
    void set_post_action_callback(node_t* node, node_t::action_handler_t&& post_handler)
    {
        node->set_post_action_callback(std::move(post_handler));
    }

    /*! Send an action to \p node
     *
     * This will call node_t::receive_action() (see that for details).
     */
    void send_action(
        node_t* node, const res_source_info& port_info, action_info::sptr action)
    {
        node->receive_action(port_info, action);
    }

    /*! Fake a post_action() call from this node
     *
     * This will call node_t::post_action() (see that for details).
     */
    void post_action(
        node_t* node, const res_source_info& edge_info, action_info::sptr action)
    {
        node->post_action(edge_info, action);
    }

    /*! Check topology for this block
     *
     * See node_t::check_topology() for details
     */
    bool check_topology(node_t* node,
        const std::vector<size_t>& connected_inputs,
        const std::vector<size_t>& connected_outputs)
    {
        return node->check_topology(connected_inputs, connected_outputs);
    }

    /*! Put a node into shutdown
     *
     * See node_t::shutdown(), and also noc_block_base::shutdown() for details.
     */
    void shutdown(node_t* node)
    {
        node->shutdown();
    }
};


}} /* namespace uhd::rfnoc */
