//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_SAFE_CALL_HPP
#define INCLUDED_UHD_UTILS_SAFE_CALL_HPP

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>

//! helper macro for safe call to produce warnings
#define _UHD_SAFE_CALL_WARNING(code, what) UHD_LOGGER_ERROR("UHD") << \
    UHD_THROW_SITE_INFO("Exception caught in safe-call.") + #code + " -> " + what \
;

/*!
 * A safe-call catches all exceptions thrown by code,
 * and creates a verbose warning about the exception.
 * Usage: UHD_SAFE_CALL(some_code_to_call();)
 * \param code the block of code to call safely
 */
#define UHD_SAFE_CALL(code) \
    try{code} \
    catch(const std::exception &e){ \
        _UHD_SAFE_CALL_WARNING(code, e.what()); \
    } \
    catch(...){ \
        _UHD_SAFE_CALL_WARNING(code, "unknown exception"); \
    }

#endif /* INCLUDED_UHD_UTILS_SAFE_CALL_HPP */
