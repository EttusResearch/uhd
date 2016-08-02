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

#include "device3_impl.hpp"
#include "graph_impl.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/rfnoc/block_ctrl_base.hpp>
#include <boost/make_shared.hpp>
#include <algorithm>

#define UHD_DEVICE3_LOG() UHD_LOGV(never)

using namespace uhd::usrp;

device3_impl::device3_impl()
    : _sid_framer(0)
{
    _type = uhd::device::USRP;
    _async_md.reset(new async_md_type(1000/*messages deep*/));
    _tree = uhd::property_tree::make();
};

//! Returns true if the integer value stored in lhs is smaller than that in rhs
bool _compare_string_indexes(const std::string &lhs, const std::string &rhs)
{
    return boost::lexical_cast<size_t>(lhs) < boost::lexical_cast<size_t>(rhs);
}

void device3_impl::merge_channel_defs(
    const std::vector<uhd::rfnoc::block_id_t> &chan_ids,
    const std::vector<uhd::device_addr_t> &chan_args,
    const uhd::direction_t dir
) {
    UHD_ASSERT_THROW(chan_ids.size() == chan_args.size());
    if (dir == uhd::DX_DIRECTION) {
        merge_channel_defs(chan_ids, chan_args, RX_DIRECTION);
        merge_channel_defs(chan_ids, chan_args, TX_DIRECTION);
        return;
    }

    uhd::fs_path chans_root = uhd::fs_path("/channels/") / (dir == RX_DIRECTION ? "rx" : "tx");
    // Store the new positions of the channels:
    std::vector<size_t> chan_idxs;

    // 1. Get sorted list of currently defined channels
    std::vector<std::string> curr_channels;
    if (_tree->exists(chans_root)) {
        curr_channels = _tree->list(chans_root);
        std::sort(curr_channels.begin(), curr_channels.end(), _compare_string_indexes);
    }

    // 2. Cycle through existing channels to find out where to merge
    //    the new channels. Rules are:
    //    - The order of chan_ids must be preserved
    //    - All block indices that are in chan_ids may be overwritten in the channel definition
    //    - If the channels in chan_ids are not yet in the property tree channel list,
    //      they are appended.
    BOOST_FOREACH(const std::string &chan_idx, curr_channels) {
        if (_tree->exists(chans_root / chan_idx)) {
            rfnoc::block_id_t chan_block_id = _tree->access<rfnoc::block_id_t>(chans_root / chan_idx).get();
            if (std::find(chan_ids.begin(), chan_ids.end(), chan_block_id) != chan_ids.end()) {
                chan_idxs.push_back(boost::lexical_cast<size_t>(chan_idx));
            }
        }
    }
    size_t last_chan_idx = curr_channels.empty() ? 0 : (boost::lexical_cast<size_t>(curr_channels.back()) + 1);
    while (chan_idxs.size() < chan_ids.size()) {
        chan_idxs.push_back(last_chan_idx);
        last_chan_idx++;
    }

    // 3. Write the new channels
    for (size_t i = 0; i < chan_ids.size(); i++) {
        if (not _tree->exists(chans_root / chan_idxs[i])) {
            _tree->create<rfnoc::block_id_t>(chans_root / chan_idxs[i]);
        }
        _tree->access<rfnoc::block_id_t>(chans_root / chan_idxs[i]).set(chan_ids[i]);
        if (not _tree->exists(chans_root / chan_idxs[i] / "args")) {
            _tree->create<uhd::device_addr_t>(chans_root / chan_idxs[i] / "args");
        }
        _tree->access<uhd::device_addr_t>(chans_root / chan_idxs[i] / "args").set(chan_args[i]);
    }
}

/***********************************************************************
 * RFNoC-Specific
 **********************************************************************/
void device3_impl::enumerate_rfnoc_blocks(
        size_t device_index,
        size_t n_blocks,
        size_t base_port,
        const uhd::sid_t &base_sid,
        uhd::device_addr_t transport_args,
        uhd::endianness_t endianness
) {
    // entries that are already connected to this block
    uhd::sid_t ctrl_sid = base_sid;
    uhd::property_tree::sptr subtree = _tree->subtree(uhd::fs_path("/mboards") / device_index);
    // 1) Clean property tree entries
    // TODO put this back once radios are actual rfnoc blocks!!!!!!
    //if (subtree->exists("xbar")) {
        //subtree->remove("xbar");
    //}
    // 2) Destroy existing block controllers
    // TODO: Clear out all the old block control classes
    // 3) Create new block controllers
    for (size_t i = 0; i < n_blocks; i++) {
        UHD_DEVICE3_LOG() << "[RFNOC] ------- Block Setup -----------" << std::endl;
        // First, make a transport for port number zero, because we always need that:
        ctrl_sid.set_dst_xbarport(base_port + i);
        ctrl_sid.set_dst_blockport(0);
        both_xports_t xport = this->make_transport(
            ctrl_sid,
            CTRL,
            transport_args
        );
        UHD_DEVICE3_LOG() << str(boost::format("Setting up NoC-Shell Control for port #0 (SID: %s)...") % xport.send_sid.to_pp_string_hex());
        uhd::rfnoc::ctrl_iface::sptr ctrl = uhd::rfnoc::ctrl_iface::make(
                endianness == ENDIANNESS_BIG,
                xport.send,
                xport.recv,
                xport.send_sid,
                str(boost::format("CE_%02d_Port_%02X") % i % ctrl_sid.get_dst_endpoint())
        );
        UHD_DEVICE3_LOG() << "OK" << std::endl;
        uint64_t noc_id = ctrl->peek64(uhd::rfnoc::SR_READBACK_REG_ID);
        UHD_DEVICE3_LOG() << str(boost::format("Port %d: Found NoC-Block with ID %016X.") % int(ctrl_sid.get_dst_endpoint()) % noc_id) << std::endl;
        uhd::rfnoc::make_args_t make_args;
        uhd::rfnoc::blockdef::sptr block_def = uhd::rfnoc::blockdef::make_from_noc_id(noc_id);
        if (not block_def) {
            UHD_DEVICE3_LOG() << "Using default block configuration." << std::endl;
            block_def = uhd::rfnoc::blockdef::make_from_noc_id(uhd::rfnoc::DEFAULT_NOC_ID);
        }
        UHD_ASSERT_THROW(block_def);
        make_args.ctrl_ifaces[0] = ctrl;
        BOOST_FOREACH(const size_t port_number, block_def->get_all_port_numbers()) {
            if (port_number == 0) { // We've already set this up
                continue;
            }
            ctrl_sid.set_dst_blockport(port_number);
            both_xports_t xport1 = this->make_transport(
                ctrl_sid,
                CTRL,
                transport_args
            );
            UHD_DEVICE3_LOG() << str(boost::format("Setting up NoC-Shell Control for port #%d (SID: %s)...") % port_number % xport1.send_sid.to_pp_string_hex());
            uhd::rfnoc::ctrl_iface::sptr ctrl1 = uhd::rfnoc::ctrl_iface::make(
                    endianness == ENDIANNESS_BIG,
                    xport1.send,
                    xport1.recv,
                    xport1.send_sid,
                    str(boost::format("CE_%02d_Port_%02d") % i % ctrl_sid.get_dst_endpoint())
            );
            UHD_DEVICE3_LOG() << "OK" << std::endl;
            make_args.ctrl_ifaces[port_number] = ctrl1;
        }

        make_args.base_address = xport.send_sid.get_dst();
        make_args.device_index = device_index;
        make_args.tree = subtree;
        make_args.is_big_endian = (endianness == ENDIANNESS_BIG);
        _rfnoc_block_ctrl.push_back(uhd::rfnoc::block_ctrl_base::make(make_args, noc_id));
    }
}


uhd::rfnoc::graph::sptr device3_impl::create_graph(const std::string &name)
{
    return boost::make_shared<uhd::rfnoc::graph_impl>(
            name,
            shared_from_this()
    );
}

