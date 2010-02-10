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
#include <uhd/utils.hpp>
#include <iostream>

using namespace uhd;

enum prop_t{
    PROP_GAIN,
    PROP_GAIN_MIN,
    PROP_GAIN_MAX,
    PROP_GAIN_STEP,
    PROP_GAIN_NAMES
};

class gainful_obj : public wax::obj{
public:
    gainful_obj(void){
        _gain_handler = gain_handler::sptr(new gain_handler(
            this, PROP_GAIN, PROP_GAIN_MIN, PROP_GAIN_MAX, PROP_GAIN_STEP, PROP_GAIN_NAMES
        ));
        _gains["g0"] = 0;
        _gains["g1"] = 0;
        _gains_min["g0"] = -10;
        _gains_min["g1"] = 0;
        _gains_max["g0"] = 0;
        _gains_max["g1"] = 100;
        _gains_step["g0"] = .1;
        _gains_step["g1"] = 1.5;
    }

    ~gainful_obj(void){}

private:
    void get(const wax::obj &key_, wax::obj &val){
        if (_gain_handler->intercept_get(key_, val)) return;

        wax::obj key; std::string name;
        boost::tie(key, name) = extract_named_prop(key_);

        //handle the get request conditioned on the key
        switch(wax::cast<prop_t>(key)){
        case PROP_GAIN:
            val = _gains[name];
            return;

        case PROP_GAIN_MIN:
            val = _gains_min[name];
            return;

        case PROP_GAIN_MAX:
            val = _gains_max[name];
            return;

        case PROP_GAIN_STEP:
            val = _gains_step[name];
            return;

        case PROP_GAIN_NAMES:
            val = prop_names_t(get_map_keys(_gains));
            return;
        }
    }

    void set(const wax::obj &key_, const wax::obj &val){
        if (_gain_handler->intercept_set(key_, val)) return;

        wax::obj key; std::string name;
        boost::tie(key, name) = extract_named_prop(key_);

        //handle the get request conditioned on the key
        switch(wax::cast<prop_t>(key)){
        case PROP_GAIN:
            _gains[name] = wax::cast<gain_t>(val);
            return;

        case PROP_GAIN_MIN:
        case PROP_GAIN_MAX:
        case PROP_GAIN_STEP:
        case PROP_GAIN_NAMES:
            throw std::runtime_error("cannot set this property");
        }
    }

    gain_handler::sptr _gain_handler;
    std::map<std::string, gain_t> _gains;
    std::map<std::string, gain_t> _gains_min;
    std::map<std::string, gain_t> _gains_max;
    std::map<std::string, gain_t> _gains_step;

};

BOOST_AUTO_TEST_CASE(test_gain_handler){
    std::cout << "Testing the gain handler..." << std::endl;
    gainful_obj go0;

    BOOST_CHECK_THROW(
        wax::cast<gain_t>(go0[named_prop_t(PROP_GAIN, "fail")]),
        std::invalid_argument
    );

    std::cout << "verifying the overall min, max, step" << std::endl;
    BOOST_CHECK_EQUAL(wax::cast<gain_t>(go0[PROP_GAIN_MIN]), gain_t(-10));
    BOOST_CHECK_EQUAL(wax::cast<gain_t>(go0[PROP_GAIN_MAX]), gain_t(100));
    BOOST_CHECK_EQUAL(wax::cast<gain_t>(go0[PROP_GAIN_STEP]), gain_t(1.5));

    std::cout << "verifying the overall gain" << std::endl;
    go0[named_prop_t(PROP_GAIN, "g0")] = gain_t(-5);
    go0[named_prop_t(PROP_GAIN, "g1")] = gain_t(30);
    BOOST_CHECK_EQUAL(wax::cast<gain_t>(go0[PROP_GAIN]), gain_t(25));
}
