//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_EXPERTS_EXPERT_CONTAINER_HPP
#define INCLUDED_UHD_EXPERTS_EXPERT_CONTAINER_HPP

#include <uhdlib/experts/expert_nodes.hpp>
#include <uhd/config.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace uhd { namespace experts {

    enum auto_resolve_mode_t {
        AUTO_RESOLVE_OFF,
        AUTO_RESOLVE_ON_READ,
        AUTO_RESOLVE_ON_WRITE,
        AUTO_RESOLVE_ON_READ_WRITE
    };

    class UHD_API expert_container : private boost::noncopyable, public node_retriever_t {
    public: //Methods
        typedef boost::shared_ptr<expert_container> sptr;

        virtual ~expert_container() {};

        /*!
         * Return the name of this container
         */
        virtual const std::string& get_name() const = 0;

        /*!
         * Resolves all the nodes in this expert graph.
         *
         * Dependency analysis is performed on the graph and nodes
         * are resolved in a topologically sorted order to ensure
         * that no nodes receive stale data.
         * Nodes and their dependencies are resolved only if they are
         * dirty i.e. their contained values have changed since the
         * last resolve.
         * This call requires an acyclic expert graph.
         *
         * \param force If true then ignore dirty state and resolve all nodes
         * \throws uhd::runtime_error if graph cannot be resolved
         */
        virtual void resolve_all(bool force = false) = 0;

        /*!
         * Resolves all the nodes that depend on the specified node.
         *
         * Dependency analysis is performed on the graph and nodes
         * are resolved in a topologically sorted order to ensure
         * that no nodes receive stale data.
         * Nodes and their dependencies are resolved only if they are
         * dirty i.e. their contained values have changed since the
         * last resolve.
         * This call requires an acyclic expert graph.
         *
         * \param node_name Name of the node to start resolving from
         * \throws uhd::lookup_error if node_name not in container
         * \throws uhd::runtime_error if graph cannot be resolved
         *
         */
        virtual void resolve_from(const std::string& node_name) = 0;

        /*!
         * Resolves all the specified node and all of its dependencies.
         *
         * Dependency analysis is performed on the graph and nodes
         * are resolved in a topologically sorted order to ensure
         * that no nodes receive stale data.
         * Nodes and their dependencies are resolved only if they are
         * dirty i.e. their contained values have changed since the
         * last resolve.
         * This call requires an acyclic expert graph.
         *
         * \param node_name Name of the node to resolve
         * \throws uhd::lookup_error if node_name not in container
         * \throws uhd::runtime_error if graph cannot be resolved
         *
         */
        virtual void resolve_to(const std::string& node_name) = 0;

        /*!
         * Return a node retriever object for this container
         */
        virtual const node_retriever_t& node_retriever() const = 0;

        /*!
         * Returns a DOT (graph description language) representation
         * of the expert graph. The output has labels for the node
         * name, node type (data or worker) and the underlying
         * data type for each node.
         *
         */
        virtual std::string to_dot() const = 0;

        /*!
         * Runs several sanity checks on the underlying graph to
         * flag dependency issues. Outputs of the checks are
         * logged to the console so UHD_EXPERTS_VERBOSE_LOGGING
         * must be enabled to see the results
         *
         */
        virtual void debug_audit() const = 0;

    private:
        /*!
         * Lookup a node with the specified name in the contained graph
         *
         * If the node is found, a reference to the node is returned.
         * If the node is not found, uhd::lookup_error is thrown
         * lookup can return a data or a worker node
         * \implements uhd::experts::node_retriever_t
         *
         * \param name Name of the node to find
         *
         */
        virtual const dag_vertex_t& lookup(const std::string& name) const = 0;
        virtual dag_vertex_t& retrieve(const std::string& name) const = 0;

        /*!
         * expert_factory is a friend of expert_container and
         * handles all operations that change the structure of
         * the underlying dependency graph.
         * The expert_container instance owns all data and worker
         * nodes and is responsible for release storage on destruction.
         * However, the expert_factory allocates storage for the
         * node and passes them into the expert_container using the
         * following "protected" API calls.
         *
         */
        friend class expert_factory;

        /*!
         * Creates an empty instance of expert_container with the
         * specified name.
         *
         * \param name Name of the container
         */
        static sptr make(const std::string& name);

        /*!
         * Returns a reference to the resolver mutex.
         *
         * The resolver mutex guarantees that external operations
         * to data-nodes are serialized with resolves of this
         * container.
         *
         */
        virtual boost::recursive_mutex& resolve_mutex() = 0;

        /*!
         * Add a data node to the expert graph
         *
         * \param data_node Pointer to a fully constructed data node object
         * \resolve_mode Auto resolve options: Choose from "disabled" and resolve on "read", "write" or "both"
         * \throws uhd::runtime_error if node already exists or is of a wrong type (recoverable)
         * \throws uhd::assertion_error for other failures (unrecoverable. will clear the graph)
         *
         */
        virtual void add_data_node(dag_vertex_t* data_node, auto_resolve_mode_t resolve_mode = AUTO_RESOLVE_OFF) = 0;

        /*!
         * Add a worker node to the expert graph
         *
         * \param worker Pointer to a fully constructed worker object
         * \throws uhd::runtime_error if worker already exists or is of a wrong type (recoverable)
         * \throws uhd::assertion_error for other failures (unrecoverable. will clear the graph)
         *
         */
        virtual void add_worker(worker_node_t* worker) = 0;

        /*!
         * Release all storage for this object. This will delete all contained
         * data and worker nodes and remove all dependency relationship and
         * resolve callbacks.
         *
         * The object will be restored to its newly constructed state. Will not
         * throw.
         */
        virtual void clear() = 0;
    };

}}

#endif /* INCLUDED_UHD_EXPERTS_EXPERT_CONTAINER_HPP */
