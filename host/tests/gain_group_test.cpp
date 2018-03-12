//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/utils/gain_group.hpp>
#include <boost/bind.hpp>
#include <boost/math/special_functions/round.hpp>
#include <iostream>

#define rint(x) boost::math::iround(x)

using namespace uhd;

/***********************************************************************
 * Define gain element classes with needed functions
 **********************************************************************/
class gain_element1{
public:

    gain_range_t get_range(void){
        return gain_range_t(0, 90, 1);
    }

    double get_value(void){
        return _gain;
    }

    void set_value(double gain){
        double step = get_range().step();
        _gain = step*rint(gain/step);
    }

private:
    double _gain;
};

class gain_element2{
public:

    gain_range_t get_range(void){
        return gain_range_t(-20, 10, 0.1);
    }

    double get_value(void){
        return _gain;
    }

    void set_value(double gain){
        double step = get_range().step();
        _gain = step*rint(gain/step);
    }

private:
    double _gain;
};

//create static instances of gain elements to be shared by the tests
static gain_element1 g1;
static gain_element2 g2;

static gain_group::sptr get_gain_group(size_t pri1 = 0, size_t pri2 = 0){
    //create instance of gain group
    gain_fcns_t gain_fcns;
    gain_group::sptr gg(gain_group::make());

    //load gain group with function sets
    gain_fcns.get_range = boost::bind(&gain_element1::get_range, &g1);
    gain_fcns.get_value = boost::bind(&gain_element1::get_value, &g1);
    gain_fcns.set_value = boost::bind(&gain_element1::set_value, &g1, _1);
    gg->register_fcns("g1", gain_fcns, pri1);

    gain_fcns.get_range = boost::bind(&gain_element2::get_range, &g2);
    gain_fcns.get_value = boost::bind(&gain_element2::get_value, &g2);
    gain_fcns.set_value = boost::bind(&gain_element2::set_value, &g2, _1);
    gg->register_fcns("g2", gain_fcns, pri2);

    return gg;
}

/***********************************************************************
 * Test cases
 **********************************************************************/
static const double tolerance = 0.001;

BOOST_AUTO_TEST_CASE(test_gain_group_overall){
    gain_group::sptr gg = get_gain_group();

    //test the overall stuff
    gg->set_value(80);
    BOOST_CHECK_CLOSE(gg->get_value(), 80.0, tolerance);
    BOOST_CHECK_CLOSE(gg->get_range().start(), -20.0, tolerance);
    BOOST_CHECK_CLOSE(gg->get_range().stop(), 100.0, tolerance);
    BOOST_CHECK_CLOSE(gg->get_range().step(), 0.1, tolerance);
}

BOOST_AUTO_TEST_CASE(test_gain_group_priority){
    gain_group::sptr gg = get_gain_group(0, 1);

    //test the overall stuff
    gg->set_value(80);
    BOOST_CHECK_CLOSE(gg->get_value(), 80.0, tolerance);
    BOOST_CHECK_CLOSE(gg->get_range().start(), -20.0, tolerance);
    BOOST_CHECK_CLOSE(gg->get_range().stop(), 100.0, tolerance);
    BOOST_CHECK_CLOSE(gg->get_range().step(), 0.1, tolerance);

    //test the the higher priority gain got filled first (gain 2)
    BOOST_CHECK_CLOSE(g2.get_value(), g2.get_range().stop(), tolerance);
}
