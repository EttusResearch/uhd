//
// Copyright 2010 Ettus Research LLC
//

#include "dboards.hpp"

/***********************************************************************
 * Basic RX dboard
 **********************************************************************/
basic_rx::basic_rx(ctor_args_t const& args) : rx_base(args){
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
basic_tx::basic_tx(ctor_args_t const& args) : tx_base(args){
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
