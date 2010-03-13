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
 * gain handler implementation interface
 **********************************************************************/
class gain_handler_impl : public gain_handler{
public:
    gain_handler_impl(
        const wax::obj &link,
        const props_t &props,
        is_equal_t is_equal
    );
    ~gain_handler_impl(void);
    bool intercept_get(const wax::obj &key, wax::obj &val);
    bool intercept_set(const wax::obj &key, const wax::obj &val);

private:
    wax::obj     _link;
    props_t _props;
    is_equal_t   _is_equal;

    prop_names_t get_gain_names(void);
    gain_t get_overall_gain_val(void);
    gain_range_t get_overall_gain_range(void);
    template <class T> T get_named_prop(const wax::obj &prop, const std::string &name){
        return wax::cast<T>(_link[named_prop_t(prop, name)]);
    }
};

/***********************************************************************
 * the make function
 **********************************************************************/
gain_handler::sptr gain_handler::make(
    const wax::obj &link,
    const props_t &props,
    is_equal_t is_equal
){
    return sptr(new gain_handler_impl(link, props, is_equal));
}

/***********************************************************************
 * gain handler implementation methods
 **********************************************************************/
gain_handler::props_t::props_t(void){
    /* NOP */
}

gain_handler_impl::gain_handler_impl(
    const wax::obj &link,
    const props_t &props,
    is_equal_t is_equal
){
    _link = link;
    _props = props;
    _is_equal = is_equal;
}

gain_handler_impl::~gain_handler_impl(void){
    /* NOP */
}

prop_names_t gain_handler_impl::get_gain_names(void){
    return wax::cast<prop_names_t>(_link[_props.names]);
}

gain_t gain_handler_impl::get_overall_gain_val(void){
    gain_t gain_val = 0;
    BOOST_FOREACH(std::string name, get_gain_names()){
        gain_val += get_named_prop<gain_t>(_props.value, name);
    }
    return gain_val;
}

gain_range_t gain_handler_impl::get_overall_gain_range(void){
    gain_t gain_min = 0, gain_max = 0, gain_step = 0;
    BOOST_FOREACH(std::string name, get_gain_names()){
        gain_t gain_min_tmp, gain_max_tmp, gain_step_tmp;
        boost::tie(gain_min_tmp, gain_max_tmp, gain_step_tmp) = \
            get_named_prop<gain_range_t>(_props.range, name);
        gain_min += gain_min_tmp;
        gain_max += gain_max_tmp;
        gain_step = std::max(gain_step, gain_step_tmp);
    }
    return gain_range_t(gain_min, gain_max, gain_step);
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

    if (_is_equal(key, _props.value)){
        val = get_overall_gain_val();
        return true;
    }

    if (_is_equal(key, _props.range)){
        val = get_overall_gain_range();
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
    if (not _is_equal(key, _props.value)) return false;

    gain_t gain_val = wax::cast<gain_t>(val);

    //not a wildcard... dont handle (but check name and range)
    if (name != ""){
        assert_has(get_gain_names(), name, "gain name");
        gain_t gain_min, gain_max, gain_step;
        boost::tie(gain_min, gain_max, gain_step) = \
            get_named_prop<gain_range_t>(_props.range, name);
        if (gain_val > gain_max or gain_val < gain_min) throw std::range_error(str(
            boost::format("A value of %f for gain %s is out of range of (%f, %f)")
            % gain_val % name % gain_min % gain_max
        ));
        return false;
    }

    //set the overall gain
    BOOST_FOREACH(std::string name, get_gain_names()){
        //get the min, max, step for this gain name
        gain_t gain_min, gain_max, gain_step;
        boost::tie(gain_min, gain_max, gain_step) = \
            get_named_prop<gain_range_t>(_props.range, name);

        //clip g to be within the allowed range
        gain_t g = std::min(std::max(gain_val, gain_min), gain_max);
        //set g to be a multiple of the step size
        g -= fmod(g, gain_step);
        //set g to be the new gain
        _link[named_prop_t(_props.value, name)] = g;
        //subtract g out of the total gain left to apply
        gain_val -= g;
    }

    return true;
}
