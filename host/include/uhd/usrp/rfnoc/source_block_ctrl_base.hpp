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

#ifndef INCLUDED_LIBUHD_RX_BLOCK_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_RX_BLOCK_CTRL_BASE_HPP

#include <uhd/usrp/rfnoc/block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/source_node_ctrl.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Extends block_ctrl_base with receive capabilities.
 *
 * In RFNoC nomenclature, a receive operation means streaming
 * data from the device (the crossbar) to the host.
 * If a block has receive capabilities, this means we can receive
 * data *from* this block.
 */
class UHD_API source_block_ctrl_base;
class source_block_ctrl_base : virtual public block_ctrl_base, public source_node_ctrl
{
public:
    typedef boost::shared_ptr<source_block_ctrl_base> sptr;

    /***********************************************************************
     * Streaming operations
     **********************************************************************/
    /*! Issue a stream command for this block.
     *
     * There is no guaranteed action for this command. The default implementation
     * is to send this command to the next upstream block, or issue a warning if
     * there is no upstream block registered.
     *
     * However, implementations of block_ctrl_base might choose to do whatever seems
     * appropriate, including throwing exceptions. This may also be true for some
     * stream commands and not for others (i.e. STREAM_MODE_START_CONTINUOUS may be
     * implemented, and STREAM_MODE_NUM_SAMPS_AND_DONE may be not).
     *
     * This function does not check for infinite loops. Example: Say you have two blocks,
     * which are both registered as upstream from one another. If they both use
     * block_ctrl_base::issue_stream_cmd(), then the stream command will be passed from
     * one block to another indefinitely. This will not happen if one the block's
     * controller classes overrides this function and actually handles it.
     *
     * See also register_upstream_block().
     *
     * \param stream_cmd The stream command.
     */
    virtual void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd);

    /*! If an overrun ("O") is received, this function is called to straighten
     * things out, if necessary.
     */
    virtual void handle_overrun(boost::weak_ptr<uhd::rx_streamer>) { /* nop */ };

    /***********************************************************************
     * Stream signatures
     **********************************************************************/
    /*! Return the output stream signature for a given block port.
     *
     * If \p block_port is not a valid output port, throws
     * a uhd::runtime_error.
     *
     * Calling set_output_signature() *or* set_input_signature() can
     * change the return value of this function.
     */
    stream_sig_t get_output_signature(size_t block_port=0) const;

    /*! Change the output stream signature for a given output block port.
     *
     * If the requested stream signature is not possible with this block,
     * it returns false (does not throw).
     * Recommended behaviour is not to modify the input signature.
     */
    virtual bool set_output_signature(const stream_sig_t &stream_sig, size_t port=0);

    /***********************************************************************
     * FPGA Configuration
     **********************************************************************/
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


protected:
    /***********************************************************************
     * Hooks
     **********************************************************************/
    /*! Like source_node_ctrl::_request_output_port(), but also checks if
     * the port has an output signature.
     */
    virtual size_t _request_output_port(
            const size_t suggested_port,
            const uhd::device_addr_t &args
    ) const;

}; /* class source_block_ctrl_base */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RX_BLOCK_CTRL_BASE_HPP */
// vim: sw=4 et:
