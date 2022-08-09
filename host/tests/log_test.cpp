//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/log_add.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_messages)
{
    UHD_LOG_FASTPATH("foo");
    UHD_LOG_FASTPATH("bar");
    uhd::log::set_log_level(uhd::log::debug);
    uhd::log::set_console_level(uhd::log::info);
    uhd::log::add_logger("test", [](const uhd::log::logging_info& I) {
        std::cout << "<TEST> " << I.message << std::endl;
    });
    uhd::log::set_logger_level("test", uhd::log::debug);
    UHD_LOGGER_DEBUG("logger_test") << "This is a test print for a debug log.";
    UHD_LOGGER_INFO("logger_test") << "This is a test print for a info log.";
    UHD_LOGGER_WARNING("logger_test") << "This is a test print for a warning log.";
    UHD_LOGGER_ERROR("logger_test") << "This is a test print for an error log.";
    UHD_LOGGER_FATAL("logger_test") << "This is a test print for a fatal error log.";
    UHD_HERE();
    const int x = 42;
    UHD_VAR(x);
    UHD_HEX(x);
}

void log_with_throw()
{
    UHD_LOG_THROW(uhd::runtime_error, "TEST", "Testing log+throw " << 1234);
}

BOOST_AUTO_TEST_CASE(test_throw)
{
    BOOST_CHECK_THROW(log_with_throw(), uhd::runtime_error);
}
