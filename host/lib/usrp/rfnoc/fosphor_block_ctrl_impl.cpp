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

#include <uhd/usrp/rfnoc/fosphor_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

class fosphor_block_ctrl_impl : public fosphor_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(fosphor_block_ctrl)
    {
        _tree->access<int>(_root_path / "args" / "decim" / "value")
            .subscribe(boost::bind(&fosphor_block_ctrl_impl::set_decim, this, _1))
            .update() // Call set_decim()
        ;
        _tree->access<int>(_root_path / "args" / "offset" / "value")
            .subscribe(boost::bind(&fosphor_block_ctrl_impl::set_offset, this, _1))
            .update() // Call set_offset()
        ;
        _tree->access<int>(_root_path / "args" / "scale" / "value")
            .subscribe(boost::bind(&fosphor_block_ctrl_impl::set_scale, this, _1))
            .update() // Call set_scale()
        ;
        _tree->access<int>(_root_path / "args" / "trise" / "value")
            .subscribe(boost::bind(&fosphor_block_ctrl_impl::set_trise, this, _1))
            .update() // Call set_trise()
        ;
        _tree->access<int>(_root_path / "args" / "tdecay" / "value")
            .subscribe(boost::bind(&fosphor_block_ctrl_impl::set_tdecay, this, _1))
            .update() // Call set_tdecay()
        ;
        _tree->access<int>(_root_path / "args" / "alpha" / "value")
            .subscribe(boost::bind(&fosphor_block_ctrl_impl::set_alpha, this, _1))
            .update() // Call set_alpha()
        ;
        _tree->access<int>(_root_path / "args" / "epsilon" / "value")
            .subscribe(boost::bind(&fosphor_block_ctrl_impl::set_epsilon, this, _1))
            .update() // Call set_epsilon()
        ;
        _tree->access<int>(_root_path / "args" / "random" / "value")
            .subscribe(boost::bind(&fosphor_block_ctrl_impl::set_random, this, _1))
            .update() // Call set_random()
        ;
    }

    void clear()
    {
        UHD_RFNOC_BLOCK_TRACE() << "clear()" << std::endl;

        sr_write("CLEAR", boost::uint32_t(1));
    }

    void set_decim(int decim)
    {
        UHD_RFNOC_BLOCK_TRACE() << "set_decim()" << std::endl;

        // Check decim is within bound (FIXME: 1024 is arbitrary ...)
        if (decim < 2 || decim > 1024) {
            // TODO read this bounds from the prop tree (block def)
            throw uhd::value_error("fosphor decim constant must be within [2, 1024]");
        }

        sr_write("DECIM", boost::uint32_t(decim - 2));
    } /* set_decim() */

    int get_decim() const
    {
        return _tree->access<int>(_root_path / "args" / "decim" / "value").get();
    } /* get_decim() */

    void set_offset(int offset)
    {
        UHD_RFNOC_BLOCK_TRACE() << "set_offset()" << std::endl;

        if (offset < 0 || offset >= 65536)
            throw uhd::value_error("fosphor offset value must be within [0, 65535]");

        sr_write("OFFSET", boost::uint32_t(offset));
    } /* set_offset() */

    int get_offset() const
    {
        return _tree->access<int>(_root_path / "args" / "offset" / "value").get();
    } /* get_offset() */

    void set_scale(int scale)
    {
        UHD_RFNOC_BLOCK_TRACE() << "set_scale()" << std::endl;

        if (scale < 0 || scale >= 65536)
            throw uhd::value_error("fosphor scale value must be within [0, 65535]");

        sr_write("SCALE", boost::uint32_t(scale));
    } /* set_scale() */

    int get_scale() const
    {
        return _tree->access<int>(_root_path / "args" / "scale" / "value").get();
    } /* get_scale() */

    void set_trise(int trise)
    {
        UHD_RFNOC_BLOCK_TRACE() << "set_trise()" << std::endl;

        if (trise < 0 || trise >= 65536)
            throw uhd::value_error("fosphor trise value must be within [0, 65535]");

        sr_write("TRISE", boost::uint32_t(trise));
    } /* set_trise() */

    int get_trise() const
    {
        return _tree->access<int>(_root_path / "args" / "trise" / "value").get();
    } /* get_trise() */

    void set_tdecay(int tdecay)
    {
        UHD_RFNOC_BLOCK_TRACE() << "set_tdecay()" << std::endl;

        if (tdecay < 0 || tdecay >= 65536)
            throw uhd::value_error("fosphor tdecay value must be within [0, 65535]");

        sr_write("TDECAY", boost::uint32_t(tdecay));
    } /* set_tdecay() */

    int get_tdecay() const
    {
        return _tree->access<int>(_root_path / "args" / "tdecay" / "value").get();
    } /* get_tdecay() */

    void set_alpha(int alpha)
    {
        UHD_RFNOC_BLOCK_TRACE() << "set_alpha()" << std::endl;

        if (alpha < 0 || alpha >= 65536)
            throw uhd::value_error("fosphor alpha value must be within [0, 65535]");

        sr_write("ALPHA", boost::uint32_t(alpha));
    } /* set_alpha() */

    int get_alpha() const
    {
        return _tree->access<int>(_root_path / "args" / "alpha" / "value").get();
    } /* get_alpha() */

    void set_epsilon(int epsilon)
    {
        UHD_RFNOC_BLOCK_TRACE() << "set_epsilon()" << std::endl;

        if (epsilon < 0 || epsilon >= 65536)
            throw uhd::value_error("fosphor epsilon value must be within [0, 65535]");

        sr_write("EPSILON", boost::uint32_t(epsilon));
    } /* set_epsilon() */

    int get_epsilon() const
    {
        return _tree->access<int>(_root_path / "args" / "epsilon" / "value").get();
    } /* get_epsilon() */

    void set_random(int random)
    {
        UHD_RFNOC_BLOCK_TRACE() << "set_random()" << std::endl;

        if (random < 0 || random >= 4)
            throw uhd::value_error("fosphor random value must be within [0, 3]");

        sr_write("RANDOM", boost::uint32_t(random));
    } /* set_random() */

    int get_random() const
    {
        return _tree->access<int>(_root_path / "args" / "random" / "value").get();
    } /* get_random() */
};

UHD_RFNOC_BLOCK_REGISTER(fosphor_block_ctrl, "fosphor");
