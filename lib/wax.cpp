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

#include <usrp_uhd/wax.hpp>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>

/*!
 * The proxy args for internal use within this cpp file:
 *
 * It contains bound functions for setting/getting a property.
 * Only the methods in this file may create or parse proxy args.
 * Class methods have special handling for the case when the
 * wax obj contains an instance of the proxy args.
 */ 
struct proxy_args_t{
    boost::function<void(wax::obj &)>       get;
    boost::function<void(const wax::obj &)> set;
};

/*!
 * The link args for internal use within this cpp file:
 *
 * It contains a link (in this case a pointer) to a wax object.
 * Only the methods in this file may create or parse link args.
 * The get_link method is the creator of a link args object.
 * The [] operator will resolve the link and make the [] call.
 */
struct link_args_t{
    wax::obj *obj_ptr;
};

/***********************************************************************
 * Structors
 **********************************************************************/
wax::obj::obj(void){
    /* NOP */
}

wax::obj::obj(const obj &o){
    _contents = o._contents;
}

wax::obj::~obj(void){
    /* NOP */
}

/***********************************************************************
 * Special Operators
 **********************************************************************/
wax::obj wax::obj::operator[](const obj &key){
    if (_contents.type() == typeid(proxy_args_t)){
        obj val = resolve();
        //check if its a special link and call
        if (val.type() == typeid(link_args_t)){
            return (*cast<link_args_t>(val).obj_ptr)[key];
        }
        //unknown obj
        throw std::runtime_error("cannot use [] on non wax::obj link");
    }
    else{
        proxy_args_t proxy_args;
        proxy_args.get = boost::bind(&obj::get, this, key, _1);
        proxy_args.set = boost::bind(&obj::set, this, key, _1);
        return wax::obj(proxy_args);
    }
}

wax::obj & wax::obj::operator=(const obj &val){
    if (_contents.type() == typeid(proxy_args_t)){
        boost::any_cast<proxy_args_t>(_contents).set(val);
    }
    else{
        _contents = val._contents;
    }
    return *this;
}

/***********************************************************************
 * Public Methods
 **********************************************************************/
wax::obj wax::obj::get_link(void) const{
    link_args_t link_args;
    link_args.obj_ptr = const_cast<obj*>(this);
    return link_args;
}

const std::type_info & wax::obj::type(void) const{
    return _contents.type();
}

/***********************************************************************
 * Private Methods
 **********************************************************************/
boost::any wax::obj::resolve(void) const{
    if (_contents.type() == typeid(proxy_args_t)){
        obj val;
        boost::any_cast<proxy_args_t>(_contents).get(val);
        return val.resolve();
    }
    else{
        return _contents;
    }
}

void wax::obj::get(const obj &, obj &){
    throw std::runtime_error("Cannot call get on wax obj base class");
}

void wax::obj::set(const obj &, const obj &){
    throw std::runtime_error("Cannot call set on wax obj base class");
}

/***********************************************************************
 * Friends
 **********************************************************************/
std::ostream& operator<<(std::ostream &os, const wax::obj &x){
    os << boost::format("WAX obj (%s)") % x.type().name();
    return os;
}
