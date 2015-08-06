/*
 * Copyright 2015 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_UHD_ERROR_H
#define INCLUDED_UHD_ERROR_H

//! UHD error codes
/*!
 * Each error code corresponds to a specific uhd::exception, with
 * extra codes corresponding to a boost::exception, std::exception,
 * and a catch-all for everything else. When an internal C++ function
 * throws an exception, UHD converts it to one of these error codes
 * to return on the C level.
 */
typedef enum {

    //! No error thrown.
    UHD_ERROR_NONE = 0,
    //! Invalid device arguments.
    UHD_ERROR_INVALID_DEVICE = 1,

    //! See uhd::index_error.
    UHD_ERROR_INDEX = 10,
    //! See uhd::key_error.
    UHD_ERROR_KEY = 11,

    //! See uhd::not_implemented_error.
    UHD_ERROR_NOT_IMPLEMENTED = 20,
    //! See uhd::usb_error.
    UHD_ERROR_USB = 21,

    //! See uhd::io_error.
    UHD_ERROR_IO = 30,
    //! See uhd::os_error.
    UHD_ERROR_OS = 31,

    //! See uhd::assertion_error.
    UHD_ERROR_ASSERTION = 40,
    //! See uhd::lookup_error.
    UHD_ERROR_LOOKUP = 41,
    //! See uhd::type_error.
    UHD_ERROR_TYPE = 42,
    //! See uhd::value_error.
    UHD_ERROR_VALUE = 43,
    //! See uhd::runtime_error.
    UHD_ERROR_RUNTIME = 44,
    //! See uhd::environment_error.
    UHD_ERROR_ENVIRONMENT = 45,
    //! See uhd::system_error.
    UHD_ERROR_SYSTEM = 46,
    //! See uhd::exception.
    UHD_ERROR_EXCEPT = 47,

    //! A boost::exception was thrown.
    UHD_ERROR_BOOSTEXCEPT = 60,

    //! A std::exception was thrown.
    UHD_ERROR_STDEXCEPT = 70,

    //! An unknown error was thrown.
    UHD_ERROR_UNKNOWN = 100
} uhd_error;

#ifdef __cplusplus
#include <uhd/config.hpp>
#include <uhd/exception.hpp>

#include <boost/exception/diagnostic_information.hpp>

UHD_API uhd_error error_from_uhd_exception(const uhd::exception* e);

/*!
 * This macro runs the given C++ code, and if there are any exceptions
 * thrown, they are caught and converted to the corresponding UHD error
 * code.
 */
#define UHD_SAFE_C(...) \
    try{ __VA_ARGS__ } \
    catch (const uhd::exception &e) { \
        return error_from_uhd_exception(&e); \
    } \
    catch (const boost::exception&) { \
        return UHD_ERROR_BOOSTEXCEPT; \
    } \
    catch (const std::exception&) { \
        return UHD_ERROR_STDEXCEPT; \
    } \
    catch (...) { \
        return UHD_ERROR_UNKNOWN; \
    } \
    return UHD_ERROR_NONE;

/*!
 * This macro runs the given C++ code, and if there are any exceptions
 * thrown, they are caught and converted to the corresponding UHD error
 * code. The error message is also saved into the given handle.
 */
#define UHD_SAFE_C_SAVE_ERROR(h, ...) \
    h->last_error.clear(); \
    try{ __VA_ARGS__ } \
    catch (const uhd::exception &e) { \
        h->last_error = e.what(); \
        return error_from_uhd_exception(&e); \
    } \
    catch (const boost::exception &e) { \
        h->last_error = boost::diagnostic_information(e); \
        return UHD_ERROR_BOOSTEXCEPT; \
    } \
    catch (const std::exception &e) { \
        h->last_error = e.what(); \
        return UHD_ERROR_STDEXCEPT; \
    } \
    catch (...) { \
        h->last_error = "Unrecognized exception caught."; \
        return UHD_ERROR_UNKNOWN; \
    } \
    h->last_error = "None"; \
    return UHD_ERROR_NONE;

#endif

#endif /* INCLUDED_UHD_ERROR_H */
