//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/device3.hpp>
#include <uhd/utils/log.hpp>
#include <boost/format.hpp>
#include <boost/thread/lock_guard.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

device3::sptr device3::make(const device_addr_t &hint, const size_t which)
{
    device3::sptr device3_sptr =
        boost::dynamic_pointer_cast< device3 >(device::make(hint, device::USRP, which));
    if (not device3_sptr) {
        throw uhd::key_error(str(
            boost::format("No gen-3 devices found for ----->\n%s") % hint.to_pp_string()
        ));
    }

    return device3_sptr;
}

bool device3::has_block(const rfnoc::block_id_t &block_id) const
{
    for (size_t i = 0; i < _rfnoc_block_ctrl.size(); i++) {
        if (_rfnoc_block_ctrl[i]->get_block_id() == block_id) {
            return true;
        }
    }
    return false;
}

block_ctrl_base::sptr device3::get_block_ctrl(const block_id_t &block_id) const
{
    for (size_t i = 0; i < _rfnoc_block_ctrl.size(); i++) {
        if (_rfnoc_block_ctrl[i]->get_block_id() == block_id) {
            return _rfnoc_block_ctrl[i];
        }
    }
    throw uhd::lookup_error(str(boost::format("This device does not have a block with ID: %s") % block_id.to_string()));
}

std::vector<rfnoc::block_id_t> device3::find_blocks(const std::string &block_id_hint) const
{
    std::vector<rfnoc::block_id_t> block_ids;
    for (size_t i = 0; i < _rfnoc_block_ctrl.size(); i++) {
        if (_rfnoc_block_ctrl[i]->get_block_id().match(block_id_hint)) {
            block_ids.push_back(_rfnoc_block_ctrl[i]->get_block_id());
        }
    }
    return block_ids;
}

void device3::clear()
{
    boost::lock_guard<boost::mutex> lock(_block_ctrl_mutex);
    for(const block_ctrl_base::sptr &block:  _rfnoc_block_ctrl) {
        block->clear();
    }
}
// vim: sw=4 et:
