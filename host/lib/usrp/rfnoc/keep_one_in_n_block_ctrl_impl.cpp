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
    UHD_RFNOC_BLOCK_CONSTRUCTOR(keep_one_in_n_block_ctrl)
    {
        _tree->access<int>(_root_path / "args" / "n" / "value")
            .subscribe(boost::bind(&keep_one_in_n_block_ctrl_impl::set_n, this, _1))
            .update() // Call set_n()
        ;
    }

    void set_n(int n)
    {
        UHD_RFNOC_BLOCK_TRACE() << "set_n()" << std::endl;
        sr_write(SR_N, boost::uint16_t(n));
    } /* set_n() */

    int get_n() const
    {
        return _tree->access<int>(_root_path / "args" / "n" / "value").get();
    } /* get_n() */

};

UHD_RFNOC_BLOCK_REGISTER(keep_one_in_n_block_ctrl, "KeepOneInN");
