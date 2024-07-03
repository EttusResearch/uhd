//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/isatty.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdio>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_isatty)
{
    // We can't really pass or fail based on the result, because it depends on
    // how the tests are executed. We'll just run it and see it doesn't crash.
    if (uhd::is_a_tty(2)) {
        std::cout << "stderr is a TTY" << std::endl;
    } else {
        std::cout << "stderr is not a TTY" << std::endl;
    }
    auto tmp_file =
        std::unique_ptr<std::FILE, int (*)(FILE*)>(std::tmpfile(), &std::fclose);
#ifdef UHD_PLATFORM_WIN32
    BOOST_REQUIRE(!uhd::is_a_tty(_fileno(tmp_file.get())));
#elif _POSIX_C_SOURCE >= _200112L
    BOOST_REQUIRE(!uhd::is_a_tty(fileno(tmp_file.get())));
#else
    // I got 99 problems but dealing with portability ain't one
    BOOST_REQUIRE(!uhd::is_a_tty(99));
#endif
}
