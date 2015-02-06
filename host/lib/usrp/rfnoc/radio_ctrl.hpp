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

#ifndef INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_HPP

#include <uhd/usrp/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/rate_node_ctrl.hpp>
#include <uhd/usrp/rfnoc/tick_node_ctrl.hpp>
#include <uhd/usrp/rfnoc/scalar_node_ctrl.hpp>
#include <uhd/types/direction.hpp>
#include "rx_vita_core_3000.hpp"
#include "tx_vita_core_3000.hpp"
#include "time_core_3000.hpp"
#include "rx_dsp_core_3000.hpp"
#include "tx_dsp_core_3000.hpp"
#include "rx_frontend_core_200.hpp"
#include "tx_frontend_core_200.hpp"

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to a radio.
 *
 */
class radio_ctrl :
    public source_block_ctrl_base,
    public sink_block_ctrl_base,
    public rate_node_ctrl,
    public tick_node_ctrl,
    public scalar_node_ctrl
{
public:
    UHD_RFNOC_BLOCK_OBJECT(radio_ctrl)

    enum dboard_type_t {
        DBOARD_TYPE_UNKNOWN,
        DBOARD_TYPE_SLOT,
        DBOARD_TYPE_AD9361
    };

    /***********************************************************************
     * Radio controls (radio_ctrl specific)
     **********************************************************************/
    virtual void set_perifs(
        time_core_3000::sptr    time64,
        rx_vita_core_3000::sptr framer,
        rx_dsp_core_3000::sptr  ddc,
        tx_vita_core_3000::sptr deframer,
        tx_dsp_core_3000::sptr  duc,
        rx_frontend_core_200::sptr rx_fe,
        tx_frontend_core_200::sptr tx_fe
    ) = 0;

    virtual void set_dboard_type(dboard_type_t type) = 0;

    virtual void update_muxes(uhd::direction_t dir) = 0;

}; /* class radio_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_HPP */
// vim: sw=4 et:
