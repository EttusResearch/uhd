//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_NODE_HPP
#define INCLUDED_LIBUHD_RFNOC_NODE_HPP

#include <uhd/rfnoc/property.hpp>
#include <uhd/utils/scope_exit.hpp>
#include <uhd/utils/log.hpp>
#include <unordered_map>
#include <unordered_set>
#include <boost/optional.hpp>
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
    using resolver_fn_t = std::function<void(void)>;

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
     * \param prop_data_t The data type of the property
     * \param id The identifier of the property to write. To find out which
     *           values of \p id are valid, call get_property_ids()
     * \param instance The instance number of this property
     * \param val The new value of the property.
     */
    template <typename prop_data_t>
    void set_property(
        const std::string& id, const prop_data_t& val, const size_t instance = 0);

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
     * \param prop_data_t The data type of the property
     * \param id The identifier of the property to write.
     * \param instance The instance number of this property
     * \return The value of the property.
     * \throws uhd::lookup_error if the property can't be found.
     */
    template <typename prop_data_t>
    const prop_data_t& get_property(
        const std::string& id, const size_t instance = 0) /* mutable */;

    /******************************************
     * Action Specific
     ******************************************/
    // TBW

protected:
    /******************************************
     * Internal Registration Functions
     ******************************************/

    /*! Register a property for this block
     *
     * \param prop A reference to the property
     *
     * \throws uhd::key_error if another property with the same ID and source
     *         type is already registered
     */
    void register_property(property_base_t* prop);

    /*! Add a resolver function to this block. A resolver function is used to
     *  reconcile state changes in the block, and is triggered by a
     *  user or other upstream/downstream blocks. A block may have multiple
     *  resolvers.
     *
     *  NOTE: Multiple resolvers may share properties for reading and a
     *        resolver may read multiple properties
     *  NOTE: The framework will perform run-time validation to
     *        ensure read/write property access is not violated. This means the
     *        resolver function can only read values that are specified in the
     *        \p inputs list, and only write values that are specified in the
     *        \p outputs list.
     *
     * \param inputs The properties that this resolver will read
     * \param outputs The properties that this resolver will write to
     * \param resolver_fn The resolver function
     */
    void add_property_resolver(std::set<property_base_t*>&& inputs,
        std::set<property_base_t*>&& outputs,
        resolver_fn_t&& resolver_fn);

    /*! Handle a request to perform an action. The default action handler
     *  ignores user action and forwards port actions.
     *
     * \param handler The function that is called to handle the action
     */
    //void register_action_handler(std::function<
    //void(const action_info& info, const res_source_info& src)
    //> handler);

    /******************************************
     * Internal action forwarding
     ******************************************/
    // TBW
    //

private:

    /*! Return a reference to a property, if it exists.
     *
     * \returns A reference to the property, if it exists, or nullptr otherwise
     */
    property_base_t* _find_property(
        res_source_info src_info, const std::string& id) const;

    /*! RAII-Style property access
     */
    uhd::utils::scope_exit::uptr _request_property_access(
        property_base_t* prop, property_base_t::access_t access) const;

    /****** Attributes *******************************************************/
    //! Mutex to lock access to the property registry
    mutable std::mutex _prop_mutex;

    //! Stores a reference to every registered property (Property Registry)
    std::unordered_map<res_source_info::source_t,
        std::vector<property_base_t*>,
        std::hash<size_t> >
        _props;

    using property_resolver_t = std::tuple<std::set<property_base_t*>,
                                    std::set<property_base_t*>,
                                    resolver_fn_t> ;
    //! Stores the list of property resolvers
    std::vector<property_resolver_t> _prop_resolvers;

};  // class node_t

}} /* namespace uhd::rfnoc */

#include <uhd/rfnoc/node.ipp>

#endif /* INCLUDED_LIBUHD_RFNOC_NODE_HPP */
