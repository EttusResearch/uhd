//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/wax.hpp>
#include <cppunit/extensions/HelperMacros.h>

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

/***********************************************************************
 * cpp unit setup
 **********************************************************************/
class wax_test : public CppUnit::TestFixture{
    CPPUNIT_TEST_SUITE(wax_test);
    CPPUNIT_TEST(test_chaining);
    CPPUNIT_TEST(test_set_get);
    CPPUNIT_TEST(test_proxy);
    CPPUNIT_TEST(test_print);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_chaining(void);
    void test_set_get(void);
    void test_proxy(void);
    void test_print(void);
};

CPPUNIT_TEST_SUITE_REGISTRATION(wax_test);

static wax_demo wd(2, 10);

void wax_test::test_chaining(void){
    std::cout << "chain 1" << std::endl;
    wd[size_t(0)];
    std::cout << "chain 2" << std::endl;
    wd[size_t(0)][size_t(0)];
    std::cout << "chain 3" << std::endl;
    wd[size_t(0)][size_t(0)][size_t(0)];
}

void wax_test::test_set_get(void){
    std::cout << "set and get all" << std::endl;
    for (size_t i = 0; i < 10; i++){
        for (size_t j = 0; j < 10; j++){
            for (size_t k = 0; k < 10; k++){
                float val = i * j * k + i + j + k;
                //std::cout << i << " " << j << " " << k << std::endl;
                wd[i][j][k] = val;
                CPPUNIT_ASSERT_EQUAL(val, wax::cast<float>(wd[i][j][k]));
            }
        }
    }
}

void wax_test::test_proxy(void){
    std::cout << "store proxy" << std::endl;
    wax::proxy p = wd[size_t(0)][size_t(0)];
    p[size_t(0)] = float(5);
}

void wax_test::test_print(void){
    std::cout << "print type" << std::endl;
    wax::type test_type = float(3.33);
    std::cout << test_type << std::endl;
}
