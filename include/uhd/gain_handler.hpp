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

#include <boost/shared_ptr.hpp>
#include <uhd/wax.hpp>
#include <uhd/props.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#ifndef INCLUDED_UHD_GAIN_HANDLER_HPP
#define INCLUDED_UHD_GAIN_HANDLER_HPP

namespace uhd{

class gain_handler{
public:
    typedef boost::shared_ptr<gain_handler> sptr;

    template <class T> gain_handler(
        wax::obj *wax_obj_ptr, const T &gain_prop,
        const T &gain_min_prop, const T &gain_max_prop,
        const T &gain_step_prop, const T &gain_names_prop
    ){
        _wax_obj_ptr = wax_obj_ptr;
        _gain_prop = gain_prop;
        _gain_min_prop = gain_min_prop;
        _gain_max_prop = gain_max_prop;
        _gain_step_prop = gain_step_prop;
        _gain_names_prop = gain_names_prop;
        _is_equal = boost::bind(&gain_handler::is_equal<T>, _1, _2);
    }

    ~gain_handler(void);

    /*!
     * Intercept gets for overall gain, min, max, step.
     * Ensures that the gain name is valid.
     * \return true for handled, false to pass on
     */
    bool intercept_get(const wax::obj &key, wax::obj &val);

    /*!
     * Intercept sets for overall gain.
     * Ensures that the gain name is valid.
     * Ensures that the new gain is within range.
     * \return true for handled, false to pass on
     */
    bool intercept_set(const wax::obj &key, const wax::obj &val);

private:

    wax::obj     *_wax_obj_ptr;
    wax::obj      _gain_prop;
    wax::obj      _gain_min_prop;
    wax::obj      _gain_max_prop;
    wax::obj      _gain_step_prop;
    wax::obj      _gain_names_prop;

    /*!
     * Verify that the key is valid:
     * If its a named prop for gain, ensure that name is valid.
     * If the name if not valid, throw a std::invalid_argument.
     * The name can only be valid if its in the list of gain names.
     */
    void _check_key(const wax::obj &key);

    /*
     * Private interface to test if two wax types are equal:
     * The constructor will bind an instance of this for a specific type.
     * This bound equals functions allows the intercept methods to be non-templated.
     */
    template <class T> static bool is_equal(const wax::obj &a, const wax::obj &b){
        try{
            return wax::cast<T>(a) == wax::cast<T>(b);
        }
        catch(const wax::bad_cast &){
            return false;
        }
    }
    boost::function<bool(const wax::obj &, const wax::obj &)> _is_equal;

};

} //namespace uhd

#endif /* INCLUDED_UHD_GAIN_HANDLER_HPP */

