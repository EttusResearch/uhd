//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/database.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/paths.hpp>
#include <stdlib.h> // putenv or _putenv
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <numeric>

using namespace uhd::usrp::cal;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_CASE(test_rc)
{
    BOOST_CHECK(!database::has_cal_data("does_not_exist", "1234", source::RC));
    BOOST_CHECK(database::has_cal_data("test", "1234", source::RC));
    BOOST_CHECK(database::has_cal_data("test", "1234"));
    BOOST_CHECK(!database::has_cal_data("test", "1234", source::FILESYSTEM));

    const auto test_data = database::read_cal_data("test", "", source::RC);
    const std::string test_str(test_data.cbegin(), test_data.cend());
    // The expected string is also in the test.cal file. We could, in this test,
    // open that file and dynamically generate the expected data, but let's not.
    // First, that adds complexity here, and second, both test.cal and this test
    // are hashed with the same git commit, and thus we also test the integrity
    // of test.cal.
    BOOST_CHECK_EQUAL(test_str, "rc::cal::test_data");
}

BOOST_AUTO_TEST_CASE(test_fs)
{
    BOOST_CHECK(!database::has_cal_data("does_not_exist", "1234", source::FILESYSTEM));

    const auto tmp_dir      = uhd::get_tmp_path();
    const auto tmp_cal_path = fs::path(tmp_dir) / "CAL_TEST";
    boost::system::error_code ec;
    fs::create_directory(tmp_cal_path, ec);
    if (ec) {
        std::cout << "WARNING: Could not create temp cal path. Skipping test."
                  << std::endl;
        return;
    }
    std::cout << "Using temporary cal path: " << tmp_cal_path << std::endl;

    // Now we do a non-portable hack to override the cal path during runtime:
#ifdef UHD_PLATFORM_WIN32
    const std::string putenv_str =
        std::string("UHD_CAL_DATA_PATH=") + tmp_cal_path.string();
    _putenv(putenv_str.c_str());
#else
    setenv("UHD_CAL_DATA_PATH", tmp_cal_path.string().c_str(), /* overwrite */ 1);
#endif

    // Because of the hack, we won't fail if it didn't work, but instead, print
    // a warning and exit this test. Running the following lines requires the
    // hack to succeed.
    if (uhd::get_cal_data_path() != tmp_cal_path) {
        std::cout << "WARNING: Unable to update UHD_CAL_DATA_PATH! get_cal_data_path(): "
                  << uhd::get_cal_data_path() << std::endl;
        return;
    }

    std::vector<uint8_t> mock_data{1, 2, 3, 4, 5};
    database::write_cal_data("mock_data", "1234", mock_data);
    auto mock_data_rb = database::read_cal_data("mock_data", "1234");
    BOOST_CHECK_EQUAL_COLLECTIONS(
        mock_data.begin(), mock_data.end(), mock_data_rb.begin(), mock_data_rb.end());

    BOOST_CHECK(!database::has_cal_data("mock_data", "abcd"));
    std::vector<uint8_t> mock_data2{2, 3, 4, 5, 6};
    database::write_cal_data("mock_data", "abcd", mock_data);
    // Write it twice to force a backup
    database::write_cal_data("mock_data", "abcd", mock_data2, "BACKUP");
    mock_data_rb = database::read_cal_data("mock_data", "abcd");
    BOOST_CHECK_EQUAL_COLLECTIONS(
        mock_data2.begin(), mock_data2.end(), mock_data_rb.begin(), mock_data_rb.end());
    BOOST_CHECK(database::has_cal_data("mock_data", "abcd"));
    BOOST_CHECK(fs::exists(tmp_cal_path / "mock_data_abcd.cal.BACKUP"));

    fs::remove_all(tmp_cal_path, ec);
    if (ec) {
        std::cout << "WARNING: Could not remove temp cal path." << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(test_flash)
{
    // 4 bytes of cal data
    std::vector<uint8_t> mock_cal_data{42, 23, 1, 2};

    database::register_lookup(
        [&](const std::string& key, const std::string&) {
            // Note: We're deliberately not checking the key here, but below, so
            // we can check all code paths in database.cpp, even the ones we're
            // not supposed to reach
            return key == "MOCK_KEY";
        },
        [&](const std::string& key, const std::string& serial) {
            if (key == "MOCK_KEY" && serial == "MOCK_SERIAL") {
                return mock_cal_data;
            }
            throw uhd::runtime_error("no such mock data!");
        });
    BOOST_CHECK(database::has_cal_data("MOCK_KEY", "MOCK_SERIAL", source::FLASH));
    BOOST_CHECK(!database::has_cal_data("FOO_KEY", "FOO_SERIAL", source::FLASH));
    auto cal_data1 = database::read_cal_data("MOCK_KEY", "MOCK_SERIAL", source::FLASH);
    BOOST_CHECK_EQUAL_COLLECTIONS(mock_cal_data.cbegin(),
        mock_cal_data.cend(),
        cal_data1.cbegin(),
        cal_data1.cend());
    auto cal_data2 = database::read_cal_data("MOCK_KEY", "MOCK_SERIAL", source::ANY);
    BOOST_CHECK_EQUAL_COLLECTIONS(mock_cal_data.cbegin(),
        mock_cal_data.cend(),
        cal_data2.cbegin(),
        cal_data2.cend());
    BOOST_REQUIRE_THROW(database::read_cal_data("MOCK_KEY", "FOO_SERIAL", source::FLASH),
        uhd::runtime_error);
}
