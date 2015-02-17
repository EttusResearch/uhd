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

#include <uhd/usrp/rfnoc/packet_resizer_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

class packet_resizer_block_ctrl_impl : public packet_resizer_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(packet_resizer_block_ctrl),
        _item_type("sc16"), // We only support sc16 in this block
        _bpi(uhd::convert::get_bytes_per_item("sc16"))
    {
        // TODO Could be useful to have output packet size automatically set based on
        // upstream block packet size
        _tree->access<int>(_root_path / "args" / "pkt_size" / "value")
            .subscribe(boost::bind(&packet_resizer_block_ctrl_impl::set_packet_size, this, _1))
            .update()
        ;
    }

    void set_packet_size_str(const std::string &pkt_size)
    {
        set_packet_size(boost::lexical_cast<boost::uint16_t>(pkt_size));
    }

    void set_packet_size(const int pkt_size)
    {
        UHD_MSG(status) << "packet_resizer::set_packet_size() pkt_size == " << pkt_size << std::endl;
        UHD_ASSERT_THROW(pkt_size);
        sr_write(SR_PKT_SIZE, boost::uint16_t(pkt_size));
    }

    int get_packet_size()
    {
        return _tree->access<int>(_root_path / "args" / "pkt_size" / "value").get();
    }

private:
    const std::string _item_type;
    const size_t _bpi;
};

UHD_RFNOC_BLOCK_REGISTER(packet_resizer_block_ctrl, "PacketResizer");
