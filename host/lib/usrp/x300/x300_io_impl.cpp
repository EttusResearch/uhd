//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_impl.hpp"
#include "x300_regs.hpp"

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Hooks for get_tx_stream() and get_rx_stream()
 **********************************************************************/
device_addr_t x300_impl::get_rx_hints(size_t mb_index)
{
    device_addr_t rx_hints = _mb[mb_index].recv_args;
    return rx_hints;
}


device_addr_t x300_impl::get_tx_hints(size_t mb_index)
{
    device_addr_t tx_hints = _mb[mb_index].send_args;
    return tx_hints;
}

void x300_impl::post_streamer_hooks(direction_t dir)
{
    if (dir != TX_DIRECTION) {
        return;
    }

    // Loop through all tx streamers. Find all radios connected to one
    // streamer. Sync those.
    for (const boost::weak_ptr<uhd::tx_streamer>& streamer_w : _tx_streamers.vals()) {
        const boost::shared_ptr<device3_send_packet_streamer> streamer =
            boost::dynamic_pointer_cast<device3_send_packet_streamer>(streamer_w.lock());
        if (not streamer) {
            continue;
        }

        std::vector<rfnoc::x300_radio_ctrl_impl::sptr> radio_ctrl_blks =
            streamer->get_terminator()
                ->find_downstream_node<rfnoc::x300_radio_ctrl_impl>();
        try {
            // UHD_LOGGER_INFO("X300") << "[X300] syncing " << radio_ctrl_blks.size() << "
            // radios " ;
            rfnoc::x300_radio_ctrl_impl::synchronize_dacs(radio_ctrl_blks);
        } catch (const uhd::io_error& ex) {
            throw uhd::io_error(
                str(boost::format("Failed to sync DACs! %s ") % ex.what()));
        }
    }
}

// vim: sw=4 expandtab:
