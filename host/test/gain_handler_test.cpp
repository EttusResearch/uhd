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

#include <boost/test/unit_test.hpp>
#include <uhd/gain_handler.hpp>
#include <uhd/props.hpp>
#include <uhd/dict.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace uhd;

enum prop_t{
    PROP_GAIN_VALUE,
    PROP_GAIN_RANGE,
    PROP_GAIN_NAMES
};

class gainful_obj : public wax::obj{
public:
    gainful_obj(void){
        //initialize gain props struct
        gain_handler::props_t gain_props;
        gain_props.value = PROP_GAIN_VALUE;
        gain_props.range = PROP_GAIN_RANGE;
        gain_props.names = PROP_GAIN_NAMES;
        //make a new gain handler
        _gain_handler = gain_handler::make(
            this->get_link(), gain_props,
            boost::bind(&gain_handler::is_equal<prop_t>, _1, _2)
        );
        _gain_values["g0"] = 0;
        _gain_values["g1"] = 0;
        _gain_ranges["g0"] = gain_range_t(-10, 0, .1);
        _gain_ranges["g1"] = gain_range_t(0, 100, 1.5);
    }

    ~gainful_obj(void){}

private:
    void get(const wax::obj &key_, wax::obj &val){
        if (_gain_handler->intercept_get(key_, val)) return;

        wax::obj key; std::string name;
        boost::tie(key, name) = extract_named_prop(key_);

        //handle the get request conditioned on the key
        switch(wax::cast<prop_t>(key)){
        case PROP_GAIN_VALUE:
            val = _gain_values[name];
            return;

        case PROP_GAIN_RANGE:
            val = _gain_ranges[name];
            return;

        case PROP_GAIN_NAMES:
            val = _gain_values.get_keys();
            return;
        }
    }

    void set(const wax::obj &key_, const wax::obj &val){
        if (_gain_handler->intercept_set(key_, val)) return;

        wax::obj key; std::string name;
        boost::tie(key, name) = extract_named_prop(key_);

        //handle the get request conditioned on the key
        switch(wax::cast<prop_t>(key)){
        case PROP_GAIN_VALUE:
            _gain_values[name] = wax::cast<gain_t>(val);
            return;

        case PROP_GAIN_RANGE:
        case PROP_GAIN_NAMES:
            throw std::runtime_error("cannot set this property");
        }
    }

    gain_handler::sptr _gain_handler;
    uhd::dict<std::string, gain_t> _gain_values;
    uhd::dict<std::string, gain_range_t> _gain_ranges;

};

BOOST_AUTO_TEST_CASE(test_gain_handler){
    std::cout << "Testing the gain handler..." << std::endl;
    gainful_obj go0;

    BOOST_CHECK_THROW(
        wax::cast<gain_t>(go0[named_prop_t(PROP_GAIN_VALUE, "fail")]),
        std::exception
    );

    std::cout << "verifying the overall min, max, step" << std::endl;
    gain_t gain_min, gain_max, gain_step;
    boost::tie(gain_min, gain_max, gain_step) = \
        wax::cast<gain_range_t>(go0[PROP_GAIN_RANGE]);
    BOOST_CHECK_EQUAL(gain_min, gain_t(-10));
    BOOST_CHECK_EQUAL(gain_max, gain_t(100));
    BOOST_CHECK_EQUAL(gain_step, gain_t(1.5));

    std::cout << "verifying the overall gain" << std::endl;
    go0[named_prop_t(PROP_GAIN_VALUE, "g0")] = gain_t(-5);
    go0[named_prop_t(PROP_GAIN_VALUE, "g1")] = gain_t(30);
    BOOST_CHECK_EQUAL(wax::cast<gain_t>(go0[PROP_GAIN_VALUE]), gain_t(25));
}
