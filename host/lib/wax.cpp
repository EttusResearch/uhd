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

#include <uhd/wax.hpp>
#include <boost/format.hpp>
#include <stdexcept>

/*!
 * The link args for internal use within this cpp file:
 *
 * It contains a link (in this case a pointer) to a wax object.
 * Only the methods in this file may create or parse link args.
 * The get_link method is the creator of a link args object.
 * The [] operator will resolve the link and make the [] call.
 *
 * TODO: register the link args with the wax obj that it links to.
 * That way, if the obj destructs, the link can be invalidated.
 * The operator() will throw, rather than dereferencing bad memory.
 */
class link_args_t{
public:
    link_args_t(const wax::obj *obj_ptr) : _obj_ptr(obj_ptr){
        /* NOP */
    }
    wax::obj & operator()(void) const{
        //recursively resolve link args to get at original pointer
        if (_obj_ptr->type() == typeid(link_args_t)){
            return _obj_ptr->as<link_args_t>()();
        }
        return *const_cast<wax::obj *>(_obj_ptr);
    }
private:
    const wax::obj *_obj_ptr;
};

/*!
 * The proxy args for internal use within this cpp file:
 *
 * It contains a link and a key for setting/getting a property.
 * Only the methods in this file may create or parse proxy args.
 * Class methods have special handling for the case when the
 * wax obj contains an instance of the proxy args.
 */
class proxy_args_t{
public:
    proxy_args_t(const wax::obj *obj_ptr, const wax::obj &key) : _key(key){
        _obj_link = obj_ptr->get_link();
    }
    wax::obj & operator()(void) const{
        return _obj_link.as<link_args_t>()();
    }
    const wax::obj & key(void) const{
        return _key;
    }
private:
    wax::obj _obj_link;
    const wax::obj _key;
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
            return val.as<link_args_t>()()[key];
        }
        //unknown obj
        throw std::runtime_error("cannot use [] on non wax::obj link");
    }
    else{
        return proxy_args_t(this, key);
    }
}

wax::obj & wax::obj::operator=(const obj &val){
    if (_contents.type() == typeid(proxy_args_t)){
        proxy_args_t proxy_args = boost::any_cast<proxy_args_t>(_contents);
        proxy_args().set(proxy_args.key(), val);
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
    return link_args_t(this);
}

const std::type_info & wax::obj::type(void) const{
    return resolve().type();
}

/***********************************************************************
 * Private Methods
 **********************************************************************/
boost::any wax::obj::resolve(void) const{
    if (_contents.type() == typeid(proxy_args_t)){
        obj val;
        proxy_args_t proxy_args = boost::any_cast<proxy_args_t>(_contents);
        proxy_args().get(proxy_args.key(), val);
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
