//
// Copyright 2010 Ettus Research LLC
//

#include <boost/test/unit_test.hpp>
#include <usrp_uhd/wax.hpp>

enum opt_a_t{OPTION_A_0, OPTION_A_1};
enum opt_b_t{OPTION_B_0, OPTION_B_1};

BOOST_AUTO_TEST_CASE(test_enums){
    wax::type opta = OPTION_A_0;
    BOOST_CHECK_THROW(wax::cast<opt_b_t>(opta), wax::bad_cast);
    BOOST_CHECK_EQUAL(wax::cast<opt_a_t>(opta), OPTION_A_0);
}

/***********************************************************************
 * demo class for wax framework
 **********************************************************************/
class wax_demo : public wax::obj{
private:
    std::vector<float> d_nums;
    std::vector<wax_demo> d_subs;
public:
    wax_demo(size_t sub_demos, size_t len){
        d_nums = std::vector<float>(len);
        if (sub_demos != 0){
            for (size_t i = 0; i < len; i++){
                d_subs.push_back(wax_demo(sub_demos-1, len));
            }
        }
    }
    ~wax_demo(void){
        /* NOP */
    }
    void get(const wax::type &key, wax::type &value){
        if (d_subs.size() == 0){
            value = d_nums[wax::cast<size_t>(key)];
        }else{
            value = obj::cast(&d_subs[wax::cast<size_t>(key)]);
        }
    }
    void set(const wax::type &key, const wax::type &value){
        if (d_subs.size() == 0){
            d_nums[wax::cast<size_t>(key)] = wax::cast<float>(value);
        }else{
            throw std::runtime_error("cant set to a wax demo with sub demos");
        }
    }
};

static wax_demo wd(2, 10);

BOOST_AUTO_TEST_CASE(test_chaining){
    std::cout << "chain 1" << std::endl;
    wd[size_t(0)];
    std::cout << "chain 2" << std::endl;
    wd[size_t(0)][size_t(0)];
    std::cout << "chain 3" << std::endl;
    wd[size_t(0)][size_t(0)][size_t(0)];
}

BOOST_AUTO_TEST_CASE(test_set_get){
    std::cout << "set and get all" << std::endl;
    for (size_t i = 0; i < 10; i++){
        for (size_t j = 0; j < 10; j++){
            for (size_t k = 0; k < 10; k++){
                float val = i * j * k + i + j + k;
                //std::cout << i << " " << j << " " << k << std::endl;
                wd[i][j][k] = val;
                BOOST_CHECK_EQUAL(val, wax::cast<float>(wd[i][j][k]));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_proxy){
    std::cout << "store proxy" << std::endl;
    wax::proxy p = wd[size_t(0)][size_t(0)];
    p[size_t(0)] = float(5);

    std::cout << "assign proxy" << std::endl;
    wax::type a = p[size_t(0)];
    BOOST_CHECK_EQUAL(wax::cast<float>(a), float(5));
}

BOOST_AUTO_TEST_CASE(test_print){
    std::cout << "print type" << std::endl;
    wax::type test_type = float(3.33);
    std::cout << test_type << std::endl;
}
