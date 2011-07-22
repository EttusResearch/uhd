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

#ifndef INCLUDED_UHD_PROPERTY_TREE_HPP
#define INCLUDED_UHD_PROPERTY_TREE_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

namespace uhd{

/*!
 * A templated property interface for holding a value
 * and registering callbacks when that value changes.
 */
template <typename T> class property : boost::noncopyable{
public:
    typedef boost::function<void(const T &)> subscriber_type;
    typedef boost::function<T(void)> publisher_type;
    typedef boost::function<T(const T &)> coercer_type;

    /*!
     * Register a coercer into the property.
     * A coercer is a special subscribes that coerces the value.
     * Only one coercer may be registered per property.
     * Registering a coercer replaces the previous coercer.
     * \param coercer the coercer callback function
     * \return a reference to this property for chaining
     */
    virtual property<T> &coerce(const coercer_type &coercer) = 0;

    /*!
     * Register a publisher into the property.
     * A publisher is a special callback the provides the value.
     * Publishers are useful for creating read-only properties.
     * Only one publisher may be registered per property.
     * Registering a publisher replaces the previous publisher.
     * \param publisher the publisher callback function
     * \return a reference to this property for chaining
     */
    virtual property<T> &publish(const publisher_type &publisher) = 0;

    /*!
     * Register a subscriber into the property.
     * All subscribers are called when the value changes.
     * Once a subscriber is registered, it cannot be unregistered.
     * \param subscriber the subscriber callback function
     * \return a reference to this property for chaining
     */
    virtual property<T> &subscribe(const subscriber_type &subscriber) = 0;

    /*!
     * Update calls all subscribers w/ the current value.
     * \return a reference to this property for chaining
     */
    virtual property<T> &update(void) = 0;

    /*!
     * Set the new value and call all subscribers.
     * The coercer (when provided) is called initially,
     * and the coerced value is used to set the subscribers.
     * \param value the new value to set on this property
     * \return a reference to this property for chaining
     */
    virtual property<T> &set(const T &value) = 0;

    /*!
     * Get the current value of this property.
     * The publisher (when provided) yields the value,
     * otherwise an internal shadow is used for the value.
     * \return the current value in the property
     */
    virtual T get(void) const = 0;

    /*!
     * A property is empty if it has never been set.
     * A property with a publisher is never empty.
     * \return true if the property is empty
     */
    virtual bool empty(void) const = 0;
};

/*!
 * FS Path: A glorified string with path manipulations.
 * Inspired by boost filesystem path, but without the dependency.
 *
 * Notice: we do not declare UHD_API on the whole structure
 * because MSVC will do weird things with std::string and linking.
 */
struct fs_path : std::string{
    UHD_API fs_path(void);
    UHD_API fs_path(const char *);
    UHD_API fs_path(const std::string &);
    UHD_API std::string leaf(void) const;
    UHD_API fs_path branch_path(void) const;
};

UHD_API fs_path operator/(const fs_path &, const fs_path &);

/*!
 * The property tree provides a file system structure for accessing properties.
 */
class UHD_API property_tree : boost::noncopyable{
public:
    typedef boost::shared_ptr<property_tree> sptr;

    //! Create a new + empty property tree
    static sptr make(void);

    //! Get a subtree with a new root starting at path
    virtual sptr subtree(const fs_path &path) const = 0;

    //! Remove a property or directory (recursive)
    virtual void remove(const fs_path &path) = 0;

    //! True if the path exists in the tree
    virtual bool exists(const fs_path &path) const = 0;

    //! Get an iterable to all things in the given path
    virtual std::vector<std::string> list(const fs_path &path) const = 0;

    //! Create a new property entry in the tree
    template <typename T> property<T> &create(const fs_path &path);

    //! Get access to a property in the tree
    template <typename T> property<T> &access(const fs_path &path);

private:
    //! Internal create property with wild-card type
    virtual void _create(const fs_path &path, const boost::shared_ptr<void> &prop) = 0;

    //! Internal access property with wild-card type
    virtual boost::shared_ptr<void> &_access(const fs_path &path) const = 0;

};

} //namespace uhd

#include <uhd/property_tree.ipp>

#endif /* INCLUDED_UHD_PROPERTY_TREE_HPP */
