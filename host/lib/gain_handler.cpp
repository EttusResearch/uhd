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

#include <uhd/gain_handler.hpp>
#include <uhd/utils.hpp>
#include <uhd/props.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <vector>

using namespace uhd;

/***********************************************************************
 * helper functions
 **********************************************************************/
static gain_t gain_max(gain_t a, gain_t b){
    return std::max(a, b);
}
static gain_t gain_sum(gain_t a, gain_t b){
    return std::sum(a, b);
}

/***********************************************************************
 * gain handler implementation interface
 **********************************************************************/
class gain_handler_impl : public gain_handler{
public:
    gain_handler_impl(
        const wax::obj &link,
        const gain_props_t &gain_props,
        is_equal_t is_equal
    );
    ~gain_handler_impl(void);
    bool intercept_get(const wax::obj &key, wax::obj &val);
    bool intercept_set(const wax::obj &key, const wax::obj &val);

private:
    wax::obj     _link;
    gain_props_t _gain_props;
    is_equal_t   _is_equal;

    prop_names_t get_gain_names(void);
    std::vector<gain_t> get_gains(const wax::obj &prop_key);
    gain_t get_overall_gain_val(void);
    gain_t get_overall_gain_min(void);
    gain_t get_overall_gain_max(void);
    gain_t get_overall_gain_step(void);
};

/***********************************************************************
 * the make function
 **********************************************************************/
gain_handler::sptr gain_handler::make(
    const wax::obj &link,
    const gain_props_t &gain_props,
    is_equal_t is_equal
){
    return sptr(new gain_handler_impl(link, gain_props, is_equal));
}

/***********************************************************************
 * gain handler implementation methods
 **********************************************************************/
gain_handler::gain_props_t::gain_props_t(void){
    /* NOP */
}

gain_handler_impl::gain_handler_impl(
    const wax::obj &link,
    const gain_props_t &gain_props,
    is_equal_t is_equal
){
    _link = link;
    _gain_props = gain_props;
    _is_equal = is_equal;
}

gain_handler_impl::~gain_handler_impl(void){
    /* NOP */
}

prop_names_t gain_handler_impl::get_gain_names(void){
    return wax::cast<prop_names_t>(_link[_gain_props.gain_names_prop]);
}

std::vector<gain_t> gain_handler_impl::get_gains(const wax::obj &prop_key){
    std::vector<gain_t> gains;
    BOOST_FOREACH(std::string name, get_gain_names()){
        gains.push_back(wax::cast<gain_t>(_link[named_prop_t(prop_key, name)]));
    }
    return gains;
}

gain_t gain_handler_impl::get_overall_gain_val(void){
    return std::reduce<gain_t>(get_gains(_gain_props.gain_val_prop), gain_sum);
}

gain_t gain_handler_impl::get_overall_gain_min(void){
    return std::reduce<gain_t>(get_gains(_gain_props.gain_min_prop), gain_sum);
}

gain_t gain_handler_impl::get_overall_gain_max(void){
    return std::reduce<gain_t>(get_gains(_gain_props.gain_max_prop), gain_sum);
}

gain_t gain_handler_impl::get_overall_gain_step(void){
    return std::reduce<gain_t>(get_gains(_gain_props.gain_step_prop), gain_max);
}

/***********************************************************************
 * gain handler implementation get method
 **********************************************************************/
bool gain_handler_impl::intercept_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //not a wildcard... dont handle (but check name)
    if (name != ""){
        assert_has(get_gain_names(), name, "gain name");
        return false;
    }

    if (_is_equal(key, _gain_props.gain_val_prop)){
        val = get_overall_gain_val();
        return true;
    }

    if (_is_equal(key, _gain_props.gain_min_prop)){
        val = get_overall_gain_min();
        return true;
    }

    if (_is_equal(key, _gain_props.gain_max_prop)){
        val = get_overall_gain_max();
        return true;
    }

    if (_is_equal(key, _gain_props.gain_step_prop)){
        val = get_overall_gain_step();
        return true;
    }

    return false; //not handled
}

/***********************************************************************
 * gain handler implementation set method
 **********************************************************************/
bool gain_handler_impl::intercept_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //not a gain value key... dont handle
    if (not _is_equal(key, _gain_props.gain_val_prop)) return false;

    gain_t gain_val = wax::cast<gain_t>(val);

    //not a wildcard... dont handle (but check name and range)
    if (name != ""){
        assert_has(get_gain_names(), name, "gain name");
        gain_t gain_min = wax::cast<gain_t>(_link[named_prop_t(_gain_props.gain_min_prop, name)]);
        gain_t gain_max = wax::cast<gain_t>(_link[named_prop_t(_gain_props.gain_max_prop, name)]);
        if (gain_val > gain_max or gain_val < gain_min) throw std::range_error(str(
            boost::format("A value of %f for gain %s is out of range of (%f, %f)")
            % gain_val % name % gain_min % gain_max
        ));
        return false;
    }

    //set the overall gain
    BOOST_FOREACH(std::string name, get_gain_names()){
        //get the min, max, step for this gain name
        gain_t gain_min  = wax::cast<gain_t>(_link[named_prop_t(_gain_props.gain_min_prop, name)]);
        gain_t gain_max  = wax::cast<gain_t>(_link[named_prop_t(_gain_props.gain_max_prop, name)]);
        gain_t gain_step = wax::cast<gain_t>(_link[named_prop_t(_gain_props.gain_step_prop, name)]);

        //clip g to be within the allowed range
        gain_t g = std::min(std::max(gain_val, gain_min), gain_max);
        //set g to be a multiple of the step size
        g -= fmod(g, gain_step);
        //set g to be the new gain
        _link[named_prop_t(_gain_props.gain_val_prop, name)] = g;
        //subtract g out of the total gain left to apply
        gain_val -= g;
    }

    return true;
}
