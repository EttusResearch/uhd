//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/soft_register.hpp>
#include <boost/test/unit_test.hpp>

using namespace uhd;

BOOST_AUTO_TEST_CASE(test_soft_reg_field) {
    UHD_DEFINE_SOFT_REG_FIELD(test_reg1, /* width */ 1, /* shift */ 0);
    BOOST_CHECK_EQUAL(soft_reg_field::width(test_reg1), 1);
    BOOST_CHECK_EQUAL(soft_reg_field::shift(test_reg1), 0);
    BOOST_CHECK_EQUAL(soft_reg_field::mask<uint32_t>(test_reg1),  1<<0);

    UHD_DEFINE_SOFT_REG_FIELD(test_reg2, /* width */ 5, /* shift */ 4);
    BOOST_CHECK_EQUAL(soft_reg_field::width(test_reg2), 5);
    BOOST_CHECK_EQUAL(soft_reg_field::shift(test_reg2), 4);
    BOOST_CHECK_EQUAL(soft_reg_field::mask<uint32_t>(test_reg2),  0x1F<<4);

    UHD_DEFINE_SOFT_REG_FIELD(test_reg3, /* width */ 9, /* shift */ 0);
    BOOST_CHECK_EQUAL(soft_reg_field::width(test_reg3), 9);
    BOOST_CHECK_EQUAL(soft_reg_field::shift(test_reg3), 0);
    BOOST_CHECK_EQUAL(soft_reg_field::mask<uint8_t>(test_reg3),  0xFF);

    // This one is platform dependent:
    UHD_DEFINE_SOFT_REG_FIELD(test_reg4, /* width */ 33, /* shift */ 0);
    BOOST_CHECK_EQUAL(soft_reg_field::width(test_reg4), 33);
    BOOST_CHECK_EQUAL(soft_reg_field::shift(test_reg4), 0);
    BOOST_CHECK_EQUAL(soft_reg_field::mask<size_t>(test_reg4), ~size_t(0) & 0x1FFFFFFFF);
}
