//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RX_BLOCK_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_RX_BLOCK_CTRL_BASE_HPP

#include <uhd/rfnoc/block_ctrl_base.hpp>
#include <uhd/rfnoc/source_node_ctrl.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Extends block_ctrl_base with receive capabilities.
 *
 * In RFNoC nomenclature, a receive operation means streaming
 * data from the device (the crossbar) to the host.
 * If a block has receive capabilities, this means we can receive
 * data *from* this block.
 */
class UHD_RFNOC_API source_block_ctrl_base;
class source_block_ctrl_base : virtual public block_ctrl_base, virtual public source_node_ctrl
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
     * \param chan Channel for which this command is meant (data shall be produced on this channel).
     */
    virtual void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, const size_t chan=0);

    /***********************************************************************
     * Stream signatures
     **********************************************************************/
    /*! Return the output stream signature for a given block port.
     *
     * The actual signature is determined by the current configuration
     * and the block definition file. The value returned here is calculated
     * on-the-fly and is only valid as long as the configuration does not
     * change.
     *
     * \returns The stream signature for port \p block_port
     * \throws uhd::runtime_error if \p block_port is not a valid port
     */
    stream_sig_t get_output_signature(size_t block_port=0) const;

    /*! Return a list of valid output ports.
     */
    std::vector<size_t> get_output_ports() const;

    /***********************************************************************
     * FPGA Configuration
     **********************************************************************/
    /*! Configures data flowing from port \p output_block_port to go to \p next_address
     *
     * \param next_address Address of the downstream block
     * \param output_block_port Port for which this is valid
     *
     * In the default implementation, this will write the value in \p next_address
     * to register SR_NEXT_DST of this blocks settings bus. The value will also
     * have bit 16 set to 1, since some blocks require this to respect this value.
     */
    virtual void set_destination(
            uint32_t next_address,
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
     * \param block_port Specify on which outgoing port this setting is valid.
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
