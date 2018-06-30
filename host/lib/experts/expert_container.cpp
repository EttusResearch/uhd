//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/experts/expert_container.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/adjacency_list.hpp>

#ifdef UHD_EXPERT_LOGGING
#define EX_LOG(depth, str) _log(depth, str)
#else
#define EX_LOG(depth, str)
#endif

namespace uhd { namespace experts {

typedef boost::adjacency_list<
    boost::vecS,        //Container used to represent the edge-list for each of the vertices.
    boost::vecS,        //container used to represent the vertex-list of the graph.
    boost::directedS,   //Directionality of graph
    dag_vertex_t*,      //Storage for each vertex
    boost::no_property, //Storage for each edge
    boost::no_property, //Storage for graph object
    boost::listS        //Container used to represent the edge-list for the graph.
> expert_graph_t;

typedef std::map<std::string, expert_graph_t::vertex_descriptor> vertex_map_t;
typedef std::list<expert_graph_t::vertex_descriptor>             node_queue_t;

typedef boost::graph_traits<expert_graph_t>::edge_iterator       edge_iter;
typedef boost::graph_traits<expert_graph_t>::vertex_iterator     vertex_iter;

class expert_container_impl : public expert_container
{
private:    //Visitor class for cycle detection algorithm
    struct cycle_det_visitor : public boost::dfs_visitor<>
    {
        cycle_det_visitor(std::vector<std::string>& back_edges):
            _back_edges(back_edges) {}

        template <class Edge, class Graph>
        void back_edge(Edge u, const Graph& g) {
            _back_edges.push_back(
                g[boost::source(u,g)]->get_name() + "->" + g[boost::target(u,g)]->get_name());
        }
        private: std::vector<std::string>& _back_edges;
    };

public:
    expert_container_impl(const std::string& name):
        _name(name)
    {
    }

    ~expert_container_impl()
    {
        clear();
    }

    const std::string& get_name() const
    {
        return _name;
    }

    void resolve_all(bool force = false)
    {
        boost::lock_guard<boost::recursive_mutex> resolve_lock(_resolve_mutex);
        boost::lock_guard<boost::mutex> lock(_mutex);
        EX_LOG(0, str(boost::format("resolve_all(%s)") % (force?"force":"")));
        // Do a full resolve of the graph
        _resolve_helper("", "", force);
    }

    void resolve_from(const std::string&)
    {
        boost::lock_guard<boost::recursive_mutex> resolve_lock(_resolve_mutex);
        boost::lock_guard<boost::mutex> lock(_mutex);
        EX_LOG(0, "resolve_from (overridden to resolve_all)");
        // Do a full resolve of the graph
        // Not optimizing the traversal using node_name to reduce experts complexity
        _resolve_helper("", "", false);
    }

    void resolve_to(const std::string&)
    {
        boost::lock_guard<boost::recursive_mutex> resolve_lock(_resolve_mutex);
        boost::lock_guard<boost::mutex> lock(_mutex);
        EX_LOG(0, "resolve_to (overridden to resolve_all)");
        // Do a full resolve of the graph
        // Not optimizing the traversal using node_name to reduce experts complexity
        _resolve_helper("", "", false);
    }

    dag_vertex_t& retrieve(const std::string& name) const
    {
        try {
            expert_graph_t::vertex_descriptor vertex = _lookup_vertex(name);
            return _get_vertex(vertex);
        } catch(std::exception&) {
            throw uhd::lookup_error("failed to find node " + name + " in expert graph");
        }
    }

    const dag_vertex_t& lookup(const std::string& name) const
    {
        return retrieve(name);
    }

    const node_retriever_t& node_retriever() const
    {
        return *this;
    }

    std::string to_dot() const
    {
        static const std::string DATA_SHAPE("ellipse");
        static const std::string WORKER_SHAPE("box");

        std::string dot_str;
        dot_str += "digraph uhd_experts_" + _name + " {\n rankdir=LR;\n";
        // Iterate through the vertices and print them out
        for (std::pair<vertex_iter, vertex_iter> vi = boost::vertices(_expert_dag);
             vi.first != vi.second;
             ++vi.first
        ) {
            const dag_vertex_t& vertex = _get_vertex(*vi.first);
            if (vertex.get_class() != CLASS_WORKER) {
                dot_str += str(boost::format(" %d [label=\"%s\",shape=%s,xlabel=\"%s\"];\n") %
                               uint32_t(*vi.first) % vertex.get_name() %
                               DATA_SHAPE % vertex.get_dtype());
            } else {
                dot_str += str(boost::format(" %d [label=\"%s\",shape=%s];\n") %
                               uint32_t(*vi.first) % vertex.get_name() % WORKER_SHAPE);
            }
        }

        // Iterate through the edges and print them out
        for (std::pair<edge_iter, edge_iter> ei = boost::edges(_expert_dag);
             ei.first != ei.second;
             ++ei.first
        ) {
            dot_str += str(boost::format(" %d -> %d;\n") %
                           uint32_t(boost::source(*(ei.first), _expert_dag)) %
                           uint32_t(boost::target(*(ei.first), _expert_dag)));
        }
        dot_str += "}\n";
        return dot_str;
    }

    void debug_audit() const
    {
#ifdef UHD_EXPERT_LOGGING
        EX_LOG(0, "debug_audit()");

        //Test 1: Check for cycles in graph
        std::vector<std::string> back_edges;
        cycle_det_visitor cdet_vis(back_edges);
        boost::depth_first_search(_expert_dag, boost::visitor(cdet_vis));
        if (back_edges.empty()) {
            EX_LOG(1, "cycle check ... PASSED");
        } else {
            EX_LOG(1, "cycle check ... ERROR!!!");
            for(const std::string& e:  back_edges) {
                EX_LOG(2, "back edge: " + e);
            }
        }
        back_edges.clear();

        //Test 2: Check data node input and output edges
        std::vector<std::string> data_node_issues;
        for(const vertex_map_t::value_type& v:  _datanode_map) {
            size_t in_count = 0, out_count = 0;
            for (std::pair<edge_iter, edge_iter> ei = boost::edges(_expert_dag);
                 ei.first != ei.second;
                 ++ei.first
            ) {
                if (boost::target(*(ei.first), _expert_dag) == v.second)
                    in_count++;
                if (boost::source(*(ei.first), _expert_dag) == v.second)
                    out_count++;
            }
            bool prop_unused = false;
            if (in_count > 1) {
                data_node_issues.push_back(v.first + ": multiple writers (workers)");
            } else if (in_count > 0) {
                if (_expert_dag[v.second]->get_class() == CLASS_PROPERTY) {
                    data_node_issues.push_back(v.first + ": multiple writers (worker and property tree)");
                }
            } else {
                if (_expert_dag[v.second]->get_class() != CLASS_PROPERTY) {
                    data_node_issues.push_back(v.first + ": unreachable (will always hold initial value)");
                } else if (_expert_dag[v.second]->get_class() == CLASS_PROPERTY and not _expert_dag[v.second]->has_write_callback()) {
                    if (out_count > 0) {
                        data_node_issues.push_back(v.first + ": needs explicit resolve after write");
                    } else {
                        data_node_issues.push_back(v.first + ": unused (no readers or writers)");
                        prop_unused = true;
                    }
                }
            }
            if (out_count < 1) {
                if (_expert_dag[v.second]->get_class() != CLASS_PROPERTY) {
                    data_node_issues.push_back(v.first + ": unused (is not read by any worker)");
                } else if (_expert_dag[v.second]->get_class() == CLASS_PROPERTY and not _expert_dag[v.second]->has_read_callback()) {
                    if (not prop_unused) {
                        data_node_issues.push_back(v.first + ": needs explicit resolve to read");
                    }
                }
            }
        }

        if (data_node_issues.empty()) {
            EX_LOG(1, "data node check ... PASSED");
        } else {
            EX_LOG(1, "data node check ... WARNING!");
            for(const std::string& i:  data_node_issues) {
                EX_LOG(2, i);
            }
        }
        data_node_issues.clear();

        //Test 3: Check worker node input and output edges
        std::vector<std::string> worker_issues;
        for(const vertex_map_t::value_type& v:  _worker_map) {
            size_t in_count = 0, out_count = 0;
            for (std::pair<edge_iter, edge_iter> ei = boost::edges(_expert_dag);
                 ei.first != ei.second;
                 ++ei.first
            ) {
                if (boost::target(*(ei.first), _expert_dag) == v.second)
                    in_count++;
                if (boost::source(*(ei.first), _expert_dag) == v.second)
                    out_count++;
            }
            if (in_count < 1) {
                worker_issues.push_back(v.first + ": no inputs (will never resolve)");
            }
            if (out_count < 1) {
                worker_issues.push_back(v.first + ": no outputs");
            }
        }
        if (worker_issues.empty()) {
            EX_LOG(1, "worker check ... PASSED");
        } else {
            EX_LOG(1, "worker check ... WARNING!");
            for(const std::string& i:  worker_issues) {
                EX_LOG(2, i);
            }
        }
        worker_issues.clear();
#endif
    }

    inline boost::recursive_mutex& resolve_mutex() {
        return _resolve_mutex;
    }

protected:
    void add_data_node(dag_vertex_t* data_node, auto_resolve_mode_t resolve_mode)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        //Sanity check node pointer
        if (data_node == NULL) {
            throw uhd::runtime_error("NULL data node passed into expert container for registration.");
        }

        //Sanity check the data node and ensure that it is not already in this graph
        EX_LOG(0, str(boost::format("add_data_node(%s)") % data_node->get_name()));
        if (data_node->get_class() == CLASS_WORKER) {
            throw uhd::runtime_error("Supplied node " + data_node->get_name() + " is not a data/property node.");
            // Throw leaves data_node undeleted
        }
        if (_datanode_map.find(data_node->get_name()) != _datanode_map.end()) {
            throw uhd::runtime_error("Data node with name " + data_node->get_name() + " already exists");
            // Throw leaves data node undeleted
        }

        try {
            //Add a vertex in this graph for the data node
            expert_graph_t::vertex_descriptor gr_node = boost::add_vertex(data_node, _expert_dag);
            EX_LOG(1, str(boost::format("added vertex %s") % data_node->get_name()));
            _datanode_map.insert(vertex_map_t::value_type(data_node->get_name(), gr_node));

            //Add resolve callbacks
            if (resolve_mode == AUTO_RESOLVE_ON_WRITE or resolve_mode == AUTO_RESOLVE_ON_READ_WRITE) {
                EX_LOG(2, str(boost::format("added write callback")));
                data_node->set_write_callback(boost::bind(&expert_container_impl::resolve_from, this, _1));
            }
            if (resolve_mode == AUTO_RESOLVE_ON_READ or resolve_mode == AUTO_RESOLVE_ON_READ_WRITE) {
                EX_LOG(2, str(boost::format("added read callback")));
                data_node->set_read_callback(boost::bind(&expert_container_impl::resolve_to, this, _1));
            }
        } catch (...) {
            clear();
            throw uhd::assertion_error("Unknown unrecoverable error adding data node. Cleared expert container.");
        }
    }

    void add_worker(worker_node_t* worker)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        //Sanity check node pointer
        if (worker == NULL) {
            throw uhd::runtime_error("NULL worker passed into expert container for registration.");
        }

        //Sanity check the data node and ensure that it is not already in this graph
        EX_LOG(0, str(boost::format("add_worker(%s)") % worker->get_name()));
        if (worker->get_class() != CLASS_WORKER) {
            throw uhd::runtime_error("Supplied node " + worker->get_name() + " is not a worker node.");
        }
        if (_worker_map.find(worker->get_name()) != _worker_map.end()) {
            throw uhd::runtime_error("Resolver with name " + worker->get_name() + " already exists.");
        }

        try {
            //Add a vertex in this graph for the worker node
            expert_graph_t::vertex_descriptor gr_node = boost::add_vertex(worker, _expert_dag);
            EX_LOG(1, str(boost::format("added vertex %s") % worker->get_name()));
            _worker_map.insert(vertex_map_t::value_type(worker->get_name(), gr_node));

            //For each input, add an edge from the input to this node
            for(const std::string& node_name:  worker->get_inputs()) {
                vertex_map_t::const_iterator node = _datanode_map.find(node_name);
                if (node != _datanode_map.end()) {
                    boost::add_edge((*node).second, gr_node, _expert_dag);
                    EX_LOG(1, str(boost::format("added edge %s->%s") % _expert_dag[(*node).second]->get_name() % _expert_dag[gr_node]->get_name()));
                } else {
                    throw uhd::runtime_error("Data node with name " + node_name + " was not found");
                }
            }

            //For each output, add an edge from this node to the output
            for(const std::string& node_name:  worker->get_outputs()) {
                vertex_map_t::const_iterator node = _datanode_map.find(node_name);
                if (node != _datanode_map.end()) {
                    boost::add_edge(gr_node, (*node).second, _expert_dag);
                    EX_LOG(1, str(boost::format("added edge %s->%s") % _expert_dag[gr_node]->get_name() % _expert_dag[(*node).second]->get_name()));
                } else {
                    throw uhd::runtime_error("Data node with name " + node_name + " was not found");
                }
            }
        } catch (uhd::runtime_error& ex) {
            clear();
            //Promote runtime_error to assertion_error
            throw uhd::assertion_error(std::string(ex.what()) + " (Cleared expert container because error is unrecoverable).");
        } catch (...) {
            clear();
            throw uhd::assertion_error("Unknown unrecoverable error adding worker. Cleared expert container.");
        }
    }

    void clear()
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        EX_LOG(0, "clear()");

        // Iterate through the vertices and release their node storage
        typedef boost::graph_traits<expert_graph_t>::vertex_iterator vertex_iter;
        for (std::pair<vertex_iter, vertex_iter> vi = boost::vertices(_expert_dag);
             vi.first != vi.second;
             ++vi.first
        ) {
            try {
                delete _expert_dag[*vi.first];
                _expert_dag[*vi.first] = NULL;
            } catch (...) {
                //If a dag_vertex is a worker, it has a virtual dtor which
                //can possibly throw an exception. We will not let that
                //terminate clear() and leave things in a bad state.
            }
        }

        //The following calls will not throw because they all contain
        //intrinsic types.

        // Release all vertices and edges in the DAG
        _expert_dag.clear();

        // Release all nodes in the map
        _worker_map.clear();
        _datanode_map.clear();
    }

private:
    void _resolve_helper(std::string start, std::string stop, bool force)
    {
        //Sort the graph topologically. This ensures that for all dependencies, the dependant
        //is always after all of its dependencies.
        node_queue_t sorted_nodes;
        try {
            boost::topological_sort(_expert_dag, std::front_inserter(sorted_nodes));
        } catch (boost::not_a_dag&) {
            std::vector<std::string> back_edges;
            cycle_det_visitor cdet_vis(back_edges);
            boost::depth_first_search(_expert_dag, boost::visitor(cdet_vis));
            if (not back_edges.empty()) {
                std::string edges;
                for(const std::string& e:  back_edges) {
                    edges += "* " + e + "";
                }
                throw uhd::runtime_error("Cannot resolve expert because it has at least one cycle!\n"
                                         "The following back-edges were found:" + edges);
            }
        }
        if (sorted_nodes.empty()) return;

        //Determine the start and stop node. If one is not explicitly specified then
        //resolve everything
        expert_graph_t::vertex_descriptor start_vertex = sorted_nodes.front();
        expert_graph_t::vertex_descriptor stop_vertex = sorted_nodes.back();
        if (not start.empty()) start_vertex = _lookup_vertex(start);
        if (not stop.empty()) stop_vertex = _lookup_vertex(stop);

        //First Pass: Resolve all nodes if they are dirty, in a topological order
        std::list<dag_vertex_t*> resolved_workers;
        bool start_node_encountered = false;
        for (node_queue_t::iterator node_iter = sorted_nodes.begin();
             node_iter != sorted_nodes.end();
             ++node_iter
        ) {
            //Determine if we are at or beyond the starting node
            if (*node_iter == start_vertex) start_node_encountered = true;

            //Only resolve if the starting node has passed
            if (start_node_encountered) {
                dag_vertex_t& node = _get_vertex(*node_iter);
                std::string node_val;
                if (force or node.is_dirty()) {
                    node.resolve();
                    if (node.get_class() == CLASS_WORKER) {
                        resolved_workers.push_back(&node);
                    }
                    EX_LOG(1, str(boost::format("resolved node %s (%s) [%s]") %
                                    node.get_name() % (node.is_dirty()?"dirty":"clean") % node.to_string()));
                } else {
                    EX_LOG(1, str(boost::format("skipped node %s (%s) [%s]") %
                                    node.get_name() % (node.is_dirty()?"dirty":"clean") % node.to_string()));
                }
            }

            //Determine if we are beyond the stop node
            if (*node_iter == stop_vertex) break;
        }

        //Second Pass: Mark all the workers clean. The policy is that a worker will mark all of
        //its dependencies clean so after this step all data nodes that are not consumed by a worker
        //will remain dirty (as they should because no one has consumed their value)
        for (std::list<dag_vertex_t*>::iterator worker = resolved_workers.begin();
             worker != resolved_workers.end();
             ++worker
        ) {
            (*worker)->mark_clean();
        }
    }

    expert_graph_t::vertex_descriptor _lookup_vertex(const std::string& name) const
    {
        expert_graph_t::vertex_descriptor vertex;
        //Look for node in the data-node map
        vertex_map_t::const_iterator vertex_iter = _datanode_map.find(name);
        if (vertex_iter != _datanode_map.end()) {
            vertex = (*vertex_iter).second;
        } else {
            //If not found, look in the worker-node map
            vertex_iter = _worker_map.find(name);
            if (vertex_iter != _worker_map.end()) {
                vertex = (*vertex_iter).second;
            } else {
                throw uhd::lookup_error("Could not find node with name " + name);
            }
        }
        return vertex;
    }

    dag_vertex_t& _get_vertex(expert_graph_t::vertex_descriptor desc) const {
        //Requirement: Node must exist in expert graph
        dag_vertex_t* vertex_ptr = _expert_dag[desc];
        if (vertex_ptr) {
            return *vertex_ptr;
        } else {
            throw uhd::assertion_error("Expert graph malformed. Found a NULL node.");
        }
    }

    void _log(size_t depth, const std::string& str) const
    {
        std::string indents;
        for (size_t i = 0; i < depth; i++) indents += "- ";
        UHD_LOG_DEBUG("EXPERT","[expert::" + _name + "] " << indents << str)
    }

private:
    const std::string       _name;
    expert_graph_t          _expert_dag;        //The primary graph data structure as an adjacency list
    vertex_map_t            _worker_map;        //A map from vertex name to vertex descriptor for workers
    vertex_map_t            _datanode_map;      //A map from vertex name to vertex descriptor for data nodes
    boost::mutex            _mutex;
    boost::recursive_mutex  _resolve_mutex;
};

expert_container::sptr expert_container::make(const std::string& name)
{
    return boost::make_shared<expert_container_impl>(name);
}

}}
