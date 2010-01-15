//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/dboard/base.hpp>

using namespace usrp_uhd::usrp::dboard;

/***********************************************************************
 * base dboard base class
 **********************************************************************/
base::base(ctor_args_t const& args)
 : _subdev_index(args.get<0>()), _dboard_interface(args.get<1>()){
    /* NOP */
}

base::~base(void){
    /* NOP */
}

size_t base::get_subdev_index(void){
    return _subdev_index;
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
