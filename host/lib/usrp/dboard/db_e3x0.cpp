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

#include <uhd/utils/static.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>

namespace uhd { namespace usrp {

/***********************************************************************
 * The E310 dboard
 *   AD9361 Interface, thus two subdevs
 **********************************************************************/
class e310_dboard : public xcvr_dboard_base{
public:
    e310_dboard(ctor_args_t args) : xcvr_dboard_base(args) {}

    ~e310_dboard(void) {}
};

/***********************************************************************
 * The E310 dboard
 *   AD9364 Interface, thus one subdev
 **********************************************************************/
class e300_dboard : public xcvr_dboard_base{
public:
    e300_dboard(ctor_args_t args) : xcvr_dboard_base(args) {}

    ~e300_dboard(void) {}
};

/***********************************************************************
 * Register the E310 dboards
 **********************************************************************/
static dboard_base::sptr make_e310_dboard(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new e310_dboard(args));
}

static dboard_base::sptr make_e300_dboard(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new e300_dboard(args));
}

}} // namespace

using namespace uhd::usrp;

UHD_STATIC_BLOCK(reg_e3x0_dboards){
    dboard_manager::register_dboard(0x0110, &make_e310_dboard, "E310 MIMO XCVR");
    dboard_manager::register_dboard(0x0100, &make_e300_dboard, "E300 SISO XCVR");
}
