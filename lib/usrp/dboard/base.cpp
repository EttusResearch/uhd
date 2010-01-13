//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/dboard/base.hpp>

using namespace usrp_uhd::usrp::dboard;

/***********************************************************************
 * xcvr dboard base class
 **********************************************************************/
xcvr_base::xcvr_base(size_t subdev_index, interface::sptr dboard_interface)
 : _subdev_index(subdev_index), _dboard_interface(dboard_interface){
    /* NOP */
}

xcvr_base::~xcvr_base(void){
    /* NOP */
}

size_t xcvr_base::get_subdev_index(void){
    return _subdev_index;
}

interface::sptr xcvr_base::get_interface(void){
    return _dboard_interface;
}

/***********************************************************************
 * rx dboard base class
 **********************************************************************/
rx_base::rx_base(size_t subdev_index, interface::sptr dboard_interface)
: xcvr_base(subdev_index, dboard_interface){
    /* NOP */
}

rx_base::~rx_base(void){
    /* NOP */
}

void rx_base::tx_get(const wax::type &key, wax::type &val){
    throw std::runtime_error("cannot call tx_get on a rx dboard");
}

void rx_base::tx_set(const wax::type &key, const wax::type &val){
    throw std::runtime_error("cannot call tx_set on a rx dboard");
}

/***********************************************************************
 * tx dboard base class
 **********************************************************************/
tx_base::tx_base(size_t subdev_index, interface::sptr dboard_interface)
: xcvr_base(subdev_index, dboard_interface){
    /* NOP */
}

tx_base::~tx_base(void){
    /* NOP */
}

void tx_base::rx_get(const wax::type &key, wax::type &val){
    throw std::runtime_error("cannot call rx_get on a tx dboard");
}

void tx_base::rx_set(const wax::type &key, const wax::type &val){
    throw std::runtime_error("cannot call rx_set on a tx dboard");
}
