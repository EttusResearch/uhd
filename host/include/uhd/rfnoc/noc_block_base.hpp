//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/node.hpp>
#include <uhd/rfnoc/register_iface_holder.hpp>
#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhd/types/device_addr.hpp>

//! Shorthand for block constructor
#define RFNOC_BLOCK_CONSTRUCTOR(CLASS_NAME) \
    CLASS_NAME##_impl(make_args_ptr make_args) : CLASS_NAME(std::move(make_args))

#define RFNOC_DECLARE_BLOCK(CLASS_NAME)       \
    using sptr = std::shared_ptr<CLASS_NAME>; \
    CLASS_NAME(make_args_ptr make_args) : noc_block_base(std::move(make_args)) {}

namespace uhd { namespace rfnoc {

class clock_iface;
class mb_controller;

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

    //! Forward declaration for the constructor arguments
    struct make_args_t;

    //! Opaque pointer to the constructor arguments
    using make_args_ptr = std::unique_ptr<make_args_t>;

    ~noc_block_base() override;

    /**************************************************************************
     * node_t API calls
     *************************************************************************/
    //! Unique ID for an RFNoC block is its block ID
    std::string get_unique_id() const override
    {
        return get_block_id().to_string();
    }

    //! Number of input ports. Note: This gets passed into this block from the
    // information stored in the global register space.
    //
    // Note: This may be overridden by the block (e.g., the X300 radio may not
    // have all ports available if no TwinRX board is plugged in), but the
    // subclassed version may never report more ports than this.
    size_t get_num_input_ports() const override
    {
        return _num_input_ports;
    }

    //! Number of output ports. Note: This gets passed outto this block from the
    // information stored in the global register space.
    //
    // Note: This may be overridden by the block (e.g., the X300 radio may not
    // have all ports available if no TwinRX board is plugged in), but the
    // subclassed version may never report more ports than this.
    size_t get_num_output_ports() const override
    {
        return _num_output_ports;
    }

    /**************************************************************************
     * RFNoC-block specific API calls
     *************************************************************************/
    /*! Return the NoC ID for this block.
     *
     * \return noc_id The 32-bit NoC ID of this block
     */
    noc_id_t get_noc_id() const
    {
        return _noc_id;
    }

    /*! Returns the unique block ID for this block.
     *
     * \return block_id The block ID of this block (e.g. "0/FFT#1")
     */
    const block_id_t& get_block_id() const
    {
        return _block_id;
    }

    /*! Returns the tick rate of the current time base
     *
     * Note there is only ever one time base (or tick rate) per block.
     */
    double get_tick_rate() const;

    /*! Return the current MTU on a given edge
     *
     * Note: The MTU is the maximum size of a CHDR packet, including header. In
     * order to find out the maximum payload size, calling get_max_payload_size()
     * is the recommended alternative.
     *
     * The MTU is determined by the block itself (i.e., how big of a packet can
     * this block handle on this edge), but also the neighboring block, and
     * possibly the transport medium between the blocks. This value can thus be
     * lower than what the block defines as MTU, but never higher.
     *
     * \param edge The edge on which the MTU is queried. edge.type must be
     *             INPUT_EDGE or OUTPUT_EDGE!
     * \returns the MTU as determined by the overall graph on this edge
     * \throws uhd::value_error if edge is not referring to a valid edge
     */
    size_t get_mtu(const res_source_info& edge);

    /*! Return the size of a CHDR packet header, in bytes.
     *
     * This helper function factors in the CHDR width for this block.
     *
     * \param account_for_ts If true (default), the assumption is that we reserve
     *                       space for a timestamp. It is possible to increase
     *                       the payload if no timestamp is used (only for 64
     *                       bit CHDR widths!), however, this is advanced usage
     *                       and should only be used in special circumstances,
     *                       as downstream blocks might not be able to handle
     *                       such packets.
     * \returns the length of a CHDR header in bytes
     */
    size_t get_chdr_hdr_len(const bool account_for_ts = true) const;

    /*! Return the maximum usable payload size on a given edge, in bytes.
     *
     * This is very similar to get_mtu(), except it also accounts for the
     * header.
     *
     * Example: Say the MTU on a given edge is 8192 bytes. The CHDR width is
     * 64 bits. If we wanted to add a timestamp, we would thus require 16 bytes
     * for the total header, leaving only 8192-16=8176 bytes for a payload,
     * which is what this function would return.
     * The same MTU, with a CHDR width of 512 bits however, would require leaving
     * 64 bytes for the header (regardless of whether or not a timestamp is
     * included). In that case, this function would return 8192-64=8128 bytes
     * max payload size.
     *
     * \param edge The edge on which the max payload size is queried. edge.type
     *             must be INPUT_EDGE or OUTPUT_EDGE! See also get_mtu().
     * \param account_for_ts If true (default), the assumption is that we reserve
     *                       space for a timestamp. It is possible to increase
     *                       the payload if no timestamp is used (only for 64
     *                       bit CHDR widths!), however, this is advanced usage
     *                       and should only be used in special circumstances,
     *                       as downstream blocks might not be able to handle
     *                       such packets.
     * \returns the max payload size as determined by the overall graph on this
     *          edge, as well as the CHDR width.
     * \throws uhd::value_error if edge is not referring to a valid edge
     */
    size_t get_max_payload_size(
        const res_source_info& edge, const bool account_for_ts = true);

    /*! Return the arguments that were passed into this block from the framework
     */
    uhd::device_addr_t get_block_args() const
    {
        return _block_args;
    }

    //! Return a reference to this block's subtree
    uhd::property_tree::sptr& get_tree() const
    {
        return _tree;
    }

    //! Return a reference to this block's subtree (non-const version)
    uhd::property_tree::sptr& get_tree()
    {
        return _tree;
    }

    /*! Get access to the motherboard controller for this block's motherboard
     *
     * This will return a nullptr if this block doesn't have access to the
     * motherboard. In order to gain access to the motherboard, the block needs
     * to have requested access to the motherboard during the registration
     * procedure. See also registry.hpp.
     *
     * Even if this block requested access to the motherboard controller, there
     * is no guarantee that UHD will honour that request. It is therefore
     * important to verify that the returned pointer is valid.
     */
    std::shared_ptr<mb_controller> get_mb_controller();

protected:
    noc_block_base(make_args_ptr make_args);

    //! Update number of input ports.
    //
    // - The new number of ports may not exceed the old number. This can only
    //   be used to 'decrease' the number of ports.
    // - This is considered an 'advanced' API and should rarely be called by
    //   blocks. See also get_num_output_ports().
    //
    // \throws uhd::value_error if \p num_ports is larger than the current
    //         number of ports.
    void set_num_input_ports(const size_t num_ports);

    //! Update number of output ports.
    //
    // - The new number of ports may not exceed the old number. This can only
    //   be used to 'decrease' the number of ports.
    // - This is considered an 'advanced' API and should rarely be called by
    //   blocks. An example of where this is useful is the X310 radio block,
    //   which has 2 output ports, but only 1 is useful for UBX/SBX/WBX boards
    //   (i.e., boards with 1 frontend). In that case, software can make a
    //   determination to 'invalidate' one of the ports.
    //
    // \throws uhd::value_error if \p num_ports is larger than the current
    //         number of ports.
    void set_num_output_ports(const size_t num_ports);

    /*! Update tick rate for this node and all the connected nodes
     *
     * Careful: Calling this function will trigger a property propagation to any
     * block this block is connected to.
     */
    void set_tick_rate(const double tick_rate);

    /*! Change the way MTUs are forwarded
     *
     * The policy will have the following effect:
     * - DROP: This means that the MTU of one port has no bearing on the MTU
     *   of another port. This is usually a valid choice if the FPGA is
     *   repacking data, for example, a block could be consuming continous
     *   streams of data, and producing small packets of a different type.
     * - ONE_TO_ONE: This means the MTU is passed through from input to output
     *   and vice versa. This is typically a good choice if packets are being
     *   passed through without modifying their size. The DDC/DUC blocks will
     *   choose this policy, because the want to relay MTU information to the
     *   radio.
     * - ONE_TO_ALL: This means the MTU is being set to the same value on all
     *   ports.
     * - ONE_TO_FAN: This means the MTU is forwarded from any input port to
     *   all opposite side ports. This is an appropriate policy for the
     *   split-stream block.
     *
     * The default policy is ONE_TO_ONE.
     *
     * Note: The MTU forwarding policy can only be set once, and only during
     * construction of a noc_block_base. If an RFNoC block subclassing
     * noc_block_base wants to modify the MTU forwarding policy, it must call
     * this function in its constructor. Once set, however, the MTU forwarding
     * policy cannot be changed. This represents a change in behaviour from UHD
     * 4.0.  Violations of this restriction will result in a uhd::runtime_error
     * being thrown.
     */
    void set_mtu_forwarding_policy(const forwarding_policy_t policy);

    /*! Update the MTU
     *
     * This is another data point in the MTU discovery process. This means that
     * the MTU cannot be increased using the method, only decreased.
     */
    void set_mtu(const res_source_info& edge, const size_t new_mtu);

    /*! Return a reference to an MTU property
     *
     * This can be used to make the MTU an input to a property resolver. For
     * example, blocks that have an spp property, such as the radio, can now
     * trigger a property resolver based on the MTU.
     *
     * The reference is guaranteed to remain valid for the lifetime of this
     * block.
     */
    property_base_t* get_mtu_prop_ref(const res_source_info& edge);

    /*! Safely de-initialize the block
     *
     * This function is called by the framework when the RFNoC session is about
     * to finish to allow blocks to safely perform actions to shut down a block.
     * For example, if your block is producing samples, like a radio or signal
     * generator, this is a good place to issue a "stop" command.
     *
     * After this function is called, register access is no more possible. So
     * make sure not to interact with regs() after this was called. Future
     * access to regs() won't throw, but will print error messages and do
     * nothing.
     *
     * The rationale for having this separate from the destructor is because
     * rfnoc_graph allows exporting references to blocks, and this function
     * ensures that blocks are safely shut down when the rest of the device
     * control goes away.
     */
    virtual void deinit();

private:
    friend class block_initializer;

    /*! Update the tick rate of this block
     *
     * This will make sure that the underlying register_iface is notified of the
     * change in timebase.
     */
    void _set_tick_rate(const double tick_rate);

    /*! Perform a shutdown sequence.
     *
     * - Call deinit()
     * - Invalidate regs()
     */
    void shutdown() override;

    /*! Run post-init tasks, i.e., after the constructor concludes.
     *
     * The purpose of this method is to make sure the block is in a good state
     * after the block controller's ctor has concluded. This allows checking
     * that block configurations follow certain rules, even though they may not
     * even be part of UHD.
     */
    void post_init();

    /**************************************************************************
     * Attributes
     **************************************************************************/
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

    //! Forwarding policy for the MTU properties
    forwarding_policy_t _mtu_fwd_policy = forwarding_policy_t::ONE_TO_ONE;

    //! Flag indicating if MTU forwarding property has been set yet
    bool _mtu_fwd_policy_set = false;

    //! Container for the 'mtu' property. This will hold one edge property
    // for all in- and output edges.
    std::vector<property_t<size_t>> _mtu_props;

    //! The actual MTU value
    std::unordered_map<res_source_info, size_t> _mtu;

    //! CHDR width of this block
    chdr_w_t _chdr_w;

    //! Reference to the ctrlport clock_iface object shared with the register_iface
    std::shared_ptr<clock_iface> _ctrlport_clock_iface;

    //! Reference to the timebase clock_iface object shared with the register_iface
    std::shared_ptr<clock_iface> _tb_clock_iface;

    //! Stores a reference to this block's motherboard's controller, if this
    // block had requested and was granted access
    std::shared_ptr<mb_controller> _mb_controller;

    //! Arguments that were passed into this block
    const uhd::device_addr_t _block_args;

    //! Reference to this block's subtree
    //
    // It is mutable because _tree->access<>(..).get() is not const, but we
    // need to do just that in some const contexts
    mutable uhd::property_tree::sptr _tree;

}; // class noc_block_base

}} /* namespace uhd::rfnoc */

#include <uhd/rfnoc/noc_block_make_args.hpp>
