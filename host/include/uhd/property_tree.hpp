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
#include <boost/filesystem/path.hpp>
#include <vector>

namespace uhd{

/*!
 * A templated property interface for holding a value
 * and registering callbacks when that value changes.
 */
template <typename T> class UHD_API property : boost::noncopyable{
public:
    typedef boost::function<void(const T &)> subscriber_type;
    typedef boost::function<T(void)> publisher_type;
    typedef boost::function<T(const T &)> master_type;

    /*!
     * Register a master subscriber into the property.
     * A master is a special subscriber that coerces the value.
     * Only one master may be registered per property.
     * Registering a master replaces the previous master.
     */
    virtual property<T> &subscribe_master(const master_type &master) = 0;

    /*!
     * Register a publisher into the property.
     * A publisher is a special callback the provides the value.
     * Publishers are useful for creating read-only properties.
     * Only one publisher may be registered per property.
     * Registering a publisher replaces the previous publisher.
     */
    virtual property<T> &publish(const publisher_type &publisher) = 0;

    /*!
     * Register a subscriber into the property.
     * All subscribers are called when the value changes.
     * Once a subscriber is registered, it cannot be unregistered.
     */
    virtual property<T> &subscribe(const subscriber_type &subscriber) = 0;

    //! Update calls all subscribers w/ the current value
    virtual property<T> &update(void) = 0;

    /*!
     * Set the new value and call all subscribers.
     * The master is called first to coerce the value.
     */
    virtual property<T> &set(const T &value) = 0;

    //! Get the current value of this property
    virtual T get(void) const = 0;
};

/*!
 * The property tree provides a file system structure for accessing properties.
 */
class UHD_API property_tree : boost::noncopyable{
public:
    typedef boost::shared_ptr<property_tree> sptr;
    typedef boost::filesystem::path path_type;

    //! Create a new + empty property tree
    static sptr make(void);

    //! Remove a property or directory (recursive)
    virtual void remove(const path_type &path) = 0;

    //! True if the path exists in the tree
    virtual bool exists(const path_type &path) = 0;

    //! Get an iterable to all things in the given path
    virtual std::vector<std::string> list(const path_type &path) = 0;

    //! Create a new property entry in the tree
    template <typename T> property<T> &create(const path_type &path);

    //! Get access to a property in the tree
    template <typename T> property<T> &access(const path_type &path);

protected:
    //! Internal create property with wild-card type
    virtual void _create(const path_type &path, const boost::shared_ptr<void> &prop) = 0;

    //! Internal access property with wild-card type
    virtual boost::shared_ptr<void> &_access(const path_type &path) = 0;

};

} //namespace uhd

#include <uhd/property_tree.ipp>

#endif /* INCLUDED_UHD_PROPERTY_TREE_HPP */
