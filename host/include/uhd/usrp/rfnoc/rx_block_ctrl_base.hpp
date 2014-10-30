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

namespace uhd {
    namespace rfnoc {

/*! \brief Extends block_ctrl_base with receive capabilities.
 *
 * In RFNoC nomenclature, a receive operation means streaming
 * data from the device (the crossbar) to the host.
 * If a block has receive capabilities, this means we can receive
 * data *from* this block.
 */
class UHD_API rx_block_ctrl_base;
class rx_block_ctrl_base : virtual public block_ctrl_base
{
public:
    typedef boost::shared_ptr<rx_block_ctrl_base> sptr;

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

    /*! Set stream args and SID before opening an RX streamer to this block.
     *
     * This is called by the streamer generators. Note this is *not* virtual.
     * To change which settings are set during rx configuration, override init_rx().
     */
    void setup_rx_streamer(uhd::stream_args_t &args);

    /*! If an overrun ("O") is received, this function is called to straighten
     * things out, if necessary.
     */
    virtual void handle_overrun(boost::weak_ptr<uhd::rx_streamer>) { /* nop */ };

protected:
    /*! Before any kind of streaming operation, this will be automatically
     * called to configure the block. Override this to set any rx specific
     * registers etc.
     *
     * Note: \p args may be changed in this function. In a chained operation,
     * the modified value of \p args will be propagated upstream.
     */
    virtual void _init_rx(uhd::stream_args_t &) { /* nop */ };

    /*! If this function returns true, rx-specific settings (such as rx streamer
     * setup) are not propagated upstream.
     * An example for a block where this is necessary is a radio: If it's
     * operating in full duplex mode, it will have an upstream block on the tx
     * side, but we don't want to configure those while setting up an rx stream
     * chain.
     */
    bool _is_final_rx_block() { return false; };

}; /* class rx_block_ctrl_base */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RX_BLOCK_CTRL_BASE_HPP */
// vim: sw=4 et:
