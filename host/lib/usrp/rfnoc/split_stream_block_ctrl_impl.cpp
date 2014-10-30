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

#include <uhd/usrp/rfnoc/split_stream_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

class split_stream_block_ctrl_impl : public split_stream_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(split_stream_block_ctrl),
        _item_type("sc16"), // We only support sc16 in this block
        _bpi(uhd::convert::get_bytes_per_item("sc16"))
    {
    }

private:
    const std::string _item_type;
    //! Bytes per item (bytes per sample)
    const size_t _bpi;
};

UHD_RFNOC_BLOCK_REGISTER(split_stream_block_ctrl, "SplitStream");
