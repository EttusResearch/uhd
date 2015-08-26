//
// Copyright 2011,2014 Ettus Research LLC
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

#ifndef INCLUDED_UHD_PROPERTY_TREE_IPP
#define INCLUDED_UHD_PROPERTY_TREE_IPP

#include <uhd/exception.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

/***********************************************************************
 * Implement templated property impl
 **********************************************************************/
namespace uhd{ namespace /*anon*/{

template <typename T> class property_impl : public property<T>{
public:

    ~property_impl<T>(void){
        /* NOP */
    }

    property<T> &set_coercer(const typename property<T>::coercer_type &coercer){
        if (not _coercer.empty()) uhd::assertion_error("cannot register more than one coercer for a property");
        if (not _publisher.empty()) uhd::assertion_error("cannot register a coercer and publisher for the same property");

        _coercer = coercer;
        return *this;
    }

    property<T> &set_publisher(const typename property<T>::publisher_type &publisher){
        if (not _publisher.empty()) uhd::assertion_error("cannot register more than one publisher for a property");
        if (not _coercer.empty()) uhd::assertion_error("cannot register a coercer and publisher for the same property");

        _publisher = publisher;
        return *this;
    }

    property<T> &add_desired_subscriber(const typename property<T>::subscriber_type &subscriber){
        _desired_subscribers.push_back(subscriber);
        return *this;
    }

    property<T> &add_coerced_subscriber(const typename property<T>::subscriber_type &subscriber){
        _coerced_subscribers.push_back(subscriber);
        return *this;
    }

    property<T> &update(void){
        this->set(this->get());
        return *this;
    }

    property<T> &set(const T &value){
        init_or_set_value(_value, value);
        BOOST_FOREACH(typename property<T>::subscriber_type &dsub, _desired_subscribers){
            dsub(get_value_ref(_value)); //let errors propagate
        }
        if (not _coercer.empty()) {
            init_or_set_value(_coerced_value, _coercer(get_value_ref(_value)));
        }
        BOOST_FOREACH(typename property<T>::subscriber_type &csub, _coerced_subscribers){
            csub(get_value_ref(_coercer.empty() ? _value : _coerced_value)); //let errors propagate
        }
        return *this;
    }

    const T get(void) const{
        if (empty()) throw uhd::runtime_error("Cannot get() on an empty property");
        if (not _publisher.empty()) {
            return _publisher();
        } else {
            return get_value_ref(_coercer.empty() ? _value : _coerced_value);
        }
    }

    const T get_desired(void) const{
        if (_value.get() == NULL) throw uhd::runtime_error("Cannot get_desired() on an empty property");
        if (not _publisher.empty()) throw uhd::runtime_error("Cannot get_desired() on a property with a publisher");

        return get_value_ref(_value);
    }

    bool empty(void) const{
        return _publisher.empty() and _value.get() == NULL;
    }

private:
    static void init_or_set_value(boost::scoped_ptr<T>& scoped_value, const T& init_val) {
        if (scoped_value.get() == NULL) {
            scoped_value.reset(new T(init_val));
        } else {
            *scoped_value = init_val;
        }
    }

    static const T& get_value_ref(const boost::scoped_ptr<T>& scoped_value) {
        if (scoped_value.get() == NULL) throw uhd::assertion_error("Cannot use uninitialized property data");
        return *static_cast<const T*>(scoped_value.get());
    }

    std::vector<typename property<T>::subscriber_type>  _desired_subscribers;
    std::vector<typename property<T>::subscriber_type>  _coerced_subscribers;
    typename property<T>::publisher_type                _publisher;
    typename property<T>::coercer_type                  _coercer;
    boost::scoped_ptr<T>                                _value;
    boost::scoped_ptr<T>                                _coerced_value;
};

}} //namespace uhd::/*anon*/

/***********************************************************************
 * Implement templated methods for the property tree
 **********************************************************************/
namespace uhd{

    template <typename T> property<T> &property_tree::create(const fs_path &path){
        this->_create(path, typename boost::shared_ptr<property<T> >(new property_impl<T>()));
        return this->access<T>(path);
    }

    template <typename T> property<T> &property_tree::access(const fs_path &path){
        return *boost::static_pointer_cast<property<T> >(this->_access(path));
    }

} //namespace uhd

#endif /* INCLUDED_UHD_PROPERTY_TREE_IPP */
