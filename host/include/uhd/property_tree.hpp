//
// Copyright 2011,2014-2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace uhd {

/*!
 * A non-templated class which exists solely so we can
 * dynamic_cast between properties.
 */
class UHD_API property_iface
{
public:
    virtual ~property_iface() = default;
};

/*!
 * A templated property interface for holding the state
 * associated with a property in a uhd::property_tree
 * and registering callbacks when that value changes.
 *
 * A property is defined to have two separate values:
 * - Desired value: Value requested by the user
 * - Coerced value: Value that was actually possible
 *                  given HW and other requirements
 *
 * By default, the desired and coerced values are
 * identical as long as the property is not coerced.
 * A property can be coerced in two ways:
 * 1. Using a coercer: A callback function that takes
 *    in a desired value and produces a coerced value.
 *    A property must have *exactly one* coercer.
 * 2. Manual coercion: Manually calling the set_coerced
 *    API function to coerce the value of the property. In
 *    order to use manual coercion, the property must be
 *    created with the MANUAL_COERCE mode.
 * If the coerce mode for a property is AUTO_COERCE then
 * it always has a coercer. If the set_coercer API is
 * never used, then the default coercer is used which
 * simply set the coerced value to the desired value.
 *
 * It is possible to get notified every time the desired
 * or coerced values of a property potentially change
 * using subscriber callbacks. Every property can have
 * zero or more desired and coerced subscribers.
 *
 * If storing the property readback state in software is
 * not appropriate (for example if it needs to be queried
 * from hardware) then it is possible to use a publisher
 * callback to get the value of the property. Calling
 * get on the property will always call the publisher and
 * the cached desired and coerced values are updated only
 * using set* calls. A property must have *at most one*
 * publisher. It is legal to have both a coercer
 * and publisher for a property but the only way to access
 * the desired and coerced values in that case would be by
 * notification using the desired and coerced subscribers.
 * Publishers are useful for creating read-only properties.
 *
 * Requirements for the template type T:
 * - T must have a copy constructor
 * - T must have an assignment operator
 */
template <typename T>
class UHD_API_HEADER property : uhd::noncopyable, public property_iface
{
public:
    typedef std::function<void(const T&)> subscriber_type;
    typedef std::function<T(void)> publisher_type;
    typedef std::function<T(const T&)> coercer_type;

    virtual ~property(void) = 0;

    /*!
     * Register a coercer into the property.
     * A coercer is a callback function that updates the
     * coerced value of a property.
     *
     * Only one coercer may be registered per property.
     * \param coercer the coercer callback function
     * \return a reference to this property for chaining
     * \throws uhd::assertion_error if called more than once
     */
    virtual property<T>& set_coercer(const coercer_type& coercer) = 0;

    /*!
     * Register a publisher into the property.
     * A publisher is a callback function the provides the value
     * for a property.
     *
     * Only one publisher may be registered per property.
     * \param publisher the publisher callback function
     * \return a reference to this property for chaining
     * \throws uhd::assertion_error if called more than once
     */
    virtual property<T>& set_publisher(const publisher_type& publisher) = 0;

    /*!
     * Register a subscriber into the property.
     * All desired subscribers are called when the desired value
     * potentially changes.
     *
     * Once a subscriber is registered, it cannot be unregistered.
     * \param subscriber the subscriber callback function
     * \return a reference to this property for chaining
     */
    virtual property<T>& add_desired_subscriber(const subscriber_type& subscriber) = 0;

    /*!
     * Register a subscriber into the property.
     * All coerced subscribers are called when the coerced value
     * potentially changes.
     *
     * Once a subscriber is registered, it cannot be unregistered.
     * \param subscriber the subscriber callback function
     * \return a reference to this property for chaining
     */
    virtual property<T>& add_coerced_subscriber(const subscriber_type& subscriber) = 0;

    /*!
     * Calls all subscribers with the current value.
     *
     * \return a reference to this property for chaining
     * \throws uhd::assertion_error
     */
    virtual property<T>& update(void) = 0;

    /*!
     * Set the new value and call all the necessary subscribers.
     * Order of operations:
     * - The desired value of the property is updated
     * - All desired subscribers are called
     * - If coerce mode is AUTO then the coercer is called
     * - If coerce mode is AUTO then all coerced subscribers are called
     *
     * \param value the new value to set on this property
     * \return a reference to this property for chaining
     * \throws uhd::assertion_error
     */
    virtual property<T>& set(const T& value) = 0;

    /*!
     * Set a coerced value and call all subscribers.
     * The coercer is bypassed, and the specified value is
     * used as the coerced value. All coerced subscribers
     * are called. This function can only be used when the
     * coerce mode is set to MANUAL_COERCE.
     *
     * \param value the new value to set on this property
     * \return a reference to this property for chaining
     * \throws uhd::assertion_error
     */
    virtual property<T>& set_coerced(const T& value) = 0;

    /*!
     * Get the current value of this property.
     * The publisher (when provided) yields the value,
     * otherwise an internal coerced value is returned.
     *
     * \return the current value in the property
     * \throws uhd::assertion_error
     */
    virtual const T get(void) const = 0;

    /*!
     * Get the current desired value of this property.
     *
     * \return the current desired value in the property
     * \throws uhd::assertion_error
     */
    virtual const T get_desired(void) const = 0;

    /*!
     * A property is empty if it has never been set.
     * A property with a publisher is never empty.
     *
     * \return true if the property is empty
     */
    virtual bool empty(void) const = 0;
};

template <typename T>
property<T>::~property(void)
{
    /* NOP */
}

/*!
 * FS Path: A glorified string with path manipulations.
 * Inspired by boost filesystem path, but without the dependency.
 */
struct UHD_API_HEADER fs_path : std::string
{
    UHD_API fs_path(void);
    UHD_API fs_path(const char*);
    UHD_API fs_path(const std::string&);
    UHD_API std::string leaf(void) const;
    UHD_API fs_path branch_path(void) const;
};

UHD_API fs_path operator/(const fs_path&, const fs_path&);
UHD_API fs_path operator/(const fs_path&, size_t);

/*!
 * The property tree provides a file system structure for accessing properties.
 */
class UHD_API property_tree : uhd::noncopyable
{
public:
    typedef std::shared_ptr<property_tree> sptr;

    enum coerce_mode_t { AUTO_COERCE, MANUAL_COERCE };

    virtual ~property_tree(void) = 0;

    //! Create a new + empty property tree
    static sptr make(void);

    //! Get a subtree with a new root starting at path
    virtual sptr subtree(const fs_path& path) const = 0;

    //! Remove a property or directory (recursive)
    virtual void remove(const fs_path& path) = 0;

    //! True if the path exists in the tree
    virtual bool exists(const fs_path& path) const = 0;

    //! Get an iterable to all things in the given path
    virtual std::vector<std::string> list(const fs_path& path) const = 0;

    //! Create a new property entry in the tree
    template <typename T>
    property<T>& create(const fs_path& path, coerce_mode_t coerce_mode = AUTO_COERCE);

    //! Get access to a property in the tree
    template <typename T>
    property<T>& access(const fs_path& path);

    //! Pop a property off the tree, and returns the property
    template <typename T>
    std::shared_ptr<property<T>> pop(const fs_path& path);

private:
    //! Internal pop function
    virtual std::shared_ptr<property_iface> _pop(const fs_path& path) = 0;

    //! Internal create property with wild-card type
    virtual void _create(
        const fs_path& path, const std::shared_ptr<property_iface>& prop) = 0;

    //! Internal access property with wild-card type
    virtual std::shared_ptr<property_iface>& _access(const fs_path& path) const = 0;
};

} // namespace uhd

#include <uhd/property_tree.ipp>
