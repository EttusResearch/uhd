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

#include <uhd/usrp/rfnoc/keep_one_in_n_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

class keep_one_in_n_block_ctrl_impl : public keep_one_in_n_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(keep_one_in_n_block_ctrl),
        _item_type("sc16"), // We only support sc16 in this block
        _bpi(uhd::convert::get_bytes_per_item("sc16")),
        _n(DEFAULT_N)
    {

        // TODO: Read the default initial keep one in n size from the block definition
        // TODO: Register the keep one in n size into the property tree

        set_n(_n);
    }

    void set_n(boost::uint16_t n)
    {
        //// 2. Update block
        sr_write(SR_N,n);
        _n = n;

    } /* set_n() */

    boost::uint16_t get_n() const
    {
        return _n;
    } /* get_n() */

protected:
    void _post_args_hook()
    {
        UHD_RFNOC_BLOCK_TRACE() << "_post_args_hook()" << std::endl;

        if (_args.has_key("n")) {
            boost::uint16_t req_n = _args.cast<boost::uint16_t>("n", _n);
            if (req_n != _n) {
                set_n(req_n);
            }
        }
    }

private:
    const std::string _item_type;
    //! Bytes per item (bytes per sample)
    const size_t _bpi;
    boost::uint16_t _n;
};

UHD_RFNOC_BLOCK_REGISTER(keep_one_in_n_block_ctrl, "KeepOneInN");
