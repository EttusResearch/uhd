//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/error.h>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>

#include <boost/thread/mutex.hpp>

#include <cstring>

#define MAP_TO_ERROR(exception_type, error_type) \
    if (dynamic_cast<const uhd::exception_type*>(e)) return error_type;

uhd_error error_from_uhd_exception(const uhd::exception* e){
    MAP_TO_ERROR(index_error,           UHD_ERROR_INDEX)
    MAP_TO_ERROR(key_error,             UHD_ERROR_KEY)
    MAP_TO_ERROR(not_implemented_error, UHD_ERROR_NOT_IMPLEMENTED)
    MAP_TO_ERROR(usb_error,             UHD_ERROR_USB)
    MAP_TO_ERROR(io_error,              UHD_ERROR_IO)
    MAP_TO_ERROR(os_error,              UHD_ERROR_OS)
    MAP_TO_ERROR(assertion_error,       UHD_ERROR_ASSERTION)
    MAP_TO_ERROR(lookup_error,          UHD_ERROR_LOOKUP)
    MAP_TO_ERROR(type_error,            UHD_ERROR_TYPE)
    MAP_TO_ERROR(value_error,           UHD_ERROR_VALUE)
    MAP_TO_ERROR(runtime_error,         UHD_ERROR_RUNTIME)
    MAP_TO_ERROR(environment_error,     UHD_ERROR_ENVIRONMENT)
    MAP_TO_ERROR(system_error,          UHD_ERROR_SYSTEM)

    return UHD_ERROR_EXCEPT;
}

// Store the error string in a single place in library
// Note: Don't call _c_global_error_string() directly, it needs to be locked
// for thread-safety. Use set_c_global_error_string() and
// get_c_global_error_string() instead.
UHD_SINGLETON_FCN(std::string, _c_global_error_string)

static boost::mutex _error_c_mutex;

std::string get_c_global_error_string(){
    boost::mutex::scoped_lock lock(_error_c_mutex);
    return _c_global_error_string();
}

void set_c_global_error_string(
    const std::string &msg
){
    boost::mutex::scoped_lock lock(_error_c_mutex);
    _c_global_error_string() = msg;
}

uhd_error uhd_get_last_error(
    char* error_out,
    size_t strbuffer_len
){
    try{
        auto error_str = get_c_global_error_string();
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, error_str.c_str(), strbuffer_len);
    }
    catch(...){
        return UHD_ERROR_UNKNOWN;
    }
    return UHD_ERROR_NONE;
}
