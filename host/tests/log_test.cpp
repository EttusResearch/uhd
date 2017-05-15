//
// Copyright 2010-2011 Ettus Research LLC
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
#include <uhd/utils/log.hpp>
#include <uhd/utils/log_add.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_messages){
    UHD_LOG_FASTPATH("foo");
    UHD_LOG_FASTPATH("bar");
    uhd::log::set_log_level(uhd::log::debug);
    uhd::log::set_console_level(uhd::log::info);
    uhd::log::add_logger("test",
        [](const uhd::log::logging_info &I){
            std::cout << "<TEST> " << I.message << std::endl;
        }
    );
    uhd::log::set_logger_level("test", uhd::log::debug);
    UHD_LOGGER_DEBUG("logger_test") <<
        "This is a test print for a debug log."
    ;
    UHD_LOGGER_INFO("logger_test") <<
        "This is a test print for a info log."
    ;
    UHD_LOGGER_WARNING("logger_test") <<
        "This is a test print for a warning log."
    ;
    UHD_LOGGER_ERROR("logger_test") <<
        "This is a test print for an error log."
    ;
    UHD_HERE();
    const int x = 42;
    UHD_VAR(x);
}
