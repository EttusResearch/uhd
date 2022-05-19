//
// Copyright 2011,2014-2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <memory>
#include <vector>

/***********************************************************************
 * Implement templated property impl
 **********************************************************************/
namespace uhd { namespace /*anon*/ {

template <typename T>
class property_impl : public property<T>
{
public:
    property_impl(property_tree::coerce_mode_t mode) : _coerce_mode(mode)
    {
        if (_coerce_mode == property_tree::AUTO_COERCE) {
            _coercer = DEFAULT_COERCER;
        }
    }

    ~property_impl(void)
    {
        /* NOP */
    }

    property<T>& set_coercer(const typename property<T>::coercer_type& coercer)
    {
        if (_coercer) {
            uhd::assertion_error("cannot register more than one coercer for a property");
        }
        if (_coerce_mode == property_tree::MANUAL_COERCE)
            uhd::assertion_error(
                "cannot register coercer for a manually coerced property");

        _coercer = coercer;
        return *this;
    }

    property<T>& set_publisher(const typename property<T>::publisher_type& publisher)
    {
        if (_publisher) {
            uhd::assertion_error(
                "cannot register more than one publisher for a property");
        }

        _publisher = publisher;
        return *this;
    }

    property<T>& add_desired_subscriber(
        const typename property<T>::subscriber_type& subscriber)
    {
        _desired_subscribers.push_back(subscriber);
        return *this;
    }

    property<T>& add_coerced_subscriber(
        const typename property<T>::subscriber_type& subscriber)
    {
        _coerced_subscribers.push_back(subscriber);
        return *this;
    }

    property<T>& update(void)
    {
        this->set(this->get());
        return *this;
    }

    void _set_coerced(const T& value)
    {
        init_or_set_value(_coerced_value, value);
        for (typename property<T>::subscriber_type& csub : _coerced_subscribers) {
            csub(get_value_ref(_coerced_value)); // let errors propagate
        }
    }

    property<T>& set(const T& value)
    {
        init_or_set_value(_value, value);
        for (typename property<T>::subscriber_type& dsub : _desired_subscribers) {
            dsub(get_value_ref(_value)); // let errors propagate
        }
        if (_coercer) {
            _set_coerced(_coercer(get_value_ref(_value)));
        } else {
            if (_coerce_mode == property_tree::AUTO_COERCE)
                uhd::assertion_error("coercer missing for an auto coerced property");
        }
        return *this;
    }

    property<T>& set_coerced(const T& value)
    {
        if (_coerce_mode == property_tree::AUTO_COERCE)
            uhd::assertion_error("cannot set coerced value an auto coerced property");
        _set_coerced(value);
        return *this;
    }

    const T get(void) const
    {
        if (empty()) {
            throw uhd::runtime_error("Cannot get() on an uninitialized (empty) property");
        }
        if (_publisher) {
            return _publisher();
        } else {
            if (_coerced_value.get() == NULL
                and _coerce_mode == property_tree::MANUAL_COERCE)
                throw uhd::runtime_error(
                    "uninitialized coerced value for manually coerced attribute");
            return get_value_ref(_coerced_value);
        }
    }

    const T get_desired(void) const
    {
        if (_value.get() == NULL)
            throw uhd::runtime_error(
                "Cannot get_desired() on an uninitialized (empty) property");

        return get_value_ref(_value);
    }

    bool empty(void) const
    {
        return !bool(_publisher) and _value.get() == NULL;
    }

private:
    static T DEFAULT_COERCER(const T& value)
    {
        return value;
    }

    static void init_or_set_value(std::unique_ptr<T>& scoped_value, const T& init_val)
    {
        if (scoped_value.get() == NULL) {
            scoped_value.reset(new T(init_val));
        } else {
            *scoped_value = init_val;
        }
    }

    static const T& get_value_ref(const std::unique_ptr<T>& scoped_value)
    {
        if (scoped_value.get() == NULL)
            throw uhd::assertion_error("Cannot use uninitialized property data");
        return *scoped_value.get();
    }

    const property_tree::coerce_mode_t _coerce_mode;
    std::vector<typename property<T>::subscriber_type> _desired_subscribers;
    std::vector<typename property<T>::subscriber_type> _coerced_subscribers;
    typename property<T>::publisher_type _publisher;
    typename property<T>::coercer_type _coercer;
    std::unique_ptr<T> _value;
    std::unique_ptr<T> _coerced_value;
};

}} // namespace uhd::

/***********************************************************************
 * Implement templated methods for the property tree
 **********************************************************************/
namespace uhd {

template <typename T>
property<T>& property_tree::create(const fs_path& path, coerce_mode_t coerce_mode)
{
    this->_create(path, std::make_shared<property_impl<T>>(coerce_mode));
    return this->access<T>(path);
}

template <typename T>
property<T>& property_tree::access(const fs_path& path)
{
    auto ptr = std::dynamic_pointer_cast<property<T>>(this->_access(path));
    if (!ptr) {
        throw uhd::type_error("Property " + path + " exists, but was accessed with wrong type");
    }
    return *ptr;
}

template <typename T>
typename std::shared_ptr<property<T>> property_tree::pop(const fs_path& path)
{
    auto ptr = std::dynamic_pointer_cast<property<T>>(this->_pop(path));
    if (!ptr) {
        throw uhd::type_error("Property " + path + " exists, but was accessed with wrong type");
    }
    return ptr;
}

} // namespace uhd
