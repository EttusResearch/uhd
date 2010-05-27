//
// Copyright 2010 Ettus Research LLC
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

#include "serdes_ctrl.hpp"
#include "usrp2_regs.hpp"

using namespace uhd;

/*!
 * A usrp2 serdes control implementation
 */
class serdes_ctrl_impl : public serdes_ctrl{
public:
    serdes_ctrl_impl(usrp2_iface::sptr iface){
        _iface = iface;
        _iface->poke32(FR_MISC_CTRL_SERDES, FRF_MISC_CTRL_SERDES_ENABLE | FRF_MISC_CTRL_SERDES_RXEN);
    }

    ~serdes_ctrl_impl(void){
        _iface->poke32(FR_MISC_CTRL_SERDES, 0); //power-down
    }

private:
    usrp2_iface::sptr _iface;
};

/***********************************************************************
 * Public make function for the usrp2 serdes control
 **********************************************************************/
serdes_ctrl::sptr serdes_ctrl::make(usrp2_iface::sptr iface){
    return sptr(new serdes_ctrl_impl(iface));
}
