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
#include <boost/shared_ptr.hpp>
#include <uhd/wax.hpp>
#include <iostream>

enum opt_a_t{OPTION_A_0, OPTION_A_1};
enum opt_b_t{OPTION_B_0, OPTION_B_1};

BOOST_AUTO_TEST_CASE(test_enums){
    wax::obj opta = OPTION_A_0;
    BOOST_CHECK_THROW(opta.as<opt_b_t>(), wax::bad_cast);
    BOOST_CHECK_EQUAL(opta.as<opt_a_t>(), OPTION_A_0);
}

/***********************************************************************
 * demo class for wax framework
 **********************************************************************/
class wax_demo : public wax::obj{
public:
    typedef boost::shared_ptr<wax_demo> sptr;

    wax_demo(size_t sub_demos, size_t len){
        d_nums = std::vector<float>(len);
        if (sub_demos != 0){
            for (size_t i = 0; i < len; i++){
                d_subs.push_back(sptr(new wax_demo(sub_demos-1, len)));
            }
        }
    }
    ~wax_demo(void){
        /* NOP */
    }
private:
    std::vector<float> d_nums;
    std::vector<sptr> d_subs;

    void get(const wax::obj &key, wax::obj &value){
        if (d_subs.size() == 0){
            value = d_nums[key.as<size_t>()];
        }else{
            value = d_subs[key.as<size_t>()]->get_link();
        }
    }
    void set(const wax::obj &key, const wax::obj &value){
        if (d_subs.size() == 0){
            d_nums[key.as<size_t>()] = value.as<float>();
        }else{
            throw std::runtime_error("cant set to a wax demo with sub demos");
        }
    }
};

BOOST_AUTO_TEST_CASE(test_chaining){
    wax_demo wd(2, 1);
    std::cout << "chain 1" << std::endl;
    wd[size_t(0)];
    std::cout << "chain 2" << std::endl;
    wd[size_t(0)][size_t(0)];
    std::cout << "chain 3" << std::endl;
    wd[size_t(0)][size_t(0)][size_t(0)];
}

BOOST_AUTO_TEST_CASE(test_set_get){
    wax_demo wd(2, 10);
    std::cout << "set and get all" << std::endl;
    for (size_t i = 0; i < 10; i++){
        for (size_t j = 0; j < 10; j++){
            for (size_t k = 0; k < 10; k++){
                float val = float(i * j * k + i + j + k);
                //std::cout << i << " " << j << " " << k << std::endl;
                wd[i][j][k] = val;
                BOOST_CHECK_EQUAL(val, wd[i][j][k].as<float>());
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_proxy){
    wax_demo wd(2, 1);
    std::cout << "store proxy" << std::endl;
    wax::obj p = wd[size_t(0)][size_t(0)];
    p[size_t(0)] = float(5);

    std::cout << "assign proxy" << std::endl;
    wax::obj a = p[size_t(0)];
    BOOST_CHECK_EQUAL(a.as<float>(), float(5));
}
