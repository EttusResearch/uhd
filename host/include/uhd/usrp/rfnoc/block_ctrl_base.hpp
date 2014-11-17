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

#ifndef INCLUDED_LIBUHD_BLOCK_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_BLOCK_CTRL_BASE_HPP

#include <uhd/property_tree.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/usrp/rfnoc/node_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/block_id.hpp>
#include <uhd/usrp/rfnoc/stream_sig.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd {
    namespace rfnoc {


// TODO: Move this out of public section
struct make_args_t
{
    make_args_t(const std::string &name = "") :
        device_index(0),
        is_big_endian(true),
        block_name(name)
    {}

    //! A valid interface that allows us to do peeks and pokes
    uhd::wb_iface::sptr ctrl_iface;
    //! The SID corresponding to ctrl_iface. ctrl_sid.get_dst_address() must yield this block's address.
    uhd::sid_t ctrl_sid;
    //! The device index (or motherboard index).
    size_t device_index;
    //! A property tree for this motherboard. Example: If the root a device's
    //  property tree is /mboards/0, pass a subtree starting at /mboards/0
    //  to the constructor.
    uhd::property_tree::sptr tree;
    bool is_big_endian;
    std::string block_name;
};

//! This macro must be put in the public section of an RFNoC
// block class
#define UHD_RFNOC_BLOCK_OBJECT(class_name)  \
    typedef boost::shared_ptr< class_name > sptr;

//! Shorthand for block constructor
#define UHD_RFNOC_BLOCK_CONSTRUCTOR(CLASS_NAME) \
    CLASS_NAME##_impl( \
        const make_args_t &make_args \
    ) : block_ctrl_base(make_args)

//! This macro must be placed inside a block implementation file
// after the class definition
#define UHD_RFNOC_BLOCK_REGISTER(CLASS_NAME, BLOCK_NAME) \
    block_ctrl_base::sptr CLASS_NAME##_make( \
        const make_args_t &make_args \
    ) { \
        return block_ctrl_base::sptr(new CLASS_NAME##_impl(make_args)); \
    } \
    UHD_STATIC_BLOCK(register_rfnoc_##CLASS_NAME) \
    { \
        uhd::rfnoc::block_ctrl_base::register_block(&CLASS_NAME##_make, BLOCK_NAME); \
    }

/*! \brief Base class for all RFNoC block controller objects.
 *
 * For RFNoC, block controller objects must be derived from
 * uhd::rfnoc::block_ctrl_base. This class provides all functions
 * that a block *must* provide. Typically, you would not derive
 * a block controller class directly from block_ctrl_base, but
 * from a class such as uhd::usrp::rfnoc::rx_block_ctrl_base or
 * uhd::usrp::rfnoc::tx_block_ctrl_base which extends its functionality.
 */
class UHD_API block_ctrl_base;
class block_ctrl_base : public node_ctrl_base
{
public:
    /***********************************************************************
     * Types
     **********************************************************************/
    typedef boost::shared_ptr<block_ctrl_base> sptr;
    typedef boost::function<sptr(const make_args_t &)> make_t;

    /***********************************************************************
     * Factory functions
     **********************************************************************/

    /*! Register a block controller class into the discovery and factory system.
     *
     * Note: It is not recommended to call this function directly.
     * Rather, use the UHD_RFNOC_BLOCK_REGISTER() macro, which will set up
     * the discovery and factory system correctly.
     *
     * \param make A factory function that makes a block controller object
     * \param name A unique block name, e.g. 'FFT'. If a block has this block name,
     *             it will use \p make to generate the block controller class.
     */
    static void register_block(const make_t &make, const std::string &name);

    /*!
     * \brief Create a block controller class given a NoC-ID or a block name.
     *
     * If a block name is given in \p make_args, it will directly try to
     * generate a block of this type. If no block name is given, it will
     * look up a name using the NoC-ID and use that.
     * If it can't find a suitable block controller class, it will generate
     * a uhd::rfnoc::block_ctrl. However, if a block name *is* specified,
     * it will throw a uhd::runtime_error if this block type is not registered.
     *
     * \param make_args Valid make args.
     * \param noc_id The 64-Bit NoC-ID.
     * \return a shared pointer to a new device instance
     */
    static sptr make(const make_args_t &make_args, boost::uint64_t noc_id = ~0);

    /***********************************************************************
     * Block Communication and Control
     *
     * These functions do not require communication with the FPGA.
     **********************************************************************/

    /*! Returns the 16-Bit address for this block.
     */
    boost::uint32_t get_address(size_t block_port=0);

    /*! Returns the unique block ID for this block (e.g. "0/FFT_1").
     */
    block_id_t get_block_id() const { return _block_id; };

    /*! Returns the SID for the control transport.
     */
    uhd::sid_t get_ctrl_sid() const { return _ctrl_sid; };

    /***********************************************************************
     * Stream signatures
     **********************************************************************/

    /*! Return the input stream signature for a given block port.
     *
     * If \p block_port is not a valid input port, throws
     * a uhd::runtime_error.
     *
     * Calling set_input_signature() can change the return value
     * of this function.
     */
    stream_sig_t get_input_signature(size_t block_port=0) const;

    /*! Return the output stream signature for a given block port.
     *
     * If \p block_port is not a valid output port, throws
     * a uhd::runtime_error.
     *
     * Calling set_output_signature() *or* set_input_signature() can
     * change the return value of this function.
     */
    stream_sig_t get_output_signature(size_t block_port=0) const;

    /*! Tell this block about the stream signature incoming on a given block port.
     *
     * If \p block_port is not a valid output port, throws
     * a uhd::runtime_error.
     *
     * If the input signature is incompatible with this block's signature,
     * it does not throw, but returns false.
     *
     * This function may also affect the output stream signature.
     */
    virtual bool set_input_signature(const stream_sig_t &stream_sig, size_t port=0);

    /*! Change the output stream signature for a given output block port.
     *
     * If the requested stream signature is not possible with this block,
     * it returns false (does not throw).
     * Recommended behaviour is not to modify the input signature.
     */
    virtual bool set_output_signature(const stream_sig_t &stream_sig, size_t port=0);

    /***********************************************************************
     * FPGA control & communication
     **********************************************************************/

    /*! Allows setting one register on the settings bus.
     *
     * Note: There is no address translation ("memory mapping") necessary.
     * Register 0 is 0, 1 is 1 etc.
     *
     * \param reg The settings register to write to.
     * \param data New value of this register.
     */
    void sr_write(const boost::uint32_t reg, const boost::uint32_t data);

    /*! Allows reading one register on the settings bus (64-Bit version).
     *
     * \param reg The settings register to be read.
     *
     * Returns the readback value.
     */
    boost::uint64_t sr_read64(const settingsbus_reg_t reg);

    /*! Allows reading one register on the settings bus (32-Bit version).
     *
     * \param reg The settings register to be read.
     *
     * Returns the readback value.
     */
    boost::uint32_t sr_read32(const settingsbus_reg_t reg);

    /*! Allows reading one user-defined register (64-Bit version).
     *
     * This is a shorthand for setting the requested address
     * through sr_write() and then reading SR_READBACK_REG_USER
     * with sr_read64().
     *
     * \param addr The user register address.
     * \returns the readback value.
     */
    boost::uint64_t user_reg_read64(const boost::uint32_t addr);

    /*! Allows reading one user-defined register (32-Bit version).
     *
     * This is a shorthand for setting the requested address
     * through sr_write() and then reading SR_READBACK_REG_USER
     * with sr_read32().
     *
     * \param addr The user register address.
     * \returns the readback value.
     */
    boost::uint32_t user_reg_read32(const boost::uint32_t addr);

    /*! Return the size of input buffer on a given block port.
     *
     * This is necessary for setting up flow control, among other things.
     * Note: This does not query the block's settings register. This happens
     * once during construction.
     *
     * \param block_port The block port (0 through 15).
     *
     * Returns the size of the buffer in bytes.
     */
    size_t get_fifo_size(size_t block_port=0) const;

    /*! Configure flow control for incoming streams.
     *
     * If flow control is enabled for incoming streams, this block will periodically
     * send out ACKs, telling the upstream block which packets have been consumed,
     * so the upstream block can increase his flow control credit.
     *
     * In the default implementation, this just sets registers
     * SR_FLOW_CTRL_CYCS_PER_ACK and SR_FLOW_CTRL_PKTS_PER_ACK accordingly.
     *
     * Override this function if your block has port-specific flow control settings.
     *
     * \param cycles Send an ACK after this many clock cycles.
     *               Setting this to zero disables this type of flow control acknowledgement.
     * \param packets Send an ACK after this many packets have been consumed.
     *               Setting this to zero disables this type of flow control acknowledgement.
     * \param block_port Set up flow control for a stream coming in on this particular block port.
     */
    virtual void configure_flow_control_in(
            size_t cycles,
            size_t packets,
            size_t block_port=0
     );

    /*! Configure flow control for outgoing streams.
     *
     * In the default implementation, this just sets registers SR_FLOW_CTRL_BUF_SIZE
     * and SR_FLOW_CTRL_ENABLE accordingly; \b block_port and \p sid are ignored.
     *
     * Override this function if your block has port-specific flow control settings.
     *
     * \param buf_size_pkts The size of the downstream block's input FIFO size in number of packets. Setting
     *                      this to zero disables flow control. The block will then produce data as fast as it can.
     *                     \b Warning: This can cause head-of-line blocking, and potentially lock up your device!
     * \param Specify on which outgoing port this setting is valid.
     * \param sid The SID for which this is valid. This is meant for cases where the outgoing block port is
     *            not sufficient to set the flow control, and as such is rarely used.
     */
    virtual void configure_flow_control_out(
            size_t buf_size_pkts,
            size_t block_port=0,
            const uhd::sid_t &sid=uhd::sid_t()
     );

    /*! Reset block after streaming operation.
     *
     * This does the following:
     * - Reset flow control (sequence numbers etc.)
     * - Clear the list of connected blocks
     *
     * Between runs, it can be necessary to call this method,
     * or blocks might be left hanging in a streaming state, and can get
     * confused when a new application starts.
     *
     * For custom behaviour, overwrite _clear(). If you do so, you must take
     * take care of resetting flow control yourself.
     */
    void clear();

    /*! Configures data flowing from port \p output_block_port to go to \p next_address
     *
     * In the default implementation, this will write the value in \p next_address
     * to register SR_NEXT_DST of this blocks settings bus. The value will also
     * have bit 16 set to 1, since some blocks require this to respect this value.
     */
    virtual void set_destination(
            boost::uint32_t next_address,
            size_t output_block_port = 0
    );

protected:
    /***********************************************************************
     * Structors
     **********************************************************************/
    block_ctrl_base(void) {}; // To allow pure virtual (interface) sub-classes
    virtual ~block_ctrl_base();

    /*! Constructor. This is only called from the internal block factory!
     *
     * \param make_args All arguments to this constructor are passed in this object.
     *                  Its details are subject to change. Use the UHD_RFNOC_BLOCK_CONSTRUCTOR()
     *                  macro to set up your block's constructor in a portable fashion.
     */
    block_ctrl_base(
            const make_args_t &make_args
    );

    /***********************************************************************
     * Hooks & Derivables
     **********************************************************************/

    //! Override this function if your block does something else
    // than reset register SR_FLOW_CTRL_CLR_SEQ.
    virtual void _clear();

    /***********************************************************************
     * Protected members
     **********************************************************************/

    //! An object to actually send and receive the commands
    wb_iface::sptr _ctrl_iface;

    //! Property sub-tree
    uhd::property_tree::sptr _tree;

    //! Root node of this block's properties
    uhd::fs_path _root_path;

    //! Endianness of underlying transport (for data transport)
    bool _transport_is_big_endian;

private:
    /***********************************************************************
     * Private members
     **********************************************************************/

    //! The SID of the control transport.
    // _ctrl_sid.get_dst_address() yields this block's address.
    uhd::sid_t _ctrl_sid;

    //! The (unique) block ID.
    block_id_t _block_id;
}; /* class block_ctrl_base */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_BLOCK_CTRL_BASE_HPP */
// vim: sw=4 et:
