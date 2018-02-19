//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_TX_BLOCK_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_TX_BLOCK_CTRL_BASE_HPP

#include <uhd/rfnoc/block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_node_ctrl.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Extends block_ctrl_base with input capabilities.
 *
 * A sink block is an RFNoC block that can receive data at an input.
 * We can use this block to transmit data (In RFNoC nomenclature, a
 * transmit operation means streaming data to the device from the host).
 *
 * Every input is defined by a port definition (port_t).
 */
class UHD_RFNOC_API sink_block_ctrl_base;
class sink_block_ctrl_base : virtual public block_ctrl_base, virtual public sink_node_ctrl
{
public:
    typedef boost::shared_ptr<sink_block_ctrl_base> sptr;

    /***********************************************************************
     * Stream signatures
     **********************************************************************/
    /*! Return the input stream signature for a given block port.
     *
     * The actual signature is determined by the current configuration
     * and the block definition file. The value returned here is calculated
     * on-the-fly and is only valid as long as the configuration does not
     * change.
     *
     * \returns The stream signature for port \p block_port
     * \throws uhd::runtime_error if \p block_port is not a valid port
     */
    stream_sig_t get_input_signature(size_t block_port=0) const;

    /*! Return a list of valid input ports.
     */
    std::vector<size_t> get_input_ports() const;

    /***********************************************************************
     * FPGA Configuration
     **********************************************************************/
    /*! Return the size of input buffer on a given block port.
     *
     * This is necessary for setting up flow control, among other things.
     * Note: This does not query the block's settings register. The FIFO size
     * is queried once during construction and cached.
     *
     * If the block port is not defined, it will return 0, and not throw.
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

    /*! Configure the behaviour for errors on incoming packets
     *  (e.g. sequence errors).
     *
     *
     */
    virtual void set_error_policy(
        const std::string &policy
    );

protected:
    /***********************************************************************
     * Hooks
     **********************************************************************/
    /*! Like sink_node_ctrl::_request_input_port(), but also checks
     * the port has an input signature.
     */
    virtual size_t _request_input_port(
            const size_t suggested_port,
            const uhd::device_addr_t &args
    ) const;

}; /* class sink_block_ctrl_base */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_TX_BLOCK_CTRL_BASE_HPP */
// vim: sw=4 et:
