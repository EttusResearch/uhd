//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/topological_sort.hpp>
#include <limits>
#include <utility>

using namespace uhd::rfnoc;
using namespace uhd::rfnoc::detail;

namespace {

const std::string LOG_ID                 = "RFNOC::GRAPH::DETAIL";
constexpr unsigned MAX_ACTION_ITERATIONS = 200;

/*! Helper function to pretty-print edge info
 */
std::string print_edge(
    graph_t::node_ref_t src, graph_t::node_ref_t dst, graph_t::graph_edge_t edge_info)
{
    return src->get_unique_id() + ":" + std::to_string(edge_info.src_port) + " -> "
           + dst->get_unique_id() + ":" + std::to_string(edge_info.dst_port);
}

/*! Return a list of dirty properties from a node
 */
auto get_dirty_props(graph_t::node_ref_t node_ref)
{
    using namespace uhd::rfnoc;
    node_accessor_t node_accessor{};
    return node_accessor.filter_props(node_ref, [](property_base_t* prop) {
        return prop->is_dirty()
               && prop->get_src_info().type != res_source_info::FRAMEWORK;
    });
}

} // namespace

/*! Graph-filtering predicate to find dirty nodes only
 */
struct graph_t::DirtyNodePredicate
{
    DirtyNodePredicate() {} // Default ctor is required
    DirtyNodePredicate(graph_t::rfnoc_graph_t& graph) : _graph(&graph) {}

    template <typename Vertex>
    bool operator()(const Vertex& v) const
    {
        return !get_dirty_props(boost::get(graph_t::vertex_property_t(), *_graph, v))
                    .empty();
    }

private:
    // Don't make any attribute const, because default assignment operator
    // is also required
    graph_t::rfnoc_graph_t* _graph;
};

/******************************************************************************
 * Public API calls
 *****************************************************************************/
void graph_t::connect(node_ref_t src_node, node_ref_t dst_node, graph_edge_t edge_info)
{
    std::lock_guard<std::recursive_mutex> l(_graph_mutex);

    node_accessor_t node_accessor{};
    UHD_LOG_TRACE(LOG_ID,
        "Connecting block " << src_node->get_unique_id() << ":" << edge_info.src_port
                            << " -> " << dst_node->get_unique_id() << ":"
                            << edge_info.dst_port);

    // Correctly populate edge_info
    edge_info.src_blockid = src_node->get_unique_id();
    edge_info.dst_blockid = dst_node->get_unique_id();

    // Add nodes to graph, if not already in there:
    _add_node(src_node);
    _add_node(dst_node);
    // Find vertex descriptors
    auto src_vertex_desc = _node_map.at(src_node);
    auto dst_vertex_desc = _node_map.at(dst_node);

    // Set resolver callbacks:
    node_accessor.set_resolve_all_callback(src_node, [this, src_node]() {
        this->resolve_all_properties(resolve_context::NODE_PROP, src_node);
    });
    node_accessor.set_resolve_all_callback(dst_node, [this, dst_node]() {
        this->resolve_all_properties(resolve_context::NODE_PROP, dst_node);
    });
    // Set post action callbacks:
    node_accessor.set_post_action_callback(
        src_node, [this, src_node](const res_source_info& src, action_info::sptr action) {
            this->enqueue_action(src_node, src, action);
        });
    node_accessor.set_post_action_callback(
        dst_node, [this, dst_node](const res_source_info& src, action_info::sptr action) {
            this->enqueue_action(dst_node, src, action);
        });

    // Check if edge exists
    auto out_edge_range = boost::out_edges(src_vertex_desc, _graph);
    for (auto edge_it = out_edge_range.first; edge_it != out_edge_range.second;
         ++edge_it) {
        auto existing_edge_info = boost::get(edge_property_t(), _graph, *edge_it);

        // if exact edge exists, do nothing and return
        if (existing_edge_info == edge_info) {
            UHD_LOG_INFO(LOG_ID,
                "Ignoring repeated call to connect "
                    << edge_info.src_blockid << ":" << edge_info.src_port << " -> "
                    << edge_info.dst_blockid << ":" << edge_info.dst_port);
            return;
        }

        // if there is already an edge for the source block and port
        if (existing_edge_info.src_port == edge_info.src_port
            && existing_edge_info.src_blockid == edge_info.src_blockid) {
            // if same destination block and port
            if (existing_edge_info.dst_port == edge_info.dst_port
                && existing_edge_info.dst_blockid == edge_info.dst_blockid) {
                // attempt to modify edge properties - throw an error
                UHD_LOG_ERROR(LOG_ID,
                    "Caught attempt to modify properties of edge "
                        << existing_edge_info.src_blockid << ":"
                        << existing_edge_info.src_port << " -> "
                        << existing_edge_info.dst_blockid << ":"
                        << existing_edge_info.dst_port);
                throw uhd::rfnoc_error("Caught attempt to modify properties of edge!");
            } else {
                // Attempt to reconnect already connected source block and port
                UHD_LOG_ERROR(LOG_ID,
                    "Attempting to reconnect output port "
                        << existing_edge_info.src_blockid << ":"
                        << existing_edge_info.src_port);
                throw uhd::rfnoc_error("Attempting to reconnect output port!");
            }
        }
    }
    auto in_edge_range = boost::in_edges(dst_vertex_desc, _graph);
    for (auto edge_it = in_edge_range.first; edge_it != in_edge_range.second; ++edge_it) {
        auto existing_edge_info = boost::get(edge_property_t(), _graph, *edge_it);
        if (edge_info.dst_blockid == existing_edge_info.dst_blockid
            && edge_info.dst_port == existing_edge_info.dst_port) {
            UHD_LOG_ERROR(LOG_ID,
                "Attempting to reconnect input port " << existing_edge_info.dst_blockid
                                                      << ":"
                                                      << existing_edge_info.dst_port);
            throw uhd::rfnoc_error("Attempting to reconnect input port!");
        }
    }

    // Create edge
    auto edge_descriptor =
        boost::add_edge(src_vertex_desc, dst_vertex_desc, edge_info, _graph);
    UHD_ASSERT_THROW(edge_descriptor.second);

    // Now make sure we didn't add an unintended cycle
    try {
        _get_topo_sorted_nodes();
    } catch (const uhd::rfnoc_error&) {
        UHD_LOG_ERROR(LOG_ID,
            "Adding edge " << src_node->get_unique_id() << ":" << edge_info.src_port
                           << " -> " << dst_node->get_unique_id() << ":"
                           << edge_info.dst_port
                           << " without disabling property_propagation_active will lead "
                              "to unresolvable graph!");
        boost::remove_edge(edge_descriptor.first, _graph);
        throw uhd::rfnoc_error(
            "Adding edge without disabling property_propagation_active will lead "
            "to unresolvable graph!");
    }
}

void graph_t::disconnect(node_ref_t src_node, node_ref_t dst_node, graph_edge_t edge_info)
{
    std::lock_guard<std::recursive_mutex> l(_graph_mutex);

    node_accessor_t node_accessor{};

    // Find vertex descriptor
    if (_node_map.count(src_node) == 0 && _node_map.count(dst_node) == 0) {
        return;
    }

    UHD_LOG_TRACE(LOG_ID,
        "Disconnecting block " << src_node->get_unique_id() << ":" << edge_info.src_port
                               << " -> " << dst_node->get_unique_id() << ":"
                               << edge_info.dst_port);

    auto src_vertex_desc = _node_map.at(src_node);
    auto dst_vertex_desc = _node_map.at(dst_node);

    edge_info.src_blockid = src_node->get_unique_id();
    edge_info.dst_blockid = dst_node->get_unique_id();

    boost::remove_out_edge_if(src_vertex_desc,
        [this, edge_info](rfnoc_graph_t::edge_descriptor edge_desc) {
            return (edge_info == boost::get(edge_property_t(), this->_graph, edge_desc));
        },
        _graph);

    if (boost::degree(src_vertex_desc, _graph) == 0) {
        _remove_node(src_node);
        UHD_LOG_TRACE(LOG_ID,
            "Removing block " << src_node->get_unique_id() << ":" << edge_info.src_port);
        node_accessor.clear_resolve_all_callback(src_node);
        node_accessor.set_post_action_callback(
            src_node, [](const res_source_info&, action_info::sptr) {});
    }

    // Re-look up the vertex descriptor for dst_node, as the act of removing
    // src_node may have modified it
    dst_vertex_desc = _node_map.at(dst_node);
    if (boost::degree(dst_vertex_desc, _graph) == 0) {
        _remove_node(dst_node);
        UHD_LOG_TRACE(LOG_ID,
            "Removing block " << dst_node->get_unique_id() << ":" << edge_info.dst_port);
        node_accessor.clear_resolve_all_callback(dst_node);
        node_accessor.set_post_action_callback(
            dst_node, [](const res_source_info&, action_info::sptr) {});
    }
}

void graph_t::remove(node_ref_t node)
{
    std::lock_guard<std::recursive_mutex> l(_graph_mutex);
    _remove_node(node);
}

void graph_t::commit()
{
    std::lock_guard<std::recursive_mutex> l(_graph_mutex);
    if (_release_count) {
        _release_count--;
    }
    if (_release_count == 0) {
        _check_topology();
        resolve_all_properties(resolve_context::INIT, *boost::vertices(_graph).first);
    }
}

void graph_t::release()
{
    std::lock_guard<std::recursive_mutex> l(_graph_mutex);
    UHD_LOG_TRACE(LOG_ID, "graph::release() => " << _release_count);
    _release_count++;
}

void graph_t::shutdown()
{
    std::lock_guard<std::recursive_mutex> l(_graph_mutex);
    UHD_LOG_TRACE(LOG_ID, "graph::shutdown()");
    _shutdown      = true;
    _release_count = std::numeric_limits<size_t>::max();
}

std::vector<graph_t::graph_edge_t> graph_t::enumerate_edges()
{
    auto e_iterators = boost::edges(_graph);
    std::vector<graph_edge_t> result;
    for (auto e_it = e_iterators.first; e_it != e_iterators.second; ++e_it) {
        graph_edge_t edge_info = boost::get(edge_property_t(), _graph, *e_it);
        // This is probably the dumbest way to make sure that the in- and out-
        // edges don't both get stashed, but it works for now
        if (std::find(result.begin(), result.end(), edge_info) == result.end()) {
            result.push_back(boost::get(edge_property_t(), _graph, *e_it));
        }
    }
    return result;
}

/******************************************************************************
 * Private methods to be called by friends
 *****************************************************************************/
void graph_t::resolve_all_properties(
    resolve_context context, rfnoc_graph_t::vertex_descriptor initial_node)
{
    if (boost::num_vertices(_graph) == 0) {
        return;
    }

    // We can't release during property propagation, so we lock this entire
    // method to make sure that a) different threads can't interfere with each
    // other, and b) that we don't release the graph while this method is still
    // running.
    std::lock_guard<std::recursive_mutex> l(_graph_mutex);
    if (_shutdown) {
        return;
    }
    if (_release_count) {
        node_accessor_t node_accessor{};
        node_ref_t current_node = boost::get(vertex_property_t(), _graph, initial_node);
        UHD_LOG_TRACE(LOG_ID,
            "Only resolving node " << current_node->get_unique_id()
                                   << ", graph is not committed!");
        // On current node, call local resolution.
        node_accessor.resolve_props(current_node);
        // Now mark all properties on this node as clean
        node_accessor.clean_props(current_node);
        return;
    }

    UHD_LOG_TRACE(LOG_ID, "Running forward edge property propagation...");
    _resolve_all_properties(context, initial_node, true);
    UHD_LOG_TRACE(LOG_ID, "Running backward edge property propagation...");
    _resolve_all_properties(context, initial_node, false);
}


void graph_t::_resolve_all_properties(resolve_context context,
    rfnoc_graph_t::vertex_descriptor initial_node,
    const bool forward)
{
    node_accessor_t node_accessor{};

    // First, find the node on which we'll start.
    auto initial_dirty_nodes = _find_dirty_nodes();
    if (initial_dirty_nodes.size() > 1) {
        UHD_LOGGER_WARNING(LOG_ID)
            << "Found " << initial_dirty_nodes.size()
            << " dirty nodes in initial search (expected one or zero). "
               "Property propagation may resolve this.";
        for (auto& vertex : initial_dirty_nodes) {
            node_ref_t node = boost::get(vertex_property_t(), _graph, vertex);
            UHD_LOG_WARNING(LOG_ID, "Dirty: " << node->get_unique_id());
        }
    }

    // Now get all nodes in topologically sorted order, and the appropriate
    // iterators.
    auto topo_sorted_nodes = _get_topo_sorted_nodes();
    auto node_it           = topo_sorted_nodes.begin();
    auto begin_it          = topo_sorted_nodes.begin();
    auto end_it            = topo_sorted_nodes.end();
    while (*node_it != initial_node) {
        if (node_it == end_it) {
            throw uhd::rfnoc_error("Cannot find node in graph!");
        }
        // We know *node_it must be == initial_node at some point, because
        // otherwise, initial_dirty_nodes would have been empty
        node_it++;
    }

    // Start iterating over nodes
    bool forward_dir   = true;
    int num_iterations = 0;
    // If all edge properties were known at the beginning, a single iteration
    // would suffice. However, usually during the first time the property
    // propagation is run, blocks create new (dynamic) edge properties that
    // default to dirty. If we had a way of knowing when that happens, we could
    // dynamically increase the number of iterations during the loop. For now,
    // we simply hard-code the number of iterations to 2 so that we catch that
    // case without any additional complications.
    constexpr int MAX_NUM_ITERATIONS = 2;
    while (true) {
        node_ref_t current_node = boost::get(vertex_property_t(), _graph, *node_it);
        UHD_LOG_TRACE(
            LOG_ID, "Now resolving next node: " << current_node->get_unique_id());

        // On current node, call local resolution. This may cause other
        // properties to become dirty.
        try {
            node_accessor.resolve_props(current_node);
        } catch (const uhd::resolve_error& ex) {
            UHD_LOG_ERROR(LOG_ID, current_node->get_unique_id() + ": " + ex.what());
            throw;
        }

        //  Forward all edge props in all directions from current node. We make
        //  sure to skip properties if the edge is flagged as
        //  !property_propagation_active
        _forward_edge_props(*node_it, forward);

        // Now mark all properties on this node as clean
        node_accessor.clean_props(current_node);

        // If the property resolution was triggered by a node updating one of
        // its properties, we can stop anytime there are no more dirty nodes.
        if (context == resolve_context::NODE_PROP && _find_dirty_nodes().empty()) {
            UHD_LOG_TRACE(LOG_ID,
                "Terminating graph resolution early during iteration " << num_iterations);
            break;
        }

        // The rest of the code in this loop is to figure out who's the next
        // node. First, increment (or decrement) iterator:
        if (forward_dir) {
            node_it++;
            // If we're at the end, flip the direction
            if (node_it == end_it) {
                forward_dir = false;
                // Back off from the sentinel:
                node_it--;
            }
        }
        if (!forward_dir) {
            if (topo_sorted_nodes.size() > 1) {
                node_it--;
                // If we're back at the front, flip direction
                if (node_it == begin_it) {
                    forward_dir = true;
                }
            } else {
                forward_dir = true;
            }
        }
        // If we're going forward, and the next node is the initial node,
        // we've gone full circle (one full iteration).
        if (forward_dir && (*node_it == initial_node)) {
            num_iterations++;
            if (num_iterations == MAX_NUM_ITERATIONS || _find_dirty_nodes().empty()) {
                UHD_LOG_TRACE(LOG_ID,
                    "Terminating graph resolution after iteration " << num_iterations);
                break;
            }
        }
    }

    // Post-iteration sanity checks:
    // Make sure that there are no dirty properties left. If there are,
    // that means our algorithm couldn't converge and we have a problem.
    auto remaining_dirty_nodes = _find_dirty_nodes();
    if (!remaining_dirty_nodes.empty()) {
        UHD_LOG_ERROR(LOG_ID, "The following properties could not be resolved:");
        for (auto& vertex : remaining_dirty_nodes) {
            node_ref_t node           = boost::get(vertex_property_t(), _graph, vertex);
            const std::string node_id = node->get_unique_id();
            auto dirty_props          = get_dirty_props(node);
            for (auto& prop : dirty_props) {
                UHD_LOG_ERROR(LOG_ID,
                    "Dirty: " << node_id << "[" << prop->get_src_info().to_string() << " "
                              << prop->get_id() << "]");
            }
        }
        throw uhd::resolve_error("Could not resolve properties.");
    }
}

void graph_t::resolve_all_properties(
    resolve_context context, node_ref_t initial_node)
{
    auto initial_node_vertex_desc = _node_map.at(initial_node);
    resolve_all_properties(context, initial_node_vertex_desc);
}

void graph_t::enqueue_action(
    node_ref_t src_node, res_source_info src_edge, action_info::sptr action)
{
    // We can't release during action handling, so we lock this entire
    // method to make sure that we don't release the graph while this method is
    // still running.
    // It also prevents a different thread from throwing in their own actions.
    std::lock_guard<std::recursive_mutex> release_lock(_graph_mutex);
    if (_shutdown) {
        return;
    }
    if (_release_count) {
        UHD_LOG_WARNING(LOG_ID,
            "Action propagation is not enabled, graph is not committed! Will not "
            "propagate action `"
                << action->key << "'");
        return;
    }

    // Check if we're already in the middle of handling actions. In that case,
    // we're already in the loop below, and then all we want to do is to enqueue
    // this action tuple. The first call to enqueue_action() within this thread
    // context will have handling_ongoing == false.
    const bool handling_ongoing = _action_handling_ongoing.test_and_set();
    // In any case, stash the new action at the end of the action queue
    _action_queue.emplace_back(std::make_tuple(src_node, src_edge, action));
    if (handling_ongoing) {
        UHD_LOG_TRACE(LOG_ID,
            "Action handling ongoing, deferring delivery of " << action->key << "#"
                                                              << action->id);
        return;
    }

    unsigned iteration_count = 0;
    while (!_action_queue.empty()) {
        if (iteration_count++ == MAX_ACTION_ITERATIONS) {
            throw uhd::runtime_error("Terminating action handling: Reached "
                                     "recursion limit!");
        }

        // Unpack next action
        auto& next_action                  = _action_queue.front();
        node_ref_t action_src_node         = std::get<0>(next_action);
        res_source_info action_src_port    = std::get<1>(next_action);
        action_info::sptr next_action_sptr = std::get<2>(next_action);
        _action_queue.pop_front();

        // Find the node that is supposed to receive this action, and if we find
        // something, then send the action. If the source port's type is USER,
        // that means the action is meant for us.
        node_ref_t recipient_node;
        res_source_info recipient_port(action_src_port);

        if (action_src_port.type == res_source_info::USER) {
            recipient_node = action_src_node;
            recipient_port = action_src_port;
        } else {
            auto recipient_info =
                _find_neighbour(_node_map.at(action_src_node), action_src_port);
            recipient_node = recipient_info.first;
            if (recipient_node == nullptr) {
                UHD_LOG_WARNING(LOG_ID,
                    "Cannot forward action "
                        << action->key << " from " << src_node->get_unique_id() << ":"
                        << src_edge.to_string() << ", no neighbour found!");
                continue;
            }
            recipient_port = {res_source_info::invert_edge(action_src_port.type),
                action_src_port.type == res_source_info::INPUT_EDGE
                    ? recipient_info.second.src_port
                    : recipient_info.second.dst_port};
        }
        // The following call can cause other nodes to add more actions to
        // the end of _action_queue!
        UHD_LOG_TRACE(LOG_ID,
            "Now delivering action "
                << next_action_sptr->key << "#" << next_action_sptr->id << " to "
                << recipient_node->get_unique_id() << "@" << recipient_port.to_string());
        node_accessor_t{}.send_action(recipient_node, recipient_port, next_action_sptr);
    }
    UHD_LOG_TRACE(LOG_ID, "Delivered all actions, terminating action handling.");

    // Release the action handling flag
    _action_handling_ongoing.clear();
    // Now, the _graph_mutex is released, and someone else can start sending
    // actions.
}

/******************************************************************************
 * Private methods
 *****************************************************************************/
graph_t::vertex_list_t graph_t::_find_dirty_nodes()
{
    // Create a view on the graph that doesn't include the back-edges
    DirtyNodePredicate vertex_filter(_graph);
    boost::filtered_graph<rfnoc_graph_t, boost::keep_all, DirtyNodePredicate> fg(
        _graph, boost::keep_all(), vertex_filter);

    auto v_iterators = boost::vertices(fg);
    return vertex_list_t(v_iterators.first, v_iterators.second);
}

graph_t::vertex_list_t graph_t::_get_topo_sorted_nodes()
{
    // Create a view on the graph that doesn't include the back-edges
    ForwardEdgePredicate edge_filter(_graph);
    boost::filtered_graph<rfnoc_graph_t, ForwardEdgePredicate> fg(_graph, edge_filter);

    // Topo-sort and return
    vertex_list_t sorted_nodes;
    try {
        boost::topological_sort(fg, std::front_inserter(sorted_nodes));
    } catch (boost::not_a_dag&) {
        throw uhd::rfnoc_error("Cannot resolve graph because it has at least one cycle!");
    }
    return sorted_nodes;
}

void graph_t::_add_node(node_ref_t new_node)
{
    if (_node_map.count(new_node)) {
        return;
    }

    _node_map.emplace(new_node, boost::add_vertex(new_node, _graph));
}

void graph_t::_remove_node(node_ref_t node)
{
    if (_node_map.count(node)) {
        auto vertex_desc = _node_map.at(node);

        // Remove all edges
        boost::clear_vertex(vertex_desc, _graph);

        // Remove the vertex
        boost::remove_vertex(vertex_desc, _graph);
        _node_map.erase(node);

        // Removing the vertex changes the vertex descriptors,
        // so update the node map
        auto vertex_range = boost::vertices(_graph);
        for (auto vertex_it = vertex_range.first; vertex_it != vertex_range.second;
             vertex_it++) {
            auto node       = boost::get(vertex_property_t(), _graph, *vertex_it);
            _node_map[node] = *vertex_it;
        }
    }
}


void graph_t::_forward_edge_props(
    graph_t::rfnoc_graph_t::vertex_descriptor origin, const bool forward)
{
    node_accessor_t node_accessor{};
    node_ref_t origin_node = boost::get(vertex_property_t(), _graph, origin);

    auto edge_props = node_accessor.filter_props(origin_node, [](property_base_t* prop) {
        return (prop->get_src_info().type == res_source_info::INPUT_EDGE
                || prop->get_src_info().type == res_source_info::OUTPUT_EDGE);
    });
    UHD_LOG_TRACE(LOG_ID,
        "Forwarding up to " << edge_props.size() << " edge properties from node "
                            << origin_node->get_unique_id() << " along "
                            << (forward ? "forward" : "back") << " edges.");

    for (auto prop : edge_props) {
        auto neighbour_node_info = _find_neighbour(origin, prop->get_src_info());
        if (neighbour_node_info.first != nullptr
            && neighbour_node_info.second.property_propagation_active == forward) {
            const size_t neighbour_port = prop->get_src_info().type
                                                  == res_source_info::INPUT_EDGE
                                              ? neighbour_node_info.second.src_port
                                              : neighbour_node_info.second.dst_port;
            node_accessor.forward_edge_property(
                neighbour_node_info.first, neighbour_port, prop);
        }
    }
}

bool graph_t::_assert_edge_props_consistent(rfnoc_graph_t::edge_descriptor edge)
{
    node_ref_t src_node =
        boost::get(vertex_property_t(), _graph, boost::source(edge, _graph));
    node_ref_t dst_node =
        boost::get(vertex_property_t(), _graph, boost::target(edge, _graph));
    graph_edge_t edge_info = boost::get(edge_property_t(), _graph, edge);

    // Helper function to get properties as maps
    auto get_prop_map = [](const size_t port,
                            res_source_info::source_t edge_type,
                            node_ref_t node) {
        node_accessor_t node_accessor{};
        // Create a set of all properties
        auto props_set =
            node_accessor.filter_props(node, [port, edge_type](property_base_t* prop) {
                return prop->get_src_info().instance == port
                       && prop->get_src_info().type == edge_type;
            });
        std::unordered_map<std::string, property_base_t*> prop_map;
        for (auto prop_it = props_set.begin(); prop_it != props_set.end(); ++prop_it) {
            prop_map.emplace((*prop_it)->get_id(), *prop_it);
        }

        return prop_map;
    };

    // Create two maps ID -> prop_ptr, so we have an easier time comparing them
    auto src_prop_map =
        get_prop_map(edge_info.src_port, res_source_info::OUTPUT_EDGE, src_node);
    auto dst_prop_map =
        get_prop_map(edge_info.dst_port, res_source_info::INPUT_EDGE, dst_node);

    // Now iterate through all properties, and make sure they match
    bool props_match = true;
    for (auto src_prop_it = src_prop_map.begin(); src_prop_it != src_prop_map.end();
         ++src_prop_it) {
        auto src_prop = src_prop_it->second;
        if (dst_prop_map.count(src_prop->get_id()) == 0) {
            UHD_LOG_DEBUG(LOG_ID,
                "On back-edge "
                    << edge_info.to_string() << ", source block has edge property `"
                    << src_prop->get_id() << "', but destination block does not.");
            continue;
        }
        auto dst_prop = dst_prop_map.at(src_prop->get_id());
        if (!src_prop->equal(dst_prop)) {
            UHD_LOG_ERROR(LOG_ID,
                "Edge property " << src_prop->get_id() << " inconsistent on edge "
                                 << print_edge(src_node, dst_node, edge_info));
            props_match = false;
        }
    }

    return props_match;
}

void graph_t::_check_topology()
{
    node_accessor_t node_accessor{};
    bool topo_ok     = true;
    auto v_iterators = boost::vertices(_graph);
    for (auto it = v_iterators.first; it != v_iterators.second; ++it) {
        node_ref_t node = boost::get(vertex_property_t(), _graph, *it);
        std::vector<size_t> connected_inputs;
        std::vector<size_t> connected_outputs;
        auto ie_iters = boost::in_edges(*it, _graph);
        for (auto it = ie_iters.first; it != ie_iters.second; ++it) {
            graph_edge_t edge_info = boost::get(edge_property_t(), _graph, *it);
            connected_inputs.push_back(edge_info.dst_port);
        }
        auto oe_iters = boost::out_edges(*it, _graph);
        for (auto it = oe_iters.first; it != oe_iters.second; ++it) {
            graph_edge_t edge_info = boost::get(edge_property_t(), _graph, *it);
            connected_outputs.push_back(edge_info.src_port);
        }

        if (!node_accessor.check_topology(node, connected_inputs, connected_outputs)) {
            std::ostringstream input_topology;
            input_topology << " requested inputs: (";
            for (auto connected_input : connected_inputs) {
                input_topology << connected_input;
                if (connected_input != connected_inputs.back()) {
                    input_topology << ", ";
                }
            }
            input_topology << ")";
            input_topology << " valid inputs: (";
            for (size_t expected_input = 0; expected_input < node->get_num_input_ports();
                 expected_input++) {
                input_topology << expected_input;
                if (expected_input < node->get_num_input_ports() - 1) {
                    input_topology << ", ";
                }
            }
            input_topology << ")";

            std::ostringstream output_topology;
            output_topology << " requested outputs: (";
            for (auto connected_output : connected_outputs) {
                output_topology << connected_output;
                if (connected_output != connected_outputs.back()) {
                    output_topology << ", ";
                }
            }
            output_topology << ")";
            output_topology << " valid outputs: (";
            for (size_t expected_output = 0;
                 expected_output < node->get_num_output_ports();
                 expected_output++) {
                output_topology << expected_output;
                if (expected_output < node->get_num_output_ports() - 1) {
                    output_topology << ", ";
                }
            }
            output_topology << ")";

            UHD_LOG_ERROR(LOG_ID,
                "Node " << node->get_unique_id() << " using invalid inputs or outputs! "
                        << input_topology.str() << ", " << output_topology.str());
            topo_ok = false;
        }
    }

    if (!topo_ok) {
        throw uhd::runtime_error("Graph topology is not valid!");
    }
}

std::pair<graph_t::node_ref_t, graph_t::graph_edge_t> graph_t::_find_neighbour(
    rfnoc_graph_t::vertex_descriptor origin, res_source_info port_info)
{
    if (port_info.type == res_source_info::INPUT_EDGE) {
        auto it_range = boost::in_edges(origin, _graph);
        for (auto it = it_range.first; it != it_range.second; ++it) {
            graph_edge_t edge_info = boost::get(edge_property_t(), _graph, *it);
            if (edge_info.dst_port == port_info.instance) {
                return {
                    boost::get(vertex_property_t(), _graph, boost::source(*it, _graph)),
                    edge_info};
            }
        }
        return {nullptr, {}};
    }
    if (port_info.type == res_source_info::OUTPUT_EDGE) {
        auto it_range = boost::out_edges(origin, _graph);
        for (auto it = it_range.first; it != it_range.second; ++it) {
            graph_edge_t edge_info = boost::get(edge_property_t(), _graph, *it);
            if (edge_info.src_port == port_info.instance) {
                return {
                    boost::get(vertex_property_t(), _graph, boost::target(*it, _graph)),
                    edge_info};
            }
        }
        return {nullptr, {}};
    }

    UHD_THROW_INVALID_CODE_PATH();
}
