//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_UHD_PROPERTY_HPP
#define INCLUDED_UHD_PROPERTY_HPP

#include <uhd/config.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <list>

namespace uhd{

template <typename T> class property{
public:
    typedef boost::function<void(const T &)> subscriber_type;
    typedef boost::function<T(const T &)> master_type;

    /*!
     * Register a master subscriber into the property.
     * A master is a special subscriber that coerces the value.
     * Only one master may be registered per property.
     * Registering a master replaces the previous master.
     */
    void subscribe_master(const master_type &master){
        _master = master;
    }

    /*!
     * Register a subscriber into the property.
     * All subscribers are called when the value changes.
     * Once a subscriber is registered, it cannot be unregistered.
     */
    void subscribe(const subscriber_type &subscriber){
        _subscribers.push_back(subscriber);
    }

    //! Update calls all subscribers w/ the current value
    void update(void){
        this->set(this->get());
    }

    /*!
     * Set the new value and call all subscribers.
     * The master is called first to coerce the value.
     */
    void set(const T &value){
        _value = _master.empty()? value : _master(value);
        BOOST_FOREACH(subscriber_type &subscriber, _subscribers){
            subscriber(_value); //let errors propagate
        }
    }

    //! Get the current value of this property
    T get(void) const{
        return _value;
    }

private:
    std::list<subscriber_type> _subscribers;
    master_type _master;
    T _value;
};

} //namespace uhd

#endif /* INCLUDED_UHD_PROPERTY_HPP */
