//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/dboard/base.hpp>

using namespace usrp_uhd::usrp::dboard;

/***********************************************************************
 * base dboard base class
 **********************************************************************/
base::base(ctor_args_t const& args){
    boost::tie(_subdev_name, _dboard_interface) = args;
}

base::~base(void){
    /* NOP */
}

std::string base::get_subdev_name(void){
    return _subdev_name;
}

interface::sptr base::get_interface(void){
    return _dboard_interface;
}

/***********************************************************************
 * xcvr dboard base class
 **********************************************************************/
xcvr_base::xcvr_base(ctor_args_t const& args) : base(args){
    /* NOP */
}

xcvr_base::~xcvr_base(void){
    /* NOP */
}

/***********************************************************************
 * rx dboard base class
 **********************************************************************/
rx_base::rx_base(ctor_args_t const& args) : base(args){
    /* NOP */
}

rx_base::~rx_base(void){
    /* NOP */
}

void rx_base::tx_get(const wax::type &, wax::type &){
    throw std::runtime_error("cannot call tx_get on a rx dboard");
}

void rx_base::tx_set(const wax::type &, const wax::type &){
    throw std::runtime_error("cannot call tx_set on a rx dboard");
}

/***********************************************************************
 * tx dboard base class
 **********************************************************************/
tx_base::tx_base(ctor_args_t const& args) : base(args){
    /* NOP */
}

tx_base::~tx_base(void){
    /* NOP */
}

void tx_base::rx_get(const wax::type &, wax::type &){
    throw std::runtime_error("cannot call rx_get on a tx dboard");
}

void tx_base::rx_set(const wax::type &, const wax::type &){
    throw std::runtime_error("cannot call rx_set on a tx dboard");
}
