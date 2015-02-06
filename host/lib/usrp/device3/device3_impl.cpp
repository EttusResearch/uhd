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
#include <algorithm>

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
    const std::vector<rfnoc::block_id_t> &chan_ids,
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
    std::vector<std::string> curr_channels = _tree->list(chans_root);
    std::sort(curr_channels.begin(), curr_channels.end(), _compare_string_indexes);

    // 2. Cycle through existing channels to find out where to merge
    //    the new channels. Rules are:
    //    - The order of chan_ids must be preserved
    //    - All block indices that are in chan_ids may be overwritten in the channel definition
    //    - If the channels in chan_ids are not yet in the property tree channel list,
    //      they are appended.
    BOOST_FOREACH(const std::string &chan_idx, curr_channels) {
        rfnoc::block_id_t chan_block_id = _tree->access<rfnoc::block_id_t>(chans_root / chan_idx).get();
        if (std::find(chan_ids.begin(), chan_ids.end(), chan_block_id) != chan_ids.end()) {
            chan_idxs.push_back(boost::lexical_cast<size_t>(chan_idx));
        }
    }
    size_t last_chan_idx = boost::lexical_cast<size_t>(curr_channels.back());
    while (chan_idxs.size() < chan_ids.size()) {
        last_chan_idx++;
        chan_idxs.push_back(last_chan_idx);
    }

    // 3. Write the new channels
    for (size_t i = 0; i < chan_ids.size(); i++) {
        if (not _tree->exists(chans_root / chan_idxs[i])) {
            _tree->create<rfnoc::block_id_t>(chans_root / chan_idxs[i]);
        }
        _tree->access<rfnoc::block_id_t>(chans_root / chan_idxs[i]).set(chan_ids[i]);
        _tree->access<uhd::device_addr_t>(chans_root / chan_idxs[i] / "args").set(chan_args[i]);
    }
}
