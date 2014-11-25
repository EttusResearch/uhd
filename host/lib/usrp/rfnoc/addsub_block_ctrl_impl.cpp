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

#include <uhd/usrp/rfnoc/addsub_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

class addsub_block_ctrl_impl : public addsub_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(addsub_block_ctrl),
        _item_type("sc16"), // We only support sc16 in this block
        _bpi(uhd::convert::get_bytes_per_item("sc16"))
    {
        // Add I/O signature
        // TODO actually use values from the block definition
        _tree->create<stream_sig_t>(_root_path / "input_sig/1").set(stream_sig_t("sc16", 0));
        // FIXME default packet size?
        _tree->create<stream_sig_t>(_root_path / "output_sig/1").set(stream_sig_t("sc16", 0, DEFAULT_PACKET_SIZE));
    }

    bool set_input_signature(const stream_sig_t &stream_sig, size_t port=0)
    {
        UHD_MSG(status) << "addsub_block::set_input_signature()" << std::endl;
        UHD_ASSERT_THROW(port == 0 or port == 1);
        if (stream_sig.get_item_type() != _item_type) {
            UHD_MSG(status) << "not valid." << std::endl;
            return false;
        }

        return true;
    }

    bool set_output_signature(const stream_sig_t &stream_sig, size_t port=0)
    {
        UHD_ASSERT_THROW(port == 0 or port == 1);
        if (stream_sig.get_item_type() != _item_type) {
            return false;
        }

        return true;
    }

private:
    const std::string _item_type;
    //! Bytes per item (bytes per sample)
    const size_t _bpi;
};

UHD_RFNOC_BLOCK_REGISTER(addsub_block_ctrl, "AddSub");
