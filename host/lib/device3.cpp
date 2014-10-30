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

#include <boost/format.hpp>
#include <uhd/device3.hpp>

#include <uhd/utils/msg.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

block_ctrl_base::sptr device3::get_block_ctrl(const block_id_t &block_id) const
{
    for (size_t i = 0; i < _rfnoc_block_ctrl.size(); i++) {
        if (_rfnoc_block_ctrl[i]->get_block_id() == block_id) {
            return _rfnoc_block_ctrl[i];
        }
    }

    throw uhd::lookup_error(str(boost::format("This device does not have a block with ID: %s") % block_id.to_string()));
}

block_ctrl_base::sptr device3::find_block_ctrl(const std::string &block_id) const
{
    for (size_t i = 0; i < _rfnoc_block_ctrl.size(); i++) {
        if (_rfnoc_block_ctrl[i]->get_block_id().match(block_id)) {
            return _rfnoc_block_ctrl[i];
        }
    }

    return block_ctrl_base::sptr();
}

void device3::clear()
{
    BOOST_FOREACH(const block_ctrl_base::sptr &block, _rfnoc_block_ctrl) {
        block->clear();
    }
}
// vim: sw=4 et:
