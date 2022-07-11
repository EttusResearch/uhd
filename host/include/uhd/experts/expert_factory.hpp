//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/experts/expert_container.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <functional>
#include <memory>


namespace uhd { namespace experts {

/*!
 * expert_factory is a friend of expert_container and
 * handles all operations to create and change the structure of
 * the an expert container.
 * The expert_factory allocates storage for the nodes in the
 * expert_container and passes allocated objects to the container
 * using private APIs. The expert_container instance owns all
 * data and workernodes and is responsible for releasing their
 * storage on destruction.
 *
 */
class UHD_API expert_factory : public uhd::noncopyable
{
public:
    /*!
     * Creates an empty instance of expert_container with the
     * specified name.
     *
     * \param name Name of the container
     */
    static expert_container::sptr create_container(const std::string& name);

    /*!
     * Add a data node to the expert graph.
     *
     * \param container A shared pointer to the container to add the node to
     * \param name The name of the data node
     * \param init_val The initial value of the data node
     * \param mode The auto resolve mode
     *
     * Requirements for data_t
     * - Must have a default constructor
     * - Must have a copy constructor
     * - Must have an assignment operator (=)
     * - Must have an equality operator (==)
     */
    template <typename data_t>
    inline static void add_data_node(expert_container::sptr container,
        const std::string& name,
        const data_t& init_val,
        const auto_resolve_mode_t mode = AUTO_RESOLVE_OFF)
    {
        container->add_data_node(new data_node_t<data_t>(name, init_val), mode);
    }

    /*!
     * Add a expert property to a property tree AND an expert graph
     *
     * The underlying property can be used like any other property tree property,
     * including setting a coercer through set_coercer(). However, this means
     * that the coercion is happening outside of the expert framework. This is
     * primarily useful for tiny coercions (e.g., we accept both upper and lower
     * case values, but only want lower case downstream) for which we don't want
     * to bother with a full expert, or for which we don't want to trigger
     * resolution at all (if mode is set to AUTO_RESOLVE_OFF).
     * For more full-fledged coercion, prefer add_dual_prop_node(). This will
     * properly engage the expert graph.
     *
     * \param container A shared pointer to the expert container to add the node to
     * \param subtree A shared pointer to subtree to add the property to
     * \param path The path of the property in the subtree
     * \param name The name of the data node in the expert graph
     * \param init_val The initial value of the data node
     * \param mode The auto resolve mode
     *
     * Requirements for data_t
     * - Must have a default constructor
     * - Must have a copy constructor
     * - Must have an assignment operator (=)
     * - Must have an equality operator (==)
     */
    template <typename data_t>
    inline static property<data_t>& add_prop_node(expert_container::sptr container,
        property_tree::sptr subtree,
        const fs_path& path,
        const std::string& name,
        const data_t& init_val,
        const auto_resolve_mode_t mode = AUTO_RESOLVE_OFF)
    {
        property<data_t>& prop =
            subtree->create<data_t>(path, property_tree::AUTO_COERCE);
        data_node_t<data_t>* node_ptr =
            new data_node_t<data_t>(name, init_val, &container->resolve_mutex());
        prop.set(init_val);
        prop.add_coerced_subscriber(
            std::bind(&data_node_t<data_t>::commit, node_ptr, std::placeholders::_1));
        prop.set_publisher(std::bind(&data_node_t<data_t>::retrieve, node_ptr));
        container->add_data_node(node_ptr, mode);
        return prop;
    }

    /*!
     * Add a expert property to a property tree AND an expert graph.
     * The property is registered with the path as the identifier for
     * both the property subtree and the expert container
     *
     * \param container A shared pointer to the expert container to add the node to
     * \param subtree A shared pointer to subtree to add the property to
     * \param path The path of the property in the subtree
     * \param init_val The initial value of the data node
     * \param mode The auto resolve mode
     *
     */
    template <typename data_t>
    inline static property<data_t>& add_prop_node(expert_container::sptr container,
        property_tree::sptr subtree,
        const fs_path& path,
        const data_t& init_val,
        const auto_resolve_mode_t mode = AUTO_RESOLVE_OFF)
    {
        return add_prop_node(container, subtree, path, path, init_val, mode);
    }

    /*!
     * Add a dual expert property to a property tree AND an expert graph.
     * A dual property is a desired and coerced value pair
     *
     * \param container A shared pointer to the expert container to add the node to
     * \param subtree A shared pointer to subtree to add the property to
     * \param path The path of the property in the subtree
     * \param desired_name The name of the desired data node in the expert graph
     * \param coerced_name The name of the coerced data node in the expert graph
     * \param init_val The initial value of both the data nodes
     * \param mode The auto resolve mode
     *
     * Requirements for data_t
     * - Must have a default constructor
     * - Must have a copy constructor
     * - Must have an assignment operator (=)
     * - Must have an equality operator (==)
     */
    template <typename data_t>
    inline static property<data_t>& add_dual_prop_node(expert_container::sptr container,
        property_tree::sptr subtree,
        const fs_path& path,
        const std::string& desired_name,
        const std::string& coerced_name,
        const data_t& init_val,
        const auto_resolve_mode_t mode = AUTO_RESOLVE_OFF)
    {
        bool auto_resolve_desired =
            (mode == AUTO_RESOLVE_ON_WRITE or mode == AUTO_RESOLVE_ON_READ_WRITE);
        bool auto_resolve_coerced =
            (mode == AUTO_RESOLVE_ON_READ or mode == AUTO_RESOLVE_ON_READ_WRITE);

        property<data_t>& prop =
            subtree->create<data_t>(path, property_tree::MANUAL_COERCE);
        data_node_t<data_t>* desired_node_ptr =
            new data_node_t<data_t>(desired_name, init_val, &container->resolve_mutex());
        data_node_t<data_t>* coerced_node_ptr =
            new data_node_t<data_t>(coerced_name, init_val, &container->resolve_mutex());
        prop.set(init_val);
        prop.set_coerced(init_val);
        prop.add_desired_subscriber(std::bind(
            &data_node_t<data_t>::commit, desired_node_ptr, std::placeholders::_1));
        prop.set_publisher(std::bind(&data_node_t<data_t>::retrieve, coerced_node_ptr));

        container->add_data_node(desired_node_ptr,
            auto_resolve_desired ? AUTO_RESOLVE_ON_WRITE : AUTO_RESOLVE_OFF);
        container->add_data_node(coerced_node_ptr,
            auto_resolve_coerced ? AUTO_RESOLVE_ON_READ : AUTO_RESOLVE_OFF);
        return prop;
    }

    /*!
     * Add a dual expert property to a property tree AND an expert graph.
     * A dual property is a desired and coerced value pair
     * The property is registered with path/desired as the desired node
     * name and path/coerced as the coerced node name
     *
     * \param container A shared pointer to the expert container to add the node to
     * \param subtree A shared pointer to subtree to add the property to
     * \param path The path of the property in the subtree
     * \param init_val The initial value of both the data nodes
     * \param mode The auto resolve mode
     *
     */
    template <typename data_t>
    inline static property<data_t>& add_dual_prop_node(expert_container::sptr container,
        property_tree::sptr subtree,
        const fs_path& path,
        const data_t& init_val,
        const auto_resolve_mode_t mode = AUTO_RESOLVE_OFF)
    {
        return add_dual_prop_node(container,
            subtree,
            path,
            path + "/desired",
            path + "/coerced",
            init_val,
            mode);
    }

    /*!
     * Add a worker node to the expert graph.
     * The expert_container owns and manages storage for the worker
     *
     * \tparam worker_t Data type of the worker class
     *
     * \param container A shared pointer to the container to add the node to
     *
     */
    template <typename worker_t>
    inline static void add_worker_node(expert_container::sptr container)
    {
        container->add_worker(new worker_t());
    }

    /*!
     * Add a worker node to the expert graph.
     * The expert_container owns and manages storage for the worker
     *
     * \tparam worker_t Data type of the worker class
     * \tparam arg1_t Data type of the first argument to the constructor
     * \tparam ...
     * \tparam argN_t Data type of the Nth argument to the constructor
     *
     * \param container A shared pointer to the container to add the node to
     * \param arg1 First arg to ctor
     * \param ...
     *
     */
    template <typename worker_t, typename arg1_t>
    inline static void add_worker_node(
        expert_container::sptr container, arg1_t const& arg1)
    {
        container->add_worker(new worker_t(arg1));
    }

    template <typename worker_t, typename arg1_t, typename arg2_t>
    inline static void add_worker_node(
        expert_container::sptr container, arg1_t const& arg1, arg2_t const& arg2)
    {
        container->add_worker(new worker_t(arg1, arg2));
    }

    template <typename worker_t, typename arg1_t, typename arg2_t, typename arg3_t>
    inline static void add_worker_node(expert_container::sptr container,
        arg1_t const& arg1,
        arg2_t const& arg2,
        arg3_t const& arg3)
    {
        container->add_worker(new worker_t(arg1, arg2, arg3));
    }

    template <typename worker_t,
        typename arg1_t,
        typename arg2_t,
        typename arg3_t,
        typename arg4_t>
    inline static void add_worker_node(expert_container::sptr container,
        arg1_t const& arg1,
        arg2_t const& arg2,
        arg3_t const& arg3,
        arg4_t const& arg4)
    {
        container->add_worker(new worker_t(arg1, arg2, arg3, arg4));
    }

    template <typename worker_t,
        typename arg1_t,
        typename arg2_t,
        typename arg3_t,
        typename arg4_t,
        typename arg5_t>
    inline static void add_worker_node(expert_container::sptr container,
        arg1_t const& arg1,
        arg2_t const& arg2,
        arg3_t const& arg3,
        arg4_t const& arg4,
        arg5_t const& arg5)
    {
        container->add_worker(new worker_t(arg1, arg2, arg3, arg4, arg5));
    }

    template <typename worker_t,
        typename arg1_t,
        typename arg2_t,
        typename arg3_t,
        typename arg4_t,
        typename arg5_t,
        typename arg6_t>
    inline static void add_worker_node(expert_container::sptr container,
        arg1_t const& arg1,
        arg2_t const& arg2,
        arg3_t const& arg3,
        arg4_t const& arg4,
        arg5_t const& arg5,
        arg6_t const& arg6)
    {
        container->add_worker(new worker_t(arg1, arg2, arg3, arg4, arg5, arg6));
    }

    template <typename worker_t,
        typename arg1_t,
        typename arg2_t,
        typename arg3_t,
        typename arg4_t,
        typename arg5_t,
        typename arg6_t,
        typename arg7_t>
    inline static void add_worker_node(expert_container::sptr container,
        arg1_t const& arg1,
        arg2_t const& arg2,
        arg3_t const& arg3,
        arg4_t const& arg4,
        arg5_t const& arg5,
        arg6_t const& arg6,
        arg7_t const& arg7)
    {
        container->add_worker(new worker_t(arg1, arg2, arg3, arg4, arg5, arg6, arg7));
    }

    template <typename worker_t,
        typename arg1_t,
        typename arg2_t,
        typename arg3_t,
        typename arg4_t,
        typename arg5_t,
        typename arg6_t,
        typename arg7_t,
        typename arg8_t>
    inline static void add_worker_node(expert_container::sptr container,
        arg1_t const& arg1,
        arg2_t const& arg2,
        arg3_t const& arg3,
        arg4_t const& arg4,
        arg5_t const& arg5,
        arg6_t const& arg6,
        arg7_t const& arg7,
        arg7_t const& arg8)
    {
        container->add_worker(
            new worker_t(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
    }
};
}} // namespace uhd::experts
