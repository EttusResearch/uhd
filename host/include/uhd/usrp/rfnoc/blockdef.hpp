//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_BLOCKDEF_HPP
#define INCLUDED_LIBUHD_RFNOC_BLOCKDEF_HPP

#include <boost/cstdint.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <uhd/config.hpp>
#include <vector>

namespace uhd { namespace rfnoc {

/*! Reads and stores block definitions for blocks and components.
 */
class UHD_API blockdef : public boost::enable_shared_from_this<blockdef>
{
public:
    typedef boost::shared_ptr<blockdef> sptr;

    //! Describes port options for a block definition.
    //
    // This is not the same as a uhd::rfnoc::stream_sig_t. This is used
    // to describe which ports are defined in a block definition, and
    // to describe what kind of connection is allowed for this port.
    struct port_t {
        std::string name;
        //! A list of valid data types
        std::vector<std::string> types;
        //! If true, this port does not need to be connected in order
        // for the block to operate.
        bool optional;

        port_t(
            const std::string &name_,
            const std::vector<std::string> &types_=std::vector<std::string>(),
            const bool optional_=false
        ) : name(name_), types(types_), optional(optional_) {};

        //! Returns true if \p type matches with this port definition.
        //
        // This is true if:
        // - \p type is in \p types
        // - \p Either type or \p types is empty
        bool match_type(const std::string &type);
    };

    typedef std::vector<port_t> ports_t;

    /*! Create a block definition object for a NoC block given
     * a NoC ID. This cannot be used for components.
     *
     * Note: If nothing is found, returns an
     * empty sptr. Does not throw.
     */
    static sptr make_from_noc_id(boost::uint64_t noc_id);

    //! Returns true if this represents a NoC block
    virtual bool is_block() const = 0;

    //! Returns true if this represents a component
    virtual bool is_component() const = 0;

    //! For blocks, returns the block name. For components, returns it's canonical name.
    virtual std::string get_name() const = 0;

    //! Return the one NoC that is valid for this block
    virtual boost::uint64_t noc_id() const = 0;

    virtual ports_t get_input_ports() = 0;
    virtual ports_t get_output_ports() = 0;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_BLOCKDEF_HPP */
// vim: sw=4 et:
