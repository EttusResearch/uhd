//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/scope_exit.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_scope_exit)
{
    bool flag = false;
    {
        auto flag_setter = uhd::utils::scope_exit::make([&flag]() { flag = true; });
        BOOST_CHECK(!flag);
    }
    BOOST_CHECK(flag);
}

BOOST_AUTO_TEST_CASE(test_scope_exit_function_object)
{
    bool flag                          = false;
    std::function<void(void)> resetter = [&flag]() { flag = true; };

    {
        auto flag_setter = uhd::utils::scope_exit::make(std::move(resetter));
        BOOST_CHECK(!flag);
    }
    BOOST_CHECK(flag);
}

BOOST_AUTO_TEST_CASE(test_scope_exit_function_named_lambda)
{
    bool flag     = false;
    auto resetter = [&flag]() { flag = true; };

    {
        // Note: Does not require std::move()
        auto flag_setter = uhd::utils::scope_exit::make(resetter);
        BOOST_CHECK(!flag);
    }
    BOOST_CHECK(flag);
}
