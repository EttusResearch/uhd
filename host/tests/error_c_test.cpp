//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/error.h>
#include <uhd/types/dict.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/format.hpp>

/*
 * Test our conversions from exceptions to C-level UHD error codes.
 * We use inline functions separate from the Boost test cases themselves
 * to test our C++ macro, which returns the error code.
 */

typedef struct {
    std::string last_error;
} dummy_handle_t;

template <typename error_type>
UHD_INLINE uhd_error throw_uhd_exception(dummy_handle_t *handle, const uhd::exception* except){
    UHD_SAFE_C_SAVE_ERROR(handle,
        throw *dynamic_cast<const error_type*>(except);
    )
}

UHD_INLINE uhd_error throw_boost_exception(dummy_handle_t *handle){
    UHD_SAFE_C_SAVE_ERROR(handle,
        std::runtime_error except("This is a std::runtime_error, thrown by Boost.");
        BOOST_THROW_EXCEPTION(except);
    )
}

UHD_INLINE uhd_error throw_std_exception(dummy_handle_t *handle){
    UHD_SAFE_C_SAVE_ERROR(handle,
        throw std::runtime_error("This is a std::runtime_error.");
    )
}

UHD_INLINE uhd_error throw_unknown_exception(dummy_handle_t *handle){
    UHD_SAFE_C_SAVE_ERROR(handle,
        throw 1;
    )
}

// There are enough non-standard names that we can't just use a conversion function
static const uhd::dict<std::string, std::string> pretty_exception_names =
    boost::assign::map_list_of
        ("assertion_error",       "AssertionError")
        ("lookup_error",          "LookupError")
        ("index_error",           "LookupError: IndexError")
        ("key_error",             "LookupError: KeyError")
        ("type_error",            "TypeError")
        ("value_error",           "ValueError")
        ("runtime_error",         "RuntimeError")
        ("not_implemented_error", "RuntimeError: NotImplementedError")
        ("usb_error",             "RuntimeError: USBError 1")
        ("environment_error",     "EnvironmentError")
        ("io_error",              "EnvironmentError: IOError")
        ("os_error",              "EnvironmentError: OSError")
        ("system_error",          "SystemError")
    ;

#define UHD_TEST_CHECK_ERROR_CODE(cpp_exception_type, c_error_code) \
    expected_msg = str(boost::format("This is a uhd::%s.") % BOOST_STRINGIZE(cpp_exception_type)); \
    uhd::cpp_exception_type cpp_exception_type ## _foo(expected_msg); \
    error_code = throw_uhd_exception<uhd::cpp_exception_type>(&handle, &cpp_exception_type ## _foo); \
    BOOST_CHECK_EQUAL(error_code, c_error_code); \
    expected_msg = str(boost::format("%s: %s") \
                       % pretty_exception_names.get(BOOST_STRINGIZE(cpp_exception_type)) \
                       % expected_msg); \
    BOOST_CHECK_EQUAL(handle.last_error, expected_msg); \
    BOOST_CHECK_EQUAL(get_c_global_error_string(), expected_msg);

// uhd::usb_error has a different constructor
#define UHD_TEST_CHECK_USB_ERROR_CODE() \
    expected_msg = "This is a uhd::usb_error."; \
    uhd::usb_error usb_error_foo(1, expected_msg); \
    error_code = throw_uhd_exception<uhd::usb_error>(&handle, &usb_error_foo); \
    BOOST_CHECK_EQUAL(error_code, UHD_ERROR_USB); \
    expected_msg = str(boost::format("%s: %s") \
                       % pretty_exception_names.get("usb_error") \
                       % expected_msg); \
    BOOST_CHECK_EQUAL(handle.last_error, expected_msg); \
    BOOST_CHECK_EQUAL(get_c_global_error_string(), expected_msg);

BOOST_AUTO_TEST_CASE(test_uhd_exception){
    dummy_handle_t handle;
    std::string expected_msg;
    uhd_error error_code;

    UHD_TEST_CHECK_ERROR_CODE(assertion_error,       UHD_ERROR_ASSERTION);
    UHD_TEST_CHECK_ERROR_CODE(lookup_error,          UHD_ERROR_LOOKUP);
    UHD_TEST_CHECK_ERROR_CODE(index_error,           UHD_ERROR_INDEX);
    UHD_TEST_CHECK_ERROR_CODE(key_error,             UHD_ERROR_KEY);
    UHD_TEST_CHECK_ERROR_CODE(type_error,            UHD_ERROR_TYPE);
    UHD_TEST_CHECK_ERROR_CODE(value_error,           UHD_ERROR_VALUE);
    UHD_TEST_CHECK_ERROR_CODE(runtime_error,         UHD_ERROR_RUNTIME);
    UHD_TEST_CHECK_ERROR_CODE(not_implemented_error, UHD_ERROR_NOT_IMPLEMENTED);
    UHD_TEST_CHECK_ERROR_CODE(io_error,              UHD_ERROR_IO);
    UHD_TEST_CHECK_ERROR_CODE(os_error,              UHD_ERROR_OS);
    UHD_TEST_CHECK_ERROR_CODE(system_error,          UHD_ERROR_SYSTEM);
    UHD_TEST_CHECK_USB_ERROR_CODE();
}

BOOST_AUTO_TEST_CASE(test_boost_exception){
    dummy_handle_t handle;
    uhd_error error_code = throw_boost_exception(&handle);

    // Boost error message cannot be determined here, so just check code
    BOOST_CHECK_EQUAL(error_code, UHD_ERROR_BOOSTEXCEPT);
}

BOOST_AUTO_TEST_CASE(test_std_exception){
    dummy_handle_t handle;
    uhd_error error_code = throw_std_exception(&handle);

    BOOST_CHECK_EQUAL(error_code, UHD_ERROR_STDEXCEPT);
    BOOST_CHECK_EQUAL(handle.last_error, "This is a std::runtime_error.");
}

BOOST_AUTO_TEST_CASE(test_unknown_exception){
    dummy_handle_t handle;
    uhd_error error_code = throw_unknown_exception(&handle);

    BOOST_CHECK_EQUAL(error_code, UHD_ERROR_UNKNOWN);
    BOOST_CHECK_EQUAL(handle.last_error, "Unrecognized exception caught.");
}
