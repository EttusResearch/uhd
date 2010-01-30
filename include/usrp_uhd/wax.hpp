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

#ifndef INCLUDED_WAX_HPP
#define INCLUDED_WAX_HPP

#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/pointer_cast.hpp>
#include <iostream>

/*!
 * WAX - it's a metaphor!
 *
 * The WAX framework allows object to have generic/anytype properties.
 * These properties can be addressed through generic/anytype identifiers.
 * A property of a WAX object may even be another WAX object.
 *
 * When a property is a WAX object, the returned value must be an obj pointer.
 * A WAX object provides two types of pointers: obj::ptr and obj::sptr.
 * The choice of pointer vs smart pointer depends on the owner of the memory.
 *
 * Proprties may be referenced though the [] overloaded operator.
 * The [] operator returns a special proxy that allows for assigment.
 * Also, the [] operators may be chained as in the folowing examples:
 *   my_obj[prop1][prop2][prop3] = value
 *   value = my_obj[prop1][prop2][prop3]
 *
 * Any value returned from an access operation is of wax::type.
 * To use this value, it must be cast with wax::cast<new_type>(value).
 */

namespace wax{

    //general typedefs
    typedef boost::any type;
    typedef boost::bad_any_cast bad_cast;

    //dummy class declarations
    class obj; class proxy;

    /*!
     * WAX object base class:
     *   A wax object subclass should override the set and get methods.
     *   The magic of operator chaining is handled by the [] operator.
     */
    class obj{
    public:
        //obj pointer typedefs
        typedef boost::shared_ptr<obj> sptr;
        typedef obj* ptr;

        //cast derived pointer to obj base class pointer
        template <class T> static sptr cast(boost::shared_ptr<T> r){
            return boost::static_pointer_cast<obj>(r);
        }
        template <class T> static ptr cast(T *r){
            return dynamic_cast<ptr>(r);
        }

        //structors
        obj(void);
        virtual ~obj(void);

        //public interface
        proxy operator[](const type &key);

    private:
        //private interface
        virtual void get(const type &key, type &val) = 0;
        virtual void set(const type &key, const type &val) = 0;
    };

    /*!
     * WAX proxy class:
     *   Allows the obj [] operator to return a proxy result.
     *   This result can be assigned to via the = operator.
     *   Or this result can be called again with the [] operator.
     */
    class proxy{
    public:
        //destructors
        ~proxy(void);

        //overloaded
        type operator()(void);
        proxy operator[](const type &key);
        proxy operator=(const type &key);

    private:
        //typedefs for callables from the object that built this proxy
        typedef boost::function<void(const type &)> setter_t;
        typedef boost::function<void(type &)> getter_t;

        //private contructor
        proxy(getter_t, setter_t);
        //access to private contructor
        friend proxy obj::operator[](const type &key);

        getter_t d_getter;
        setter_t d_setter;
    };

    /*!
     * Cast a wax::type into the desired type
     * Usage wax::cast<new_type>(my_value).
     *
     * \param val the any type to cast
     * \return data of the desired type
     * \throw wax::bad_cast when the cast fails
     */
    template<typename T> T cast(const type & val){
        //special case to handle the proxy
        if (val.type() == typeid(proxy)){
            return cast<T>(boost::any_cast<proxy>(val)());
        }
        //do the type cast
        return boost::any_cast<T>(val);
    }

} //namespace wax

//ability to use types with stream operators
std::ostream& operator<<(std::ostream &os, const wax::type &x);

#endif /* INCLUDED_WAX_HPP */
