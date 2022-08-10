//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/dirtifier.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/scope_exit.hpp>
#include <unordered_map>
#include <unordered_set>
#include <boost/graph/adjacency_list.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

namespace uhd { namespace rfnoc {

/*! The base class for all nodes within an RFNoC graph
 *
 * The block supports the following types of data access:
 * - High-level property access
 * - Action execution
 */
class UHD_API node_t
{
public:
    using resolver_fn_t      = std::function<void(void)>;
    using resolve_callback_t = std::function<void(void)>;
    using graph_mutex_callback_t = std::function<std::recursive_mutex&(void)>;
    using action_handler_t =
        std::function<void(const res_source_info&, action_info::sptr)>;
    using forwarding_map_t =
        std::unordered_map<res_source_info, std::vector<res_source_info>>;

    //! Types of property/action forwarding for those not defined by the block itself
    enum class forwarding_policy_t {
        //! Forward the property/action to the opposite port with the same index
        //(e.g., if it comes from input port 0, forward it to output port 0).
        ONE_TO_ONE,
        //! Fan-out forwarding: Forward to all opposite ports
        ONE_TO_FAN,
        //! Forward the property to all input ports
        ONE_TO_ALL_IN,
        //! Forward the property to all output ports
        ONE_TO_ALL_OUT,
        //! Forward the property to all ports
        ONE_TO_ALL,
        //! Property propagation ends here
        DROP,
        //! Forward the property based on a client-provided map
        USE_MAP
    };

    static const size_t ANY_PORT = size_t(~0);

    /**************************************************************************
     * Structors
     *************************************************************************/
    node_t();

    virtual ~node_t() {}

    /******************************************
     * Basic Operations
     ******************************************/
    //! Return a unique identifier string for this node. In every RFNoC graph,
    // no two nodes cannot have the same ID.
    //
    // \returns The unique ID as a string
    virtual std::string get_unique_id() const;

    /*! Return the number of input ports for this block.
     *
     * This function needs to be overridden.
     *
     * \return noc_id The number of ports
     */
    virtual size_t get_num_input_ports() const = 0;

    /*! Return the number of output ports for this block.
     *
     * This function needs to be overridden.
     *
     * \return noc_id The number of ports
     */
    virtual size_t get_num_output_ports() const = 0;

    /******************************************
     * Property Specific
     ******************************************/

    /*! Return the names of all possible user properties that can be
     *  accessed for this block.
     *
     * Note that the type of the property is not auto-detectable.
     *
     * \returns A vector of all possible IDs of user properties supported by
     *          this block.
     */
    std::vector<std::string> get_property_ids() const;

    /*! Set a specific user property that belongs to this block.
     *
     * Setting a user property will trigger a property resolution. This means
     * that changing this block can have effects on other nodes.
     *
     * If the property does not exist, or if the property can be determined to
     * be of a different type than \p prop_data_t due to the usage of runtime
     * type information (RTTI), a lookup_error is thrown.
     *
     * \tparam prop_data_t The data type of the property
     * \param id The identifier of the property to write. To find out which
     *           values of \p id are valid, call get_property_ids()
     * \param instance The instance number of this property
     * \param val The new value of the property.
     */
    template <typename prop_data_t>
    void set_property(
        const std::string& id, const prop_data_t& val, const size_t instance = 0);

    /*! Set multiple properties coming from a dictionary
     *
     * This is equivalent to calling set_property() individually for every
     * key/value pair of props. However, the type of the property will be
     * automatically derived using RTTI. Only certain types are supported.
     *
     * Property resolution happens after all properties have been updated.
     *
     * This function allows the client to override the \p instance parameter
     * for each property key/value pair passed in via the \p props parameter.
     * If the key consists of the property name, followed by a colon (':') and
     * then a number, the number following the colon is used to determine
     * which instance of the property this set pertains to, and the \p
     * instance parameter is ignored for that property. (Note that if the key
     * does not have the colon and instance number override syntax, then
     * \p instance is still used to determine which instance of the property
     * to set. For example, in the following call:
     *
     *     node->set_properties("dog=10,cat:2=5,bird:0=0.5", 1)
     *
     * instance 1 of node's 'dog' property is set to 10, the 1 coming from the
     * instance parameter, instance 2 of the node's 'cat' property is set to
     * 5 due to the override syntax provided in the string, and instance 0 of
     * the node's 'bird' property is set to 0.5 due to its override.
     *
     * If the instance override is malformed, that is, there is no
     * number following the colon, or the number cannot be parsed as an
     * integer, a value_error is thrown.
     *
     * If a key in \p props is not a valid property of this block, a warning is
     * logged, but no error is raised.
     */
    void set_properties(const uhd::device_addr_t& props, const size_t instance = 0);

    /*! Get the value of a specific block argument. \p The type of an argument
     *  must be known at compile time.
     *
     * If the property does not exist, or if the property can be determined to
     * be of a different type than \p prop_data_t due to the usage of runtime
     * type information (RTTI), a lookup_error is thrown.
     *
     * Note: Despite this being a "getter", this function is not declared const.
     * This is because internally, it can resolve properties, which may cause
     * changes within the object.
     *
     * \tparam prop_data_t The data type of the property
     * \param id The identifier of the property to write.
     * \param instance The instance number of this property
     * \return The value of the property.
     * \throws uhd::lookup_error if the property can't be found.
     */
    template <typename prop_data_t>
    const prop_data_t& get_property(
        const std::string& id, const size_t instance = 0) /* mutable */;

    /*! Standard API for setting the command time
     *
     * There are instances where commands need a time associated with them.
     * For example, a block could have a 'freq' user property, which should be
     * changed at a certain time. In that case, the block would have to be
     * written to handle command times.
     *
     * The reason there is no 'time' parameter in set_property() or other API
     * calls is because there is no uniform definition of what the time means;
     * it can change from block to block. The transformation of \p time to a
     * tick count, for example, is non-standard.
     *
     * The default implementation will simply stash away the time; it can be
     * retrieved by calling get_command_time();
     */
    virtual void set_command_time(uhd::time_spec_t time, const size_t instance);

    /*! Return a previously set command time
     *
     * When no time was set, this will return uhd::time_spec_t::ASAP
     */
    virtual uhd::time_spec_t get_command_time(const size_t instance) const;

    /*! Standard API for resetting the command time
     *
     * This will clear the time previously set by set_command_time(). It
     * defaults to calling set_command_time(time_spec_t(0.0), instance)
     */
    virtual void clear_command_time(const size_t instance);

protected:
    /******************************************
     * Internal Registration Functions
     ******************************************/
    using prop_ptrs_t = std::vector<property_base_t*>;

    /*! Register a property for this block
     *
     * This is typically called from the constructor. It is possible to register
     * properties later, but then the node must take care of serialization.
     *
     * This has the intentional side-effect of setting the access mode to RW for
     * the property. The idea is that after registering a property, the node
     * might need some time to settle on the default value. The access mode will
     * either be reset after the constructor is finished, or the next time
     * properties are resolved.
     *
     * For more information, see \ref props_define.
     *
     * \param prop A reference to the property
     * \param clean_callback A callback that gets executed whenever this property
     *                       is dirty and gets marked clean
     *
     * \throws uhd::key_error if another property with the same ID and source
     *         type is already registered
     */
    void register_property(
        property_base_t* prop, resolve_callback_t&& clean_callback = nullptr);

    /*! Add a resolver function to this block.
     *
     * A resolver function is used to reconcile state changes in the block, and
     * is triggered by a user or other upstream/downstream blocks. A block may
     * have multiple resolvers.
     *
     * Notes on resolvers:
     * - Multiple resolvers may share properties for reading and a resolver may
     *   read multiple properties
     * - A resolver may assume the properties are in a consistent state before
     *   it executes, but it must leave the properties in a consistent state
     *   when it completes.
     * - The framework will perform run-time validation to ensure read/write
     *   property access is not violated. All properties can be read during
     *   execution, but only properties in the \p outputs list can be written
     *   to.
     * - Resolvers are stored and executed in the same order they are added.
     *   That is to say, if two resolvers both share a condition that will
     *   trigger them, the first resolver to be added will be the first resolver
     *   to be executed. This allows to make some assumptions on the order of
     *   execution, in case resolvers have dependencies.
     * - This method has no built-in thread safety, since it is typically only
     *   called in the constructor. If resolvers need to be added at runtime
     *   (which is considered advanced usage), then the block needs to serialize
     *   access to this function itself.
     *
     * For more information, see \ref props_resolvers.
     *
     * \param inputs The properties that will cause this resolver to run
     * \param outputs The properties that this resolver will write to
     * \param resolver_fn The resolver function
     * \throws uhd::runtime_error if any of the properties listed is not
     *         registered
     */
    void add_property_resolver(
        prop_ptrs_t&& inputs, prop_ptrs_t&& outputs, resolver_fn_t&& resolver_fn);

    /**************************************************************************
     * Property forwarding
     *************************************************************************/
    /*! Set a property forwarding policy for dynamic properties
     *
     * Whenever this node is asked to handle a property that is not registered,
     * this is how the node knows what to do with the property. For example, the
     * FIFO block controller will almost always want to pass on properties to
     * the next block.
     *
     * This method can be called more than once, and it will overwrite previous
     * policies. However, once a property has been registered with this block,
     * the policy is set.
     * Typically, this function should only ever be called from within the
     * constructor.
     *
     * See also \ref props_unknown.
     *
     * \param policy The policy that is applied (see also forwarding_policy_t).
     * \param prop_id The property ID that this forwarding policy is applied to.
     *                If \p prop_id is not given, it will apply to all properties,
     *                unless a different policy was given with a matching ID.
     */
    void set_prop_forwarding_policy(
        forwarding_policy_t policy, const std::string& prop_id = "");

    /*! Specify a table that maps how a property should be forwarded
     *
     * Whenever this node is asked to handle a property that is not registered,
     * and the forwarding policy for the particular property is set to
     * USE_MAP, the node will consult a user-provided map to determine what
     * to do with the property. The map's keys are the source edges for the
     * incoming property and the value associated with each key is a vector of
     * destination edges to which the property should be propagated.
     *
     * If there is no key in the map matching an incoming property's source
     * edge, or if the value of the key is the empty vector, the property is
     * dropped and not propagated further.
     *
     * The following conditions will generate exceptions at property
     * propagation time:
     *   - Any value in the destination vector represents a non-existent port
     *
     * See also \ref props_unknown.
     *
     * \param map The map describing how properties should be propagated
     */
    void set_prop_forwarding_map(const forwarding_map_t& map);

    /*! Set a specific property that belongs to this block.
     *
     * This is like set_property(), but it also allows setting edge properties.
     * All comments from set_property() still apply.
     *
     * \tparam prop_data_t The data type of the property
     * \param id The identifier of the property to write. To find out which
     *           values of \p id are valid, call get_property_ids()
     * \param val The new value of the property.
     * \param src_info Source info of the property
     */
    template <typename prop_data_t>
    void set_property(
        const std::string& id, const prop_data_t& val, const res_source_info& src_info);

    /*! Get the value of a property.
     *
     * This is like get_property(), but it also allows reading edge properties.
     * All comments from get_property() still apply.
     *
     * \tparam prop_data_t The data type of the property
     * \param id The identifier of the property to write.
     * \param src_info Source info of this property
     * \return The value of the property.
     * \throws uhd::lookup_error if the property can't be found.
     */
    template <typename prop_data_t>
    const prop_data_t& get_property(
        const std::string& id, const res_source_info& src_info) /* mutable */;

    /******************************************
     * Internal action forwarding
     ******************************************/
    /*! Handle a request to perform an action. The default action handler
     *  ignores user action and forwards port actions.
     *
     * \param id The action ID for which this action handler is valid. The first
     *           argument to the handler will be a uhd::rfnoc::action_info::sptr,
     *           and its `id` value will match this parameter (unless the same
     *           action handler is registered multiple times).
     *           If this function was previously called with the same `id` value,
     *           the previous action handler is overwritten.
     * \param handler The function that is called to handle the action. It needs
     *                to accept a uhd::rfnoc::res_source_info object, and a
     *                uhd::rfnoc::action_info::sptr.
     */
    void register_action_handler(const std::string& id, action_handler_t&& handler);

    /*! Set an action forwarding policy
     *
     * Whenever this node is asked to handle an action that is not registered,
     * this is how the node knows what to do with the action. For example, the
     * FIFO block controller will almost always want to pass on actions to
     * the next block.
     *
     * This method can be called more than once, and it will overwrite previous
     * policies.
     * Typically, this function should only ever be called from within the
     * constructor.
     *
     * \param policy The policy that is applied (see also forwarding_policy_t).
     * \param action_key The action key that this forwarding policy is applied
     *                   to. If \p action_key is not given, it will apply to all
     *                   properties, unless a different policy was given with a
     *                   matching key.
     */
    void set_action_forwarding_policy(
        forwarding_policy_t policy, const std::string& action_key = "");

    /*! Specify a table that maps how an action should be forwarded
     *
     * Whenever this node is asked to handle an action that is not registered,
     * and the forwarding policy for the particular action is set to
     * USE_MAP, the node will consult a user-provided map to determine what
     * to do with the action. The map's keys are the source edges for the
     * incoming action and the value associated with each key is a vector of
     * destination edges to which the action should be forwarded.
     *
     * incoming action and the value is a vector of destination edges to
     * which the action should be forwarded.
     *
     * If there is no key in the map matching an incoming action's source
     * edge, or if the value of the key is the empty vector, the action is
     * dropped and not forwarded further.
     *
     * The following conditions will generate exceptions at action
     * forwarding time:
     *   - Any value in the destination vector represents a non-existent port
     *
     * \param map The map describing how actions should be forwarded
     */
    void set_action_forwarding_map(const forwarding_map_t& map);

    /*! Post an action to an up- or downstream node in the graph.
     *
     * If the action is posted to an edge which is not connected, the action
     * is lost.
     *
     * \param edge_info The edge to which this action is posted. If
     *                  edge_info.type == INPUT_EDGE, the that means the action
     *                  will be posted to an upstream node, on port edge_info.instance.
     * \param action A reference to the action info object.
     * \throws uhd::runtime_error if edge_info is not either INPUT_EDGE or OUTPUT_EDGE
     */
    void post_action(const res_source_info& edge_info, action_info::sptr action);

    /**************************************************************************
     * Graph Interaction
     *************************************************************************/
    /*! Check if the current connections "work" for this block
     *
     * The default implementation simply checks if all connections are within
     * the valid range, i.e., no \p connected_inputs element is larger than
     * get_num_input_ports(), etc. This can be overridden, but keep in mind that
     * blocks need some kind of tolerance here, because blocks may simply not
     * be part of the current application, and left unconnected. This check is
     * more meant for blocks that simply don't work of only one of two ports is
     * connected, or situations like that.
     *
     * Note that this method is always called when a graph is committed, i.e.,
     * no connections will be added or removed without calling this method
     * again, unless the user bypasses calling uhd::rfnoc_graph::commit(). This
     * method can therefore be used to make decisions about the behaviour of
     * the block.
     *
     * \param connected_inputs A list of input ports that are connected
     * \param connected_outputs A list of output ports that are connected
     * \returns true if the block can deal with this configuration
     */
    virtual bool check_topology(const std::vector<size_t>& connected_inputs,
        const std::vector<size_t>& connected_outputs);

    /*! Perform a shutdown sequence
     *
     * This is mostly relevant for noc_block_base implementations. See also
     * noc_block_base::shutdown().
     */
    virtual void shutdown();

    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! A dirtifyer object, useful for properties that always need updating.
    static dirtifier_t ALWAYS_DIRTY;

private:
    friend class node_accessor_t;

    /*! Return a reference to a property, if it exists.
     *
     * \returns A reference to the property, if it exists, or nullptr otherwise
     */
    property_base_t* _find_property(
        res_source_info src_info, const std::string& id) const;

    /*! RAII-Style property access
     *
     * Returns an object which will grant temporary \p access to the property
     * \p prop until the returned object goes out of scope.
     */
    uhd::utils::scope_exit::uptr _request_property_access(
        property_base_t* prop, property_base_t::access_t access) const;

    /*! Return a set of properties that match a predicate
     *
     * Will return an empty set if none match.
     */
    template <typename PredicateType>
    prop_ptrs_t filter_props(PredicateType&& predicate)
    {
        prop_ptrs_t filtered_props{};
        for (const auto& type_prop_pair : _props) {
            for (const auto& prop : type_prop_pair.second) {
                if (predicate(prop)) {
                    filtered_props.push_back(prop);
                }
            }
        }

        return filtered_props;
    }

    /*! Set up a new, unknown edge property
     *
     * This function is called when forward_edge_property() receives a new
     * property it doesn't know about. Using the policy set in
     * set_prop_forwarding_policy(), we figure our which resolvers to set, and
     * install them.
     */
    property_base_t* inject_edge_property(
        property_base_t* blueprint, res_source_info new_src_info);

    /*! This will run all the resolvers once to put the block into a valid
     * state. It will execute the following algorithm:
     *
     * - Iterate through all resolvers
     * - For all resolvers, mark its output properties as RWLOCKED (the
     *   assumption is that all other properties are RO).
     * - Run the resolver. If the default values were inconsistent, this can
     *   cause a uhd::resolve_error.
     * - Reset the properties to RO and continue with the next resolver.
     * - When all resolvers have been run, mark all properties as clean.
     *
     * \throws uhd::resolve_error if the default values were inconsistent
     */
    void init_props();

    /*! This will find dirty properties, and call their respective resolvers.
     *
     * It will execute the following algorithm:
     * - Create set D of all dirty properties
     * - Create empty set W
     * - Create a set R of resolvers which have the dirty properties in their
     *   input list
     * - For ever resolver:
     *   - For all outputs, set the access mode to RWLOCKED if it's in W, or to
     *     RW if it's not
     *   - Run the resolver
     *   - Reset the access modes on all outputs to RO
     *   - Add all outputs to W
     *
     * The assumption is that the properties are internally consistent before
     * this function was called.
     *
     * Note: This does not mark any properties as cleaned! All modified outputs
     * will be marked dirty.
     *
     * \throws uhd::resolve_error if the properties could not be resolved. This
     *         typically indicates that the resolvers were set up inconsistently.
     */
    void resolve_props();

    /*! This will trigger a graph-wide property resolution
     */
    void resolve_all();

    /*! Mark all properties as clean and read-only
     *
     * When dirty properties have a clean-callback registered, that will also
     * get triggered.
     */
    void clean_props();

    /*! Sets a callback that the framework can call when it needs to trigger a
     * property resolution.
     */
    void set_resolve_all_callback(resolve_callback_t&& resolver)
    {
        _resolve_all_cb = resolver;
    }

    /*! Restores the default property resolution behavior of the node.
     */
    void clear_resolve_all_callback()
    {
        _resolve_all_cb = _default_resolve_all_cb;
    }

    /*! Sets a callback that the node can call to the mutex to the graph it
     * is connected to. Used for setting properties and propagation.
     */
    void set_graph_mutex_callback(graph_mutex_callback_t&& mutex)
    {
        _graph_mutex_cb = mutex;
    }

    /*! Clears the graph mutex callback. Called when node is disconnected
     * from the graph.
     */
    void clear_graph_mutex_callback()
    {
        _graph_mutex_cb = NULL;
    }

    /*! Forward the value of an edge property into this node
     *
     * Note that \p incoming_prop is a reference to the neighbouring node's
     * property. That means if incoming_prop.get_src_info().type == OUTPUT_EDGE,
     * then this will update a property on this node with the same ID, port
     * number, but one that has source type INPUT_EDGE.
     *
     * This method is meant to be called by the framework during resolution of
     * properties, and shouldn't be called by the class itself.
     *
     * If this method is called with an unknown property, a new dynamic property
     * is created. Then, the forwarding policy is looked up to make a decision
     * what to do next:
     * - forwarding_policy_t::DROP: Nothing happens.
     * - forwarding_policy_t::ONE_TO_ONE: A new property on the opposite
     *   port is created if it doesn't yet exist. A resolver is registered that
     *   copies the value from one property to another.
     *   If there is no opposite port, then we continue as if the policy had
     *   been DROP.
     * - forwarding_policy_t::ONE_TO_ALL_IN: New properties on all input
     *   ports are created if they don't yet exist. A resolver is created that
     *   copies from this new property to all inputs.
     * - forwarding_policy_t::ONE_TO_ALL_OUT: Same as before, except the
     *   property is forwarded to the outputs.
     * - forwarding_policy_t::ONE_TO_ALL: Same as before, except the
     *   property is forwarded to all ports.
     *
     * \param incoming_prop Pointer to the other node's property that is being
     *                      forwarded. We read the value from that property, and
     *                      check the types match.
     * \param incoming_port The port on which this property is incoming.
     *
     * \throws uhd::type_error if the properties do not have the same type
     */
    void forward_edge_property(
        property_base_t* incoming_prop, const size_t incoming_port);

    /**************************************************************************
     * Action-Related Methods
     *************************************************************************/
    /*! Sets a callback that this node can call if it wants to post actions to
     * other nodes.
     */
    void set_post_action_callback(action_handler_t&& post_handler)
    {
        _post_action_cb = std::move(post_handler);
    }

    /*! This function gets called by the framework when there's a new action for
     * this node. It will then dispatch appropriate action handlers.
     *
     * \param src_info Tells us on which edge this came in. If
     *                 src_info.type == INPUT_EDGE, then we received this action
     *                 on an input edge.
     * \param action A reference to the action object
     */
    void receive_action(const res_source_info& src_info, action_info::sptr action);

    /**************************************************************************
     * Private helpers
     *************************************************************************/
    //! Return true if this node has a port that matches \p port_info
    bool _has_port(const res_source_info& port_info) const;

    //! Implementation for set_property \p id \p prop_data_t \p src_info
    template <typename prop_data_t>
    void _set_property(const std::string& id, const prop_data_t& val, const res_source_info& src_info);

    /****** Attributes *******************************************************/
    //! Mutex to lock access to the property registry. Note: This is not the
    // global property mutex, this only write-protects access to the property-
    // related containers in this class.
    mutable std::mutex _prop_mutex;

    //! Stores a reference to every registered property (Property Registry)
    std::unordered_map<res_source_info::source_t,
        std::vector<property_base_t*>,
        std::hash<size_t>>
        _props;

    //! Stores a clean callback for some properties
    std::unordered_map<property_base_t*, resolve_callback_t> _clean_cb_registry;

    using property_resolver_t = std::tuple<prop_ptrs_t, prop_ptrs_t, resolver_fn_t>;
    //! Stores the list of property resolvers
    std::vector<property_resolver_t> _prop_resolvers;

    //! A callback that the graph sets when the node is connected to graph.
    // This will return a global mutex to the graph. It is required to propagate
    // properties on multithread applications.
    graph_mutex_callback_t _graph_mutex_cb;

    //! A callback that can be called to notify the graph manager that something
    // has changed, and that a property resolution needs to be performed.
    resolve_callback_t _resolve_all_cb;

    //! This is the default implementation of the property resolution
    // method.
    const resolve_callback_t _default_resolve_all_cb = [this]() {
        resolve_props();
        clean_props();
    };


    //! This is permanent storage for all properties that don't get stored
    // explicitly.
    //
    // Dynamic properties include properties defined in the block descriptor
    // file, as well as new properties that get passed in during property
    // propagation.
    std::unordered_set<std::unique_ptr<property_base_t>> _dynamic_props;

    //! Forwarding policy for specific properties
    //
    // The entry with the empty-string-key is the default policy.
    std::unordered_map<std::string, forwarding_policy_t> _prop_fwd_policies{
        {"", forwarding_policy_t::ONE_TO_ONE}};

    //! Map describing how incoming properties should be propagated for USE_MAP
    forwarding_map_t _prop_fwd_map;

    /**************************************************************************
     * Action-related attributes
     *************************************************************************/
    mutable std::mutex _action_mutex;

    //! Storage for action handlers
    std::unordered_map<std::string, action_handler_t> _action_handlers;

    //! Default action forwarding policies
    std::unordered_map<std::string, forwarding_policy_t> _action_fwd_policies{
        {"", forwarding_policy_t::ONE_TO_ONE}};

    //! Callback which allows us to post actions to other nodes in the graph
    //
    // The default callback will simply drop actions
    action_handler_t _post_action_cb = [](const res_source_info&,
                                           action_info::sptr) { /* nop */ };

    //! Map describing how incoming actions should be forwarded for USE_MAP
    forwarding_map_t _action_fwd_map;

    /**************************************************************************
     * Other attributes
     *************************************************************************/
    std::vector<uhd::time_spec_t> _cmd_timespecs;
}; // class node_t

}} /* namespace uhd::rfnoc */

#include <uhd/rfnoc/node.ipp>
