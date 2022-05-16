//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/topo_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

using namespace uhd::rfnoc;
using namespace uhd::rfnoc::detail;

namespace {
//! Calculates the distance along a given route
int get_route_distance(const route_type& route)
{
    return std::accumulate(route.cbegin(),
        route.cend(),
        0,
        [](int a, const route_element_type& route_element) {
            return a + route_element.edge.get_weight();
        });
}
} // namespace

const std::string LOG_ID = "RFNOC::GRAPH::DETAIL";

std::string uhd::rfnoc::detail::to_string(const route_type& route)
{
    if (!route.empty()) {
        std::string str("");
        for (const auto& hop : route) {
            str += hop.node.to_string() + std::string(",");
            switch (hop.edge.type) {
                case topo_edge_t::edge_type::END_OF_ROUTE:
                    break;
                case topo_edge_t::edge_type::ON_CHIP: {
                    str += std::to_string(hop.edge.src_port) + std::string("->");
                } break;
                case topo_edge_t::edge_type::OFF_CHIP: {
                    str += std::to_string(hop.edge.src_port) + std::string("-=->");
                } break;
                case topo_edge_t::edge_type::HOST: {
                    str += std::to_string(hop.edge.src_port) + std::string("-H->");
                } break;
                case topo_edge_t::edge_type::ETHERNET: {
                    str += std::to_string(hop.edge.src_port) + std::string("-ETH->");
                } break;
                default:
                    UHD_THROW_INVALID_CODE_PATH();
            };
        }
        return str;
    } else {
        return std::string("<empty>");
    }
}

// Visitor object to quickly find a node
//
// The BFS-exit-by-exception is actually what we're supposed to do:
// https://www.boost.org/doc/libs/1_59_0/libs/graph/doc/faq.html
class topo_graph_t::node_finder_visitor : public boost::default_bfs_visitor
{
    topo_node_t::node_hash_type _id;

public:
    // This "exception" is how we alert the call site that the search is over
    struct node_found_event : std::runtime_error
    {
        node_found_event() : std::runtime_error("node_found") {}
    };

    node_finder_visitor(const topo_node_t::node_hash_type id) : _id(id) {}

    template <typename Vertex, typename Graph>
    void discover_vertex(Vertex u, const Graph& g) const
    {
        if (boost::get(vertex_property_t(), g, u).unique_id() == _id) {
            throw node_found_event();
        }
    }
};


// Visitor object to store a list of nodes
//
// The BFS-exit-by-exception is actually what we're supposed to do:
// https://www.boost.org/doc/libs/1_59_0/libs/graph/doc/faq.html
class topo_graph_t::node_storer_visitor : public boost::default_bfs_visitor
{
public:
    node_storer_visitor(
        std::map<topo_node_t::node_hash_type, topo_node_t>& found_nodes_ptr)
        : found_nodes_ptr(found_nodes_ptr)
    {
    }

    template <typename Vertex, typename Graph>
    void discover_vertex(Vertex u, const Graph& g) const
    {
        auto node = boost::get(vertex_property_t(), g, u);
        found_nodes_ptr.insert({node.unique_id(), node});
    }

    std::map<topo_node_t::node_hash_type, topo_node_t>& found_nodes_ptr;
};

bool topo_graph_t::add_node(const topo_node_t& node)
{
    if (_node_map.count(node.unique_id())) {
        return false;
    }

    _node_map.emplace(node.unique_id(), boost::add_vertex(node, _graph));
    return true;
}

std::list<topo_node_t> topo_graph_t::get_nodes(node_filter_type&& filter_predicate) const
{
    std::list<topo_node_t> result;
    auto vertices = _get_vertices(std::move(filter_predicate));
    for (auto& vertex : vertices) {
        result.push_back(boost::get(vertex_property_t(), _graph, vertex));
    }

    return result;
}

std::list<topo_node_t> topo_graph_t::get_connected_nodes(
    const topo_node_t& src, node_filter_type&& filter_predicate) const
{
    std::list<topo_node_t> result;
    std::map<topo_node_t::node_hash_type, topo_node_t> found_nodes;
    // Create a visitor. Surely there's a built-in way to do this with BGL,
    // maybe using copy_graph() or something like that but for now this what we
    // have.
    topo_graph_t::node_storer_visitor node_storer(found_nodes);
    boost::breadth_first_search(
        _graph, _node_map.at(src.unique_id()), boost::visitor(node_storer));
    for (auto& node_result : found_nodes) {
        if ((!filter_predicate || filter_predicate(node_result.second))
            && node_result.second != src) {
            result.push_back(node_result.second);
        }
    }

    return result;
}

bool topo_graph_t::add_edge(
    const topo_node_t& src, const topo_node_t& dst, const topo_edge_t& edge)
{
    // Check source node is registered
    if (!_node_map.count(src.unique_id())) {
        const std::string err_msg =
            "Attempting to add a route from unknown source node: " + src.to_string();
        UHD_LOG_ERROR(LOG_ID, err_msg);
        throw uhd::runtime_error(err_msg);
    }

    // Check if this source port is already connected
    auto src_vertex_desc = _node_map.at(src.unique_id());
    auto out_edge_range  = boost::out_edges(src_vertex_desc, _graph);
    for (auto edge_it = out_edge_range.first; edge_it != out_edge_range.second;
         ++edge_it) {
        const size_t existing_edge_idx =
            boost::get(boost::get(&edge_property_t::edge_index, _graph), *edge_it);

        if (edge.src_port != topo_edge_t::ANY_PORT
            && (_edge_info.at(existing_edge_idx).src_port == edge.src_port)) {
            const std::string err_msg = "Attempting to reconnect source port "
                                        + std::to_string(edge.src_port) + " of node "
                                        + src.to_string() + "!";
            UHD_LOG_ERROR(LOG_ID, err_msg);
            throw uhd::runtime_error(err_msg);
        }
    }

    // Also check if this destination port is already connected. Note that the
    // destination node may not yet be registered, in which case it's definitely
    // not connected.
    if (_node_map.count(dst.unique_id())) {
        // Check if this destination port is already connected
        const auto dst_vertex_desc = _node_map.at(dst.unique_id());
        auto in_edge_range         = boost::in_edges(dst_vertex_desc, _graph);
        for (auto edge_it = in_edge_range.first; edge_it != in_edge_range.second;
             ++edge_it) {
            const size_t existing_edge_idx =
                boost::get(boost::get(&edge_property_t::edge_index, _graph), *edge_it);

            if (edge.dst_port != topo_edge_t::ANY_PORT
                && (_edge_info.at(existing_edge_idx).dst_port == edge.dst_port)) {
                const std::string err_msg = "Attempting to reconnect destination port "
                                            + std::to_string(edge.dst_port) + " of node "
                                            + dst.to_string() + "!";
                UHD_LOG_ERROR(LOG_ID, err_msg);
                throw uhd::runtime_error(err_msg);
            }
        }
    }

    // Register the destination node:
    const bool add_node_result = add_node(dst);

    // All clear! We can add this route to the topology.
    // Now create the edge:
    const auto dst_vertex_desc = _node_map.at(dst.unique_id());
    _edge_info.push_back(edge);
    const size_t new_edge_index = _edge_info.size() - 1;

    // TODO: Remove the duplication of edge_property_t::weight and
    // topo_edge_t::weight. See also get_route() and update_weight().
    auto success = boost::add_edge(
        src_vertex_desc, dst_vertex_desc, {edge.get_weight(), new_edge_index}, _graph);
    UHD_ASSERT_THROW(success.second);

    return add_node_result;
}

bool topo_graph_t::add_biedge(
    const topo_node_t& src, const topo_node_t& dst, const topo_edge_t& edge)
{
    topo_edge_t rev_edge = edge;
    std::swap(rev_edge.src_port, rev_edge.dst_port);

    bool result = add_edge(src, dst, edge);
    add_edge(dst, src, rev_edge);
    return result;
}


void topo_graph_t::_assert_route(
    const topo_node_t& src, const topo_node_t& dst, const std::string& verb) const
{
    if (!_has_node(src)) {
        UHD_LOG_THROW(uhd::runtime_error,
            LOG_ID,
            "Cannot " << verb << " route from: " + src.to_string()
                      << ". Node not found in topology graph.");
    }
    if (!_has_node(dst)) {
        UHD_LOG_THROW(uhd::runtime_error,
            LOG_ID,
            "Cannot " << verb << " route to: " << dst.to_string()
                      << ". Node not found in topology graph.");
    }
}

bool topo_graph_t::has_route(const topo_node_t& src, const topo_node_t& dst) const
{
    _assert_route(src, dst, "check");

    // Create a visitor
    topo_graph_t::node_finder_visitor node_finder(dst.unique_id());
    // Run BFS
    bool route_found = false;
    try {
        boost::breadth_first_search(
            _graph, _node_map.at(src.unique_id()), boost::visitor(node_finder));
    } catch (topo_graph_t::node_finder_visitor::node_found_event&) {
        route_found = true;
    }

    return route_found;
}

route_type topo_graph_t::get_route(const topo_node_t& src, const topo_node_t& dst) const
{
    _assert_route(src, dst, "get");
    auto src_vertex_desc = _node_map.at(src.unique_id());

    // Run the Dijkstra
    std::vector<topo_adjlist_t::vertex_descriptor> predec_map(
        boost::num_vertices(_graph));
    boost::dijkstra_shortest_paths(_graph,
        src_vertex_desc,
        boost::predecessor_map(boost::make_iterator_property_map(predec_map.begin(),
                                   boost::get(boost::vertex_index, _graph)))
            .weight_map(boost::get(&edge_property_t::weight, _graph)));
    // TODO: The weight_map pulls weights out of the edge_property_t, and not
    // the topo_edge_t, which is the one that matters. We thus have to duplicate
    // the weights on both those data structures. A good improvement would be
    // to remove that duplication.

    // Convert the predecessor map to a reverse path map, so we can trace from
    // dst to src more easily
    std::map<topo_adjlist_t::vertex_descriptor, topo_adjlist_t::vertex_descriptor>
        rpath_map{};
    auto v_iterators = boost::vertices(_graph);
    for (auto it = v_iterators.first; it != v_iterators.second; ++it) {
        // The algorithm will choose a node itself as its own predecessor for
        // paths that don't lead to the source node. This includes the entry
        // for the source node itself (i.e., src == predec_map[src]).
        if (*it == predec_map[*it]) {
            continue;
        }
        rpath_map.insert({*it, predec_map[*it]});
    }
    // Verify route even exists
    if (!rpath_map.count(_node_map.at(dst.unique_id()))) {
        const std::string err_msg = "Cannot create route from " + src.to_string()
                                    + " and " + dst.to_string() + ", no route was found!";
        UHD_LOG_ERROR(LOG_ID, err_msg);
        throw uhd::runtime_error(err_msg);
    }

    // Now extract the route from the reverse path map
    route_type route{};
    topo_node_t curr_node = dst;
    topo_node_t prev_node;
    topo_edge_t edge_from_curr;
    edge_from_curr.type     = topo_edge_t::edge_type::END_OF_ROUTE;
    edge_from_curr.src_port = topo_edge_t::ANY_PORT;
    route.push_front({curr_node /* == dst */, edge_from_curr});
    while (curr_node != src) {
        // Look up previous node in path
        prev_node = boost::get(vertex_property_t(),
            _graph,
            rpath_map.at(_node_map.at(curr_node.unique_id())));
        // Find edge info and store it in the route together with the node
        route.push_front({prev_node, get_edge(prev_node, curr_node)});
        curr_node = prev_node;
    }
    UHD_ASSERT_THROW(route.back().edge.type == topo_edge_t::edge_type::END_OF_ROUTE);
    UHD_ASSERT_THROW(route.back().node == dst);
    UHD_ASSERT_THROW(route.front().node == src);

    return route;
}


route_type topo_graph_t::get_best_route(
    topo_graph_t::node_filter_type&& src_filter, const topo_node_t& dst) const
{
    auto vertices = _get_vertices(std::move(src_filter));
    route_type best_route{};
    int shortest_distance = std::numeric_limits<int>::max();
    for (auto& vertex : vertices) {
        auto src = boost::get(vertex_property_t(), _graph, vertex);
        if (!has_route(src, dst)) {
            continue;
        }
        auto route   = get_route(src, dst);
        int distance = get_route_distance(route);
        if (distance < shortest_distance) {
            best_route        = route;
            shortest_distance = distance;
        }
    }

    return best_route;
}

int topo_graph_t::get_distance(const topo_node_t& src, const topo_node_t& dst) const
{
    return get_route_distance(get_route(src, dst));
}


topo_edge_t topo_graph_t::get_edge(const topo_node_t& src, const topo_node_t& dst) const
{
    auto edge_result =
        boost::edge(_node_map.at(src.unique_id()), _node_map.at(dst.unique_id()), _graph);
    if (!edge_result.second) {
        const std::string err_msg = "Failed to find edge between " + src.to_string()
                                    + " and " + dst.to_string() + "!";
        UHD_LOG_ERROR(LOG_ID, err_msg);
        throw uhd::assertion_error(err_msg);
    }
    return _edge_info.at(
        boost::get(boost::get(&edge_property_t::edge_index, _graph), edge_result.first));
}


topo_node_t& topo_graph_t::access_node(const topo_node_t& node_id)
{
    return boost::get(vertex_property_t(), _graph, _node_map.at(node_id.unique_id()));
}

void topo_graph_t::update_weight(
    const topo_node_t& src, const topo_edge_t::port_t src_port, const int new_weight)
{
    auto src_vertex_desc = _node_map.at(src.unique_id());
    auto out_edge_range  = boost::out_edges(src_vertex_desc, _graph);
    for (auto edge_it = out_edge_range.first; edge_it != out_edge_range.second;
         ++edge_it) {
        size_t edge_idx =
            boost::get(boost::get(&edge_property_t::edge_index, _graph), *edge_it);
        UHD_ASSERT_THROW(_edge_info.size() > edge_idx);
        if (_edge_info.at(edge_idx).src_port == src_port) {
            // TODO: Remove the duplication of edge_property_t::weight and
            // topo_edge_t::weight. See also get_route() and update_weight().
            _edge_info.at(edge_idx).weight = new_weight;
            boost::get(boost::get(&edge_property_t::weight, _graph), *edge_it) =
                _edge_info.at(edge_idx).get_weight();
            return;
        }
    }
    const std::string err_msg("Could not find edge for node " + src.to_string()
                              + ", source port " + std::to_string(src_port));
    throw uhd::runtime_error(err_msg);
}

std::string topo_graph_t::to_dot() const
{
    static const std::map<topo_node_t::node_type, std::string> SHAPE_MAP{
        {topo_node_t::node_type::INVALID, "circle"},
        {topo_node_t::node_type::XBAR, "hexagon"},
        {topo_node_t::node_type::STRM_EP, "house"},
        {topo_node_t::node_type::XPORT, "diamond"},
        {topo_node_t::node_type::VIRTUAL, "box"}};

    const std::string graph_name = "rfnoc_topo_graph";
    std::string dot_str;
    dot_str += "digraph " + graph_name + " {\n";
    dot_str += "rankdir=TB;\n"; // TB: top-to-bottom
    dot_str += "node [colorscheme=paired12];\n";
    // Iterate through the vertices and print them out
    for (auto vi = boost::vertices(_graph); vi.first != vi.second; ++vi.first) {
        const uint32_t node_id   = uint32_t(*vi.first);
        auto node                = boost::get(vertex_property_t(), _graph, *vi.first);
        const std::string shape  = SHAPE_MAP.at(node.type);
        const uint32_t colorcode = uint32_t(node.device_id) % 12;
        dot_str += str(boost::format(" %d [label=\"%s\",shape=%s,color=%d];\n") % node_id
                       % node.to_string() % shape % (colorcode));
    }

    // Iterate through the edges and print them out
    for (auto ei = boost::edges(_graph); ei.first != ei.second; ++ei.first) {
        size_t edge_idx =
            boost::get(boost::get(&edge_property_t::edge_index, _graph), *ei.first);
        dot_str += str(boost::format(" %d -> %d [xlabel=\"%d\"];\n")
                       % uint32_t(boost::source(*(ei.first), _graph))
                       % uint32_t(boost::target(*(ei.first), _graph))
                       % _edge_info.at(edge_idx).get_weight());
    }
    dot_str += "}\n";
    return dot_str;
}


/******************************************************************************
 * Private graph helpers
 *****************************************************************************/
std::list<topo_graph_t::topo_adjlist_t::vertex_descriptor> topo_graph_t::_get_vertices(
    node_filter_type&& filter) const
{
    std::list<topo_adjlist_t::vertex_descriptor> nodes;
    auto v_iterators = boost::vertices(_graph);
    for (auto it = v_iterators.first; it != v_iterators.second; ++it) {
        auto node = boost::get(vertex_property_t(), _graph, *it);
        if (filter(node)) {
            nodes.push_back(*it);
        }
    }

    return nodes;
}
