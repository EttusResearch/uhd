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

#ifndef INCLUDED_UHD_UTILS_GAIN_HANDLER_HPP
#define INCLUDED_UHD_UTILS_GAIN_HANDLER_HPP

#include <uhd/config.hpp>
#include <uhd/wax.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

namespace uhd{

class UHD_API gain_handler{
public:
    typedef boost::shared_ptr<gain_handler> sptr;
    typedef boost::function<bool(const wax::obj &, const wax::obj &)> is_equal_t;

    /*!
     * A set of properties for dealing with gains.
     */
    struct UHD_API props_t{
        wax::obj value, range, names;
        props_t(void); //default constructor
    };

    /*!
     * Make a new gain handler.
     * The construction arguments are agnostic to the property type.
     * It is up to the caller to provide an "is_equal" function that
     * can tell weather two properties (in a wax obj) are equal.
     * \param link a link to the wax obj with properties
     * \param props a struct of properties keys
     * \param is_equal the function that tests for equal properties
     */
    static sptr make(
        const wax::obj &link,
        const props_t &props,
        is_equal_t is_equal
    );

    /*!
     * Intercept gets for overall gain, min, max, step.
     * Ensures that the gain name is valid.
     * \return true for handled, false to pass on
     */
    virtual bool intercept_get(const wax::obj &key, wax::obj &val) = 0;

    /*!
     * Intercept sets for overall gain.
     * Ensures that the gain name is valid.
     * Ensures that the new gain is within range.
     * \return true for handled, false to pass on
     */
    virtual bool intercept_set(const wax::obj &key, const wax::obj &val) = 0;

    /*!
     * Function template to test if two wax types are equal:
     * The constructor will bind an instance of this for a specific type.
     * This bound equals functions allows the intercept methods to be non-templated.
     */
    template <class T> static bool is_equal(const wax::obj &a, const wax::obj &b){
        try{
            return a.as<T>() == b.as<T>();
        }
        catch(const wax::bad_cast &){
            return false;
        }
    }

};

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_GAIN_HANDLER_HPP */

