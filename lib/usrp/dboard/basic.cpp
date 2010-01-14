//
// Copyright 2010 Ettus Research LLC
//

#include "dboards.hpp"

/***********************************************************************
 * Basic RX dboard
 **********************************************************************/
basic_rx::basic_rx(size_t subdev_index, interface::sptr dboard_interface)
: rx_base(subdev_index, dboard_interface){
    /* NOP */
}

basic_rx::~basic_rx(void){
    /* NOP */
}

void basic_rx::rx_get(const wax::type &, wax::type &){
    /* TODO */
}

void basic_rx::rx_set(const wax::type &, const wax::type &){
    /* TODO */
}

/***********************************************************************
 * Basic TX dboard
 **********************************************************************/
basic_tx::basic_tx(size_t subdev_index, interface::sptr dboard_interface)
: tx_base(subdev_index, dboard_interface){
    /* NOP */
}

basic_tx::~basic_tx(void){
    /* NOP */
}

void basic_tx::tx_get(const wax::type &, wax::type &){
    /* TODO */
}

void basic_tx::tx_set(const wax::type &, const wax::type &){
    /* TODO */
}
