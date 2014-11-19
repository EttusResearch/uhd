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
#include <uhd/usrp/rfnoc/block_ctrl_base.hpp> // For the block macros

namespace uhd {
    namespace rfnoc {

/*! \brief Transmit-end block control terminator.
 *
 */
class terminator_send : public source_node_ctrl
{
public:
    UHD_RFNOC_BLOCK_OBJECT(terminator_send)

    static sptr make()
    {
        return sptr(new terminator_send);
    }

    void issue_stream_cmd(const uhd::stream_cmd_t &)
    {
        UHD_MSG(status) << "terminator_send::issue_stream_cmd()" << std::endl;
    }

protected:

    /*! Nothing may come after the streamer.
     */
    bool _is_final_rx_block() { return true; };

}; /* class terminator_send */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_TERMINATOR_SEND_HPP */
// vim: sw=4 et:
