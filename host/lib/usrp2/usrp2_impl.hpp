//
// Copyright 2010-2011 Ettus Research LLC
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

#ifndef INCLUDED_USRP2_IMPL_HPP
#define INCLUDED_USRP2_IMPL_HPP

#include "usrp2_iface.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include "rx_frontend_core_200.hpp"
#include "tx_frontend_core_200.hpp"
#include "rx_dsp_core_200.hpp"
#include "tx_dsp_core_200.hpp"
#include <uhd/property_tree.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/device.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/subdev_spec.hpp>

/*!
 * Make a usrp2 dboard interface.
 * \param iface the usrp2 interface object
 * \param clk_ctrl the clock control object
 * \return a sptr to a new dboard interface
 */
uhd::usrp::dboard_iface::sptr make_usrp2_dboard_iface(
    usrp2_iface::sptr iface,
    usrp2_clock_ctrl::sptr clk_ctrl
);

/*!
 * USRP2 implementation guts:
 * The implementation details are encapsulated here.
 * Handles device properties and streaming...
 */
class usrp2_impl{
public:
    usrp2_impl(const uhd::device_addr_t &_device_addr);
    uhd::property_tree::sptr _tree;
private:
    struct mboard_stuff_type{
        usrp2_iface::sptr iface;
        usrp2_clock_ctrl::sptr clock;
        usrp2_codec_ctrl::sptr codec;
        rx_frontend_core_200::sptr rx_fe;
        tx_frontend_core_200::sptr tx_fe;
        rx_dsp_core_200::sptr rx_dsp;
        tx_dsp_core_200::sptr tx_dsp;
        //TODO time core
    };
    std::vector<mboard_stuff_type> _mboard_stuff;

    void set_mb_eeprom(const size_t which_mb, const uhd::usrp::mboard_eeprom_t &mb_eeprom);

};

#endif /* INCLUDED_USRP2_IMPL_HPP */
