//
// Copyright 2014-2016 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_LIBUHD_NODE_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_NODE_CTRL_BASE_HPP

#include <uhd/types/device_addr.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhd/utils/log.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <map>
#include <set>

namespace uhd {
    namespace rfnoc {

#define UHD_RFNOC_BLOCK_TRACE() UHD_LOGV(never) << "[" << unique_id() << "] "

/*! \brief Abstract base class for streaming nodes.
 *
 */
class UHD_RFNOC_API node_ctrl_base;
class node_ctrl_base : boost::noncopyable, public boost::enable_shared_from_this<node_ctrl_base>
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<node_ctrl_base> sptr;
    typedef boost::weak_ptr<node_ctrl_base> wptr;
    typedef std::map< size_t, wptr > node_map_t;
    typedef std::pair< size_t, wptr > node_map_pair_t;

    /***********************************************************************
     * Node control
     **********************************************************************/
    //! Returns a unique string that identifies this block.
    virtual std::string unique_id() const;

    /***********************************************************************
     * Connections
     **********************************************************************/
    /*! Clears the list of connected nodes.
     */
    virtual void clear();

    node_map_t list_downstream_nodes() { return _downstream_nodes; };
    node_map_t list_upstream_nodes() { return _upstream_nodes; };

    // TODO we need a more atomic connect procedure, this is too error-prone.

    /*! For an existing connection, store the remote port number.
     *
     * \throws uhd::value_error if \p this_port is not connected.
     */
    void set_downstream_port(const size_t this_port, const size_t remote_port);

    /*! Return the remote port of a connection on a given port.
     *
     * \throws uhd::value_error if \p this_port is not connected.
     */
    size_t get_downstream_port(const size_t this_port);

    /*! For an existing connection, store the remote port number.
     *
     * \throws uhd::value_error if \p this_port is not connected.
     */
    void set_upstream_port(const size_t this_port, const size_t remote_port);

    /*! Return the remote port of a connection on a given port.
     *
     * \throws uhd::value_error if \p this_port is not connected.
     */
    size_t get_upstream_port(const size_t this_port);

    /*! Find nodes downstream that match a predicate.
     *
     * Uses a non-recursive breadth-first search algorithm.
     * On every branch, the search stops if a block matches.
     * See this example:
     * <pre>
     * A -> B -> C -> C
     * </pre>
     * Say node A searches for nodes of type C. It will only find the
     * first 'C' block, not the second.
     *
     * Returns blocks that are of type T.
     *
     * Search only goes downstream.
     */
    template <typename T>
    UHD_INLINE std::vector< boost::shared_ptr<T> > find_downstream_node()
    {
        return _find_child_node<T, true>();
    }

    /*! Same as find_downstream_node(), but only search upstream.
     */
    template <typename T>
    UHD_INLINE std::vector< boost::shared_ptr<T> > find_upstream_node()
    {
        return _find_child_node<T, false>();
    }

    /*! Checks if downstream nodes share a common, unique property.
     *
     * This will use find_downstream_node() to find all nodes downstream of
     * this that are of type T. Then it will use \p get_property to return a
     * property from all of them. If all these properties are identical, it will
     * return that property. Otherwise, it will throw a uhd::runtime_error.
     *
     * \p get_property A functor to return the property from a node
     * \p null_value If \p get_property returns this value, that node is skipped.
     * \p explored_nodes A list of nodes to exclude from the search. This is typically
     *                   to avoid recursion loops.
     */
    template <typename T, typename value_type>
    UHD_INLINE value_type find_downstream_unique_property(
            boost::function<value_type(boost::shared_ptr<T> node, size_t port)> get_property,
            value_type null_value,
            const std::set< boost::shared_ptr<T> > &exclude_nodes=std::set< boost::shared_ptr<T> >()
    ) {
        return _find_unique_property<T, value_type, true>(get_property, null_value, exclude_nodes);
    }

    /*! Like find_downstream_unique_property(), but searches upstream.
     */
    template <typename T, typename value_type>
    UHD_INLINE value_type find_upstream_unique_property(
            boost::function<value_type(boost::shared_ptr<T> node, size_t port)> get_property,
            value_type null_value,
            const std::set< boost::shared_ptr<T> > &exclude_nodes=std::set< boost::shared_ptr<T> >()
    ) {
        return _find_unique_property<T, value_type, false>(get_property, null_value, exclude_nodes);
    }

protected:
    /***********************************************************************
     * Structors
     **********************************************************************/
    node_ctrl_base(void) {};
    virtual ~node_ctrl_base() {};

    /***********************************************************************
     * Protected members
     **********************************************************************/

    //! Stores default arguments
    uhd::device_addr_t _args;

    // TODO make these private

    //! List of upstream nodes
    node_map_t _upstream_nodes;

    //! List of downstream nodes
    node_map_t _downstream_nodes;

    /***********************************************************************
     * Connections
     **********************************************************************/
    /*! Registers another node as downstream of this node, connected to a given port.
     *
     * This implies that this node is a source node, and the downstream node is
     * a sink node.
     * See also uhd::rfnoc::source_node_ctrl::_register_downstream_node().
     */
    virtual void _register_downstream_node(
            node_ctrl_base::sptr downstream_node,
            size_t port
    );

    /*! Registers another node as upstream of this node, connected to a given port.
     *
     * This implies that this node is a sink node, and the upstream node is
     * a source node.
     * See also uhd::rfnoc::sink_node_ctrl::_register_upstream_node().
     */
    virtual void _register_upstream_node(
            node_ctrl_base::sptr upstream_node,
            size_t port
    );

private:
    /*! Implements the search algorithm for find_downstream_node() and
     * find_upstream_node().
     *
     * Depending on \p downstream, "child nodes" are either defined as
     * nodes connected downstream or upstream.
     *
     * \param downstream Set to true if search goes downstream, false for upstream.
     */
    template <typename T, bool downstream>
    std::vector< boost::shared_ptr<T> > _find_child_node();

    /*! Implements the search algorithm for find_downstream_unique_property() and
     * find_upstream_unique_property().
     *
     * Depending on \p downstream, "child nodes" are either defined as
     * nodes connected downstream or upstream.
     *
     * \param downstream Set to true if search goes downstream, false for upstream.
     */
    template <typename T, typename value_type, bool downstream>
    value_type _find_unique_property(
            boost::function<value_type(boost::shared_ptr<T>, size_t)> get_property,
            value_type NULL_VALUE,
            const std::set< boost::shared_ptr<T> > &exclude_nodes
    );

    /*! Stores the remote port number of a downstream connection.
     */
    std::map<size_t, size_t> _upstream_ports;

    /*! Stores the remote port number of a downstream connection.
     */
    std::map<size_t, size_t> _downstream_ports;

}; /* class node_ctrl_base */

}} /* namespace uhd::rfnoc */

#include <uhd/rfnoc/node_ctrl_base.ipp>

#endif /* INCLUDED_LIBUHD_NODE_CTRL_BASE_HPP */
// vim: sw=4 et:
