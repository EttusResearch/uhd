//
// Copyright 2011,2014-2016 Ettus Research
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
    property_impl<T>(property_tree::coerce_mode_t mode) : _coerce_mode(mode){
        if (_coerce_mode == property_tree::AUTO_COERCE) {
            _coercer = DEFAULT_COERCER;
        }
    }

    ~property_impl<T>(void){
        /* NOP */
    }

    property<T> &set_coercer(const typename property<T>::coercer_type &coercer){
        if (not _coercer.empty()) uhd::assertion_error("cannot register more than one coercer for a property");
        if (_coerce_mode == property_tree::MANUAL_COERCE) uhd::assertion_error("cannot register coercer for a manually coerced property");

        _coercer = coercer;
        return *this;
    }

    property<T> &set_publisher(const typename property<T>::publisher_type &publisher){
        if (not _publisher.empty()) uhd::assertion_error("cannot register more than one publisher for a property");

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

    void _set_coerced(const T &value){
        init_or_set_value(_coerced_value, value);
        BOOST_FOREACH(typename property<T>::subscriber_type &csub, _coerced_subscribers){
            csub(get_value_ref(_coerced_value)); //let errors propagate
        }
    }

    property<T> &set(const T &value){
        init_or_set_value(_value, value);
        BOOST_FOREACH(typename property<T>::subscriber_type &dsub, _desired_subscribers){
            dsub(get_value_ref(_value)); //let errors propagate
        }
        if (not _coercer.empty()) {
            _set_coerced(_coercer(get_value_ref(_value)));
        } else {
            if (_coerce_mode == property_tree::AUTO_COERCE) uhd::assertion_error("coercer missing for an auto coerced property");
        }
        return *this;
    }

    property<T> &set_coerced(const T &value){
        if (_coerce_mode == property_tree::AUTO_COERCE) uhd::assertion_error("cannot set coerced value an auto coerced property");
        _set_coerced(value);
        return *this;
    }

    const T get(void) const{
        if (empty()) {
            throw uhd::runtime_error("Cannot get() on an uninitialized (empty) property");
        }
        if (not _publisher.empty()) {
            return _publisher();
        } else {
            if (_coerced_value.get() == NULL and _coerce_mode == property_tree::MANUAL_COERCE)
                throw uhd::runtime_error("uninitialized coerced value for manually coerced attribute");
            return get_value_ref(_coerced_value);
        }
    }

    const T get_desired(void) const{
        if (_value.get() == NULL) throw uhd::runtime_error("Cannot get_desired() on an uninitialized (empty) property");

        return get_value_ref(_value);
    }

    bool empty(void) const{
        return _publisher.empty() and _value.get() == NULL;
    }

private:
    static T DEFAULT_COERCER(const T& value) {
        return value;
    }

    static void init_or_set_value(boost::scoped_ptr<T>& scoped_value, const T& init_val) {
        if (scoped_value.get() == NULL) {
            scoped_value.reset(new T(init_val));
        } else {
            *scoped_value = init_val;
        }
    }

    static const T& get_value_ref(const boost::scoped_ptr<T>& scoped_value) {
        if (scoped_value.get() == NULL) throw uhd::assertion_error("Cannot use uninitialized property data");
        return *scoped_value.get();
    }

    const property_tree::coerce_mode_t                  _coerce_mode;
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

    template <typename T> property<T> &property_tree::create(const fs_path &path, coerce_mode_t coerce_mode){
        this->_create(path, typename boost::shared_ptr<property<T> >(new property_impl<T>(coerce_mode)));
        return this->access<T>(path);
    }

    template <typename T> property<T> &property_tree::access(const fs_path &path){
        return *boost::static_pointer_cast<property<T> >(this->_access(path));
    }

} //namespace uhd

#endif /* INCLUDED_UHD_PROPERTY_TREE_IPP */
