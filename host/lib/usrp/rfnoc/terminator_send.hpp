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

#ifndef INCLUDED_LIBUHD_RFNOC_TERMINATOR_SEND_HPP
#define INCLUDED_LIBUHD_RFNOC_TERMINATOR_SEND_HPP

#include <uhd/usrp/rfnoc/source_node_ctrl.hpp>
#include <uhd/usrp/rfnoc/rate_node_ctrl.hpp>
#include <uhd/usrp/rfnoc/tick_node_ctrl.hpp>
#include <uhd/usrp/rfnoc/scalar_node_ctrl.hpp>
#include <uhd/usrp/rfnoc/block_ctrl_base.hpp> // For the block macros
#include <uhd/utils/msg.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Transmit-end block control terminator.
 *
 * This node terminates an RFNoC flow graph in the tx streamer.
 */
class terminator_send :
    public source_node_ctrl,
    public rate_node_ctrl,
    public tick_node_ctrl,
    public scalar_node_ctrl
{
public:
    UHD_RFNOC_BLOCK_OBJECT(terminator_send)

    static sptr make()
    {
        return sptr(new terminator_send);
    }

    void issue_stream_cmd(const uhd::stream_cmd_t &)
    {
        UHD_RFNOC_BLOCK_TRACE() << "terminator_send::issue_stream_cmd()" << std::endl;
    }

    // A tx streamer doesn't set its output sampling rate,
    // rather, it sets a downstream block's sampling rate.
    double get_output_samp_rate(size_t) { return rate_node_ctrl::RATE_NONE; };

    // Same for the scaling factor
    double get_output_scale_factor(size_t) { return scalar_node_ctrl::SCALE_NONE; };

    std::string unique_id() const;

protected:
    terminator_send() : _term_index(_count) { _count++; };

    /*! Nothing may come after the streamer.
     */
    bool _is_final_rx_block() { return true; };

private:
    //! Every terminator has a unique index
    const size_t _term_index;
    static size_t _count;

}; /* class terminator_send */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_TERMINATOR_SEND_HPP */
// vim: sw=4 et:
