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

#ifndef INCLUDED_LIBUHD_TX_BLOCK_CTRL_BASE_HPP
#define INCLUDED_LIBUHD_TX_BLOCK_CTRL_BASE_HPP

#include <uhd/usrp/rfnoc/block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Extends block_ctrl_base with transmit capabilities.
 *
 * In RFNoC nomenclature, a transmit operation means streaming
 * data to the device (the crossbar) from the host.
 * If a block has transmit capabilities, this means we can transmit
 * data *to* this block.
 */
class UHD_API tx_block_ctrl_base;
class tx_block_ctrl_base : virtual public block_ctrl_base
{
public:
    typedef boost::shared_ptr<tx_block_ctrl_base> sptr;

    /*! Set stream args and SID before opening a TX streamer to this block.
     *
     * This is called by the streamer generators. Note this is *not* virtual.
     * To change which settings are set during rx configuration, override _init_tx().
     */
    void setup_tx_streamer(uhd::stream_args_t &args);

protected:

    /*! Before any kind of streaming operation, this will be automatically
     * called to configure the block. Override this to set any tx specific
     * registers etc.
     *
     * Note: \p args may be changed in this function. In a chained operation,
     * the modified value of \p args will be propagated upstream.
     */
    virtual void _init_tx(uhd::stream_args_t &) { /* nop */ };

    /*! If this function returns true, tx-specific settings (such as tx streamer
     * setup) are not propagated downstream.
     * An example for a block where this is necessary is a radio: If it's
     * operating in full duplex mode, it will have a downstream block on the rx
     * side, but we don't want to configure those while setting up an tx stream
     * chain.
     */
    bool _is_final_tx_block() { return false; };

}; /* class tx_block_ctrl_base */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_TX_BLOCK_CTRL_BASE_HPP */
// vim: sw=4 et:
