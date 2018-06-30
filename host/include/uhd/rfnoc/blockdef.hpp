//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_BLOCKDEF_HPP
#define INCLUDED_LIBUHD_RFNOC_BLOCKDEF_HPP

#include <stdint.h>
#include <boost/enable_shared_from_this.hpp>
#include <uhd/config.hpp>
#include <uhd/types/device_addr.hpp>
#include <vector>
#include <set>

namespace uhd { namespace rfnoc {

/*! Reads and stores block definitions for blocks and components.
 */
class UHD_RFNOC_API blockdef : public boost::enable_shared_from_this<blockdef>
{
public:
    typedef boost::shared_ptr<blockdef> sptr;

    //! Describes port options for a block definition.
    //
    // This is not the same as a uhd::rfnoc::stream_sig_t. This is used
    // to describe which ports are defined in a block definition, and
    // to describe what kind of connection is allowed for this port.
    //
    // All the keys listed in PORT_ARGS will be available in this class.
    class port_t : public uhd::dict<std::string, std::string> {
      public:
        //! A list of args a port can have.
        static const device_addr_t PORT_ARGS;

        port_t();

        //! Checks if the value at \p key is a variable (e.g. '$fftlen')
        bool is_variable(const std::string &key) const;
        //! Checks if the value at \p key is a keyword (e.g. '%vlen')
        bool is_keyword(const std::string &key) const;
        //! Basic validity check of this port definition. Variables and
        //  keywords are not resolved.
        bool is_valid() const;
        //! Returns a string with the most important keys
        std::string to_string() const;
    };
    typedef std::vector<port_t> ports_t;

    //! Describes arguments in a block definition.
    class arg_t : public uhd::dict<std::string, std::string> {
      public:
        //! A list of args an argument can have.
        static const device_addr_t ARG_ARGS;
        static const std::set<std::string> VALID_TYPES;

        arg_t();

        //! Basic validity check of this argument definition.
        bool is_valid() const;
        //! Returns a string with the most important keys
        std::string to_string() const;

    };
    typedef std::vector<arg_t> args_t;

    typedef uhd::dict<std::string, size_t> registers_t;

    /*! Create a block definition object for a NoC block given
     * a NoC ID. This cannot be used for components.
     *
     * Note: If nothing is found, returns an
     * empty sptr. Does not throw.
     */
    static sptr make_from_noc_id(uint64_t noc_id);

    //! Returns true if this represents a NoC block
    virtual bool is_block() const = 0;

    //! Returns true if this represents a component
    virtual bool is_component() const = 0;

    //! Returns block key (i.e. what is used for the registry)
    virtual std::string get_key() const = 0;

    //! For blocks, returns the block name. For components, returns it's canonical name.
    virtual std::string get_name() const = 0;

    //! Return the one NoC that is valid for this block
    virtual uint64_t noc_id() const = 0;

    virtual ports_t get_input_ports() = 0;
    virtual ports_t get_output_ports() = 0;

    //! Returns the full list of port numbers used
    virtual std::vector<size_t> get_all_port_numbers() = 0;

    //! Returns the args for this block. Checks if args are valid.
    //
    // \throws uhd::runtime_error if args are invalid.
    virtual args_t get_args() = 0;

    //! Returns a list of settings registers by name.
    virtual registers_t get_settings_registers() = 0;

    //! Returns a list of readback (user) registers by name.
    virtual registers_t get_readback_registers() = 0;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_BLOCKDEF_HPP */
// vim: sw=4 et:
