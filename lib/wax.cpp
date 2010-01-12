//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/wax.hpp>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/format.hpp>

/***********************************************************************
 * WAX Object
 **********************************************************************/
wax::obj::obj(void){
    /* NOP */
}

wax::obj::~obj(void){
    /* NOP */
}

wax::proxy wax::obj::operator[](const type &key){
    return proxy(
        boost::bind(&obj::get, this, key, _1),
        boost::bind(&obj::set, this, key, _1)
    );
}

/***********************************************************************
 * WAX Proxy
 **********************************************************************/
wax::proxy::proxy(wax::proxy::getter_t getter, wax::proxy::setter_t setter)
: d_getter(getter), d_setter(setter){
    /* NOP */
}

wax::proxy::~proxy(void){
    /* NOP */
}

wax::proxy wax::proxy::operator[](const type &key){
    type val((*this)());
    //check if its a regular pointer and call
    if (val.type() == typeid(obj::ptr)){
        return (*cast<obj::ptr>(val))[key];
    }
    //check if its a smart pointer and call
    if (val.type() == typeid(obj::sptr)){
        return (*cast<obj::sptr>(val))[key];
    }
    //unknown type
    throw std::runtime_error("cannot use [] on non wax::obj pointer");
}

wax::proxy wax::proxy::operator=(const type &val){
    d_setter(val);
    return *this;
}

wax::type wax::proxy::operator()(void){
    type val;
    d_getter(val);
    return val;
}

/***********************************************************************
 * WAX Type
 **********************************************************************/
std::ostream& operator<<(std::ostream &os, const wax::type &x){
    os << boost::format("WAX type (%s)") % x.type().name();
    return os;
}
