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

#include <boost/bind.hpp>
#include "usrp_e_impl.hpp"

using namespace uhd::usrp;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
// Check if requested decim/interp rate is:
//      multiple of 4, enable two halfband filters
//      multiple of 2, enable one halfband filter
//      handle remainder in CIC
static boost::uint32_t calculate_cic_word(size_t rate){
    int hb0 = 0, hb1 = 0;
    if (not (rate & 0x1)){
        hb0 = 1;
        rate /= 2;
    }
    if (not (rate & 0x1)){
        hb1 = 1;
        rate /= 2;
    }
    return (hb1 << 9) | (hb0 << 8) | (rate & 0xff);
}

static boost::uint32_t calculate_iq_scale_word(boost::int16_t i, boost::int16_t q){
    return (boost::uint16_t(i) << 16) | (boost::uint16_t(q) << 0);
}

/***********************************************************************
 * RX DDC Initialization
 **********************************************************************/
void usrp_e_impl::rx_ddc_init(void){
    _rx_ddc_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e_impl::rx_ddc_get, this, _1, _2),
        boost::bind(&usrp_e_impl::rx_ddc_set, this, _1, _2)
    );
}

/***********************************************************************
 * RX DDC Get
 **********************************************************************/
void usrp_e_impl::rx_ddc_get(const wax::obj &, wax::obj &){
    UHD_THROW_PROP_GET_ERROR();
}

/***********************************************************************
 * RX DDC Set
 **********************************************************************/
void usrp_e_impl::rx_ddc_set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}

/***********************************************************************
 * TX DUC Initialization
 **********************************************************************/
void usrp_e_impl::tx_duc_init(void){
    _tx_duc_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e_impl::tx_duc_get, this, _1, _2),
        boost::bind(&usrp_e_impl::tx_duc_set, this, _1, _2)
    );
}

/***********************************************************************
 * TX DUC Get
 **********************************************************************/
void usrp_e_impl::tx_duc_get(const wax::obj &, wax::obj &){
    UHD_THROW_PROP_GET_ERROR();
}

/***********************************************************************
 * TX DUC Set
 **********************************************************************/
void usrp_e_impl::tx_duc_set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}
