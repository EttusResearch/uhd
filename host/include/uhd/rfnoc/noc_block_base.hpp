//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_NOC_BLOCK_BASE_HPP
#define INCLUDED_LIBUHD_NOC_BLOCK_BASE_HPP

#include <uhd/config.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/node.hpp>
#include <uhd/rfnoc/register_iface_holder.hpp>

//! Shorthand for block constructor
#define RFNOC_BLOCK_CONSTRUCTOR(CLASS_NAME) \
    CLASS_NAME##_impl(make_args_ptr make_args) : CLASS_NAME(std::move(make_args))

#define RFNOC_DECLARE_BLOCK(CLASS_NAME) \
    using sptr = std::shared_ptr<CLASS_NAME>;\
    CLASS_NAME(make_args_ptr make_args) : noc_block_base(std::move(make_args)) {}

namespace uhd { namespace rfnoc {

class clock_iface;

/*!
 * The primary interface to a NoC block in the FPGA
 *
 * The block supports three types of data access:
 * - Low-level register access
 * - High-level property access
 * - Action execution
 *
 * The main difference between this class and its parent is the direct access to
 * registers, and the NoC&block IDs.
 */
class UHD_API noc_block_base : public node_t, public register_iface_holder
{
public:
    /*! A shared pointer to allow easy access to this class and for
     *  automatic memory management.
     */
    using sptr = std::shared_ptr<noc_block_base>;

    /*! The NoC ID is the unique identifier of the block type. All blocks of the
     * same type have the same NoC ID.
     */
    using noc_id_t = uint32_t;

    //! Forward declaration for the constructor arguments
    struct make_args_t;

    //! Opaque pointer to the constructor arguments
    using make_args_ptr = std::unique_ptr<make_args_t>;

    virtual ~noc_block_base();

    /**************************************************************************
     * node_t API calls
     *************************************************************************/
    //! Unique ID for an RFNoC block is its block ID
    std::string get_unique_id() const { return get_block_id().to_string(); }

    //! Number of input ports. Note: This gets passed into this block from the
    // information stored in the global register space.
    size_t get_num_input_ports() const { return _num_input_ports; }

    //! Number of output ports. Note: This gets passed outto this block from the
    // information stored in the global register space.
    size_t get_num_output_ports() const { return _num_output_ports; }

    /**************************************************************************
     * RFNoC-block specific API calls
     *************************************************************************/
    /*! Return the NoC ID for this block.
     *
     * \return noc_id The 32-bit NoC ID of this block
     */
    noc_id_t get_noc_id() const { return _noc_id; }

    /*! Returns the unique block ID for this block.
     *
     * \return block_id The block ID of this block (e.g. "0/FFT#1")
     */
    const block_id_t& get_block_id() const { return _block_id; }

    /*! Returns the tick rate of the current time base
     *
     * Note there is only ever one time base (or tick rate) per block.
     */
    double get_tick_rate() const { return _tick_rate; }

protected:
    noc_block_base(make_args_ptr make_args);

    /*! Update tick rate for this node and all the connected nodes
     *
     * Careful: Calling this function will trigger a property propagation to any
     * block this block is connected to.
     */
    void set_tick_rate(const double tick_rate);

private:
    /*! Update the tick rate of this block
     *
     * This will make sure that the underlying register_iface is notified of the
     * change in timebase.
     */
    void _set_tick_rate(const double tick_rate);

    //! This block's Noc-ID
    noc_id_t _noc_id;

    //! This block's block-ID
    //
    // The framework will guarantee that no one else has the same block ID
    block_id_t _block_id;

    //! Number of input ports
    size_t _num_input_ports;

    //! Number of output ports
    size_t _num_output_ports;

    //! Container for the 'tick rate' property. This will hold one edge property
    // for all in- and output edges.
    std::vector<property_t<double>> _tick_rate_props;

    //! The actual tick rate of the current time base
    double _tick_rate;

    std::shared_ptr<clock_iface> _clock_iface;

}; // class noc_block_base

}} /* namespace uhd::rfnoc */

#include <uhd/rfnoc/noc_block_make_args.hpp>

#endif /* INCLUDED_LIBUHD_NOC_BLOCK_BASE_HPP */
