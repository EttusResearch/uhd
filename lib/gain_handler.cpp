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

#include <usrp_uhd/gain_handler.hpp>
#include <usrp_uhd/utils.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <vector>

using namespace usrp_uhd;

/***********************************************************************
 * Helper functions and macros
 **********************************************************************/
#define GET_PROP_NAMES() \
    wax::cast<prop_names_t>((*_wax_obj_ptr)[_gain_names_prop])

/*!
 * Helper function to simplify getting a named gain (also min, max, step).
 */
static gain_t get_named_gain(wax::obj::ptr wax_obj_ptr, wax::type prop, std::string name){
    return wax::cast<gain_t>((*wax_obj_ptr)[named_prop_t(prop, name)]);
}

/***********************************************************************
 * Class methods of gain handler
 **********************************************************************/
gain_handler::~gain_handler(void){
    /* NOP */
}

void gain_handler::_check_key(const wax::type &key_){
    wax::type key; std::string name;
    tie(key, name) = extract_named_prop(key_);
    
    try{
        //only handle non wildcard names
        ASSERT_THROW(name != "");

        //only handle these gain props
        ASSERT_THROW(
            _is_equal(key, _gain_prop)     or
            _is_equal(key, _gain_min_prop) or
            _is_equal(key, _gain_max_prop) or
            _is_equal(key, _gain_step_prop)
        );

        //check that the name is allowed
        prop_names_t prop_names = GET_PROP_NAMES();
        ASSERT_THROW(not std::has(prop_names.begin(), prop_names.end(), name));

        //if we get here, throw an exception
        throw std::invalid_argument(str(
            boost::format("Unknown gain name %s") % name
        ));
    }
    catch(const std::assert_error &){}
}

bool gain_handler::intercept_get(const wax::type &key, wax::type &val){
    _check_key(key); //verify the key

    // use a vector of tuples to map properties to a reducer function
    // we cant use a map because the wax::type cant be sorted
    typedef boost::function<gain_t(gain_t, gain_t)> reducer_t;
    typedef boost::tuple<wax::type, reducer_t> tuple_t;
    reducer_t reducer_sum = boost::bind(std::sum<gain_t>, _1, _2);
    reducer_t reducer_max = boost::bind(std::max<gain_t>, _1, _2);
    std::vector<tuple_t> prop_to_reducer = boost::assign::tuple_list_of
        (_gain_prop,      reducer_sum)(_gain_min_prop,  reducer_sum)
        (_gain_max_prop,  reducer_sum)(_gain_step_prop, reducer_max);

    /*!
     * Handle getting overall gains when a name is not specified.
     * For the gain props below, set the overall value and return true. 
     */
    BOOST_FOREACH(tuple_t p2r, prop_to_reducer){
        if (_is_equal(key, p2r.get<0>())){
            //form the gains vector from the props vector
            prop_names_t prop_names = GET_PROP_NAMES();
            std::vector<gain_t> gains(prop_names.size());
            std::transform(
                prop_names.begin(), prop_names.end(), gains.begin(),
                boost::bind(get_named_gain, _wax_obj_ptr, p2r.get<0>(), _1)
            );

            //reduce across the gain vector
            val = std::reduce<gain_t>(gains.begin(), gains.end(), p2r.get<1>());
            return true;
        }
    }

    return false;
}

bool gain_handler::intercept_set(const wax::type &key_, const wax::type &val){
    _check_key(key_); //verify the key

    wax::type key; std::string name;
    tie(key, name) = extract_named_prop(key_);

    /*!
     * Verify that a named gain component is in range.
     */
    try{
        //only handle the gain props
        ASSERT_THROW(_is_equal(key, _gain_prop));

        //check that the gain is in range
        gain_t gain = wax::cast<gain_t>(val);
        gain_t gain_min = get_named_gain(_wax_obj_ptr, _gain_min_prop,  name);
        gain_t gain_max = get_named_gain(_wax_obj_ptr, _gain_max_prop,  name);
        ASSERT_THROW(gain > gain_max or gain < gain_min);

        //if we get here, throw an exception
        throw std::range_error(str(
            boost::format("gain %s is out of range of (%f, %f)") % name % gain_min % gain_max
        ));
    }
    catch(const std::assert_error &){}

    /*!
     * Handle setting the overall gain.
     */
    if (_is_equal(key, _gain_prop) and name == ""){
        gain_t gain = wax::cast<gain_t>(val);
        prop_names_t prop_names = GET_PROP_NAMES();
        BOOST_FOREACH(std::string name, prop_names){
            //get the min, max, step for this gain name
            gain_t gain_min  = get_named_gain(_wax_obj_ptr, _gain_min_prop,  name);
            gain_t gain_max  = get_named_gain(_wax_obj_ptr, _gain_max_prop,  name);
            gain_t gain_step = get_named_gain(_wax_obj_ptr, _gain_step_prop, name);

            //clip g to be within the allowed range
            gain_t g = std::min(std::max(gain, gain_min), gain_max);
            //set g to be a multiple of the step size
            g -= fmod(g, gain_step);
            //set g to be the new gain
            (*_wax_obj_ptr)[named_prop_t(_gain_prop, name)] = g;
            //subtract g out of the total gain left to apply
            gain -= g;
        }
        return true;
    }

    return false;
}
